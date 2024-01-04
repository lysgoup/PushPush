#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "cJSON/cJSON.c"
#include "cJSON/cJSON.h"
#include <gtk/gtk.h>

#define USER_COUNT 4
#define MAX_NAME_SIZE 8
#define BUF_SIZE 9

typedef struct User{
    int user_x;
    int user_y;
    int base_x;
    int base_y;
    int id;
    char name[MAX_NAME_SIZE+1];
    int score;
}User;

typedef struct Pos{
    int x;
    int y;
}Pos;

typedef struct Model{
    int row;
    int col;
    struct timeval timeout;
    User users[USER_COUNT];
    Pos* obstacles;
    int n_obstacles;
    Pos* balls;
    int n_balls;
}Model;

void show_view();
int send_bytes(int fd, void * buf, size_t len);
int recv_bytes(int fd, void * buf, size_t len);
void parse_json(int filesize);
int read_mapfile(char * map_info);
void print_model(Model *map);
void init_view(int row, int col);
void update_view();

//Global
Model map;
char ** view;
struct timeval start_time;
char myname[MAX_NAME_SIZE+1];
char* usernames[USER_COUNT];

void show_view(){
    for(int i=0;i<map.row;i++){
        for(int j=0;j<map.col;j++){
            if(view[i][j] == '\0'){
               printf("?  "); 
            }
            else{
                printf("%c  ",view[i][j]);
            }
        }
        printf("\n");
    }
}

// if it was succeed return 0, otherwise return 1
int send_bytes(int fd, void * buf, size_t len) {
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = send(fd, p, len - acc, MSG_NOSIGNAL) ;
        if (sent == 0)
            return 1 ;
        p += sent ;
        acc += sent ;
    }
    return 0 ;
}

// if it was succeed return 0, otherwise return 1
int recv_bytes(int fd, void * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t received ;
        received = recv(fd, p, len - acc, MSG_NOSIGNAL) ;
        // printf("%ld: %s\n",received,(char*)buf);
        if (received == 0)
            return 1 ;
        p += received ;
        acc += received ;
    }
    return 0 ;
}


void parse_json(int filesize){
    char map_info[filesize];
    FILE * fp = fopen("map.json", "rb");
    size_t result = fread(map_info, 1, filesize, fp);
    printf("%s",map_info);
    fclose(fp); 
    read_mapfile(map_info);
}

// if it was succeed return 0, otherwise return 1
int read_mapfile(char * map_info) {
    // parse the JSON data 
    cJSON *json = cJSON_Parse(map_info); 

    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            printf("Error: %s\n", error_ptr); 
        } 
        cJSON_Delete(json); 
        return 1; 
    }

    cJSON *mapsize = cJSON_GetObjectItem(json,"mapsize");
    cJSON *j_row = cJSON_GetObjectItem(mapsize, "rows");
    cJSON *j_col = cJSON_GetObjectItem(mapsize, "cols");
    map.row = j_row->valueint;
    map.col = j_col->valueint;

    cJSON *j_timeout = cJSON_GetObjectItemCaseSensitive(json, "timeout");
    gettimeofday(&map.timeout, NULL);

    int i;

    cJSON *j_user = cJSON_GetObjectItem(json,"user");
    cJSON *id, *j_base, *base_x, *base_y, *j2_user, *user_x, *user_y, *j_score, *j_name;
    cJSON * subitem;
    for (i = 0 ; i < cJSON_GetArraySize(j_user) ; i++){
        subitem = cJSON_GetArrayItem(j_user, i);
        id = cJSON_GetObjectItem(subitem, "id");
        j_base = cJSON_GetObjectItem(subitem, "base");
        base_x = cJSON_GetArrayItem(j_base, 0);
        base_y = cJSON_GetArrayItem(j_base, 1);
        j2_user = cJSON_GetObjectItem(subitem, "user");
        user_x = cJSON_GetArrayItem(j2_user, 0);
        user_y = cJSON_GetArrayItem(j2_user, 1);
        j_score = cJSON_GetObjectItem(subitem, "score"); 
        j_name = cJSON_GetObjectItem(subitem, "name"); 

        map.users[i].id = id->valueint;
        map.users[i].user_x = user_x->valueint;
        map.users[i].user_y = user_y->valueint;
        map.users[i].base_x = base_x->valueint;
        map.users[i].base_y = base_y->valueint;
        map.users[i].score = j_score->valueint;
        strcpy(map.users[i].name, j_name->valuestring);

    }
    cJSON * j_obstacle = cJSON_GetObjectItem(json,"obstacle");
    cJSON * x, * y;
    map.n_obstacles = cJSON_GetArraySize(j_obstacle);
    map.obstacles = malloc(sizeof(Pos) * map.n_obstacles);
    
    for (i = 0 ; i < cJSON_GetArraySize(j_obstacle) ; i++){
        subitem = cJSON_GetArrayItem(j_obstacle, i);
        x = cJSON_GetArrayItem(subitem, 0);
        y = cJSON_GetArrayItem(subitem, 1);

        map.obstacles[i].x = x->valueint;
        map.obstacles[i].y = y->valueint;
    }

    cJSON * j_ball = cJSON_GetObjectItem(json,"ball");
    map.n_balls = cJSON_GetArraySize(j_ball);
    map.balls = malloc(sizeof(Pos) * map.n_balls);
    
    for (i = 0 ; i < cJSON_GetArraySize(j_ball) ; i++){
        subitem = cJSON_GetArrayItem(j_ball, i);
        x = cJSON_GetArrayItem(subitem, 0);
        y = cJSON_GetArrayItem(subitem, 1);

        map.balls[i].x = x->valueint;
        map.balls[i].y = y->valueint;
    }
    
    return 0;
}

