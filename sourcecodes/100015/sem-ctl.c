/*
 * File: sem-ctl.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * .
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall sem-ctl.c -o sem-ctl
 * Usage: $ ./sem-ctl
 *
 * Example source code for article 《IPC之五：使用 System V 信号量集进行进程间同步的实例》
 * https://whowin.gitee.io/post/blog/linux/0015-systemv-semaphore-sets/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
//#include <linux/sem.h>

int sem_stat(int semid, struct semid_ds *p_semds) {
    memset(p_semds, 0, sizeof(struct semid_ds));
    int retval = semctl(semid, 0, IPC_STAT, p_semds);
    if (retval == -1) {
        perror("semctl()-IPC_STAT");
        return -1;
    }

    return 0;
}

int sem_setperms(int semid, uint mode) {
    struct semid_ds semds;

    if (sem_stat(semid, &semds) == -1) {
        return -1;
    }

    semds.sem_perm.mode = mode;
    int retval = semctl(semid, 0, IPC_SET, &semds);
    if (retval == -1) {
        perror("setctl()-IPC_SET");
        return -1;
    } else {
        sem_stat(semid, &semds);
    }

    return 0;
}

int sem_getall(int semid, int nsems) {
    unsigned short *semval = malloc(nsems * sizeof(unsigned short));
    int retval = semctl(semid, 0, GETALL, semval);
    if (retval == -1) {
        perror("selctl()-GETALL");
        return -1;
    }
    printf("The values in semaphore set are:\n");
    int i;
    for (i = 0; i < nsems; ++i) {
        printf("%d\t", semval[i]);
    }
    printf("\n");
    free(semval);

    return 0;
}

int sem_setall(int semid, ushort *semvals, int nsems) {
    int retval = semctl(semid, 0, SETALL, semvals);
    if (retval == -1) {
        perror("selctl()-SETALL");
        return -1;
    }

    return 0;
}

int main() {
    struct semid_ds semds;
    int retval = EXIT_SUCCESS;

    key_t sem_key = ftok("/tmp/", 4321);
    int semid = semget(sem_key, 3, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        perror("semget()");
        exit(EXIT_FAILURE);
    }
    printf("sem ID: %d\n", semid);

    printf("\nOperation: IPC_STAT\n");
    if (sem_stat(semid, &semds) == -1) {
        goto FAILURE_EXIT;
    }
    printf("The R/W permision: %04o\n", semds.sem_perm.mode & 0x01ff);
    printf("The number of semaphores in set: %lu\n", semds.sem_nsems);

    printf("\nOperation: IPC_SET\n");
    semds.sem_perm.mode &= 0xfffffff8;
    if (sem_setperms(semid, semds.sem_perm.mode) == -1){
        goto FAILURE_EXIT;
    }
    if (sem_stat(semid, &semds) == -1){
        goto FAILURE_EXIT;
    }
    printf("The R/W permision: %04o\n", semds.sem_perm.mode & 0x01ff);

    printf("\nOperation: GETALL\n");
    if (sem_getall(semid, semds.sem_nsems) == -1) {
        goto FAILURE_EXIT;
    }

    printf("\nOperation: SETALL\n");
    ushort *semvals = malloc(semds.sem_nsems * sizeof(ushort));
    int i;
    for (i = 0; i < semds.sem_nsems; ++i) {
        semvals[i] = i + 1;
    }
    if (sem_setall(semid, semvals, semds.sem_nsems) == -1) {
        free(semvals);
        goto FAILURE_EXIT;
    }
    free(semvals);
    if (sem_getall(semid, semds.sem_nsems) == -1) {
        goto FAILURE_EXIT;
    }

    printf("\nOperation: GETVAL\n");
    int semval = semctl(semid, (semds.sem_nsems - 1), GETVAL);
    if (semval == -1) {
        perror("selctl()-GETVAL");
        goto FAILURE_EXIT;
    }
    printf("The value of the %ld-th semaphore in the set: %d\n", semds.sem_nsems, semval);

    printf("\nOperation: SETVAL\n");
    semval += 2;
    if (semctl(semid, (semds.sem_nsems - 1), SETVAL, (ushort)semval) == -1) {
        perror("selctl()-SETVAL");
        goto FAILURE_EXIT;
    }
    if (sem_getall(semid, semds.sem_nsems) == -1) {
        goto FAILURE_EXIT;
    }

PROG_EXIT:
    semctl(semid, 0, IPC_RMID);
    return retval;

FAILURE_EXIT:
    retval = EXIT_FAILURE;
    goto PROG_EXIT;
}