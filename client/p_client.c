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

typedef struct Timer{
    GtkWidget *label;
    int counter;
} Timer;

int compare(const void* a, const void* b);
void show_view();
int send_bytes(int fd, void * buf, size_t len);
int recv_bytes(int fd, void * buf, size_t len);
int read_mapinfo(char * map_info);
void print_model();
void init_view(int row, int col);
void update_view();
int find_id(char * nickname);
void * input(void * arg);
void * free_view(void * arg);
char *select_image(int i, int j);

gboolean update_counter(Timer *data);
gboolean update_game_and_score_board();
void on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

//Global
Model map;
char ** view = NULL;
char ** old_view = NULL;
struct timeval start_time;
char myname[MAX_NAME_SIZE+1];
int sock;
int userid;
int modified;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//Global for GTK
GtkWidget *window;
GtkWidget *main_grid;
GtkWidget *board_grid = NULL;
GtkWidget *time_and_score_grid;
GtkWidget *score_label;
GtkWidget *image;
Timer data;
char scoreboard[128];
char time_left[64];
char msg_info[16];

int compare(const void* a, const void* b) {
    User* userA = *(User**)a;
    User* userB = *(User**)b;
    return userB->score - userA->score;
}

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
        if (received == 0)
            return 1 ;
        p += received ;
        acc += received ;
    }
    return 0 ;
}


// if it was succeed return 0, otherwise return 1
int read_mapinfo(char * map_info) {
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
    map.timeout.tv_sec = (j_timeout->valueint)/1000;
    map.timeout.tv_usec = 0;

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
    
    printf("%d\n", map.n_obstacles);
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
    free(json);
    return 0;
}

void print_model() {
    printf("Row: %d\n", map.row);
    printf("Col: %d\n", map.col);

    printf("Timeout: %ld seconds\n", map.timeout.tv_sec);

    printf("Users:\n");
    for (int i = 0; i < 4; i++) {
        printf("  User %d:\n", i + 1);
        printf("    ID: %d\n", map.users[i].id);
        printf("    User X: %d\n", map.users[i].user_x);
        printf("    User Y: %d\n", map.users[i].user_y);
        printf("    Base X: %d\n", map.users[i].base_x);
        printf("    Base Y: %d\n", map.users[i].base_y);
        printf("    Score: %d\n", map.users[i].score);
        printf("    Name: %s\n", map.users[i].name);
    }

    printf("Obstacles:\n");
    for (int i = 0; i < map.n_obstacles; i++) {
        printf("  Obstacle %d: X: %d, Y: %d\n", i + 1, map.obstacles[i].x, map.obstacles[i].y);
    }

    printf("Balls:\n");
    for (int i = 0; i < map.n_balls; i++) {
        printf("  Ball %d: X: %d, Y: %d\n", i + 1, map.balls[i].x, map.balls[i].y);
    }
}



