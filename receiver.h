#ifndef _RECEIVER_
#define _RECEIVER_
#include <pthread.h>

int receiverListen(int *psocketDescriptor, pthread_mutex_t *globalListMutex);

void receiverShutdown();

#endif
