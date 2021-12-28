#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <limits.h>

#define MAX_THREADS 10
pthread_mutex_t lock;
pthread_cond_t cond;
int* vruntime;
int* waitingtime;


struct rqNode{
    struct rqNode* next;
    int threadIndex;
    int burstIndex;
    int length;
    unsigned long time_ms;
};
struct rqNode* globalHead;

struct arg{
    int bCount;
    int index;
    int avgA;
    int avgB;
    int minA;
    int minB;
};

struct arg2{
    int numThreads;
    char algorithm[10];
    int totalBursts;
};

struct arg_file{
    int* interarrival;
    int* burstLength;
    int index;
    int burstCount;
};

int rand_exp( double avg){
    double x;
    double lambda = 1 / avg;
    x = rand() /  (RAND_MAX + 1.0);
    return (int)(-log(1 - x) / lambda);
}

//Inserts the burst to the runqueue and returns the head
struct rqNode* insertrq(struct rqNode* head, int threadIndex, int burstIndex, int length, unsigned long time_ms){
    
    struct rqNode *new = (struct rqNode*)malloc(sizeof(struct rqNode));
    new->next = NULL;
    new->threadIndex = threadIndex;
    new->burstIndex = burstIndex;
    new->length = length;
    new->time_ms = time_ms;
    
    if(head == NULL){
        head = new;
        return head;
    } else{
        struct rqNode* temp;
        temp = head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = new;
        return head;
    }
}

