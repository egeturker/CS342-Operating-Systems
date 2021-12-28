#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>

#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[]){

    int mode = atoi(argv[2]);
    int n = atoi(argv[1]);
    char buffer[n];

    if (argc != 3){
        printf("%s", "Run with arguments <N> <mode>");
        return -1;
    }

    if((mode != 1) && (mode != 2) ){
        printf("%s", "Mode can be 1 -> normal mode or 2 -> tapped mode");
        return -1;
    }

    struct timeval start, end;
    double execution_time;
    
  
    char input[50];
    char command1[50] ;
    char command2[50] ;
    char* token;
    pid_t child1, child2;

    //Mode 1 - Normal Mode
    if (mode == 1){
        //Ask for input
        printf("Input the shell command: \n");
        do{

            //Get input
            fgets( input, 50, stdin);

            //If pipe needed
            if(strstr(input, "|") != NULL){
                
                gettimeofday(&start, NULL);
                //Create pipe
                int fd[2];
                pipe(fd);

                //Split the input into two commands
                token = strtok(input, "|");
                strcpy(command1, token);
                token = strtok(NULL, "|");
                strcpy(command2, token);


                //Remove unnecessary spaces from the commands
                while ( *command1 == ' '){
                    memmove(command1, command1 + 1, strlen(command1));
                }   
                while ( *command2 == ' '){
                    memmove(command2, command2 + 1, strlen(command2));
                }

                //Remove \n character from the commands
                command1[strcspn(command1, "\n")] = 0;
                command2[strcspn(command2, "\n")] = 0;


                //Create child1
                child1 = fork();

                //Enter child1
                if (child1 == 0){
                    //Direct I/O through the pipe
                    dup2(fd[WRITE_END], STDOUT_FILENO);
                    close(fd[READ_END]);
                    close(fd[WRITE_END]);

                    //Split command1 to its arguments
                    char* arguments[50];
                    int i = 0;
                    token = strtok(command1, " ");
                    while( token != NULL){
                        arguments[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }
                    //Add NULL to the end of the array for execvp
                    arguments[i] = NULL;

                    //Run command 1
                    execvp(arguments[0],arguments);

                    exit(1);

                }else{
                    //Create child2
                    child2 = fork();
                    
                    //Enter child2
                    if (child2 == 0){

                        //Direct I/O through the pipe
                        dup2(fd[READ_END], STDIN_FILENO);
                        close(fd[READ_END]);
                        close(fd[WRITE_END]);

                        //Split command2 to its arguments
                        char* arguments2[50];
                        int i = 0;
                        token = strtok(command2, " ");
                        while( token != NULL){
                            arguments2[i] = token;
                            token = strtok(NULL, " ");
                            i++;
                        }

                        //Add NULL to the end of the array for execvp
                        arguments2[i] = NULL;

                        //Run command 2
                        execvp(arguments2[0],arguments2);

                        exit(1);
                    }
                    //Parent
                    else{

                        int status;
                        //Close pipe in the parent
                        close(fd[READ_END]);
                        close(fd[WRITE_END]);

                        //Wait for child processes to terminate
                        waitpid(child1, &status, 0);
                        waitpid(child2, &status, 0);

                        gettimeofday(&end, NULL);

                        execution_time = (end.tv_sec - start.tv_sec) * 1e6;
                        execution_time = (execution_time + (end.tv_usec - start.tv_usec)) * 1e-6;

                        printf("Execution time: %f \n", execution_time);

                        //Ask for input
                        if (strcmp(input,"-1\n") != 0)
                            printf("Input the shell command: \n"); 
                        else  
                            printf("Goodbye !\n");
                    }
                }   
            }
            else{
                gettimeofday(&start, NULL);
                //Pipe not needed
                strcpy(command1, input);
                while ( *command1 == ' '){
                    memmove(command1, command1 + 1, strlen(command1));
                }   
                command1[strcspn(command1, "\n")] = 0;

                child1 = fork();

                if (child1 == 0){
                    char* arguments[50];
                    int i = 0;
                    token = strtok(command1, " ");
                    while( token != NULL){
                        arguments[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }
                    arguments[i] = NULL;

                    execvp(arguments[0],arguments);
                                
                }else {
                    int status;
                    waitpid(child1, &status, 0); 
                    //waitpid(child2, &status, 0);

                    gettimeofday(&end, NULL);

                    execution_time = (end.tv_sec - start.tv_sec) * 1e6;
                    execution_time = (execution_time + (end.tv_usec - start.tv_usec)) * 1e-6;

                    printf("Execution time: %f \n", execution_time);

                    if (strcmp(input,"-1\n") != 0)
                    
                        printf("Input the shell command: \n"); 
                    else  
                        printf("Goodbye ! \n");
                }
            }
        }while (strcmp(input,"-1\n") != 0);

        return 0;
    }

    if (mode == 2){
        //Ask for input
        printf("Input the shell command: \n");
        do{

            //Get input
            fgets( input, 50, stdin);
            
            //If pipe needed
            gettimeofday(&start, NULL);

            if(strstr(input, "|") != NULL){
                //Create pipe
                int pipe1[2];
                int pipe2[2];
                pipe(pipe1);
                pipe(pipe2);

                //Split the input into two commands
                token = strtok(input, "|");
                strcpy(command1, token);
                token = strtok(NULL, "|");
                strcpy(command2, token);


                //Remove unnecessary spaces from the commands
                while ( *command1 == ' '){
                    memmove(command1, command1 + 1, strlen(command1));
                }   
                while ( *command2 == ' '){
                    memmove(command2, command2 + 1, strlen(command2));
                }

                //Remove \n character from the commands
                command1[strcspn(command1, "\n")] = 0;
                command2[strcspn(command2, "\n")] = 0;


                //Create child1
                child1 = fork();

                //Enter child1
                if (child1 == 0){
                    //Direct I/O through the pipe
                    dup2(pipe1[WRITE_END], STDOUT_FILENO);

                    close(pipe1[READ_END]);
                    close(pipe1[WRITE_END]);
                    close(pipe2[READ_END]);
                    close(pipe2[WRITE_END]);

                                     

                    //Split command1 to its arguments
                    char* arguments[50];
                    int i = 0;
                    token = strtok(command1, " ");
                    while( token != NULL){
                        arguments[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }
                    //Add NULL to the end of the array for execvp
                    arguments[i] = NULL;

                    //Run command 1
                    execvp(arguments[0],arguments);

                    exit(1);

                }else{
                    //Create child2
                    child2 = fork();
                    
                    //Enter child2
                    if (child2 == 0){

                        //Direct I/O through the pipe
                        dup2(pipe2[READ_END], STDIN_FILENO);
                        close(pipe1[READ_END]);
                        close(pipe1[WRITE_END]);
                        close(pipe2[READ_END]);
                        close(pipe2[WRITE_END]);

                        //Split command2 to its arguments
                        char* arguments2[50];
                        int i = 0;
                        token = strtok(command2, " ");
                        while( token != NULL){
                            arguments2[i] = token;
                            token = strtok(NULL, " ");
                            i++;
                        }

                        //Add NULL to the end of the array for execvp
                        arguments2[i] = NULL;

                        //Run command 2
                        execvp(arguments2[0],arguments2);

                        exit(1);
                    }
                    //Parent
                    else{
                        int status;
                        ssize_t nbytes, nbytes2;
                        int writeCalls = 0;
                        int readCalls = 0;
                        ssize_t charCount = 0;
                        

                        close(pipe1[WRITE_END]);
                        close(pipe2[READ_END]);
                        
                        while((nbytes = read(pipe1[READ_END], buffer, n)) > 0 &&
                            (nbytes2 = write(pipe2[WRITE_END], buffer,nbytes)) > 0)
                        {
                            writeCalls++;
                            readCalls++;
                            charCount += nbytes;     
                        }

                        close(pipe1[READ_END]);
                        close(pipe2[WRITE_END]);

                        waitpid(child1, &status, 0); 
                        waitpid(child2, &status, 0); 

                        printf("Write Call Count: %d\n",writeCalls );
                        printf("Read Call Count: %d\n",readCalls );
                        printf("Character Count: %ld\n",charCount );

                        gettimeofday(&end, NULL);

                        execution_time = (end.tv_sec - start.tv_sec) * 1e6;
                        execution_time = (execution_time + (end.tv_usec - start.tv_usec)) * 1e-6;

                        printf("Execution time: %f \n", execution_time);

                        //Ask for input
                        if (strcmp(input,"-1\n") != 0)
                            printf("Input the shell command: \n"); 
                        else  
                            printf("Goodbye !\n");
                    }
                }   
            }
            else{
                //Pipe not needed

                gettimeofday(&start, NULL);

                strcpy(command1, input);
                while ( *command1 == ' '){
                    memmove(command1, command1 + 1, strlen(command1));
                }   
                command1[strcspn(command1, "\n")] = 0;

                child1 = fork();

                if (child1 == 0){
                    char* arguments[50];
                    int i = 0;
                    token = strtok(command1, " ");
                    while( token != NULL){
                        arguments[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }
                    arguments[i] = NULL;

                    execvp(arguments[0],arguments);
                                
                }else {
                    int status;
                    waitpid(child1, &status, 0); 
                    waitpid(child2, &status, 0);

                    gettimeofday(&end, NULL);

                    execution_time = (end.tv_sec - start.tv_sec) * 1e6;
                    execution_time = (execution_time + (end.tv_usec - start.tv_usec)) * 1e-6;

                    printf("Execution time: %f \n", execution_time);

                    if (strcmp(input,"-1\n") != 0)
                    
                        printf("Input the shell command: \n"); 
                    else  
                        printf("Goodbye ! \n");
                }
            }
        }while (strcmp(input,"-1\n") != 0);

        return 0;
    }
}
