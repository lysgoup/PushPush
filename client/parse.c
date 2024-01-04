#include "cJSON/cJSON.c"
#include "cJSON/cJSON.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define USER_COUNT 4
#define MAX_NAME_SIZE 8
#define BUF_SIZE 9

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

Model model;

void parse_json(int filesize){
    char jsonfile[filesize];
    FILE * fp = fopen("map.json", "rb");
    size_t result = fread(jsonfile, 1, filesize, fp);
    printf("%ld\n",result);
    printf("%s",jsonfile);
    fclose(fp); 
}

int main(int argc, char * argv[]){
    struct stat fileStat;
    if (stat("map.json", &fileStat) != 0) {
        fprintf(stderr, "stat\n");
    }
    int data_len = fileStat.st_size;
    printf("filesize: %d\n",data_len);
    parse_json(data_len);
}