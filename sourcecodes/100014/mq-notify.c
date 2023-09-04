/*
 * File: mq-notify.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Demenstrat the asynchronous notification of POSIX message queue.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall mq-notify.c -o mq-notify -lrt
 * Usage: $ ./mq-notify
 *
 * Example source code for article 《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <pthread.h>

#include <mqueue.h>

#define MAX_MSG                 10
#define MSG_SIZE                256
#define MQ_NAME                 "/mq_for_notify_test"

static void notify_func(union sigval sig_val) {
    ssize_t nbytes;
    char *buf;

    buf = malloc(MSG_SIZE);
    if (buf == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    nbytes = mq_receive(sig_val.sival_int, buf, MSG_SIZE, NULL);
    if (nbytes == -1) {
        perror("mq_receive()");
        free(buf);
        exit(EXIT_FAILURE);
    }

    printf("Received %ld bytes from '%s': %s\n", nbytes, MQ_NAME, buf);
    free(buf);
    exit(EXIT_SUCCESS);
}

int main() {
    mqd_t mqdes;
    struct mq_attr attr;
    struct sigevent sig_event;

    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;

    mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0660, &attr);
    if (mqdes == -1) {
        perror("mq_open()");
        exit(EXIT_FAILURE);
    }

    memset(&sig_event, 0, sizeof(struct sigevent));
    sig_event.sigev_notify = SIGEV_THREAD;
    sig_event.sigev_notify_function = notify_func;      // notify function
    sig_event.sigev_value.sival_int = mqdes;            // pass mq descriptor to notify function as a integer

    if (mq_notify(mqdes, &sig_event) == -1) {
        perror("mq_notify()");
        mq_close(mqdes);
        exit(EXIT_FAILURE);
    }
    puts("Wait a moment...");
    sleep(1);

    const char *msg = "Hello World!";
    if (mq_send(mqdes, msg, strlen(msg), 0) == -1) {
        perror("mq_send()");
        mq_close(mqdes);
        exit(EXIT_FAILURE);
    }
    pause();

    mq_close(mqdes);
    mq_unlink(MQ_NAME);
    exit(EXIT_SUCCESS);
}
