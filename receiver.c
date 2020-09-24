#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "receiver.h"
#include "list.h"
#include "shutdown.h"

#define MSG_MAX_LEN 512

static pthread_t r_pid;													//Receiver Process ID
static pthread_t s_pid;													//Sender Process ID
static pthread_mutex_t receiveSyncMutex = PTHREAD_MUTEX_INITIALIZER;	//Synchronization mutex
static pthread_cond_t receiveSyncCond = PTHREAD_COND_INITIALIZER;   	//Synchronization conditional
static pthread_mutex_t *receiveListMutex;								//Global List mutex
static List *receiveList;												//Received messages list
static int *s_psocketDescriptor;										//Local bound socket
struct sockaddr_storage sinRemote;										//Remote sender information storage
static char* p_message;													//Mallocs that could be dangling
static char* messageRx;													//  mid-process on a shutdown

static void *receiveThread()
{
	//Remote sender information storage
	socklen_t sin_len = sizeof sinRemote;
	char messageRecv[MSG_MAX_LEN];
	int bytesRx;

	int recvFlag = 1;
	while (recvFlag) {
		//Receive call, blocking
		if((bytesRx = recvfrom(*s_psocketDescriptor,
			messageRecv, MSG_MAX_LEN, 0,
			(struct sockaddr *)&sinRemote, &sin_len)) == -1) {
			perror("Receive error");
			break;									//If recvfrom has issues that's a critical error
		}											//So we exit the loop and initiate shutdown

		// Make it null terminated (so string functions work):
		int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
		messageRecv[terminateIdx] = 0;
		
		p_message = malloc(terminateIdx+1); 		//Allocate just the right amount
        strcpy(p_message, messageRecv); 			//heap-alloced msg ready to enter shared list
		memset(messageRecv, 0, bytesRx); 			//Clear up to the size of the write
		recvFlag = strcmp(p_message, "!\n"); 		//Kill urself at least if u receive !

        //Message has been produced, now we send it
		pthread_mutex_lock(&receiveSyncMutex);
			pthread_mutex_lock(receiveListMutex);
			if(List_prepend(receiveList, p_message)){
				printf("Message lists overloaded: your message can't be sent\n");
				free(p_message); //Free on failure to insert
			}
			pthread_mutex_unlock(receiveListMutex);
			p_message = NULL;	//We have to make it null as it is now part of the list or free
			pthread_cond_signal(&receiveSyncCond);
		pthread_mutex_unlock(&receiveSyncMutex);

	}
	sleep(1);			//Let the messages list clear out for those fast typers
	triggerShutdown();	//Then trigger a shutdown
	return NULL;	
}

static void *screenThread()
{
	int listcount;
	int recvFlag = 1;
	while (recvFlag) {
		pthread_mutex_lock(receiveListMutex);
			listcount = List_count(receiveList);
		pthread_mutex_unlock(receiveListMutex);

		if(listcount == 0){									//We sleep if there is nothing in list
			pthread_mutex_lock(&receiveSyncMutex);
				pthread_cond_wait(&receiveSyncCond, &receiveSyncMutex);
			pthread_mutex_unlock(&receiveSyncMutex);
		}

		pthread_mutex_lock(receiveListMutex);
		    messageRx = (char *)List_trim(receiveList);   	//Remove from the tail
		pthread_mutex_unlock(receiveListMutex);
		
		fputs( messageRx, stdout );							//Write the message

		//Check for end case
		recvFlag = strcmp(messageRx, "!\n");				//if the message was !\n then end thread
		if(recvFlag==0){
			printf("Your chat session has been terminated by the other user\n");
		}
		pthread_mutex_lock(&receiveSyncMutex);
			free(messageRx);
			messageRx = NULL;
		pthread_mutex_unlock(&receiveSyncMutex);			//These two actions must be atomic
	}
	return NULL;
}

//Start up procedure for the receiver and the screen threads
int receiverListen(int *psocketDescriptor, pthread_mutex_t* globalListMutex)
{
	receiveListMutex = globalListMutex;
	s_psocketDescriptor = psocketDescriptor;
	receiveList = List_create();
	if(pthread_create(&r_pid, NULL, receiveThread, NULL)){		
		printf("receiver thread creation failed\n");
		return -1;
	}
	if(pthread_create(&s_pid, NULL, screenThread, NULL)){
		printf("screen thread creation failed\n");
		return -1;
	}
	return 0;
}

void receiverShutdown()	
{  
	pthread_cancel(r_pid);
	pthread_cancel(s_pid);
	pthread_join(r_pid, NULL);
	pthread_join(s_pid, NULL);
	
	if(p_message)
		free(p_message);
	p_message = NULL;

	if(messageRx)
		free(messageRx);
	messageRx = NULL;
	close(*s_psocketDescriptor);
	
	pthread_mutex_lock(receiveListMutex);				//This must be guarded as keyboard_send has access
	if(receiveList){
		List_free(receiveList);
		receiveList = NULL;
	}
	pthread_mutex_unlock(receiveListMutex);
	pthread_cond_destroy(&receiveSyncCond);
	pthread_mutex_destroy(&receiveSyncMutex);
}