/*
 * File: listmsq.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * List all active Message Queues via reading the /proc/sysvipc/msg file.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall listmsq.c -o listmsq
 * Usage: $ ./listmsq
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

#define PROC_MSG_FILE       "/proc/sysvipc/msg"
#define BUF_SIZE            256

struct msq {
    key_t key;
    uint msqid;
    uint perms;
    uint cbytes;
    uint qnum;
};

int main() {
    FILE *fp;
    int msq_count = 0;
    int i, j;
    char line[BUF_SIZE];
    int len;
    char *p;
    struct msq curr_msq;


    if((fp = fopen(PROC_MSG_FILE, "r")) == NULL){
        perror("fopen()");
        exit(EXIT_FAILURE);
    }
    fgets(line, BUF_SIZE, fp);          // skip the title
    printf("key\t\tmsqid\tperms\tqbytes\tqnum\n");
    while(!feof(fp)) {
        fgets(line, BUF_SIZE - 1, fp);
        if (feof(fp)) break;
        msq_count++;
        i = 0;
        j = 0;
        len = strlen(line);
        while (i < len) {
            while (line[i] == ' ') i++;
            p = &line[i];
            while (line[i] != ' ') i++;
            line[i] = 0;
            i++;
            switch (j) {
                case 0:
                    curr_msq.key = atol(p);
                    break;
                case 1:
                    curr_msq.msqid = atoi(p);
                    break;
                case 2:
                    sscanf(p, "%o", &curr_msq.perms);
                    break;
                case 3:
                    curr_msq.cbytes = atoi(p);
                    break;
                case 4:
                    curr_msq.qnum = atoi(p);
                    break;
                default:
                    break; 
            }
            if (++j > 4) break;
        }
        printf("0x%x\t%d\t%o\t%d\t%d\n", curr_msq.key, curr_msq.msqid, curr_msq.perms, curr_msq.cbytes, curr_msq.qnum);
        memset(&curr_msq, 0, sizeof(struct msq));
    }
    printf("Totaly %d message queues.\n", msq_count);

    fclose(fp);
    return EXIT_SUCCESS;
}
