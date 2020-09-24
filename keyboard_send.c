#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "keyboard_send.h"
#include "list.h"
#include "shutdown.h"

#define BUFF_LEN 512                                                //Static buffer length

static struct addrinfo *remote;                                     //Remote peer address
static pthread_t k_pid;                                             //PID for keyboard
static pthread_t s_pid;                                             //PID for screen
static pthread_mutex_t sendSyncMutex = PTHREAD_MUTEX_INITIALIZER;   //Mutex for synchronization
static pthread_cond_t sendSyncCond = PTHREAD_COND_INITIALIZER;      //Synchronization conditional
static pthread_mutex_t *sendListMutex;                              //Global list access mutex
static List *sendList;                                              //Shared List
static int  *s_psocketDescriptor;                                   //Local socket
static char *pmessage_in;                                           //Mallocs that could be dangling
static char *pmessage_out;                                          //  mid-process on a shutdown

static void *keyboardThread()
{
    char buff[BUFF_LEN];
    memset(buff, 0, BUFF_LEN);
    char* p_buff = NULL;

    int loopFlag = 1;
    while(loopFlag){
        p_buff = fgets(buff, BUFF_LEN, stdin);
        if(p_buff == NULL){
            break;
        }
            loopFlag = strcmp(buff, "!\n");                 //Check for end condition
            pmessage_in = malloc(strlen(buff)+1);           //Allocate just the right amount
            if(pmessage_in==NULL){
                printf("Out of application memory, terminating now...");
                break;
            }
            strcpy(pmessage_in, buff);                      //heap-alloced msg ready to enter shared list

            //Producer has produced the item
        pthread_mutex_lock(&sendSyncMutex);		            //Signal after list operation, atomically
            pthread_mutex_lock(sendListMutex);
                if(List_prepend(sendList, pmessage_in))
                    printf("Message lists overloaded: incoming messages have been dropped\n");
            pthread_mutex_unlock(sendListMutex);
            pthread_cond_signal(&sendSyncCond);
            pmessage_in = NULL;                             //At this point it doesn't need to be freed
        pthread_mutex_unlock(&sendSyncMutex);
        memset(buff, 0, strlen(buff)+1);                    //Reset static array
    }

    sleep(1);	                                            //Let the messages to send clear out
    triggerShutdown();                                      //  Then trigger shutdown
    return NULL;
}

static void *sendThread()
{
    //Prints out address and port of local sender
    char addrPrintable[INET6_ADDRSTRLEN];
    inet_ntop(
        remote->ai_family,
        &(((struct sockaddr_in*)(remote->ai_addr))->sin_addr),
        addrPrintable,
        sizeof addrPrintable
    );
    printf("Sending to %s:%d\n", addrPrintable, ntohs(((struct sockaddr_in*)(remote->ai_addr))->sin_port));

    int listcount;
    int keepGoingFlag = 1;
    while(keepGoingFlag){                                  
        pthread_mutex_lock(sendListMutex);
            listcount = List_count(sendList);
        pthread_mutex_unlock(sendListMutex);

        if(listcount == 0){		                                //If there is nothing we will wait
            pthread_mutex_lock(&sendSyncMutex);
                pthread_cond_wait(&sendSyncCond, &sendSyncMutex);
                pthread_mutex_unlock(&sendSyncMutex);
        }
        pthread_mutex_lock(sendListMutex);
            pmessage_out = (char *)List_trim(sendList);         //Remove from the tail
        pthread_mutex_unlock(sendListMutex);
            
        keepGoingFlag = strcmp(pmessage_out, "!\n");            //check for !\n then send that last message
        int numbytes;
        if ((numbytes = sendto(*s_psocketDescriptor, pmessage_out, strlen(pmessage_out), 0,
            remote->ai_addr, remote->ai_addrlen)) == -1) {
            perror("Message send failure");
        }
        pthread_mutex_lock(&sendSyncMutex);
            free(pmessage_out);
            pmessage_out = NULL;
        pthread_mutex_unlock(&sendSyncMutex);                   //Free and null must be atomic
    }
    return NULL;
}

//Start up procedure for the keyboard and the sender threads
int keyboardSenderListen(struct addrinfo *remoteinfo, int *psocketDescriptor, pthread_mutex_t *globalListMutex)
{
    sendListMutex = globalListMutex;
    s_psocketDescriptor = psocketDescriptor;
    remote = (struct addrinfo *)remoteinfo;
    
    if(pthread_create(&k_pid, NULL, keyboardThread, NULL)){
		printf("keyboard thread creation failed\n");
		return -1;
	}
    if(pthread_create(&s_pid, NULL, sendThread, NULL)){
		printf("sender thread creation failed\n");
		return -1;
	}
    sendList = List_create();   //Global-ish list created
   
    return 0;
}

void keyboardSenderShutdown()
{
    pthread_cancel(k_pid);
    pthread_cancel(s_pid);
    pthread_join(s_pid, NULL);
    pthread_join(k_pid, NULL);

    //We don't need to wrap this in a mutex because the other processes are done
    if(pmessage_in)
	    free(pmessage_in);
    pmessage_in = NULL;
    if(pmessage_out)
	    free(pmessage_out);
    pmessage_out = NULL;

    close(*s_psocketDescriptor);

    pthread_mutex_lock(sendListMutex);  //This may be touched by receiver.c so it must be guarded
        if(sendList){
            List_free(sendList);
            sendList = NULL;
        }
    pthread_mutex_unlock(sendListMutex);
    pthread_cond_destroy(&sendSyncCond);
    pthread_mutex_destroy(&sendSyncMutex);
}
