/*
 * File: pizzeria.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Simulate the process of cooking pizza at a pizzeria using shared memory.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall pizzeria.c -o pizzeria -lpthread -lrt
 * Usage: $ ./pizzeria
 *
 * Example source code for article 《IPC之八：使用 POSIX 共享内存进行进程间通信的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>

#include <sys/wait.h>

#define COOKS_NUM           2       // # of cooks
#define WAITERS_NUM         3       // # of waiters
#define MAX_PIZZA_PER_SHELF 5       // Max. # of pizzas per shelf

#define SHM_NAME            "shm_cook_waiters"
#define SHM_SIZE            COOKS_NUM * sizeof(int)

#define SEM_FILL_NAME       "sem_shelf_fill"
#define SEM_AVAIL_NAME      "sem_shelf_availiable"
#define SEM_MUTEX_NAME      "sem_pizzeria_mutex"

#define SHM_PERMS           0666
#define SEM_PERMS           0666

#define SEM_FILL_VALUE      0       // initial value of sem_fill
#define SEM_AVAIL_VALUE     MAX_PIZZA_PER_SHELF * COOKS_NUM   // initial value of sem_avail
#define SEM_MUTEX_VALUE     1       // initial value of sem_fill

#define FONT_RED            "\033[31m"
#define FONT_YELLOW         "\033[33m"
#define FONT_DEFAULT        "\033[0m"

void error(char *msg) {
    perror(msg);
    kill(getpid(), SIGINT);
}

int shmfd = -1;                     // file descriptor of shared memory
int *shm_shelf = NULL;              // for mapping the shared memory
sem_t *sem_fill = NULL, *sem_avail = NULL, *sem_mutex = NULL;

int pizza_count = 0, shelf_index = 0;

// CTRL+C handler of cook process
void cook_sigint(int signum) {
    printf("\nCook(%d): I cooked %d pizzas. %d %s left.\n", 
            getpid(), pizza_count, shm_shelf[shelf_index], (shm_shelf[shelf_index] == 1)?"is":"are");
    /* remove semaphores*/
    if (sem_fill != NULL) {
        sem_close(sem_fill);
        sem_unlink(SEM_FILL_NAME);
    }
    if (sem_avail != NULL) {
        sem_close(sem_avail);
        sem_unlink(SEM_AVAIL_NAME);
    }
    if (sem_mutex != NULL) {
        sem_close(sem_mutex);
        sem_unlink(SEM_MUTEX_NAME);
    }

    /* remove shared memory segment*/
    if (shm_shelf != NULL) {
        munmap(shm_shelf, sizeof(int));
    }
    if (shmfd != -1) {
        close(shmfd);
    }
    shm_unlink(SHM_NAME);
    printf("Cook(%d): Terminated by user.\n", getpid());

    exit(EXIT_SUCCESS);
}
// cook process
void cook_process(int index) {
    shelf_index = index;            // index number of pizza shelf

    srand((uint)time(NULL));
    signal(SIGINT, cook_sigint);

    // 1. make *shm_shelf shared between processes
    // create the shared memory segment
    shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR, SHM_PERMS);
    if (shmfd == -1) {
        error("shm_get()");
    }
    // configure the size of the shared memory segment
    ftruncate(shmfd, SHM_SIZE);
    // map the shared memory segment in process address space
    shm_shelf = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shm_shelf == MAP_FAILED) {
        error("mmap()");
    }
    close(shmfd);
    shmfd = -1;

    // 2. creat/open semaphores
    // cook post sem_fill after cooking a pizza
    sem_fill = sem_open(SEM_FILL_NAME, O_CREAT, SEM_PERMS, SEM_FILL_VALUE);
    if (sem_fill == SEM_FAILED) {
        error("sem_open()");
    }
    // waiter post sem_avail after picking up a pizza, at the beginning avail = 5
    sem_avail = sem_open(SEM_AVAIL_NAME, O_CREAT, SEM_PERMS, SEM_AVAIL_VALUE);
    // mutex for mutual exclusion of shelf
    sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, SEM_PERMS, SEM_MUTEX_VALUE);

    // 3. forever loop until ctrl+c
    printf("\nCook(%d): I have started cooking pizza.\n", getpid());
    while (1) {
        sleep(rand() % 3 + 1);
        if (shm_shelf[shelf_index] < (SEM_AVAIL_VALUE / COOKS_NUM)) {
            //sem_wait(sem_avail);
            sem_wait(sem_mutex);
            shm_shelf[shelf_index]++;
            sem_post(sem_mutex);
            printf("%sCook(%d)%s: cook a pizza on the shelf No.%d, there %s %d pizza now\n", 
                    FONT_RED, getpid(), FONT_DEFAULT, shelf_index + 1,
                    (shm_shelf[shelf_index] == 1)?"is":"are", shm_shelf[shelf_index]);
            pizza_count++;
            sem_post(sem_fill);
        }
    }

    return;
}

