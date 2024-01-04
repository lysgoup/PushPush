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

#define BUF_SIZE 1024
#define MAX_USER 4

int user_count = 0;
int user_sock[MAX_USER];
char user_name[MAX_USER][8];
pthread_t tid[MAX_USER];
int is_end = 0;
char *json_name = "sample.json";

typedef struct User{
    int user_x;
    int user_y;
    int base_x;
    int base_y;
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
    user users[MAX_USER];
    pos *obstacles;
    int n_obstacles;
    pos *balls;
    int n_balls;
}model;

model game;

void handle_timeout(int sig){
    is_end = 1;
    exit(EXIT_SUCCESS);
}


void parse_json(FILE *json_data) {
    json_t *root, *mapsize, *timeout, *user, *obstacle, *ball;
    size_t index;

    root = json_loadf(json_data, 0, NULL);

    if (!root) {
        fprintf(stderr, "JSON 파싱 에러\n");
        return;
    }

    // mapsize
    mapsize = json_object_get(root, "mapsize");
    game.row = json_integer_value(json_object_get(mapsize, "rows"));
    game.col = json_integer_value(json_object_get(mapsize, "cols"));
    printf("rows: %d, cols: %d\n", game.row, game.col);

    // timeout
    timeout = json_object_get(root, "timeout");
    int timeval = json_integer_value(timeout);
    game.timeout.tv_sec = timeval/1000;
    game.timeout.tv_usec = 0;
    printf("timeout: %ld\n", game.timeout.tv_sec);

    // user

    user = json_object_get(root, "user");
    json_array_foreach(user, index, user) {
        game.users[index].id = json_integer_value(json_object_get(user, "id"));
        game.users[index].base_x = json_integer_value(json_object_get(user, "baseX"));
        game.users[index].base_y = json_integer_value(json_object_get(user, "baseY"));
        game.users[index].user_x = json_integer_value(json_object_get(user, "startX"));
        game.users[index].user_y = json_integer_value(json_object_get(user, "startY"));
        printf("user: id=%d, baseX=%d, baseY=%d, startX=%d, startY=%d\n", game.users[index].id, game.users[index].base_x, game.users[index].base_y, game.users[index].user_x, game.users[index].user_y);
    }

    // obstacle
    obstacle = json_object_get(root, "obstacle");
    game.n_obstacles = (int)(json_array_size(obstacle));
    game.obstacles = (pos *)malloc(sizeof(pos) * game.n_obstacles);

    json_array_foreach(obstacle, index, obstacle) {
        game.obstacles[index].x = json_integer_value(json_object_get(obstacle, "obstacleX"));
        game.obstacles[index].y = json_integer_value(json_object_get(obstacle, "obstacleY"));
        printf("obstacle: obstacleX=%d, obstacleY=%d\n", game.obstacles[index].x, game.obstacles[index].y);
    }

    // ball
    ball = json_object_get(root, "ball");
    game.n_balls = (int)(json_array_size(ball));
    game.balls = (pos *)malloc(sizeof(pos) * game.n_balls); 
    printf("Number of balls: %zu\n", game.n_balls);
    printf("ball: ");
    json_array_foreach(ball, index, ball) {
        game.balls[index].x = json_integer_value(json_object_get(ball, "ballX"));
        game.balls[index].y = json_integer_value(json_object_get(ball, "ballY"));
        printf("(ballX=%d, ballY=%d) ", game.balls[index].x, game.balls[index].y );
    }
    printf("\n");

    json_decref(root);
}
void client_thread(void * arg){
    // int serv_sock = (int*)arg;
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
    // struct thread_arg thr_arg;
    // thr_arg.serv_adr = serv_adr;

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
        strncpy(user_name[user_count], buf, 8);
        printf("%s\n", user_name[user_count]);
        int *arg_sock = (int*)malloc(sizeof(int));
        *arg_sock = clnt_sock;

        if (pthread_create(&tid[user_count], NULL, (void *)client_thread, arg_sock) != 0) {
			perror("pthread_create : ") ;
			exit(EXIT_FAILURE) ;
		}
        
        user_count += 1;
	}

    //All users are connected.
    //TODO 1.send user-list.
    for(int i = 0; i <= MAX_USER; i++){
        write(user_sock[i], user_name[i], 8);
        char id = i;
        write(user_sock[i], &id, sizeof(id));
    }
    //TODO 2.read JSON and make model.
    FILE *fp;
    fp = fopen(json_name, "r");
    if(fp == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    //send json to users
    struct stat fileStat;
    if (stat(json_name, &fileStat) != 0) {
        fprintf(stderr, "stat\n");
    }
    int data_len = fileStat.st_size;
    for(int i = 0; i <= MAX_USER; i++){
        write(user_sock[i], &data_len, sizeof(data_len));
    }
    int num_read = 0;
    while((num_read = fread(buf, 1, BUF_SIZE, fp)) > 0){
        for(int i = 0; i <= MAX_USER; i++){
            write(user_sock[i], buf, num_read);
        }
    }

    fseek(fp, 0, SEEK_SET);

    // parse_json(fp);

    //TODO 3.send timestamp.

    struct timeval timestamp;

    // gettimeofday 함수를 사용하여 현재 시간을 가져옵니다.
    if (gettimeofday(&timestamp, NULL) != 0) {
        perror("gettimeofday");
        return 1;
    }

    for(int i = 0; i <= MAX_USER; i++){
        write(user_sock[i], &timestamp, sizeof(struct timeval));
    } 
    //TODO 4.start timer(signal).

    for(int i = 0; i <= MAX_USER; i++){
        pthread_join(tid[i], NULL);
    }
}