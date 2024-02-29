/*
 * File: nonblock-signal.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Example of using signal to terminate blocked socket function.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g nonblock-signal.c -o nonblock-signal
 * Usage: $ ./nonblock-signal
 * 
 * Example source code for article 《使用signal中止阻塞的socket函数的应用实例》
 *
 */
#define _POSIX_SOURCE
#define _ALARM_FUNC

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#ifndef _ALARM_FUNC
#include <sys/time.h>
#endif
#include <netinet/in.h>

#define PORT                (8888)
#define ALARM_PERIOD        (5)

#define FONT_RED            "\033[31m"
#define FONT_YELLOW         "\033[33m"
#define FONT_DEFAULT        "\033[0m"

int loop = 1;
static int count = 0;

/******************************************************************
 * Function: void signal_handler(int sig)
 * Description: Handler for signal SIGALRM & SIGINT
 ******************************************************************/
void signal_handler(int sig) {
    signal(sig, signal_handler);

    printf(FONT_RED);
    if (sig == SIGALRM) {
        printf("\tSignal catcher called for signal SIGALRM(%d)\n", sig);
#ifdef _ALARM_FUNC
        alarm(ALARM_PERIOD);
#endif
    } else if (sig == SIGINT) {
        printf("\tSignal catcher called for signal SIGINT(%d)\n", sig);
        if (++count > 3) loop = 0;
    } else if (sig == SIGQUIT) {
        printf("\tSignal catcher called for signal SIGQUIT(%d)\n", sig);
    } else printf("\tSignal catcher called for unknown signal(%d)\n", sig);
    printf(FONT_DEFAULT);
}
/******************************************************************
 * Main program
 ******************************************************************/
int main(int argc, char *argv[]) {
    struct sockaddr_in6 addr;
    int sockfd, rc;

    // Step 1: Create an AF_INET6, SOCK_STREAM socket
    //=================================================
    printf("Create a TCP socket\n");
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("\tsocket() failed");
        return -1;
    }

    // Step 2: Bind the socket
    //==========================
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = PORT;
    printf("Bind the socket\n");
    rc = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc != 0) {
        perror("\tbind() failed");
        close(sockfd);
        return -2;
    }

    // Step 3: Perform a listen on the socket
    //=========================================
    printf("Set the listen backlog\n");
    rc = listen(sockfd, 5);
    if (rc != 0) {
        perror("\tlisten() failed");
        close(sockfd);
        return -3;
    }

    // Step 4: Set up signal handler for SIGALRM and SIGINT
    //======================================================
    signal(SIGINT, signal_handler);         // CTRL + C
    signal(SIGQUIT, signal_handler);        // CTRL + '\'
    signal(SIGALRM, signal_handler);        // ALARM
    printf("\nSet an alarm to go off in 5 seconds. This alarm will cause the\n");
    printf("blocked accept() to return a -1 and an errno value of EINTR.\n\n");
    // Set up an alarm that will go off in 5 seconds.
#ifdef _ALARM_FUNC
    alarm(ALARM_PERIOD);
#else
    struct itimerval new_value;
    new_value.it_value.tv_sec = ALARM_PERIOD;
    new_value.it_value.tv_usec = 0;
    new_value.it_interval.tv_sec = ALARM_PERIOD;
    new_value.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &new_value, NULL);
#endif

    // Step 5: Call accept(). This call will block.
    //===============================================
    do {
        printf("\n\tWait for an incoming connection to arrive\n");
        rc = accept(sockfd, NULL, NULL);
        printf("\taccept() completed.  rc = %d, errno = %d\n", rc, errno);
        if (rc >= 0) {
            printf("\tIncoming connection was received\n");
            close(rc);
            loop = 0;
        } else {
            perror("\terror message: ");
        }
    } while (loop == 1);

    close(sockfd);
    return(0);
}
