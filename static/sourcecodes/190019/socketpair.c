/*
 * File: socketpair.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Demonstrate how to use socketpair() to create a pair of sockets and communicate between child processes.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall socketpair.c -o socketpair
 * Usage: $ ./socketpair
 *
 * Example source code for article 《IPC之九：使用UNIX Domain Socket进行进程间通信的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define BUFFER_LENGTH       128
#define SOCK_RECV           0
#define SOCK_SEND           1
#define MSG_FROM_CLIENT     "Hello server. This message is from the client process."

void server(int sv[2]) {
    int rc;
    char buffer[BUFFER_LENGTH];

    close(sv[SOCK_SEND]);
    // receive data from sv[0]
    rc = recv(sv[SOCK_RECV], buffer, BUFFER_LENGTH, 0);
    if (rc < 0) {
        perror("server - recv()");
    } else {
        buffer[rc] = '\0';
        printf("Received a message from the client: \n%s\n", buffer);
    }
    close(sv[SOCK_RECV]);
    return;
}

void client(int sv[2]) {
    int rc;
    char buffer[BUFFER_LENGTH];

    close(sv[SOCK_RECV]);
    // send message to sv[1]
    strcpy(buffer, MSG_FROM_CLIENT);
    rc = send(sv[SOCK_SEND], buffer, strlen(buffer), 0);
    if (rc < 0) {
        perror("client - send()");
    }

    close(sv[SOCK_SEND]);
    return;
}

int main() {
    int sv[2] = {-1};
    pid_t pid[2];
    int i = 0, rc;

    rc = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
    if (rc < 0) {
        perror("main: socketpair()");
        exit(EXIT_FAILURE);
    }
    // Fork server process and client processes
    for (i = 0; i < 2; ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        sleep(1);
    }
    // the first child is server process, others are client processes
    if (i == 0) {
        // child process, for server
        printf("%d. Server process. PID=%d.\n", i + 1, getpid());
        server(sv);
    } else if (i == 1) {
        // child process, for clients
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client(sv);
    } else if (i == 2) {
        // Parent process, just waiting all children exit
        printf("\n=== Parent process. Waiting for child processes exiting ===\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < 2; ++j) {
            pid_wait = wait(NULL);
            printf("Client exited. PID=%d.\n", pid_wait);
        }
    }

    return EXIT_SUCCESS;
}
