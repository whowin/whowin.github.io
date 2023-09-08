/*
 * File: sem-philo.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Solution for the dinning philosopers problem with system V semaphore set.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall sem-philo.c -o sem-philo
 * Usage: $ ./sem-philo
 *
 * Example source code for article 《IPC之五：使用 System V 信号量集解决经典的'哲学家就餐问题'》
 * https://whowin.gitee.io/post/blog/linux/0015-systemv-semaphore-sets/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
 
#define PHIL_NUM            5               // # of philospher
#define STATE_THINKING      2               // the philospher is thinking
#define STATE_HUNGRY        1               // the philosopher is hungry
#define STATE_EATING        0               // the philosopher is eating
#define LEFT(phnum)         (phnum + PHIL_NUM - 1) % PHIL_NUM
#define RIGHT(phnum)        (phnum + 1) % PHIL_NUM

#define MUTEX_IND           0               // the index of the first semaphore in the set
#define PHIL(n)             (n + 1)         // the index of the semaphore for each philosopher in the set

#define FONT_RED            "\033[31m"
#define FONT_YELLOW         "\033[33m"
#define FONT_DEFAULT        "\033[0m"
 
int state[PHIL_NUM];        // philosophers' state
int phil[PHIL_NUM];         // parameters that pass to threads

int semid = -1;             // identifier of semaphore set
pthread_t thread_id[PHIL_NUM];

/***************************************************************
 * Function: int sem_op(int semnum, int op)
 * Description: Operate a semaphore in the set
 * 
 * Arguments: 
 *      semnum      the index of semaphore in the set
 *      op          semaphore operation
 * Return:
 *      0       success
 *      -1      failure
 ***************************************************************/
int sem_op(int semnum, int op) {
    struct sembuf sops;

    sops.sem_num = semnum;
    sops.sem_op = op;
    sops.sem_flg = 0;
    if (semop(semid, &sops, 1) == -1) {
        perror("semctl()");
        return -1;
    }

    return 0;
}
/**************************************************************
 * Function: int sem_p(int semnum)
 * Description: Execute P operation for a semaphore in the set
 * 
 * Arguments:   semnum  index of semaphore in the set
 * Return:      0       success
 *              -1      failure
 **************************************************************/
int sem_p(int semnum) {
    return sem_op(semnum, -1);
}
/**************************************************************
 * Function: int sem_v(int semnum)
 * Description: Execute V operation for a semaphore in the set
 * 
 * Arguments:   semnum  index of semaphore in the set
 * Return:      0       success
 *              -1      failure
 **************************************************************/
int sem_v(int semnum) {
    return sem_op(semnum, 1);
}
/**************************************************************
 * Function: void sigint(int signum)
 * Description: A signal handler. 
 *              handle ctrl+c signal and exit the program
 **************************************************************/
void sigint(int signum) {
    fprintf(stdout, "\nThe program terminated by user.\n");

    for (int i = 0; i < PHIL_NUM; i++) {
        pthread_cancel(thread_id[i]);
        sem_v(PHIL(i));
        pthread_join(thread_id[i], NULL);       // waiting a thread exits
        fprintf(stdout, "The thread #%d has exited.\n", i);
    }

    if (semid >= 0) {
        semctl(semid, 0, IPC_RMID);
    }
    exit(EXIT_SUCCESS);
}
/***********************************************************************
 * Function: void hungry_to_eating(int phnum)
 * Description: Try to set a philosopher's state from hungry to eating
 * 
 * Arguments:   phnum   philosopher's number
 * Return:  none
 ***********************************************************************/
void hungry_to_eating(int phnum) {
    if (state[phnum] == STATE_HUNGRY && 
        state[LEFT(phnum)] != STATE_EATING && 
        state[RIGHT(phnum)] != STATE_EATING)
    {
        // The philosopher eats if his neighbors are not eating
        // set state to eating
        state[phnum] = STATE_EATING;
        printf("%sPhilosopher #%d takes forks %d and %d%s\n", 
                FONT_YELLOW, phnum + 1, LEFT(phnum) + 1, phnum + 1, FONT_DEFAULT);
        printf("Philosopher #%d is Eating\n", phnum + 1);
    }
}
/*********************************************************************
 * Function: void *philospher(void *num)
 * Description: Thread program. Control the actions of a philosopher
 * 
 * Arguments:   num     a pointer of the philosopher's number
 *********************************************************************/
void *philospher(void *num) {
    int i = *(int *)num;        // the philosopher's number

    while (1) {
        sleep(rand() % 8);
        // 1. update state from thinking to hungry
        //-----------------------------------------
        sem_p(MUTEX_IND);            // access the critical section
        state[i] = STATE_HUNGRY;
        printf("Philosopher #%d is Hungry\n", i + 1);

        // 2. update state from hungry to eating
        hungry_to_eating(i);
        sem_v(MUTEX_IND);                   // leave the critical section
        while (state[i] == STATE_HUNGRY) {
            sem_p(PHIL(i));                 // wait for eating
            sem_p(MUTEX_IND);               // access the critical section
            hungry_to_eating(i);            // try to update the state from hungry to eating
            sem_v(MUTEX_IND);               // leave the critical section
            sleep(0);
        }

        sleep(rand() % 8);                  // eat for a while

        // 3. update state from eating to thinking
        sem_p(MUTEX_IND);                   // leave the critical section
        state[i] = STATE_THINKING;
        printf("%sPhilosopher #%d puts", FONT_RED, i + 1);
        printf(" forks %d and %d down%s\n", LEFT(i) + 1, i + 1, FONT_DEFAULT);
        printf("Philosopher #%d is thinking\n", i + 1);

        if (state[LEFT(i)] == STATE_HUNGRY) {
            // if the philosopher on the left is waiting to eat, give him a notice
            sem_v(PHIL(LEFT(i)));
        }

        if (state[RIGHT(i)] == STATE_HUNGRY) {
            // if the philosopher on the right is waiting to eat, give him a notice
            sem_v(PHIL(RIGHT(i)));
        }

        sem_v(MUTEX_IND);                   // leave the critical section
    }
}

// main program
int main() {
    int i;
    //pthread_t thread_id[PHIL_NUM];

    srand((uint)time(NULL));
    // initialize stste and phil arraies
    for (i = 0; i < PHIL_NUM; ++i) {
        state[i] = STATE_THINKING;
        phil[i] = i;                // arguments that pass to threads
    }
    // initialize the semaphore set
    // There are N + 1 semaphores in the set. The first is for mutex, others for philosophers
    key_t key = ftok("./", 'p');
    semid = semget(key, PHIL_NUM + 1, IPC_CREAT | IPC_EXCL | 0666);

    signal(SIGINT, sigint);         // Catch ctrl+c 
    for (i = 0; i < PHIL_NUM; i++) {
        // create philosopher threads
        pthread_create(&thread_id[i], NULL, philospher, &phil[i]);
        printf("Philosopher #%d is thinking\n", i + 1);
    }
    sem_v(MUTEX_IND);        // initial the mutex to start the threads

    // Acturally the program never reaches the codes below
    // because the only exit way is ctrl+c, so signal handler handles the exit
    for (i = 0; i < PHIL_NUM; i++) {
        pthread_join(thread_id[i], NULL);       // waiting a thread exits
    }

    semctl(semid, 0, IPC_RMID);         // Remove the semaphore set

    exit(EXIT_SUCCESS);
}

