/*
TODO: verify 수정
*/
#include <stdio.h>
#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <pthread.h>
#include <libgen.h>
#include <signal.h>
#include <ctype.h>
#include <sys/select.h>

#define BUF_SIZE 1024
#define MAX_player 4
int MAX_USER = MAX_player;
int start_flag = 0;
int user_count = 0;
int user_sock[MAX_player];
char user_name[MAX_player][9];
pthread_t tid[MAX_player];
int is_end = 0;
char *json_name = "board3.json";
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;


char command_buf[10][3];
int head = 0, tail = 0;


typedef struct User{
    int user_x;
    int user_y;
    int base_x;
    int base_y;
    char name[9];
    int score;
    int id;
}user;

typedef struct Pos{
    int x;
    int y;
}pos;

typedef struct Model{
    int row;
    int col;
    struct timeval timeout;
    user users[MAX_player];
    pos *obstacles;
    int n_obstacles;
    pos *balls;
    int n_balls;
}model;

model game;

json_t *parse_json(FILE *json_data) {
    json_t *root, *mapsize, *timeout, *user, *obstacle, *ball;
    size_t index;
    json_t *json_value;
    root = json_loadf(json_data, 0, NULL);

    if (!root) {
        fprintf(stderr, "JSON 파싱 에러\n");
        return NULL;
    }

    // mapsize
    mapsize = json_object_get(root, "mapsize");
    game.row = json_integer_value(json_object_get(mapsize, "rows"));
    game.col = json_integer_value(json_object_get(mapsize, "cols"));

    // timeout
    timeout = json_object_get(root, "timeout");
    int timeval = json_integer_value(timeout);
    game.timeout.tv_sec = timeval/1000;
    game.timeout.tv_usec = 0;

    user = json_object_get(root, "user");
    json_array_foreach(user, index, json_value) {
        game.users[index].id = json_integer_value(json_object_get(json_value, "id"));
        game.users[index].base_x = json_integer_value(json_array_get(json_object_get(json_value, "base"), 0));
        game.users[index].base_y = json_integer_value(json_array_get(json_object_get(json_value, "base"), 1));
        game.users[index].user_x = json_integer_value(json_array_get(json_object_get(json_value, "user"), 0));
        game.users[index].user_y = json_integer_value(json_array_get(json_object_get(json_value, "user"), 1));
        json_object_set_new(json_value, "name", json_string(user_name[index]));
        strncpy(game.users[index].name, json_string_value(json_object_get(json_value, "name")), sizeof(game.users[index].name) - 1);
    }

    // obstacle
    obstacle = json_object_get(root, "obstacle");
    game.n_obstacles = (int)(json_array_size(obstacle));
    game.obstacles = (pos *)malloc(sizeof(pos) * game.n_obstacles);

    json_array_foreach(obstacle, index, json_value) {
        game.obstacles[index].x = json_integer_value(json_array_get(json_value, 0));
        game.obstacles[index].y = json_integer_value(json_array_get(json_value, 1));
    }

    // ball
    ball = json_object_get(root, "ball");
    game.n_balls = (int)(json_array_size(ball));
    game.balls = (pos *)malloc(sizeof(pos) * game.n_balls); 
    json_array_foreach(ball, index, json_value) {
        game.balls[index].x = json_integer_value(json_array_get(json_value, 0));
        game.balls[index].y = json_integer_value(json_array_get(json_value, 1));
    }

    return root;
}

int is_cell_empty(int x, int y){
    //user
    for(int i = 0; i < MAX_USER; i++){
        if(game.users[i].user_x == x && game.users[i].user_y == y){
            return 0;
        }
    }
    //base
    for(int i = 0; i < MAX_USER; i++){
        if(game.users[i].base_x == x && game.users[i].base_y == y){
            return 0;
        }
    }
    //obstacle
    for(int i = 0; i < game.n_obstacles; i++){
        if(game.obstacles[i].x == x && game.obstacles[i].y == y){
            return 0;
        }
    }
    //boarder
    if(x <= 0 || x >= game.col - 1){
        return 0;
    }
    if(y <= 0 || y >= game.row - 1){
        return 0;
    }
    //ball
    for(int i = 0; i < game.n_balls; i++){
        if(game.balls[i].x == x && game.balls[i].y == y){
            return 0;
        }
    }
    return 1;
}