// CTRL+C handler
void waiter_sigint(int signum) {

    /* remove semaphores*/
    if (sem_fill != NULL) {
        sem_close(sem_fill);
    }
    if (sem_avail != NULL) {
        sem_close(sem_avail);
    }
    if (sem_mutex != NULL) {
        sem_close(sem_mutex);
    }

    /* remove shared memory segment*/
    if (shm_shelf != NULL) {
        munmap(shm_shelf, sizeof(int));
    }
    if (shmfd != -1) {
        close(shmfd);
    }
    printf("Waiter(%d): Terminated by user.\n", getpid());

    exit(EXIT_SUCCESS);
}

void waiter_process() {
    int i;
    int count = 0;

    srand((uint)time(NULL));
    signal(SIGINT, waiter_sigint);
    // 1. open the shared memory object
    shmfd = shm_open(SHM_NAME, O_RDWR, SHM_PERMS);
    if (shmfd == -1) {
        error("shm_get()");
    }

    // 2. map the shared memory object in the address space of the process
    shm_shelf = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shm_shelf == MAP_FAILED) {
        error("mmap()");
    }
    close(shmfd);
    shmfd = -1;

    // 3. open semaphores
    sem_fill = sem_open(SEM_FILL_NAME, O_EXCL);
    sem_avail = sem_open(SEM_AVAIL_NAME, O_EXCL);
    sem_mutex = sem_open(SEM_MUTEX_NAME, O_EXCL);

    // 4. forever loop until ctrl+c
    while (1) {
        sleep(rand() % 4 + 1);
        sem_wait(sem_fill);
        sem_wait(sem_mutex);
        i = ++count % COOKS_NUM;
        if (shm_shelf[i] > 0) {
            shm_shelf[i]--;
        } else {
            for (i = 0; i < COOKS_NUM; ++i) {
                if (shm_shelf[i] > 0) {
                    shm_shelf[i]--;
                    break;
                }
            }
        }
        sem_post(sem_mutex);
        printf("%sWaiter(%d)%s: I pick up a pizza from the shelf No.%d\n", 
                FONT_YELLOW, getpid(), FONT_DEFAULT, i + 1);
        //sem_post(sem_avail);
    }

    return;
}

int main() {
    pid_t pid[WAITERS_NUM];
    int i = 0;

    srand((uint)time(NULL));

    // 1. Fork cook processes and waiter processes
    for (i = 0; i < (COOKS_NUM + WAITERS_NUM); ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        sleep((int)(rand() % 5) + 1);
    }
    // 2. the first COOKS_NUM process are cook_process, others are waiter_process
    if (i < COOKS_NUM) {
        // child process, for cooks
        printf("%d. Cook process. PID=%d.\n", i + 1, getpid());
        cook_process(i);        // i is the index # of shelfs
    } else if (i < (COOKS_NUM + WAITERS_NUM)) {
        // child process, for waiters
        printf("%d. Waiter process. PID=%d.\n", i + 1, getpid());
        waiter_process();
    } else if (i == (COOKS_NUM + WAITERS_NUM)) {
        // Parent process, just waiting all children exit
        printf("Parent process. Waiting for child processes exiting.\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < (COOKS_NUM + WAITERS_NUM); ++j) {
            pid_wait = wait(NULL);
            printf("%sClient exited. PID=%d.%s\n", FONT_RED, pid_wait, FONT_DEFAULT);
        }
    }

    return EXIT_SUCCESS;
}
