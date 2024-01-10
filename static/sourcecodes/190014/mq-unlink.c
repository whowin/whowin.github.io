/*
 * File: mq-unlink.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Demonstrate how to remove the specified message queue name.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall mq-unlink.c -o mq-unlink -lrt
 * Usage: $ ./mq-unlink mq-name
 *
 * Example source code for article 《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <mqueue.h>

#define MSG_QUEUE_NAME_SIZE     255
#define MAX_MESSAGES            10
#define MAX_MSG_SIZE            256
#define MSG_QUEUE_PERM          0660

void print_attr(struct mq_attr *p) {
    printf("\n");
    printf("mq_flags: 0x%lx(O_NONBLOCK=0x%x)\n", p->mq_flags, O_NONBLOCK);
    printf("mq_maxmsg: %ld\n", p->mq_maxmsg);
    printf("mq_msgsize: %ld\n", p->mq_msgsize);
    printf("mq_curmsgs: %ld\n", p->mq_curmsgs);

    return;
}

int main(int argc, char **argv) {
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        printf("%s mq-name\n", argv[0]);
    }

    if (mq_unlink(argv[1]) == -1){
        perror("mq_unlink()");
    }

    exit(EXIT_SUCCESS);
}




