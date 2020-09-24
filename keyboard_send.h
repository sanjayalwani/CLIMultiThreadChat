#ifndef _KEYBOARD_SEND_
#define _KEYBOARD_SEND_
#include "list.h"
#include <netdb.h>
#include <pthread.h>

int keyboardSenderListen(struct addrinfo *remoteinfo, int *psocketDescriptor, pthread_mutex_t *globalListMutex);

//Shuts down the keyboard sender
void keyboardSenderShutdown();

#endif
