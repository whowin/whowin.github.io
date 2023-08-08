/*
 * File: traversemsq.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * List all active Message Queues via traversing all the msg queue IDs
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall traversemsq.c -o traversemsq
 * Usage: $ ./traversemsq
 *
 * Example source code for article 《IPC之三：使用 System V 消息队列进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/msg.h>

struct msq {
    key_t key;
    uint msqid;
    uint perms;
    uint cbytes;
    uint qnum;
};

int main() {
    int ret_val;
    int msgmni;             // max # of msg queue identifiers

    struct msginfo msg_info;

    ret_val = msgctl(0, MSG_INFO, (struct msqid_ds *)&msg_info);
    if (ret_val == -1) {
        perror("msgctl()");
        exit(EXIT_FAILURE);
    }
    msgmni = msg_info.msgmni;
    //printf("The MSGMNI is %u.\n", msgmni);

    int i;
    int msq_count = 0;
    struct msqid_ds msg_ds;
    struct msq curr_msq;
    memset(&curr_msq, 0, sizeof(struct msq));
    printf("key\t\tmsqid\tperms\tqbytes\tqnum\n");
    for (i = 0; i <= msgmni; ++i) {
        ret_val = msgctl(i, MSG_STAT_ANY, &msg_ds);
        if (ret_val >= 0) {
            curr_msq.msqid = ret_val;
            curr_msq.key = msg_ds.msg_perm.__key;
            curr_msq.perms = msg_ds.msg_perm.mode;
            curr_msq.cbytes = msg_ds.msg_qbytes;
            curr_msq.qnum = msg_ds.msg_qnum;
            printf("0x%x\t%d\t%o\t%d\t%d\n", curr_msq.key, curr_msq.msqid, curr_msq.perms, curr_msq.cbytes, curr_msq.qnum);
            memset(&curr_msq, 0, sizeof(struct msq));
            msq_count++;
        }
    }

    printf("Totaly %d message queues.\n", msq_count);
    return EXIT_SUCCESS;
}
