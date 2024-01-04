#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_USER 4
#define BUF_SIZE 1024

int user_count;
char user_name[MAX_USER][9];
int user_sock[MAX_USER];

int main(int argc, char * argv[]){
    user_count = 0;
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

    for(int i=0; i < MAX_USER; i++){
        memset(user_name[i], 0, sizeof(user_name[i]));
    }

    while(user_count < MAX_USER){
        printf("here: %d\n",user_count);
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
        user_count++;
    }

    for(int i=0;i<4;i++){
        printf("%d: %s\n",user_sock[i],user_name[i]);
    }

    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            write(user_sock[i],user_name[j],sizeof(user_name[i]));
        }
    }
    char *name = "board1.json";
    FILE * fp = fopen(name, "rb");
    struct stat fileStat;
    if (stat("board1.json", &fileStat) != 0) {
        fprintf(stderr, "stat\n");
    }
    int data_len = fileStat.st_size;
    printf("filesize: %d\n",data_len);
    for(int i=0;i<4;i++){
        write(user_sock[i], &data_len, sizeof(data_len));
    }
    strcpy(buf,"hello\n");
    for(int i=0;i<4;i++){
        memset(buf,0,strlen(buf));
        write(user_sock[i], &buf, sizeof(buf));
    }
    for(int i=0;i<4;i++){
        close(user_sock[i]);
    } 
}  