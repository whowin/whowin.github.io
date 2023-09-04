/*
 * File: mq-attr.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Demonstrate how to get and set the attributes of a message queue.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall mq-attr.c -o mq-attr -lrt
 * Usage: $ ./mq-attr
 *
 * Example source code for article 《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/
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

int main() {
    char msg_queue_name[MSG_QUEUE_NAME_SIZE + 1];
    mqd_t mqdes;        // queue descriptor
    int ret_val = EXIT_SUCCESS;

    sprintf(msg_queue_name, "/posix-mq-attr-%d", getpid());
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((mqdes = mq_open(msg_queue_name, O_RDONLY | O_CREAT | O_NONBLOCK, MSG_QUEUE_PERM, &attr)) == -1) {
        perror("Client: mq_open(client)");
        exit(EXIT_FAILURE);
    }

    memset(&attr, 0, sizeof(struct mq_attr));
    if (mq_getattr(mqdes, &attr) == -1) {
        perror("mq_setattr()");
        ret_val = EXIT_FAILURE;
        goto PROG_EXIT;
    }
    print_attr(&attr);

    struct mq_attr new_attr;
    memset(&new_attr, 0, sizeof(struct mq_attr));
    if (attr.mq_flags != 0) {
        new_attr.mq_flags = 0;
    } else {
        new_attr.mq_flags = O_NONBLOCK;
    }
    memset(&attr, 0, sizeof(struct mq_attr));

    if (mq_setattr(mqdes, &new_attr, &attr) == -1) {
        perror("mq_setattr()");
        ret_val = EXIT_FAILURE;
        goto PROG_EXIT;
    }
    print_attr(&attr);

    memset(&attr, 0, sizeof(struct mq_attr));
    if (mq_getattr(mqdes, &attr) == -1) {
        perror("mq_setattr()");
        ret_val = EXIT_FAILURE;
        goto PROG_EXIT;
    }
    print_attr(&attr);

PROG_EXIT:
    mq_close(mqdes);
    //mq_unlink(msg_queue_name);
    return ret_val;
}