//simulates the burst with the given algorithm
void execute(char *algorithm, int numThreads, int totalBursts){
    int executeTime;
    int threadIndex;
    int burstIndex;
    int time_ms;
    struct rqNode* toBeExecuted;
    struct rqNode* temp;
    struct rqNode* prev;
    struct timeval tv;
    int curr_time;

    if(strcmp(algorithm,"FCFS") == 0){
        
        //removes the head from rq and simulates it
        toBeExecuted = globalHead;
        globalHead = globalHead->next;
        executeTime = toBeExecuted->length;
        threadIndex = toBeExecuted->threadIndex;
        burstIndex = toBeExecuted->burstIndex;
        time_ms = toBeExecuted->time_ms;
        free(toBeExecuted);
        pthread_mutex_unlock(&lock);

        printf("Executing -> Thread %d Burst %d \n",threadIndex,burstIndex);

        //record the waiting time of the burst
        gettimeofday(&tv,NULL);
        curr_time = (tv.tv_sec * 1000 + tv.tv_usec/1000); //current time to ms
        waitingtime[threadIndex - 1] += (curr_time - time_ms);
        

        usleep( 1000 * executeTime);//simulation
    }
    else if(strcmp(algorithm,"SJF") == 0){
        int* earliest = (int*)malloc(numThreads * sizeof(int));
        int* time = (int*)malloc(numThreads * sizeof(int));

        for(int i = 0; i < numThreads; i++){
            earliest[i] = -1;
            time[i] = -1;
        }

        //finds the earliest burst of each thread, if all bursts of a thread is completed
        //it is assigned to RAND_MAX
        for(int i = 0; i < numThreads; i++){
            temp = globalHead;
            earliest[i] = RAND_MAX;
            while(temp != NULL){
                if((temp->burstIndex < earliest[i])&&(temp->threadIndex == (i+1))){
                    earliest[i] = temp->burstIndex;
                    time[i] = temp->length;
                }
                temp = temp->next;
            }
        }
        
        int shortest = RAND_MAX;
        int shortestIndex = -1;
        //find the index of the shortest thread
        for(int i = 0; i < numThreads; i++){
            //if no burst remains from current thread
            if(earliest[i] == RAND_MAX)
                continue;
    
            if(time[i] < shortest){
                shortest = time[i];
                shortestIndex = i;
            }
        }

        //initialize the index of the shortest of earliest
        int removalIndexBurst = earliest[shortestIndex];
        int removalIndexThread = (shortestIndex + 1);

        temp = globalHead;
        prev = NULL;

        //remove the burst from rq
        if((temp->threadIndex == removalIndexThread)
        && temp->burstIndex == removalIndexBurst){
            executeTime = temp->length;
            time_ms = temp->time_ms;
            globalHead = globalHead->next;
            free(temp);
        }
        else{
            while(temp != NULL){
                if((temp->threadIndex == removalIndexThread)
                && (temp->burstIndex == removalIndexBurst)){
                    executeTime = temp->length;
                    time_ms = temp->time_ms;
                    prev->next = temp->next;
                    free(temp);
                    break;
                }
                    prev = temp;
                    temp = temp ->next;
            }
        }

        free(earliest);
        free(time);
        printf("Executing -> Thread %d Burst %d \n",removalIndexThread,removalIndexBurst);

        pthread_mutex_unlock(&lock);

        //record the waiting time of the burst
        gettimeofday(&tv,NULL);
        curr_time = (tv.tv_sec * 1000 + tv.tv_usec/1000);
        waitingtime[removalIndexThread - 1] += (curr_time - time_ms);

        usleep( 1000 * executeTime);//simulate
    }
    else if(strcmp(algorithm,"PRIO") == 0){
        int* earliest = (int*)malloc(numThreads * sizeof(int));

        for(int i = 0; i < numThreads; i++){
            earliest[i] = -1;
        }

        //finds the earliest burst of each thread, if all bursts of a thread is completed
        //it is assigned to RAND_MAX
        for(int i = 0; i < numThreads; i++){
            temp = globalHead;
            earliest[i] = RAND_MAX;
            while(temp != NULL){
                if((temp->burstIndex < earliest[i])&&(temp->threadIndex == (i+1))){
                    earliest[i] = temp->burstIndex;
                }
                temp = temp->next;
            }
        }

        int prioIndex = -1;
        //select the thread with highest prio
        for(int i = 0; i < numThreads; i++){
            if(earliest[i] == RAND_MAX)
                continue;
            else{
                prioIndex = i;
                break;
            }
        }

        //init the burst and thread index values for removal
        int removalIndexBurst = earliest[prioIndex];
        int removalIndexThread = (prioIndex + 1);


        temp = globalHead;
        prev = NULL;

        //remove the burst from rq
        if((temp->threadIndex == removalIndexThread)
        && temp->burstIndex == removalIndexBurst){
            executeTime = temp->length;
            time_ms = temp->time_ms;
            globalHead = globalHead->next;
            free(temp);
        }
        else{
            while(temp != NULL){
                if((temp->threadIndex == removalIndexThread)
                && (temp->burstIndex == removalIndexBurst)){
                    executeTime = temp->length;
                    time_ms = temp->time_ms;
                    prev->next = temp->next;
                    free(temp);
                    break;
                }
                    prev = temp;
                    temp = temp ->next;
            }
        }

        free(earliest);
        printf("Executing -> Thread %d Burst %d \n",removalIndexThread,removalIndexBurst);
        pthread_mutex_unlock(&lock);

        //record the waiting time for burst
        gettimeofday(&tv,NULL);
        curr_time = (tv.tv_sec * 1000 + tv.tv_usec/1000);
        waitingtime[removalIndexThread - 1] += (curr_time - time_ms);

        usleep( 1000 * executeTime);//simulate

    }
    else if(strcmp(algorithm,"VRUNTIME") == 0){
        int* earliest = (int*)malloc(numThreads * sizeof(int));
        for(int i = 0; i < numThreads; i++){
            earliest[i] = -1;
        }

        if(!vruntime){
            vruntime = (int*)malloc(numThreads * sizeof(int));
            for(int i = 0; i < numThreads; i++){
                vruntime[i] = 0;
            }
        }

        //finds the earliest burst of each thread, if all bursts of a thread is completed
        //it is assigned to RAND_MAX
        for(int i = 0; i < numThreads; i++){
            temp = globalHead;
            earliest[i] = RAND_MAX;
            while(temp != NULL){
                if((temp->burstIndex < earliest[i])&&(temp->threadIndex == (i+1))){
                    earliest[i] = temp->burstIndex;
                }
                temp = temp->next;
            }
        }

        int vrunIndex = 0;
        int smallestVRT = RAND_MAX;

        //pick the smallest vruntime
        for(int i = 0; i < numThreads; i++){
            if(earliest[i] == RAND_MAX)
                continue;
            if(vruntime[i] < smallestVRT){
                smallestVRT = vruntime[i];
                vrunIndex = i;
            }
        }

        //init the burst and thread index values for removal
        int removalIndexBurst = earliest[vrunIndex];
        int removalIndexThread = (vrunIndex + 1);

        temp = globalHead;
        prev = NULL;

        //remove from rq
        if((temp->threadIndex == removalIndexThread)
        && temp->burstIndex == removalIndexBurst){
            executeTime = temp->length;
            time_ms = temp->time_ms;
            globalHead = globalHead->next;
            free(temp);
        }
        else{
            while(temp != NULL){
                if((temp->threadIndex == removalIndexThread)
                && (temp->burstIndex == removalIndexBurst)){
                    executeTime = temp->length;
                    time_ms = temp->time_ms;
                    prev->next = temp->next;
                    free(temp);
                    break;
                }
                    prev = temp;
                    temp = temp ->next;
            }
        }


        //compute new vrt
        vruntime[vrunIndex] += executeTime * (0.7 + 0.3*(removalIndexThread));

        free(earliest);
        printf("Executing -> Thread %d Burst %d \n",removalIndexThread,removalIndexBurst);
        pthread_mutex_unlock(&lock);

        gettimeofday(&tv,NULL);
        curr_time = (tv.tv_sec * 1000 + tv.tv_usec/1000);
        waitingtime[removalIndexThread - 1] += (curr_time - time_ms);

        usleep( 1000 * executeTime);
    }
    else{
        printf("Invaild algorithm input.\n");
    }
}

