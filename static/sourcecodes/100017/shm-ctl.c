/*
 * File: shm-ctl.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Demonstrate how to use shmctl().
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall shm-ctl.c -o shm-ctl
 * Usage: $ ./shm-ctl
 *
 * Example source code for article 《IPC之七：使用 System V 共享内存段进行进程间通信的实例》
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h> 

#define BUF_SIZE            256

// Get the attributes of shared memory
int get_attr(int shmid, struct shmid_ds *shm_ds) {
    struct shmid_ds shm_attr;

    if (shmctl(shmid, IPC_STAT, &shm_attr) == -1) {
        perror("shmctl()-IPC_STAT");
        return -1;
    }

    *shm_ds = shm_attr;
    return 0;
}

int main() {
    key_t key;
    int shmid;

    if ((key = ftok("./", 'Z')) == -1) {
        perror("ftok()");
        exit(EXIT_FAILURE);
    }
    // Create/Get the ID of shared memorg segment
    if ((shmid = shmget(key, BUF_SIZE, IPC_CREAT | 0666)) == -1) {
        perror("shmget()");
        exit(EXIT_FAILURE);
    }

    struct shmid_ds shm_attr;

    // IPC_STAT
    if (get_attr(shmid, &shm_attr) == -1) {
        goto PROG_EXIT;
    }
    printf("The R/W permission is %04o\n", shm_attr.shm_perm.mode & 0x1FF);

    // IPC_SET
    shm_attr.shm_perm.mode &= 0x1F8;
    if (shmctl(shmid, IPC_SET, &shm_attr) == -1) {
        perror("shmctl()-IPC_SET");
        goto PROG_EXIT;
    }

    if (get_attr(shmid, &shm_attr) == -1) {
        goto PROG_EXIT;
    }
    printf("The modified R/W permission is %04o\n", shm_attr.shm_perm.mode & 0x1FF);
    printf("The LOCKED flag: %s\n", (shm_attr.shm_perm.mode & SHM_LOCKED) ? "True" : "False");

    // SHM_INFO
    struct shm_info shminfo;
    if (shmctl(shmid, SHM_INFO, (struct shmid_ds *)&shminfo) == -1) {
        perror("shmctl()-SHM_INFO");
        goto PROG_EXIT;
    }
    printf("# of currently existing segments: %d\n", shminfo.used_ids);
    printf("Total number of shared memory pages: %ld\n", shminfo.shm_tot);
    printf("# of resident shared memory pages: %ld\n", shminfo.shm_rss);
    printf("# of swapped shared memory pages: %ld\n", shminfo.shm_swp);

    // SHM_LOCK
    if (shmctl(shmid, SHM_LOCK, NULL) == -1) {
        perror("shmctl()-SHM_LOCK");
        goto PROG_EXIT;
    }
    if (get_attr(shmid, &shm_attr) == -1) {
        goto PROG_EXIT;
    }
    printf("The LOCKED flag: %s\n", (shm_attr.shm_perm.mode & SHM_LOCKED) ? "True" : "False");

    shmctl(shmid, SHM_UNLOCK, NULL);

PROG_EXIT:
    shmctl(shmid, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);

}
