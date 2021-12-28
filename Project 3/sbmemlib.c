#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "sbmem.h"
#include <math.h>
#include <stdint.h>

#define MIN_REQUEST 128
#define OVERHEAD 8

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
const char* smname = "/MEM";
const char* semaphorename = "/SEM";

// Define semaphore(s)
sem_t* sem;
// Define your stuctures and variables. 
int shm_fd;
pid_t p;
void* addr;
int globalsize = 32768;
int* valp;
int ret;

struct memlist{
    struct memlist *prev, *next;
};

struct memlist* buddy_list; 
uint8_t* buddy_split;
uint8_t *base;
uint8_t *max;
int buddy_lim = 8;
int bookkeep_total;
int bookkeep1;
int bookkeep2;
int buddy_count;

void init_buddy_list(struct memlist* list){
    list->prev = list;
    list->next = list;
}

void push_node(struct memlist* list, struct memlist* node){
    struct memlist* prev = list->prev;    
    node->prev = prev;
    node->next = list;
    prev->next = node;
    list->prev = node;
}

struct memlist* pop_node(struct memlist* list){
    struct memlist* popped = list->prev;
    if(popped == list)
        return NULL;
    else{
        remove_node(popped);
        return popped;
    }
}

void remove_node(struct memlist* node){
    struct memlist* prev = node->prev;
    struct memlist* next = node->next;
    prev->next = next;
    next->prev = prev;
}

uint8_t* get_node_pointer(int i, int buddy){
    shm_fd = shm_open(smname, O_RDWR, 0666);
    base = mmap(0,globalsize,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,8192);
    return base + ((i - (1 << buddy) + 1) << (int)(log2(globalsize) - buddy));
}

int get_node(uint8_t* p, int buddy){
    shm_fd = shm_open(smname, O_RDWR, 0666);
    base = mmap(0,globalsize,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,8192);
    return ((p - base) >>  (int)(log2(globalsize) - buddy)) + (1 << buddy) - 1;
}

int parent_split(int i){
    shm_fd = shm_open(smname, O_RDWR, 0666);
    buddy_split = mmap(0,4096,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,0);
    i = (i - 1)/2;
    return(buddy_split[i / 8] >> (i % 8)) & 1;
}

void parent_split_inverse(int i){
    shm_fd = shm_open(smname, O_RDWR, 0666);
    buddy_split = mmap(0,4096,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,0);
    i = (i - 1)/2;
    buddy_split[i / 8] = (buddy_split[i / 8] ^ 1);
    buddy_split[i / 8] = buddy_split[i / 8] << (i % 8);
}

int find_buddy(int req_size){
    int buddy = log2(globalsize) - log2(MIN_REQUEST);
    int size = MIN_REQUEST;
    while(size < req_size){
        buddy--;
        size = 2 * size;
    }
    return buddy;
}

int grow_buddy_tree(int buddy){
    shm_fd = shm_open(smname, O_RDWR, 0666);
    base = mmap(0,globalsize,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,8192);
    buddy_list = mmap(0,4096,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,4096);
    while(buddy_lim > buddy){

        int head;
        head = get_node(base,buddy_lim);
        uint8_t* right;

        if (!parent_split(head)){
            remove_node((struct memlist*)base);
            init_buddy_list(&buddy_list[--buddy_lim]);
            push_node((&buddy_list[buddy_lim]),(struct memlist*)base);
            continue;
        }

        right = get_node_pointer( head + 1, buddy_lim);
        push_node(&buddy_list[buddy_lim], (struct memlist*)right);
        init_buddy_list(&buddy_list[--buddy_lim]);
        head = (head - 1) / 2;
        if (head != 0){
            parent_split_inverse(head);
        }
    }
    return 1;
}

