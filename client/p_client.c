#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define USER_COUNT 4
#define MAX_NAME_SIZE 8
#define BUF_SIZE 9

int send_bytes(int fd, void * buf, size_t len);
int recv_bytes(int fd, void * buf, size_t len);

typedef struct User{
    int x;
    int y;
    int base_x;
    int base_y;
    int id;
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
    int n_onstacles;
    Pos* balls;
    int n_balls;
}Model;

//Global
Model model;
char myname[MAX_NAME_SIZE+1];
char* usernames[USER_COUNT];

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

    //JSON 파일 크기 수신
    if(recv_bytes(sock, &filesize, sizeof(filesize))){
        fprintf(stderr,"Failed to receive filesize\n");
        exit(EXIT_FAILURE);
    };
    //JSON 파일 수신
    FILE * fp = fopen("map.json", "wb");
    int received = 0;
    while(received < filesize){
        received += recv(sock, buf, sizeof(buf), MSG_NOSIGNAL) ;
        fwrite(buf,sizeof(buf),1,fp);
    }
    fclose(fp);

    // timestamp 수신
    if(recv_bytes(sock, &model.timeout, sizeof(model.timeout))){
        fprintf(stderr,"Failed to receive timeout\n");
        exit(EXIT_FAILURE);
    }; 

    // pthread_t tid[2];
    return 0;
}