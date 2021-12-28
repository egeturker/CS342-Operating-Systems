#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <limits.h>

int main(int argc, char *argv[]){

    int m = atoi(argv[1]);
    int line;
    char fileName[50];
    char fileNameFormatted[50];
    strcpy(fileName, argv[2]);
    FILE *fp;

    for(int i = 0; i < m; i++){
        line = 20 + rand() % 10;

        strcpy(fileNameFormatted, fileName);
        strcat(fileNameFormatted,"-");
        char fileNo[5];
        snprintf(fileNo,5,"%d",i + 1);
        strcat(fileNameFormatted,fileNo);
        strcat(fileNameFormatted,".txt");
        fp = fopen(fileNameFormatted,"w");

        for(int j = 0; j < line; j++){
            fprintf(fp,"%d ", 100 + (rand() % 1000));
            fprintf(fp,"%d\n", 100 + (rand() % 1000));

        }
    }
    return(0);
}