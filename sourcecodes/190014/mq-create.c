/*
 * File: mq-create.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * A simple command line tool for creating a new message queue.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall mq-create.c -o mq-create -lrt
 * Usage: $ ./mq-create [-cx] [-m maxmsg] [-s msgsize] mq-name [octal-perms]
 *
 * Example source code for article 《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》
 * 
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <mqueue.h>

#define DEFAULT_MAXMSG              10
#define DEFAULT_MSGSIZE             256

void usage(const char *prog_name) {
    printf("Usage: %s [-cx] [-m maxmsg] [-s msgsize] mq-name [octal-perms]\n", prog_name);
    printf("\t\t-c Create queue (O_CREAT)\n");
    printf("\t\t-m maxmsg Set maximum # of messages\n");
    printf("\t\t-s msgsize Set maximum message size\n");
    printf("\t\t-x Create exclusively (O_EXCL)\n");
    exit(EXIT_FAILURE);
}

// Convert an octal number to decimal
mode_t get_octal_perms(char *octal_perms) {
    int i = 0;
    mode_t perms = 0;

    while (octal_perms[i] > 0) {
        if (octal_perms[i] < '0' || octal_perms[i] > '7') {
            perms = S_IRUSR | S_IWUSR;
            break;
        } else {
            perms = perms * 8 + octal_perms[i] - '0';
        }
        ++i;
    }
    return perms;
}

int main(int argc, char *argv[]) {
    int flags, opt;
    mode_t perms = S_IRUSR | S_IWUSR;
    mqd_t mqdes;
    struct mq_attr attr;

    attr.mq_maxmsg = DEFAULT_MAXMSG;
    attr.mq_msgsize = DEFAULT_MSGSIZE;
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    flags = O_RDWR;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "cm:s:x")) != -1) {
        switch (opt) {
            case 'c':
                flags |= O_CREAT;
                break;

            case 'm':
                attr.mq_maxmsg = atoi(optarg);
                break;

            case 's':
                attr.mq_msgsize = atoi(optarg);
                break;

            case 'x':
                flags |= O_EXCL;
                break;

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // optind is the index of message queue name in argv. See man 3 getopt
    // If there is no [octal-perms] argument, mq-name should be the last argument 
    // and optind=argc-1, so argc=optind+1
    // If [octal-perms] exists, mq-name should be the second-to-last argument 
    // and optind=argc-2, so argc=optind+2
    // So if optind must be less than argc and (argc>optind+1) means thare are some arguments after mq-name
    if (optind >= argc) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > optind + 1) {
        // Convert octal to decimal
        perms = get_octal_perms(argv[optind + 1]);
    }

    // Create a message queue
    mqdes = mq_open(argv[optind], flags, perms, &attr);
    if (mqdes == -1) {
        perror("mq_open()");
        exit(EXIT_FAILURE);
    }

    mq_close(mqdes);
    printf("The message queue %s has been created.\n", argv[optind]);

    exit(EXIT_SUCCESS);
}
