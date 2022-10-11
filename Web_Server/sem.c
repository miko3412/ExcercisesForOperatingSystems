#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "sem.h"
#include "bbuffer.h"

struct SEM{
    int value;                  //current calue of the semaphore    
    int wakeups;                //possible wakeups
    pthread_mutex_t mutex;      //mutex to lock critical sections
    pthread_cond_t cond;        //condition variable for wait-function

};

SEM *sem_init(int initVal){

    //allocating memory for struct SEM and initializing mutex and condition variable
    SEM* sem = malloc(sizeof(SEM));
    int mutex_init = pthread_mutex_init(&sem->mutex, NULL );
    int cond_init  = pthread_cond_init(&sem->cond, NULL);

    //check on errors while allocating memory and initializing mutex and condition variable
    if(sem == NULL || mutex_init != 0 || cond_init != 0){
        sem_del(sem);
        return NULL;
    }

    //set value and wakeups
    sem->value   = initVal;
    sem->wakeups = 0;

    return sem;    
}


int sem_del(SEM *sem){

    //destroying mutex and condition variable
    int mutex_destroy = pthread_mutex_destroy(&sem->mutex);
    int cond_destroy = pthread_cond_destroy(&sem->cond);

    //releasing the allocated memory of the struct SEM
    free(sem);

    //check on errors while destroying mutex and condition variable
    if(mutex_destroy == 0 && cond_destroy == 0){
        return 0;
    }
    else{
        return -1;
    }
}


void P(SEM *sem){

    //lock critical section and lower semaphore value
    pthread_mutex_lock(&sem->mutex);
    sem->value--;

    //if semaphore should block, wait for condition variable, while no wakeups available
    if(sem->value<0){
        do{
            pthread_cond_wait(&sem->cond, &sem->mutex);
        } 
        while(sem->wakeups<1);
        //lower possible wakeups
        sem->wakeups--;
      }

    //unlock critical section
    pthread_mutex_unlock(&sem->mutex);
}


void V(SEM *sem){

    //lock critical section and higher semaphore value
    pthread_mutex_lock(&sem->mutex);
    sem->value++;

    //if something in queue, add possible wakeup and signal condition variable
    if(sem->value<=0){
        sem->wakeups++;
        pthread_cond_signal(&sem->cond);
    }

    //unlock critical section
    pthread_mutex_unlock(&sem->mutex);
}


