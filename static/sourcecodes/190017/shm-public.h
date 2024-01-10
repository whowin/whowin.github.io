/*
 * File: shm-public.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Demonstrate multiple client communcating with single server using System V shared memory segment.
 * This is a including file to be included by server and client parts of the demonstration program.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ N/A
 * Usage: $ N/A
 *
 * Example source code for article 《IPC之七：使用 System V 共享内存段进行进程间通信的实例》
 * 
 */

#ifndef _SHM_PUBLIC_H
#define _SHM_PUBLIC_H   1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <sys/sem.h>

#define MAX_STR_LEN             256     // Max. length of each string
#define MAX_BUFFERS             10      // Max. number of buffer

// the 3 macros below are for generating key_t(s) of semaphore set and shared memory segment
#define SEMSET_KEY              "/tmp/sems_key"
#define SHARED_MEMORY_KEY       "/tmp/shared_memory_key"
#define PROJECT_ID              'K'

#define SEM_NUM                 3       // number of semaphores in the set
#define SEM_MUTEX               0       // index # of MUTEX semaphore in the set
#define SEM_BUF_FULL            1       // index # of BUFFER_FULL semphore in the set
#define SEM_BUF_EMPTY           2       // index # of BUFFER_EMPTY semaphore in the set

#define FLAG_SERVER             0       // indicates the current operation is from server
#define FLAG_CLIENT             1       // indicates the current operation is from client

#define SEM_PERMS               0660    // R/W permissions for semaphore set
#define SHM_PERMS               0660    // R/W permissions for shared memory

#define FONT_RED                "\033[31m"
#define FONT_YELLOW             "\033[33m"
#define FONT_DEFAULT            "\033[0m"

struct shared_memory {
    char buf[MAX_BUFFERS][MAX_STR_LEN];     // buffer which can store MAX_BUFFERS number of strings.
    int string_index;                       // string index using for placing a string into buffer
    int print_index;                        // print index using for taking out a string from buffer
};

// For setting semaphore's value
union semun {
    int val;
    struct semid_ds *buf;
    ushort array[1];
};

/****************************************************************
 * Function: void error (char *msg)
 * Description: print system error message and 
 *              send out SIGINT signal to itself and parent
 * 
 * Argument:
 *      msg     message printed before system error message
 * Return:      NONE
 ****************************************************************/
void error (char *msg) {
    perror(msg);
    kill(getppid(), SIGINT);        // signal parent
    kill(getpid(), SIGINT);         // signal itself
}
/*****************************************************************
 * Function: void touch_key_files()
 * Description: Create 2 empty files for generating key_t
 * 
 * Arguments: NONE
 * Return: NONE
 *****************************************************************/
