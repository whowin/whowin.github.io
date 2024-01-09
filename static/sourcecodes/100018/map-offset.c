/*
 * File: map-offset.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Example for mapping POSIX shared memory with offset.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall map-offset.c -o map-offset -lpthread -lrt
 * Usage: $ ./map-offset
 *
 * Example source code for article 《IPC之八：使用 POSIX 共享内存进行进程间通信的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define PAGE_SIZE           getpagesize()               // get PAGE SIZE 
#define SHM_SIZE            2 * PAGE_SIZE
#define SHM_NAME            "shm-offset-testing"
#define OFFSET_PARENT       0
#define LENGTH_PARENT       SHM_SIZE
#define OFFSET_CHILD        PAGE_SIZE
#define LENGTH_CHILD        256
#define STRING              "Hello parent, I am from child."

int main() {
    int shmfd;
    pid_t pid;

    // Create a new shared memory object
    shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shmfd == -1) {
        perror("shm_get()");
        exit(EXIT_FAILURE);
    }
    ftruncate(shmfd, SHM_SIZE);         // setting the size of shared memory
    close(shmfd);

    pid = fork();
    if (pid == 0) {
        // child process
        int shmfd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shmfd < 0) {
            perror("shm_get()-child");
            _exit(EXIT_FAILURE);
        }
        char *p = mmap(NULL, LENGTH_CHILD, PROT_WRITE, MAP_SHARED, shmfd, OFFSET_CHILD);
        if (p == MAP_FAILED) {
            perror("mmap()-child");
            close(shmfd);
            _exit(EXIT_FAILURE);
        }
        close(shmfd);
        strcpy(p, STRING);
        printf("Child: copy a string to shared memory.\n");
        munmap(p, LENGTH_CHILD);
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        // parent process
        int shmfd = shm_open(SHM_NAME, O_RDWR, 0666);
        char *p = mmap(NULL, LENGTH_PARENT, PROT_READ, MAP_SHARED, shmfd, OFFSET_PARENT);
        close(shmfd);
        wait(NULL);         // Waiting for child exiting
        printf("Parent: the child has exited.\n");
        printf("Parent: %s\n", (p + PAGE_SIZE));
        munmap(p, LENGTH_PARENT);
    } else {
        // fork() error
        perror("fork()");
    }

    shm_unlink(SHM_NAME);

    return 0;
}
