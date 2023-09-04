/*
 * File: mq-server.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Demonstrate Interprocess Communication with POSIX message queue. This is server part.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall mq-server.c -o mq-server -lrt
 * Usage: $ ./mq-server
 *
 * Example source code for article 《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <mqueue.h>

#include "mq-const.h"

mqd_t qd_server = 0, qd_client = 0;         // queue descriptors

// Signal handler for catching ctrl+c
void sigint(int signum) {
    printf("\nServer terminated by user.\n");
    if (qd_server > 0) {
        mq_close(qd_server);
    }
    if (qd_client > 0) {
        mq_close(qd_client);
    }

    mq_unlink(SERVER_QUEUE_NAME);

    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    long token_number = 1;              // next token to be given to client
    struct mq_attr attr;
    char in_buffer[MSG_BUFFER_SIZE];    // receive buffer
    char out_buffer[MSG_BUFFER_SIZE];   // send buffer

    printf("Server: Hello, World!\n");

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((qd_server = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Server: mq_open(server)");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, sigint);         // Catch ctrl+c 

    while (1) {
        // get the oldest message with highest priority
        if (mq_receive(qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror("Server: mq_receive");
            exit(EXIT_FAILURE);
        }

        printf("Server: message received: %s.\n", in_buffer);

        // send reply message to client
        if ((qd_client = mq_open(in_buffer, O_WRONLY)) == 1) {
            perror ("Server: Can't open client queue");
            continue;
        }

        sprintf(out_buffer, "%ld", token_number);

        if (mq_send(qd_client, out_buffer, strlen(out_buffer) + 1, 0) == -1) {
            perror("Server: Can't send message to client");
            continue;
        }

        printf("Server: Sent response to client.\n");
        mq_close(qd_client);
        qd_client = 0;
        token_number++;
    }
}
