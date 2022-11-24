#include <stdio.h>
#include "port_chat.h"
#include <signal.h>
#include <pthread.h>    // Use pthreads to create sender and receiver threads

pthread_t receriver_thread;

int main(int argc, char *argv[]) {

    if (argc <= 1) {
        fprintf(stderr, "Port Num Error!\n");
        exit(1);
    }

    short src_port = atoi(argv[1]);
    short dst_port = atoi(argv[2]);
    printf("=========================================================\n");
    printf("==                Port Chatting Start!                 ==\n");
    printf("== Source Port[%d] <--------> Destination Port[%d] ==\n", src_port, dst_port);
    printf("=========================================================\n");
    printf("                  USER PROFILE [%d]                    \n", src_port);
    printf("---------------------------------------------------------\n");
    // Create a receiver thread
    pthread_create(&receriver_thread, NULL, receiver, (void *)&src_port);
    pthread_detach(receriver_thread);
    sender(src_port, dst_port);
    return 0;
}
