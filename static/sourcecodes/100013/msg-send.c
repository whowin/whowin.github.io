/*
 * File: msg-send.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * This is an example of how to send a message to the message queue.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall msg-send.c -o msg-send
 * Usage: $ ./msg-send
 *
 * Example source code for article 《IPC之三：使用 System V 消息队列进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#define PROJ_ID         1234

int main() {
    key_t ipc_key;
    int msqid;
    int ret_val;

    struct msgbuf {
        long mtype;
        char mtext[64];
    } msg;

    ipc_key = ftok("./", PROJ_ID);
    // Create or get the ID of message queue associated with the key
    msqid = msgget(ipc_key, IPC_CREAT | 0666);

    msg.mtype = 1;
    strcpy(msg.mtext, "This is the first message.");
    ret_val = msgsnd(msqid, (void *)&msg, strlen(msg.mtext), IPC_NOWAIT);
    if (ret_val == -1) {
        perror("msgsnd()");
        exit(EXIT_FAILURE);
    }
    printf("The first message has been sent into the message queue.\n");

    msg.mtype = 2;
    strcpy(msg.mtext, "This is the second message.");
    ret_val = msgsnd(msqid, (void *)&msg, strlen(msg.mtext), IPC_NOWAIT);
    if (ret_val == -1) {
        perror("msgsnd()");
        exit(EXIT_FAILURE);
    }
    printf("The second message has been sent into the message queue.\n");

    msg.mtype = 3;
    strcpy(msg.mtext, "This is the third message.");
    ret_val = msgsnd(msqid, (void *)&msg, strlen(msg.mtext), IPC_NOWAIT);
    if (ret_val == -1) {
        perror("msgsnd()");
        exit(EXIT_FAILURE);
    }
    printf("The third message has been sent into the message queue.\n");

    return(EXIT_SUCCESS);
}
