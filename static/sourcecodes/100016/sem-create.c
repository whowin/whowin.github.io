/*
 * File: sem-create.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * A simple command line tool for creating a new named POSIX semaphore.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall sem-create.c -o sem-create -lpthread
 * Usage: $ ./sem-create [-cx] [-v value] sem-name [octal-perms]
 *
 * Example source code for article 《IPC之六：使用 POSIX 信号量解决经典的'生产者消费者问题'》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <semaphore.h>

void usage(const char *prog_name) {
    printf("Usage: %s [-cx] [-v value] sem-name [octal-perms]\n", prog_name);
    printf("\t\t-c Create semaphore (O_CREAT)\n");
    printf("\t\t-v initial value\n");
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
    sem_t *sem;
    uint value = 0;

    flags = O_RDWR;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "cv:x")) != -1) {
        switch (opt) {
            case 'c':
                flags |= O_CREAT;
                break;

            case 'v':
                value = (uint)atoi(optarg);
                break;

            case 'x':
                flags |= O_EXCL;
                break;

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // optind is the index of semaphore name in argv. See man 3 getopt
    // If there is no [octal-perms] argument, sem-name should be the last argument 
    // and optind=argc-1, so argc=optind+1
    // If [octal-perms] exists, sem-name should be the second-to-last argument 
    // and optind=argc-2, so argc=optind+2
    // So if optind must be less than argc and (argc>optind+1) means thare are some arguments after sem-name
    if (optind >= argc) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > optind + 1) {
        // Convert octal to decimal
        perms = get_octal_perms(argv[optind + 1]);
    }

    // Create a named semaphore
    sem = sem_open(argv[optind], flags, perms, value);
    if (sem == SEM_FAILED) {
        perror("sem_open()");
        exit(EXIT_FAILURE);
    }

    sem_close(sem);
    printf("The semaphore %s has been created.\n", argv[optind]);

    exit(EXIT_SUCCESS);
}
