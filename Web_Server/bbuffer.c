#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "sem.h"
#include "bbuffer.h"



struct BNDBUF{
    int* buffer;            //pointer on int, functioning as the container

    int capacity;           //size of the bounded buffer
    int head;               //int locating the head of the buffer
    int tail;               //int locating the tail of the buffer

    pthread_mutex_t mtx;    //mutex to lock critical sections
    SEM* write;             //semaphore to write into buffer
    SEM* read;              //semaphore to read from buffer
};


BNDBUF *bb_init(unsigned int size){

    //allocate memory for struct BNDBUF and buffer inside the struct
    BNDBUF* bb = malloc(sizeof(BNDBUF));
    bb->buffer = malloc(sizeof(int) * size);
    
    //check on errors while allocating memory
    if(bb == NULL || bb->buffer == NULL){
        bb_del(bb);
        return NULL;
    }

    //set capacity + head and tail
    bb->capacity    = size;
    bb->head        = 0;
    bb->tail        = 0;    

    //initialize mutex and semaphores with corresponding values
    pthread_mutex_init(&bb->mtx, NULL);
    bb->write = sem_init(size);
    bb->read  = sem_init(0);

    return bb;    
}


void bb_del(BNDBUF *bb){

    //destroying mutex and semaphores 
    int mutex_destroy = pthread_mutex_destroy(&bb->mtx);
    int write_delete  = sem_del(bb->write);
    int read_delete   = sem_del(bb->read);

    //releasing the allocated memory of struct BNDBUF and buffer inside the struct
    free(bb->buffer);
    free(bb);
}


int  bb_get(BNDBUF *bb){

    //calling P of the read-semaphore and locking the section
    P(bb->read);
    pthread_mutex_lock(&bb->mtx);
    
    //retrieving the value and updating the tail position
    int fd = bb->buffer[bb->tail];
    bb->tail = (bb->tail+1) % bb->capacity;

    //printf("Request retrieved from a worker thread. Thread id: %d \n", pthread_self());
    //bb_print(bb);

    //unlocking the section and calling V of the write-semaphore
    pthread_mutex_unlock(&bb->mtx);
    V(bb->write);

    return fd;
}


void bb_add(BNDBUF *bb, int fd){

    //calling P of the write-semaphore and locking the section
    P(bb->write);
    pthread_mutex_lock(&bb->mtx);

    //adding the value and updating the head position
    bb->buffer[bb->head] = fd;
    bb->head = (bb->head+1) % bb->capacity;

    //printf("Request added from the main thread.\n");
    //bb_print(bb);

    //unlocking the section and calling V of the read-semaphore
    pthread_mutex_unlock(&bb->mtx);
    V(bb->read); 
}

void bb_print(BNDBUF *bb){
    
    printf("[");
    for(int j = 0; j<bb->capacity-1; j++){
        printf("%d, ", bb->buffer[j]);
    }
    printf("%d] \n", bb->buffer[bb->capacity-1]);
}
