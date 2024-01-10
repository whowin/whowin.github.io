/*
 * File: shm-unlink.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Check if the existing mapping is still effective after executing shm_unlink().
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall shm-unlink.c -o shm-unlink -lrt
 * Usage: $ ./shm-unlink <shm name>
 *
 * Example source code for article 《IPC之八：使用 POSIX 共享内存进行进程间通信的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <semaphore.h>

#define SHM_NAME        "shm_unlink_testing"
#define SHM_SIZE        256
#define SHM_PERMS       0660

#define SEM_NAME        "sem_unlink_testing"
#define SEM_PERMS       0660
#define SEM_VALUE       0

#define MAX_CMD_LEN     128
#define BUF_SIZE        1024

void cmd_pipe(char *cmd) {
    char buf[BUF_SIZE];

    printf("\nComand: %s\n", cmd);
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Can't open pipe - %s\n", cmd);
    } else {
        int nread = fread(buf, sizeof(char), BUF_SIZE, fp);
        if (nread > 0) {
            buf[nread] = 0;
            printf("%s\n", buf);
        }
        fclose(fp);
    }
    printf("\n");
}


int main(int argc, char **argv) {
    int shmfd;
    sem_t *sem_parent;
    pid_t pid;
    
    // Create a new shared memory object
    shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR, SHM_PERMS);
    if (shmfd < -1) {
        perror("shm_open()");
        exit(EXIT_FAILURE);
    }

    ftruncate(shmfd, SHM_SIZE);         // Setting the size of shared memory
    close(shmfd);

    // Create a semaphore
    sem_parent = sem_open(SEM_NAME, O_CREAT, SEM_PERMS, SEM_VALUE);

    pid = fork();
    if (pid == 0) {
        // Child process
        //===============
        sem_t *sem_child = sem_open(SEM_NAME, O_EXCL);      // open the semaphore that is created by parent
        if (sem_child == NULL) {
            perror("sem_open()-child");
            exit(EXIT_FAILURE);            
        }

        int shmfd = shm_open(SHM_NAME, O_RDWR, SHM_PERMS);  // open the shared memory that is created by parent
        if (shmfd < -1) {
            perror("shm_open()-child");
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
        // mapping the shared memory to process address space
        char *p = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
        if (p == MAP_FAILED) {
            perror("mmap()");
            close(shmfd);
            shm_unlink(SHM_NAME);
        }
        close(shmfd);           // close the shared memory object
        sem_post(sem_child);    // V (sen_child)

        int i;
        const char *working_str = "Child: the mapping is still working.";
        for (i = 0; i < 10; ++i) {
            strcpy(p, working_str);     // copy a string to the shared memory
            printf("%s\n", p);          // read the string from the shared memory 
            sleep(1);
        }
        munmap(p, SHM_SIZE);            // unmapping
        printf("Child: I unmap the mapping.\n");
        sleep(1);
        printf("Child: Now I exits.\n");
        exit(EXIT_SUCCESS);

    } else if (pid < 0) {
        // fork() error
        perror("fork()");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
    }

    // parent process
    //=================
    char cmd[MAX_CMD_LEN];
    int ret = -1;
    sleep(1);       // Let the child has enough time running
    ret = sem_trywait(sem_parent);      // P (sem_parent) with NON-BLOCK
    if (ret == -1) {
        perror("sem_trywait()");
    }
    printf("The child process is ready.\n");
    
    sprintf(cmd, "ls -l /dev/shm/%s", SHM_NAME);
    cmd_pipe(cmd);              // execute `ls` command
    shm_unlink(SHM_NAME);
    printf("Parent: I have unlinked the object - %s\n", SHM_NAME);
    cmd_pipe(cmd);              // execute `ls` command again

    wait(NULL);
    sleep(1);
    printf("Parent: the child has exited.\n");

    sem_close(sem_parent);
    sem_unlink(SEM_NAME);

    return 0;
}