void* scheduler(void *arg_ptr){
    int executedBursts = 0;
    int totalBursts = ((struct arg2*)arg_ptr)->totalBursts;
    int numThreads = ((struct arg2*)arg_ptr)->numThreads;
    char algorithm[10];
    strcpy(algorithm,((struct arg2*)arg_ptr)->algorithm);

    while (totalBursts > executedBursts){
        pthread_mutex_lock(&lock);
        while(globalHead == NULL)
            pthread_cond_wait(&cond,&lock);
        
        execute(algorithm,numThreads,totalBursts);
        executedBursts++;
    }
}

//create burst randomly
void* burst(void *arg_ptr){
    int index = ((struct arg*)arg_ptr)->index;
    double avgA = ((struct arg*)arg_ptr)->avgA;
    double avgB = ((struct arg*)arg_ptr)->avgB;
    double minA = ((struct arg*)arg_ptr)->minA;
    double minB = ((struct arg*)arg_ptr)->minB;
    int bCount = ((struct arg*)arg_ptr)->bCount;
    int burstIndex = 1;
    double length;
    double sleepLength;
    double initSleep;
    struct timeval tv;

    unsigned long seed = (getpid() + time(NULL));
    srand(seed);

    do{
        initSleep = rand_exp(avgA);
    }while(initSleep < minA);

    usleep(1000 * initSleep);

    while(burstIndex < (bCount+1)){
        do{
            length = rand_exp(avgB);
        }while(length < minB);

        pthread_mutex_lock(&lock);
        gettimeofday(&tv,NULL);
        unsigned long burstTime = (tv.tv_sec * 1000 + tv.tv_usec/1000);
        globalHead = insertrq(globalHead, index, burstIndex, length, burstTime);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
        burstIndex++;

        do{
            sleepLength = rand_exp(avgA);
        }while(sleepLength < minA);

        usleep(sleepLength);
    }
}

//create burst from file info
void* burstFile(void *arg_ptr){
    int index = ((struct arg_file*)arg_ptr)->index;
    int* length = ((struct arg_file*)arg_ptr)->burstLength;
    int* sleepLength = ((struct arg_file*)arg_ptr)->interarrival;
    int burstCount = ((struct arg_file*)arg_ptr)->burstCount;

    int burstIndex = 1;
    struct timeval tv;
    
    usleep(1000 * sleepLength[0]);

    while(burstIndex < (burstCount+1)){
        pthread_mutex_lock(&lock);
        gettimeofday(&tv,NULL);
        unsigned long burstTime = (tv.tv_sec * 1000 + tv.tv_usec/1000);
        globalHead = insertrq(globalHead, index, burstIndex, length[burstIndex - 1], burstTime);
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);

        usleep(1000 * sleepLength[burstIndex]);
        burstIndex++;
    }
}


