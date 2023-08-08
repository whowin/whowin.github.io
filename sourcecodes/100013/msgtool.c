/*
 * File: msgtool.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * A command line tool for tinkering with SysV style Message Queues.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall msgtool.c -o msgtool
 * Usage: $ ./msgtool [options]
 *
 * Example source code for article 《IPC之三：使用 System V 消息队列进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>          // tolower()
#include <errno.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY_PATHNAME        "./"
#define PROJ_ID             'm'
#define MAX_SEND_SIZE       128

struct msgbuf {
    long mtype;
    char mtext[MAX_SEND_SIZE];
};

void usage(void) {
    printf("msgtool - A utility for tinkering with msg queues\n");
    printf("\nUSAGE: msgtool (s)end <type> <messagetext>\n");
    printf("               (r)ecv <type>\n");
    printf("               (d)elete\n");
    printf("               (m)ode <octal mode>\n");
    exit(EXIT_FAILURE);
}

int get_msg_queue_id(key_t key) {
    int msqid;

    // check if the msg queue exists
    if ((msqid = msgget(key, IPC_EXCL)) == -1) {
        perror("msgget()");
        printf("The message queue doesn't exist.\n");
        exit(EXIT_FAILURE);
    }
    //printf("The ID is %d.\n", msqid);
    return msqid;
}

void send_message(int qid, struct msgbuf *qbuf, long type, char *text) {
    // Send a message to the queue
    printf("Sending a message ...\n");
    qbuf->mtype = type;
    strncpy(qbuf->mtext, text, sizeof(qbuf->mtext) - 1);

    if ((msgsnd(qid, (struct msgbuf *)qbuf, strlen(qbuf->mtext) + 1, 0)) == -1) {
        perror("msgsnd()");
        exit(EXIT_FAILURE);
    }
}

void read_message(int qid, struct msgbuf *qbuf, long type) {
    int nbytes = 0;

    // Read a message from the queue
    printf("Reading a message ...\n");
    qbuf->mtype = type;
    nbytes = msgrcv(qid, (struct msgbuf *)qbuf, MAX_SEND_SIZE, type, IPC_NOWAIT);
    if (nbytes == -1) {
        if (errno == ENOMSG) {
            printf("There is no message in message queue.\n");
        } else {
            perror("msgrcv()");
        }
    } else {
        printf("Type: %ld Text: %s\n", qbuf->mtype, qbuf->mtext);
    }
}

void remove_queue(int qid) {
    // Remove the queue
    msgctl(qid, IPC_RMID, 0);
}

void change_queue_mode(int qid, char *mode) {
    struct msqid_ds queue_ds;

    // Get current info
    msgctl(qid, IPC_STAT, &queue_ds);
    // Convert and load the mode
    sscanf(mode, "%o", &queue_ds.msg_perm.mode);
    // Update the mode
    msgctl(qid, IPC_SET, &queue_ds);
}


int main(int argc, char *argv[]) {
    key_t key;
    int msqid;
    struct msgbuf qbuf;

    if (argc == 1) {
        usage();
    }

    // Create unique key via call to ftok()
    key = ftok(KEY_PATHNAME, PROJ_ID);

    switch (tolower(argv[1][0])) {
        case 's':       // Send a message to the message queue
            // Open the queue - create if necessary
            if ((msqid = msgget(key, IPC_CREAT | 0660)) == -1) {
                perror("msgget()");
                exit(EXIT_FAILURE);
            }

            if (argc < 4) {
                printf("Wrong options.\n");
                usage();
                exit(EXIT_FAILURE);
            }
            send_message(msqid, (struct msgbuf *)&qbuf, atol(argv[2]), argv[3]);
            printf("finished.\n");
            break;

        case 'r':       // receive a message from the message queue
            msqid = get_msg_queue_id(key);
            long msgtype = 0;
            if (argc > 2) {
                msgtype = atol(argv[2]);
            }
            read_message(msqid, &qbuf, msgtype);
            break;

        case 'd':       // Remove the message queue
            msqid = get_msg_queue_id(key);
            remove_queue(msqid);
            printf("The message queue(ID:%d) has been removed.\n", msqid);
            break;        

        case 'm':       // Change read/write permission of the message queue
            msqid = get_msg_queue_id(key);
            if (argc < 3) {
                printf("Wrong options.\n");
                usage();
                exit(EXIT_FAILURE);
            }
            change_queue_mode(msqid, argv[2]);
            printf("The Read/Write permission has been changed to %s.\n", argv[2]);
            break;

        default: 
            usage();
    }
    
    return(EXIT_SUCCESS);
}