void add_command(char *command){
    pthread_mutex_lock(&mutex_queue);
    if( ((head + 1)%10) != tail){
        strncpy(command_buf[head], command, 3);
        head = (head + 1)%10;
    }
    pthread_mutex_unlock(&mutex_queue);
}

void handle_timeout(int sig){
    char *to = "TO";
    add_command(to);
    printf("time out!\n");
    // exit(EXIT_SUCCESS);
}

void verify_command(char *command){
    //check validity
    //check obstacles, balls, base, another users
    //goal -> n_balls - 1
    int player = command[0] - '0';
    char direction = command[1];
    int current_x = game.users[player].user_x;
    int current_y = game.users[player].user_y;

    int delta;
    if(direction == 'D' || direction == 'R'){
        delta = 1;
    }
    else if(direction == 'U' || direction == 'L'){
        delta = -1;
    }
    else{
        fprintf(stderr, "verify command error\n");
        return;
    }

    if(direction == 'U' || direction == 'D'){
        if(!is_cell_empty(current_x,current_y + delta)){
            //막혀있는 것이 공이라면?
            for(int i = 0; i < game.n_balls; i++){ //무슨 공인가?
                if(game.balls[i].x == current_x && game.balls[i].y == (current_y + delta)){
                    if(is_cell_empty(current_x, current_y + (2 * delta))){ //공 옆이 비었음. move ball!
                        game.balls[i].y += delta;
                        game.users[player].user_y += delta; 
                        add_command(command);
                    }
                    else{//공 옆이 막혀있음
                        //공이 베이스에 들어갔다면?
                        for(int j = 0; j < MAX_USER; j++){
                            if(game.users[j].base_x == game.balls[i].x && game.users[j].base_y == (game.balls[i].y + delta)){
                                //공 삭제 및 점수 증가. 
                                game.balls[i] = game.balls[game.n_balls-1];
                                game.n_balls -= 1; //free?
                                game.users[j].score += 1;
                                game.users[player].user_y += delta;
                                add_command(command);
                                printf("goal!! %d balls remain\n", game.n_balls);
                                //공이 더이상 없을 때 종료
                                if(game.n_balls <= 0){
                                    char *fn = "FN";
                                    add_command(fn);
                                }
                                break;
                            }
                        }
                        
                    }
                    break;
                }
            }
            // 못움직이는 상황 : 아무것도 안 함.
        }
        else{
           game.users[player].user_y += delta;
           add_command(command);
        }
    }
    else if(direction == 'R' || direction == 'L'){
        if(!is_cell_empty(current_x + delta ,current_y)){
            //막혀있는 것이 공이라면?
            for(int i = 0; i < game.n_balls; i++){ //무슨 공인가?
                if(game.balls[i].x == (current_x + delta) && game.balls[i].y == current_y){
                    if(is_cell_empty(current_x + (2 * delta), current_y)){ //공 옆이 비었음. move ball!
                        game.balls[i].x += delta;
                        game.users[player].user_x += delta; 
                        add_command(command);
                        break;
                    }
                    else{
                        //공이 베이스에 들어갔다면?
                        for(int j = 0; j < MAX_USER; j++){
                            if(game.users[j].base_x == (game.balls[i].x + delta) && game.users[j].base_y == game.balls[i].y){
                                //공 삭제 및 점수 증가.
                                game.balls[i] = game.balls[game.n_balls-1];
                                game.n_balls -= 1; //free?
                                game.users[j].score += 1;
                                game.users[player].user_x += delta;
                                add_command(command);
                                if(game.n_balls <= 0){
                                    char *fn = "FN";
                                    add_command(fn);
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            // 못움직이는 상황 : 아무것도 안 함.
        }
        else{//빈공간이 있어서 움직일 수 있음
           game.users[player].user_x += delta;
           add_command(command);
        }
    }
}

void client_thread(void * arg){
    int player = *((int*)arg);
    free(arg);

    fd_set read_fds;
    struct timeval to;
    while (is_end == 0) {
        // 파일 디스크립터 세트 초기화
        FD_ZERO(&read_fds);
        FD_SET(user_sock[player], &read_fds);

        // 타임아웃 설정 (0초 0 마이크로초로 설정하면 non-blocking으로 동작)
        to.tv_sec = 0;
        to.tv_usec = 0;

        // select 호출
        int ready = select(user_sock[player] + 1, &read_fds, NULL, NULL, &to);

        if (ready == -1) {
            perror("select");
            break;
        } else if (ready == 0) {
            // 타임아웃 발생 (non-blocking에서는 항상 0이 리턴됨)
            continue;
        } else {
            char buf[2];
            if(read(user_sock[player], buf, 2) == 0){
                printf("player: %s disconnected\n", user_name[player]);
                user_count -= 1;
                int temp = user_sock[player];
                user_sock[player] = user_sock[user_count];
                strcpy(user_name[player], user_name[user_count]);
                return;
            }
            char *command = (char*)malloc(sizeof(char) * 2);
            command[0] = player + '0';
            strncpy(command + 1, buf+1, 1);
            verify_command(command);
        }
    }
}


void send_command(){
    while(is_end == 0){
        //No new command in buffer
        pthread_mutex_lock(&mutex_queue);
        if(head == tail){
            pthread_mutex_unlock(&mutex_queue);
            continue;
        }
        pthread_mutex_unlock(&mutex_queue);

        //send command to users
        pthread_mutex_lock(&mutex_queue);
        for(int i = 0; i < user_count; i++){
            int written = write(user_sock[i], command_buf[tail], 2);
            if(strcmp(command_buf[tail], "FN") == 0 || strcmp(command_buf[tail], "TO") == 0){
                is_end = 1;
                alarm(0);
            }
        }
        printf("command [%s] is sent\n", command_buf[tail]);
        tail = (tail + 1)%10;
        pthread_mutex_unlock(&mutex_queue);
    }
    return;
}

int main(int argc, char *argv[])
{
	int serv_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;
    char buf[BUF_SIZE];

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1){
		perror("bind()");
        exit(EXIT_FAILURE);
    }
	if (listen(serv_sock, 10) == -1){
		perror("listen()");
        exit(EXIT_FAILURE);
    }

    socklen_t adr_sz;
    int clnt_sock;

    //initialize user_name
    for(int i=0; i < MAX_USER; i++){
        memset(user_name[i], 0, sizeof(user_name[i]));
    }

	while (1){
        if(user_count >= MAX_USER){
            break;
        }

		adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		if (clnt_sock == -1){
			continue;
        }
		
        puts("new client connected...");
        
        user_sock[user_count] = clnt_sock;
        memset(buf, 0, BUF_SIZE);
        if(read(clnt_sock, buf, BUF_SIZE) == -1){
            close(clnt_sock);
            continue;
        }
        strncpy(user_name[user_count], buf, 9);
        printf("%s\n", user_name[user_count]);
        int *arg_player = (int*)malloc(sizeof(int));
        *arg_player = user_count;

        if (pthread_create(&tid[user_count], NULL, (void *)client_thread, arg_player) != 0) {
			perror("pthread_create : ") ;
			exit(EXIT_FAILURE) ;
		}
        
        user_count += 1;
	}

    //All users are connected.

    FILE *fp;
    fp = fopen(json_name, "r");
    if(fp == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    json_t *root = parse_json(fp);
    char *json_str = json_dumps(root, JSON_ENCODE_ANY); //serialize
    
    int json_str_size = strlen(json_str);
    printf("size : %d \n%s\n", json_str_size,json_str);
    for(int i = 0; i < MAX_USER; i++){
        write(user_sock[i], &json_str_size, sizeof(json_str_size));
        write(user_sock[i], json_str, json_str_size);
    }
    json_decref(root);

    struct timeval timestamp;

    // gettimeofday 함수를 사용하여 현재 시간을 가져옵니다.
    if (gettimeofday(&timestamp, NULL) != 0) {
        perror("gettimeofday");
        return 1;
    }

    for(int i = 0; i < MAX_USER; i++){
        write(user_sock[i], &timestamp, sizeof(struct timeval));
    } 
    start_flag = 1;

    pthread_t temptid;
    if (pthread_create(&temptid ,NULL, (void *)send_command, NULL) != 0) {
        perror("pthread_create - send_command : ") ;
        exit(EXIT_FAILURE) ;
    }

    signal(SIGALRM, handle_timeout);
    alarm(game.timeout.tv_sec);


    //wait untill game end.
    for(int i = 0; i < MAX_USER; i++){
        pthread_join(tid[i], NULL);
    }

    printf("Game Finish\n");
    //disconnect all socket
    for(int i = 0; i < MAX_USER; i++){
        close(user_sock[i]);
    }
    return 0;
}