void touch_key_files() {
    int fd = open(SEMSET_KEY, O_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    close(fd);

    fd = open(SHARED_MEMORY_KEY, O_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    close(fd);
}
/************************************************************************
 * Function: int get_semset(int flag)
 * Description: Create a semaphore set for server
 *              Open a semaphore set for client
 * 
 * Arguments:   flag        FLAG_SERVER: caller is server
 *                          FLAG_CLIENT: caller is client
 * Return:  on success, return the ID of semaphore set
 *          on error, print a error message and exit
 ************************************************************************/
int get_semset(int flag) {
    key_t s_key;
    int semid;
    int semflag = IPC_EXCL;

    union semun sem_attr;

    s_key = ftok(SEMSET_KEY, PROJECT_ID);
    if (s_key == -1) {
        error("ftok");
    }
    if (flag == FLAG_SERVER) {
        semflag = semflag | IPC_CREAT | SEM_PERMS;
    }
    semid = semget(s_key, SEM_NUM, semflag);
    if (semid == -1) {
        if (flag == FLAG_SERVER) {
            printf("%sCan't create a new semaphore set.%s\n", FONT_RED, FONT_DEFAULT);
        } else {
            if (errno == ENOENT) {
                printf("%sThe semaphore set to be obained doesn't exist.%s\n", FONT_RED, FONT_DEFAULT);
            } else {}
                printf("%sCan't get the semaphore set with unknown reason.%s\n", FONT_RED, FONT_DEFAULT);
        }
        error("semget");
    }
    if (flag == FLAG_SERVER) {
        // initial the semaphore set. 
        sem_attr.val = MAX_BUFFERS;     // counting semaphore for buffer full
        if (semctl(semid, SEM_BUF_FULL, SETVAL, sem_attr) == -1) {
            error("semctl()-SETVAL-SEM_BUF_FULL");
        }
        sem_attr.val = 0;               // counting semaphore for buffer empty
        if (semctl(semid, SEM_BUF_EMPTY, SETVAL, sem_attr) == -1) {
            error("semctl-SETVAL-SEM_BUF_EMPTY");
        }
        sem_attr.val = 1;               // unlocked the MUTEX
        if (semctl(semid, SEM_MUTEX, SETVAL, sem_attr) == -1) {
            error("semctl-SETVAL-SEM_MUTEX");
        }
    }

    return semid;
}
/*********************************************************************************
 * Function: int get_shared_mem(int flag, struct shared_memory **shared_mem_p)
 * Description: Create a shared memory segment for server
 *              Open a shared memory segment for client
 * 
 * Arguments:   flag            FLAG_SERVER: caller is server
 *                              FLAG_CLIENT: caller is client
 *              shared_mem_p    point to address pointer to attach the shared
 *                              memory segment
 * Return:  on success, return the ID of the shared memory segmant
 *          on error, print a error message and exit
 *********************************************************************************/
int get_shared_mem(int flag, struct shared_memory **shared_mem_p) {
    key_t s_key;
    int shmid;
    int shmflag = IPC_EXCL;
    struct shared_memory *shared_mem_ptr;

    s_key = ftok(SHARED_MEMORY_KEY, PROJECT_ID);
    if (s_key == -1) {
        error("ftok");
    }
    if (flag == FLAG_SERVER) {
        shmflag = shmflag | IPC_CREAT | SHM_PERMS;
    }
    shmid = shmget(s_key, sizeof(struct shared_memory), shmflag);
    if (shmid == -1) {
        if (flag == FLAG_SERVER) {
            printf("%sCan't create a new shared memory segment.%s\n", FONT_RED, FONT_DEFAULT);
        } else {
            if (errno == ENOENT) {
                printf("%sThe shared memory segment to be obained doesn't exist.%s\n", FONT_RED, FONT_DEFAULT);
            } else {}
                printf("%sCan't get the shared memory segment with unknown reason.%s\n", FONT_RED, FONT_DEFAULT);
        }
        error("shmget");
    }
    shared_mem_ptr = (struct shared_memory *)shmat(shmid, NULL, flag);
    if (shared_mem_ptr == (struct shared_memory *)-1) {
        printf("%sCan't attach the shared memory segment to the user address space.%s\n", FONT_RED, FONT_DEFAULT);
        error("shmat");
    }
    *shared_mem_p = shared_mem_ptr;
    if (flag == FLAG_SERVER) {
        // Initialize the shared memory
        shared_mem_ptr->string_index = shared_mem_ptr->print_index = 0;
    }

    return shmid;
}
/*********************************************************************************
 * Function: void sem_op(int semid, int sem_index, int value, int flag)
 * Description: Operate a semaphore's value in the set
 * 
 * Arguments:
 *      semid           ID of the semaphore set
 *      sem_index       semaphore index in the set
 *      value           adjusted value of the semaphore
 *      flag            FLAG_SERVER: caller is server
 *                      FLAG_CLIENT: caller is client
 
 * Return:  NONE
 *********************************************************************************/
void sem_op(int semid, int sem_index, int value, int flag) {
    struct sembuf asem[1];

    asem[0].sem_num = sem_index;
    asem[0].sem_op = value;
    asem[0].sem_flg = 0;

    if (semop(semid, asem, 1) == -1) {
        perror("semop");
        if (flag == FLAG_CLIENT) {
            exit(EXIT_FAILURE);
        }
    }
}
/*********************************************************************************
 * Function: void sem_p(int semid, int sem_index, int flag)
 * Description: P operation for a semaphore in the set
 * 
 * Arguments:
 *      semid           ID of the semaphore set
 *      sem_index       semaphore index in the set
 *      flag            FLAG_SERVER: caller is server
 *                      FLAG_CLIENT: caller is client
 
 * Return:  NONE
 *********************************************************************************/
void sem_p(int semid, int sem_index, int flag) {
    sem_op(semid, sem_index, -1, flag);
}
/*********************************************************************************
 * Function: void sem_p(int semid, int sem_index, int flag)
 * Description: V operation for a semaphore in the set
 * 
 * Arguments:
 *      semid           ID of the semaphore set
 *      sem_index       semaphore index in the set
 *      flag            FLAG_SERVER: caller is server
 *                      FLAG_CLIENT: caller is client
 
 * Return:  NONE
 *********************************************************************************/
void sem_v(int semid, int sem_index, int flag) {
    sem_op(semid, sem_index, 1, flag);
}

#endif
