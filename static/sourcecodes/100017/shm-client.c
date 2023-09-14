/*
 * File: shm-client.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Demonstrate multiple client communcating with single server using System V shared memory segment.
 * This is the client part of the demonstration program.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall shm-client.c -o shm-client
 * Usage: $ ./shm-client
 *
 * Example source code for article 《IPC之七：使用 System V 共享内存段进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0017-systemv-shared-memory/
 * 
 */
#include "shm-public.h"

#define MAX_PROCESSES       5       // the Max. number of processes
#define STRINGS_NUM         10      // the number of strings
const char strings[][64] = {
    "Hello world.",
    "Don't give up.",
    "If you can dream it, you can do it.",
    "The secret of getting ahead is getting started.",
    "Give it a try.",
    "Keep up the good work.",
    "Believe in yourself.",
    "Follow your dreams.",
    "There is no substitute for hard work.",
    "Today a reader, tomorrow a leader.",
};

struct shared_memory *shared_mem_ptr = NULL;

/**************************************************************
 * Function: void void client_sigint(int signum)
 * Description: A signal handler for child process.
 *              handle ctrl+c signal and exit the program
 **************************************************************/
void client_sigint(int signum) {
    fprintf(stdout, "%sGot ctrl+c signal in child. pid = %d.%s\n", FONT_RED, getpid(), FONT_DEFAULT);
    if (shared_mem_ptr != NULL) {
        shmdt(shared_mem_ptr);
    }

    exit(EXIT_SUCCESS);
}
/*************************************************************************
 * Function: int client_process()
 * Decription: Client program. Run in child processes.
 * 
 * Arguments: NONE
 * Return: 0
 *************************************************************************/
int client_process() {
    int semid;
    
    srand((uint)time(NULL));

    // 1. catch (ctrl + c) siganl
    signal(SIGINT, client_sigint);
    // 2. get shared memory
    get_shared_mem(FLAG_CLIENT, &shared_mem_ptr);
    // 3. get semaphore set
    semid = get_semset(FLAG_CLIENT);
    // 4. loop forever until ctrl+c
    while (1) {
        // get a buffer: P(SEM_BUF_FULL);
        sem_p(semid, SEM_BUF_FULL, FLAG_CLIENT);

        // There might be multiple clients. 
        // We must ensure that only one client uses string_index at a time.
        // P(SEM_MUTEX);
        sem_p(semid, SEM_MUTEX, FLAG_CLIENT);

	    // Critical section.
        // Place a random string into shared memory
        int str_index = rand() % STRINGS_NUM;
        sprintf(shared_mem_ptr->buf[shared_mem_ptr->string_index], "(%d): %s\n", getpid(), strings[str_index]);

        (shared_mem_ptr->string_index)++;
        if (shared_mem_ptr->string_index == MAX_BUFFERS) {
            shared_mem_ptr->string_index = 0;
        }

        // Release SEM_MUTEX: V(SEM_MUTEX)
        sem_v(semid, SEM_MUTEX, FLAG_CLIENT);
	    // Tell server that a new string is placed into shared memory. 
        // V(SEM_BUF_EMPTY);
        sem_v(semid, SEM_BUF_EMPTY, FLAG_CLIENT);

        sleep(1);
    }

    // never reaches
    return 0;
}
/**************************************************************
 * Function: void parent_sigint(int signum)
 * Description: A signal handler for parent process.
 *              handle ctrl+c signal and exit the program
 **************************************************************/
void parent_sigint(int signum) {
    fprintf(stdout, "%sGot ctrl+c signal in parent. pid = %d.%s\n", FONT_YELLOW, getpid(), FONT_DEFAULT);

    pid_t pid_wait;
    int j;
    for (j = 0; j < MAX_PROCESSES; ++j) {
        pid_wait = wait(NULL);
        printf("%sClient exited. PID=%d.%s\n", FONT_YELLOW, pid_wait, FONT_DEFAULT);
    }

    exit(EXIT_SUCCESS);
}

// main program
int main() {
    pid_t pid[MAX_PROCESSES];
    int i = 0;

    srand((uint)time(NULL));
    // 1. Catch ctrl+c signal for parent process
    signal(SIGINT, parent_sigint);
    // 2. Fork some child processes
    for (i = 0; i < MAX_PROCESSES; ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        sleep((int)(rand() % 5) + 1);
    }
    // 3. make the children running client_process() and the parent waiting for children exiting
    if (i < MAX_PROCESSES) {
        // child process
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client_process();
    } else if (i == MAX_PROCESSES) {
        // Parent process
        printf("Parent process. Waiting for child processes exiting.\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < MAX_PROCESSES; ++j) {
            pid_wait = wait(NULL);
            printf("%sClient exited. PID=%d.%s\n", FONT_RED, pid_wait, FONT_DEFAULT);
        }
    }

    return EXIT_SUCCESS;
}