int sbmem_init(int segmentsize)
{
    if(!(((segmentsize & (segmentsize - 1))== 0) && (segmentsize > 0)))
        return (-1);

    shm_fd = shm_open(smname, O_RDWR, 0666);
    if (shm_fd > 0)
        shm_unlink(smname);

    shm_fd = shm_open(smname, O_RDWR | O_CREAT, 0666);

    ftruncate(shm_fd, segmentsize + 9012);

    sem = sem_open(semaphorename, O_CREAT, 0644, 1);
    if (sem > 0)
        sem_unlink(sem);

    sem = sem_open(semaphorename, O_CREAT, 0644, 1);

    //sem_wait(sem);

    buddy_count = log2(segmentsize) - log2(MIN_REQUEST) + 1;
    buddy_lim = buddy_count - 1;

    bookkeep1 = (1 << (buddy_count - 1)) / 8 * sizeof(uint8_t);
    bookkeep2 = sizeof(struct memlist) * buddy_count;
    bookkeep_total = bookkeep1 + bookkeep2;


    buddy_split = mmap(0,4096,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,0);
    buddy_list = mmap(0,4096,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,4096);
    base = mmap(0,segmentsize,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,8192);

    init_buddy_list(&buddy_list[(int)(log2(segmentsize) - log2(MIN_REQUEST))]);
    push_node(&buddy_list[(int)(log2(segmentsize) - log2(MIN_REQUEST))], (struct memlist*)base);

    sem_post(sem);

    globalsize = segmentsize;
    printf("%p\n", base);
    printf("%p\n", buddy_split);
    printf("%p\n", buddy_list);

    printf ("Size: %d",sysconf(_SC_PAGE_SIZE));
    printf ("Size: %d",bookkeep1);
    printf ("Size: %d",bookkeep2);

    
    printf (" sbmem init called\n"); // remove all printfs when you are submitting to us.  
    return (0); 
}

int sbmem_remove()
{   
    sem = sem_open(semaphorename, O_CREAT, 0644, 0);
    sem_unlink(sem);
    return (shm_unlink(smname));
}

int sbmem_open()
{   
    shm_fd = shm_open(smname, O_RDWR, 0666);
    sem = sem_open(semaphorename, O_CREAT, 0644, 1);

    sem_wait(sem);
    base = mmap(0,8,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,8192);
    sem_post(sem);
    printf("Memory opened!");
    return (0); 
}


void *sbmem_alloc (int size)
{
    shm_fd = shm_open(smname, O_RDWR, 0666);
    sem = sem_open(semaphorename, O_CREAT, 0644, 1);
    
    sem_wait(sem);

    int o_buddy;
    int buddy;

    buddy = find_buddy(size + OVERHEAD);
    o_buddy = buddy;

    printf("buddy: %d", buddy);

    while(buddy + 1 != 0){
        int size;
        uint8_t* p;
        grow_buddy_tree(buddy);

        p = (uint8_t)pop_node(&buddy_list[buddy]);
        if(!p){
            if (buddy != buddy_lim || buddy == 0){
                buddy--;
                continue;
            }

            grow_buddy_tree(buddy - 1);
            p = (uint8_t)pop_node(&buddy_list[buddy]);
        }


        size = (1 << (int)(log2(globalsize) - buddy));

        int tmp = get_node(p, buddy);
        if(tmp != 0)
            parent_split_inverse(tmp);

        while(buddy < o_buddy){
            tmp = 2 * tmp + 1;
            buddy++;
            parent_split_inverse(tmp);
            push_node(&buddy_list[buddy], (struct memlist*)get_node_pointer(tmp + 1, buddy));
        }
        *(int *)p = size;

        munmap(base,8);
        addr = mmap(0,size,PROT_WRITE | PROT_READ, MAP_SHARED,shm_fd,(uintptr_t)p + OVERHEAD + 8192);
        sem_post(sem);
        printf("Addr pointer:%p\n",addr);
        return (addr);
    }
    sem_post(sem);
    return NULL;
}

void sbmem_free (void *p)
{
    sem_wait(sem);

    p = (u_int8_t*)p - OVERHEAD;
    int buddy = find_buddy(*(int*)p + OVERHEAD);
    int tmp = get_node((u_int8_t*)p, buddy);

    while(tmp != 0){
        parent_split_inverse(tmp);
        if(parent_split(tmp) || buddy == buddy_lim){
            break;
        }

        remove_node((struct memlist*)get_node_pointer(((tmp-1) ^ 1) + 1, buddy));
        tmp = (tmp -1) / 2;
        buddy--;
    }
    push_node(&buddy_list[buddy], (struct memlist*)get_node_pointer(tmp, buddy));
    sem_post(sem);

}

int sbmem_close()
{
    sem_wait(sem);
    close(shm_fd);
    sem_post(sem);

    return (0); 
}

