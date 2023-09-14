/*
 * File: mq-client.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Demonstrate Interprocess Communication with POSIX message queue. This is Client part.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall mq-client.c -o mq-client -lrt
 * Usage: $ ./mq-client
 *
 * Example source code for article 《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include <signal.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <mqueue.h>

#include "mq-const.h"

#define MAX_PROCESSES       5

mqd_t qd_server = 0, qd_client = 0;         // queue descriptors
char client_queue_name[QD_NAME_SIZE];

int client_process() {
    char in_buffer[MSG_BUFFER_SIZE];
    int ret_value = EXIT_SUCCESS;

    // create the client queue for receiving messages from server
    sprintf(client_queue_name, "/posix-mq-client-%d", getpid());

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    // Create message queue for client
    if ((qd_client = mq_open(client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Client: mq_open(client)");
        exit(EXIT_FAILURE);
    }

    // Open server message queue
    if ((qd_server = mq_open(SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror("Client: mq_open(server)");
        ret_value = EXIT_FAILURE;
        goto CLIENT_EXIT;
    }

    int i;
    for (i = 0; i < 5; ++i) {
        // send message to server
        if (mq_send(qd_server, client_queue_name, strlen(client_queue_name) + 1, 0) == -1) {
            perror("Client: Not able to send message to server");
            continue;
        }

        // receive response from server
        if (mq_receive(qd_client, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror("Client: mq_receive");

            ret_value = EXIT_FAILURE;
            goto CLIENT_EXIT;
        }
        // display token received from server
        printf("Client(%d): Token received from server: %s\n", getpid(), in_buffer);
        sleep((int)(random() % 5) + 1);
    }

CLIENT_EXIT:
    if (qd_server > 0) {
        mq_close(qd_server);
    }
    if (qd_client > 0) {
        mq_close(qd_client);
        if (mq_unlink(client_queue_name) == -1) {
            perror("Client: mq_unlink");
            exit(EXIT_FAILURE);
        }
    }

    printf("Client(%d): bye\n", getpid());
    return ret_value;
}

int main() {
    pid_t pid[MAX_PROCESSES];
    int i = 0;
    

    for (i = 0; i < MAX_PROCESSES; ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        sleep((int)(random() % 5) + 1);
    }

    if (i < MAX_PROCESSES) {
        // child process
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client_process();
    } else if (i == MAX_PROCESSES) {
        // Parent process
        printf("Parent process. Waiting for child processes exiting.\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < MAX_PROCESSES; ++j) {
            pid_wait = wait(NULL);
            printf("Client exited. PID=%d.\n", pid_wait);
        }
    }

    return EXIT_SUCCESS;

}