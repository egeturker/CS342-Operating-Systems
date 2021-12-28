#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[]){

    int m = atoi(argv[1]);
    char tmp;
    
    for(int i = 0; i < m; i++){
        read(STDIN_FILENO, &tmp, 1);
    }

    return(0);

}