int main( int argc, char *argv[]){
    srand(time(NULL));

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond,NULL);

    if(strcmp(argv[3],"-f") == 0){
        char fileName[100];
        char algorithm[10];
        int number_of_threads;
        strcpy(algorithm,argv[2]);
        number_of_threads = atoi(argv[1]);
        strcpy(fileName,argv[4]);

        FILE *fp;
        int totalLine = 0;

        pthread_t s_thread;
        pthread_t w_thread_ids[MAX_THREADS];
        struct arg_file w_thread_args[MAX_THREADS];
        struct arg2* s_thread_args;

        waitingtime = (int*)malloc(number_of_threads* sizeof(int));
        for(int i = 0; i < number_of_threads; i++){
            waitingtime[i] = 0;
        }
        
        char fileNameFormatted[100];
        for(int i = 0; i < number_of_threads; i++){
            strcpy(fileNameFormatted, fileName);
            strcat(fileNameFormatted,"-");
            char fileNo[5];
            snprintf(fileNo,5,"%d",i + 1);
            strcat(fileNameFormatted,fileNo);
            strcat(fileNameFormatted,".txt");
            fp = fopen(fileNameFormatted,"r");

            int c;
            int lineC = 0;
            while(!feof(fp)){
                c = fgetc(fp);
                if (c == '\n')
                    lineC++;
            }
            
            totalLine += lineC;

            w_thread_args[i].burstLength = (int*)malloc(sizeof(int) * lineC);
            w_thread_args[i].interarrival = (int*)malloc(sizeof(int) * lineC);
            w_thread_args[i].index = (i + 1);
            

            fp = fopen(fileNameFormatted,"r");

            int x = 0;
            

            for(int j=0; j < lineC; j++){
                fscanf(fp,"%d",&x);
                w_thread_args[i].interarrival[j] = x;
                fscanf(fp,"%d",&x);
                w_thread_args[i].burstLength[j] = x;
            }
            w_thread_args[i].burstCount = lineC;
            
            pthread_create(&(w_thread_ids[i]), NULL, burstFile, (void *) &(w_thread_args[i]));
        }

        s_thread_args = malloc(sizeof(struct arg2));
        s_thread_args->numThreads = number_of_threads;
        s_thread_args->totalBursts = totalLine;
        strcpy(s_thread_args->algorithm,algorithm);
        pthread_create(&s_thread, NULL, scheduler, (void*)s_thread_args);

        


        pthread_join(s_thread,NULL);

        for(int i = 0; i < number_of_threads; i++){
            pthread_join(w_thread_ids[i],NULL);
        }

        pthread_mutex_destroy(&lock);

        double avg;
        double avgofall = 0;
        for(int i = 0; i < number_of_threads; i++){
            avg = (double)waitingtime[i]/(double)totalLine;
            printf("Average waiting time for Thread %d -> %.2f ms\n",i +1,avg);
            avgofall += avg;
        }
        printf("Average waiting time  -> %.2f ms\n",avgofall / number_of_threads);


        if(!vruntime){
            free(vruntime);
        }
        free(waitingtime);
        free(s_thread_args);
        
    } else {
        pthread_t s_thread;
        pthread_t w_thread_ids[MAX_THREADS];
        struct arg w_thread_args[MAX_THREADS];
        struct arg2* s_thread_args;
    
        int number_of_threads;
        int bCount;
        double avgA;
        double avgB;
        double minA;
        double minB;
        char algorithm[10];
        if(argc != 8){
            printf("Incorrect arguments. \n./schedule <N> <minB> <avgB> <minA> <avgA> <ALG> \n");
            return(-1);
        }

        number_of_threads = atoi(argv[2]);
        if(number_of_threads > 10 || number_of_threads < 1){
            printf("Number of threads(N) can be a value between 1 and 10.");
            return(-1);
        }   

        bCount = atoi(argv[1]);
        minB = atof(argv[3]);
        avgB = atof(argv[4]);
        minA = atof(argv[5]);
        avgA = atof(argv[6]);
        strcpy(algorithm,argv[7]);

        waitingtime = (int*)malloc(number_of_threads* sizeof(int));
        for(int i = 0; i < number_of_threads; i++){
            waitingtime[i] = 0;
        }

        pthread_mutex_init(&lock, NULL);
        pthread_cond_init(&cond,NULL);

        s_thread_args = malloc(sizeof(struct arg2));
        s_thread_args->numThreads = number_of_threads;
        s_thread_args->totalBursts = bCount * number_of_threads;
        strcpy(s_thread_args->algorithm,algorithm);
        pthread_create(&s_thread, NULL, scheduler, (void*)s_thread_args);

        for(int i = 0; i < number_of_threads; i++){
            w_thread_args[i].index = i + 1;
            w_thread_args[i].bCount = bCount;
            w_thread_args[i].avgA = avgA;
            w_thread_args[i].avgB = avgB;
            w_thread_args[i].minA = minA;
            w_thread_args[i].minB = minB;  
            pthread_create(&(w_thread_ids[i]), NULL, burst, (void *) &(w_thread_args[i]));
        }
        pthread_join(s_thread,NULL);

        for(int i = 0; i < number_of_threads; i++){
            pthread_join(w_thread_ids[i],NULL);
        }

        pthread_mutex_destroy(&lock);

        struct rqNode* temp = globalHead;
        while(temp != NULL){
            printf("%d,%d,%d,%lu \n",temp->threadIndex,temp->burstIndex,temp->length,temp->time_ms);
            temp = temp->next;
        }

        double avg;
        double avgofall = 0;
        for(int i = 0; i < number_of_threads; i++){
            avg = (double)waitingtime[i]/(double)bCount;
            printf("Average waiting time for Thread %d -> %.2f ms\n",i +1,avg);
            avgofall += avg;
        }

        printf("Average waiting time  -> %.2f ms\n",avgofall / number_of_threads);
        
        if(!vruntime){
            free(vruntime);
        }
        free(waitingtime);
        free(s_thread_args);
    }  

}