void print_model(Model *map) {
    printf("Row: %d\n", map->row);
    printf("Col: %d\n", map->col);

    printf("Timeout: %ld seconds\n", map->timeout.tv_sec);

    printf("Users:\n");
    for (int i = 0; i < 4; i++) {
        printf("  User %d:\n", i + 1);
        printf("    ID: %d\n", map->users[i].id);
        printf("    User X: %d\n", map->users[i].user_x);
        printf("    User Y: %d\n", map->users[i].user_y);
        printf("    Base X: %d\n", map->users[i].base_x);
        printf("    Base Y: %d\n", map->users[i].base_y);
        printf("    Score: %d\n", map->users[i].score);
        printf("    Name: %s\n", map->users[i].name);
    }

    printf("Obstacles:\n");
    for (int i = 0; i < map->n_obstacles; i++) {
        printf("  Obstacle %d: X: %d, Y: %d\n", i + 1, map->obstacles[i].x, map->obstacles[i].y);
    }

    printf("Balls:\n");
    for (int i = 0; i < map->n_balls; i++) {
        printf("  Ball %d: X: %d, Y: %d\n", i + 1, map->balls[i].x, map->balls[i].y);
    }
}



void init_view(int row, int col){
    view = (char **)malloc(row * sizeof(char *));
    //!
    for(int i = 0; i < row; i++) {
        view[i] = (char *)malloc(col * sizeof(char));
        memset(view[i], 0, col);
    }

    //테두리 처리
    for(int i=0; i<row; i++){
        view[i][0] = 'O';
        view[i][col-1] = 'O'; 
    }
    for(int i=0; i<col; i++){
        view[0][i] = 'O';
        view[row-1][i] = 'O'; 
    }
}

void update_view(){
    //obstacle
    for (int i = 0; i < map.n_obstacles; i++) {
        view[map.obstacles[i].y][map.obstacles[i].x] = 'O';
    }
    //ball
    for (int i = 0; i < map.n_balls; i++) {
        view[map.balls[i].y][map.balls[i].x] = 'B';
    }

    //player
    for (int i = 0; i < USER_COUNT; i++) {
        char c = map.users[i].id + '0';
        view[map.users[i].user_y][map.users[i].user_x] = c;
    }

    //Base
    for (int i = 0; i < USER_COUNT; i++) {
        char c = (map.users[i].id+4) + '0';
        view[map.users[i].base_y][map.users[i].base_x] = c;
    }
}

int main (int argc, char * argv[])
{
    int sock, filesize;
	struct sockaddr_in serv_adr;
    char buf[BUF_SIZE];

    if (argc != 4) {
		printf("Usage: %s <IP> <port> <User name> \n", argv[0]) ;
		exit(1) ;
	}
    while(strlen(argv[3]) > MAX_NAME_SIZE){
        printf("Set your user name to 8 characters or less\n User Name: ");
        scanf("%s",argv[3]);
    }
    strcpy(myname,argv[3]);
    myname[MAX_NAME_SIZE] = '\0';
    printf("%s\n",myname);

    sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));
	connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    //user name 전송
    if(send_bytes(sock, myname, sizeof(myname))){
        fprintf(stderr,"Failed to send user name\n");
        exit(EXIT_FAILURE);
    }
    shutdown(sock,SHUT_WR);

    // user list 수신
    for(int i=0;i<4;i++){
        if(recv_bytes(sock, buf, sizeof(buf))){
            fprintf(stderr,"Failed to receive user list\n");
            exit(EXIT_FAILURE);
        };
        usernames[i] = (char*)malloc(sizeof(char*));
        strcpy(usernames[i],buf);
    }

    //! test
    for(int i=0;i<4;i++){
        printf("username: %s\n", usernames[i]);
    }
    
    //JSON 파일 크기 수신
    if(recv_bytes(sock, &filesize, sizeof(filesize))){
        fprintf(stderr,"Failed to receive filesize\n");
        exit(EXIT_FAILURE);
    };
    //! test
    printf("filesize: %d\n",filesize);

    //JSON 파일 수신
    FILE * fp = fopen("map.json", "wb");
    size_t received = 0;
    size_t r = 0;
    size_t acc = 0;
    while(received < filesize){
        acc = filesize-received;
        if(sizeof(buf)>acc){
            r = recv(sock, buf, acc, MSG_NOSIGNAL) ; 
        }
        else{
            r = recv(sock, buf, sizeof(buf), MSG_NOSIGNAL) ;
        }
        received += r;
        printf("%ld\n",received);
        fwrite(buf,r,1,fp);
    }
    fclose(fp);

    //TODO: JSON파일 파싱하고 모델 생성
    parse_json(filesize);
    print_model(&map);

    //TODO: 모델을 가지고 view 생성
    init_view(map.row, map.col);
    update_view();
    //! test
    show_view();

    // timestamp 수신
    if(recv_bytes(sock, &start_time, sizeof(start_time))){
        fprintf(stderr,"Failed to receive timeout\n");
        exit(EXIT_FAILURE);
    }; 
    //! test
    printf("%ld:%ld\n",start_time.tv_sec,start_time.tv_usec);

    //TODO: Game Start
    

    return 0;
}