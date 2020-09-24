#include <pthread.h>
#include <stdio.h>

#include "keyboard_send.h"
#include "receiver.h"

static pthread_mutex_t  shutdownSyncMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   shutdownSyncCond = PTHREAD_COND_INITIALIZER;
static pthread_t        killer_pid;

//Handles the shutdown sequence
static void* executeShutdownThread(){  
    receiverShutdown();         
    keyboardSenderShutdown();   
    return NULL;
}

//Listens for shutdown signal and waits till shutdown is over
void awaitShutdown(){       
    pthread_mutex_lock(&shutdownSyncMutex);
        pthread_cond_wait(&shutdownSyncCond, &shutdownSyncMutex);  //Sleeps till shutdown signalled
    pthread_mutex_unlock(&shutdownSyncMutex);
    if(pthread_create(&killer_pid, NULL, executeShutdownThread, NULL)){
        printf("Application failed to exit smoothly\n");
    }

    pthread_join(killer_pid, NULL);
    pthread_cond_destroy(&shutdownSyncCond);
    pthread_mutex_destroy(&shutdownSyncMutex);
}
//Triggers the shutdown sequence
void triggerShutdown(){                                 
    pthread_mutex_lock(&shutdownSyncMutex);
        pthread_cond_signal(&shutdownSyncCond);                     //Only first signal will have an effect
    pthread_mutex_unlock(&shutdownSyncMutex);
}