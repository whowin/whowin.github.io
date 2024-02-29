/*
 * File: msq-recv.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * This is an example of how to receive a message from a message queue.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall msg-recv.c -o msg-recv
 * Usage: $ ./msg-recv [msgtype]
 *
 * Example source code for article 《IPC之三：使用 System V 消息队列进行进程间通信的实例》
 * 
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#define PROJ_ID         1234

int main(int argc, char **argv) {
    key_t ipc_key;
    int msqid;
    int ret_val;
    long msgtype = 0;

    struct msgbuf {
        long mtype;
        char mtext[64];
    } msg;

    if (argc > 1) {
        msgtype = atoi(argv[1]);
        if (strcmp(argv[1], "-h") == 0) {
            printf("Usage: %s [msgtype]\n", argv[0]);
            printf("       [msgtype] will be set to 0 if there is no [msgtype]\n");

            exit(EXIT_SUCCESS);
        }
    }

    ipc_key = ftok("./", PROJ_ID);
    // check if the key exists
    msqid = msgget(ipc_key, IPC_EXCL);
    if (msqid == -1) {
        // key doesn't exist, create it
        printf("The message queue associated with key 0x%x doesn't exist.\n", ipc_key);
        exit(EXIT_FAILURE);
    }

    ret_val = msgrcv(msqid, (void *)&msg, sizeof(msg.mtext), msgtype, MSG_NOERROR | IPC_NOWAIT);
    if (ret_val == -1) {
        perror("msgrcv()");
        exit(EXIT_FAILURE);
    } else {
        msg.mtext[ret_val] = 0;
        printf("Received %d bytes: %s \n", ret_val, msg.mtext);
    }

    return EXIT_SUCCESS;
}

