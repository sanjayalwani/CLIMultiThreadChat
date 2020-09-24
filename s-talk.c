#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <pthread.h>

#include "keyboard_send.h"
#include "receiver.h"
#include "list.h"
#include "shutdown.h"

int main(int argc, char* args[])
{
    if(argc < 4){   //Argument check; We disregard any extras
        printf("! Correct usage is s-talk <your-port> <remote-machine-name> <remote-machine-port>\n");
        return -EINVAL;
    }
    
    //Address information setup for networking, heavily inspired by Beej
    int     status;
    int     rv;
    struct  addrinfo hints;
    struct  addrinfo *localinfo;        // Will point to our local address
    struct  addrinfo *remoteinfo;       // Will point to the remote address
    int     socketDescriptor;           // Our local socket at our port

    //Local address info setup for our socket() and recvfrom()
    memset(&hints, 0, sizeof hints);    // Empty the init struct
    hints.ai_family = AF_UNSPEC;        // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;     // UDP sockets
    hints.ai_flags = AI_PASSIVE;        // Fill in my IP for me
    
    if ((status = getaddrinfo(NULL, args[1], &hints, &localinfo)) != 0) {
        fprintf(stderr, "Local network error: %s\n", gai_strerror(status));
        return -1;
    }

    struct  addrinfo *p;                // Loop through addrinfo structs in case of failure
    for(p = localinfo; p != NULL; p = p->ai_next) {
        if ((socketDescriptor = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            char* eMessage = (p->ai_family==AF_INET) ? "IPv4 socket create failed" : "IPv6 socket create failed";
            perror(eMessage);
            continue;
        }
        if (bind(socketDescriptor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketDescriptor);
            char* eMessage = (p->ai_family==AF_INET) ? "IPv4 socket bind failed" : "IPv6 socket bind failed";
            perror(eMessage);
            continue;
        }
        break;
    }
    if(socketDescriptor == -1 || p == NULL){    //If we exhausted it all, with no luck
        perror("Network connection could not be setup: ");
        return -1;
    }
    //Remote address info resolution for our sendto()
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(args[2], args[3], &hints, &remoteinfo)) != 0) {
        fprintf(stderr, "Remote network error: %s\n", gai_strerror(rv));
        return -1;
    }
    //End of Networking Setup :D

    pthread_mutex_t globalListMutex = PTHREAD_MUTEX_INITIALIZER;
    if(receiverListen(&socketDescriptor, &globalListMutex) == -1){
        perror("Thread creation failed: ");
        receiverShutdown();
        return -1;
    }
    if(keyboardSenderListen(remoteinfo, &socketDescriptor, &globalListMutex) == -1){
        perror("Thread creation failed: ");
        receiverShutdown();     //Because this was setup succesfully first
        keyboardSenderShutdown();
        return -1;
    }

    awaitShutdown();            //Wait for shutdown listener (cond_wait) 

    freeaddrinfo(localinfo);    // free the local address linked-list
    freeaddrinfo(remoteinfo);   // free the remote address linked-list
    pthread_mutex_destroy(&globalListMutex);
    return 0;
}
