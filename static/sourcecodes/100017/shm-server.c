/*
 * File: shm-server.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Demonstrate multiple client communcating with single server using System V shared memory segment.
 * This is the server part of the demonstration program.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall shm-server.c -o shm-server
 * Usage: $ ./shm-server
 *
 * Example source code for article 《IPC之七：使用 System V 共享内存段进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0017-systemv-shared-memory/
 * 
 */

#include "shm-public.h"

int semid = -1, shmid = -1;
struct shared_memory *shared_mem_ptr = NULL;

/**************************************************************
 * Function: void sigint(int signum)
 * Description: A signal handler. 
 *              handle ctrl+c signal and exit the program
 **************************************************************/
void sigint(int signum) {
    // detach the shared memory segment
    if (shared_mem_ptr != NULL) {
        shmdt(shared_mem_ptr);
    }
    // Remove the shared memory segment 
    if (shmid >= 0) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    // Remove the semaphore set
    if (semid >= 0) {
        semctl(semid, 0, IPC_RMID);
    }
    unlink(SEMSET_KEY);
    unlink(SHARED_MEMORY_KEY);

    exit(EXIT_SUCCESS);
}

int main (int argc, char **argv) {
    printf("Server: hello world\n");

    // 1. catch (ctrl + c) siganl
    signal(SIGINT, sigint);     // catch (ctrl + c)
    // 2. Create a new shared memory segment
    touch_key_files();
    shmid = get_shared_mem(FLAG_SERVER, &shared_mem_ptr);
    // 3. initial semapore set
    semid = get_semset(FLAG_SERVER);
    // 4. forever untill ctrl + c
    while (1) {
        // Is there a string to print? P(SEM_BUF_EMPTY);
        sem_p(semid, SEM_BUF_EMPTY, FLAG_SERVER);
        // Print a string if the buffer isn't empty
        printf("%s", shared_mem_ptr->buf[shared_mem_ptr->print_index]);
        // Since there is only one process (the shm-server) using the print_index, 
        // mutex semaphore is not necessary
        (shared_mem_ptr->print_index)++;
        if (shared_mem_ptr->print_index == MAX_BUFFERS) {
            shared_mem_ptr ->print_index = 0;
        }

        // Contents of one buffer has been printed. One more buffer is available for use by clients.
        // Release buffer: V(SEM_BUF_FULL);
        sem_v(semid, SEM_BUF_FULL, FLAG_SERVER);
    }

    // Never reaches here
    return 0;
}