void init_view(int row, int col){
    view = (char **)malloc(row * sizeof(char *));
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
    pthread_mutex_lock(&lock);
    char **temp = view;
    pthread_t free_pid;
    if (pthread_create(&free_pid, NULL, (void*)free_view, (void*)temp)) {
        perror("making thread failed\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(free_pid);

    init_view(map.row, map.col);

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
    modified = 1;
    pthread_mutex_unlock(&lock);
}

// if it was succeed return positive num, otherwise return -1
int find_id(char * nickname) {
    for (int i = 0; i < USER_COUNT; i++) {
        if(strcmp(map.users[i].name, nickname)==0){
            return map.users[i].id;
        }
    }
    return -1;
}

void * input(void * arg) {
    char buf[2];
    int result;
    int k, c, i;

    // receive echoed direction
    while ((result = recv_bytes(sock, buf, 2)) != 1) {
        printf("<<<<<<<------k: %d      buf: %s------>>>>>>>>>\n", k++, buf);

        if (strncmp(buf, "FN", 2) == 0 || strncmp(buf, "TO", 2) == 0) {
            data.counter = -1; 
            break;
        }

        // update user
        if (buf[1] == 'U') {
            // move user
            map.users[buf[0] - '0'].user_y--;
        }
        else if (buf[1] == 'L') {
            map.users[buf[0] - '0'].user_x--;
        }
        else if (buf[1] == 'R') {
            map.users[buf[0] - '0'].user_x++;
        }
        else if (buf[1] == 'D') {
            map.users[buf[0] - '0'].user_y++;
        }

        // update ball
        for (i = 0; i < map.n_balls; i++) {
            if ((map.users[buf[0] - '0'].user_x == map.balls[i].x) && (map.users[buf[0] - '0'].user_y == map.balls[i].y)) {
                // move ball
                if (buf[1] == 'U') {
                    map.balls[i].y--;
                }
                else if (buf[1] == 'L') {
                    map.balls[i].x--;
                }
                else if (buf[1] == 'R') {
                    map.balls[i].x++;
                }
                else if (buf[1] == 'D') {
                    map.balls[i].y++;
                }
                
                // if ball encouter in base
                for (c = 0; c < 4; c++) {
                    if ((map.users[c].base_x == map.balls[i].x) && (map.users[c].base_y == map.balls[i].y)) {
                        // update score;
                        map.users[c].score++;
                        map.balls[i].x = map.balls[map.n_balls - 1].x;
                        map.balls[i].y = map.balls[map.n_balls - 1].y;
                        map.n_balls--;
                    }
                }
                break;
            }
        }
        print_model();
        update_view(); 
        show_view();
    }

    if (result == 1) {
        fprintf(stderr, "receive echoed direction failed\n");
    }
    
    return NULL;
} 

void * free_view(void *arg){
    char ** view_to_free = (char**)arg;
    if(arg == NULL){
        return NULL;
    }
    for(int i = 0; i < map.row; i++) {
        free(view_to_free[i]);
    }
    free(view_to_free);
    return NULL;
}

char *select_image(int i, int j){
    char *selected_image = (char *)malloc(sizeof(char)*32);
    if(view[i][j]=='\0'){
        strcpy(selected_image,"../image/background.png");
    }
    else if(view[i][j]=='O'){
        strcpy(selected_image,"../image/tree.png");
    }
    else if(view[i][j]=='B'){
        strcpy(selected_image,"../image/egg.png");
    }
    else if(view[i][j]=='0'){
        strcpy(selected_image,"../image/player0.png");
    }
    else if(view[i][j]=='1'){
        strcpy(selected_image,"../image/player1.png");
    }
    else if(view[i][j]=='2'){
        strcpy(selected_image,"../image/player2.png");
    }
    else if(view[i][j]=='3'){
        strcpy(selected_image,"../image/player3.png");
    }
    else if(view[i][j]=='4'){
        strcpy(selected_image,"../image/base0.png");
    }
    else if(view[i][j]=='5'){
        strcpy(selected_image,"../image/base1.png");
    }
    else if(view[i][j]=='6'){
        strcpy(selected_image,"../image/base2.png");
    }
    else if(view[i][j]=='7'){
        strcpy(selected_image,"../image/base3.png");
    }
    else{
        strcpy(selected_image,"../image/ground.png");
    } 
    return selected_image;
}

gboolean update_counter(Timer *data) {
    char t_buffer[256];
    if(data->counter > 0){
        data->counter--;
        sprintf(t_buffer, "%d seconds left!!\n", data->counter);
    
        gtk_label_set_text(GTK_LABEL(data->label), t_buffer); 
    }
    else if(data->counter == -1){
        gtk_label_set_text(GTK_LABEL(data->label), "Game Over!!!\n");  
    }

    if (data->counter < 0) {
        return FALSE; 
    } else {
        return TRUE;
    }
}

gboolean update_game_and_score_board() {
    pthread_mutex_lock(&lock);
    if(modified == 0) {
        pthread_mutex_unlock(&lock); 
        return TRUE;
    }
    for (int i = 0; i < map.row; i++) {
        for (int j = 0; j < map.col; j++) {
            if(old_view[i][j] != view[i][j]){
                image = gtk_grid_get_child_at(GTK_GRID(board_grid), j, i);
                char *selected_image = select_image(i,j);
                gtk_image_set_from_file(GTK_IMAGE(image), selected_image);
                free(selected_image);
            }
        }
    }
    char **temp = old_view;
    pthread_t free_pid;
    if (pthread_create(&free_pid, NULL, (void*)free_view, (void*)temp)) {
        perror("making thread failed\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(free_pid);

    old_view = view;
    view = NULL;
    modified = 0;

    User * score_chart[USER_COUNT];
    for (int i = 0; i < USER_COUNT; i++) {
        score_chart[i] = &map.users[i];
    }
    qsort(score_chart, 4, sizeof(User*), compare);
    sprintf(scoreboard,"****Score Board****\n 1. %s: %d\n 2. %s: %d\n 3. %s: %d\n 4. %s: %d\n", score_chart[0]->name,score_chart[0]->score,score_chart[1]->name,score_chart[1]->score,score_chart[2]->name,score_chart[2]->score,score_chart[3]->name,score_chart[3]->score);
    gtk_label_set_text(GTK_LABEL(score_label), scoreboard);
    pthread_mutex_unlock(&lock);
    return TRUE;
}

void on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    char cmd[3];
    printf("%d\n", userid);

    switch (event->keyval) {
        case GDK_KEY_Up:
            g_print("Up key pressed\n");
            snprintf(cmd, 3, "%dU", userid);
            break;
        case GDK_KEY_Down:
            g_print("Down key pressed\n");
            snprintf(cmd, 3, "%dD", userid);
            break;
        case GDK_KEY_Left:
            g_print("Left key pressed\n");
            snprintf(cmd, 3, "%dL", userid);
            break;
        case GDK_KEY_Right:
            g_print("Right key pressed\n");
            snprintf(cmd, 3, "%dR", userid);
            break;
        default:
            break;
    }

    // send cmd to server
    if (send_bytes(sock, cmd, 2) == 1) {
        fprintf(stderr, "send key failed\n");
    }
    printf("msg sent: %s\n", cmd);
}


int main (int argc, char * argv[])
{
    int filesize;
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
    
    //JSON 파일 크기 수신
    if(recv_bytes(sock, &filesize, sizeof(filesize))){
        fprintf(stderr,"Failed to receive filesize\n");
        exit(EXIT_FAILURE);
    };
    //! test
    printf("filesize: %d\n",filesize);
    
    
    char map_info[filesize+1];
    char * p = map_info ;
    size_t acc = 0 ;
    while (acc < filesize) {
        size_t received ;
        received = recv(sock, p, filesize - acc, MSG_NOSIGNAL) ;
        // printf("%ld: %s\n",received,(char*)buf);
        if (received == 0)
            break;
        p += received ;
        acc += received ;
    }
    map_info[filesize] = '\0';
    printf("%ld\n",acc);
    printf("%s\n",map_info);
     

    //* JSON 파싱하고 모델 생성
    read_mapinfo(map_info);
    print_model();
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

    userid = find_id(myname); 

    //* Game Start
    //initialize board
    gtk_init(&argc, &argv);
    //window setting
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "PushPush");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    //make main grid
    main_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), main_grid);
    //make board grid
    board_grid = gtk_grid_new();
    gtk_widget_set_size_request(board_grid, 200, 200);
    gtk_grid_attach(GTK_GRID(main_grid), board_grid, 0, 0, 1, 4);
    for (int i = 0; i < map.row; i++) {
        for (int j = 0; j < map.col; j++) {
            char *selected_image = select_image(i,j);
            image = gtk_image_new_from_file(selected_image);
            gtk_grid_attach(GTK_GRID(board_grid), image, j, i, 1, 1);
            free(selected_image);
        }
    }
    old_view = view;
    view = NULL;
    modified = 0;

    //make timer
    sprintf(time_left,"%ld seconds left!!\n",map.timeout.tv_usec);
    data.label = gtk_label_new(time_left);
    data.counter = (int)map.timeout.tv_sec;
    gtk_grid_attach(GTK_GRID(main_grid), data.label, 2, 1, 1, 1);
    g_timeout_add_seconds(1, (GSourceFunc)update_counter, &data);

    //show icon
    if(userid==0){
        image = gtk_image_new_from_file("../image/player0.png");
    }
    else if(userid==1){
        image = gtk_image_new_from_file("../image/player1.png");
    }
    else if(userid==2){
        image = gtk_image_new_from_file("../image/player2.png");
    }
    else if(userid==3){
        image = gtk_image_new_from_file("../image/player3.png");
    }
    gtk_grid_attach(GTK_GRID(main_grid), image, 2, 2, 1, 1); 

    //make score board
    sprintf(scoreboard,"****Score Board****\n 1. %s: %d\n 2. %s: %d\n 3. %s: %d\n 4. %s: %d\n", map.users[0].name,map.users[0].score,map.users[1].name,map.users[1].score,map.users[2].name,map.users[2].score,map.users[3].name,map.users[3].score);
    score_label = gtk_label_new(scoreboard);
    gtk_grid_attach(GTK_GRID(main_grid), score_label, 2, 3, 1, 1);
    g_timeout_add(1,(GSourceFunc)update_game_and_score_board,NULL);
    gtk_widget_show_all(window);
    gtk_widget_grab_focus(window);
    pthread_t input_pid;
    if (pthread_create(&input_pid, NULL, (void*)input, NULL)) {
        perror("making thread failed\n");
        exit(EXIT_FAILURE);
    }
    gtk_main();
    pthread_join(input_pid, NULL);
    close(sock);
    printf("THE END\n");
    return 0;
}