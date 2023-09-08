/*
 * File: producer-consumer.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Solution for producer consumer problem using POSIX semaphore.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall producer-consumer.c -o producer-consumer -lpthread
 * Usage: $ ./producer-consumer [production rate] [consumption rate]
 *
 * Example source code for article 《IPC之六：使用 POSIX 信号量解决经典的'生产者消费者问题'》
 * https://whowin.gitee.io/post/blog/linux/0016-posix-semaphores/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>


#define PROD_NUM        5           // number of producers
#define CONS_NUM        5           // number of consumers
#define PROD_RATE       5           // default production rate
#define CONS_RATE       6           // default consumption rate
#define MAX_RATE        20          // MAX. value of rate

#define BUF_SIZE        16          // Size of the buffer

#define FONT_RED        "\033[31m"
#define FONT_YELLOW     "\033[33m"
#define FONT_DEFAULT    "\033[0m"

sem_t sem_empty;                    // 0 means the buffer is emoty
sem_t sem_full;                     // 0 means the buffer is full
sem_t sem_mutex;                    // mutex for shared buffer
int in = 0;                         // index of the item that is placed in buffer
int out = 0;                        // index of the item that is taken from buffer
int buffer[BUF_SIZE];               // shared buffer

pthread_t prod_pid[PROD_NUM], cons_pid[CONS_NUM];   // threads' id

uint prod_rate = (MAX_RATE - PROD_RATE);    // default production rate 
uint cons_rate = (MAX_RATE - CONS_RATE);    // default consumption rates

/**************************************************************
 * Function: void sigint(int signum)
 * Description: A signal handler. 
 *              handle ctrl+c signal and exit the program
 **************************************************************/
void sigint(int signum) {
    fprintf(stdout, "\n%sThe program terminated by user.%s\n", FONT_RED, FONT_DEFAULT);

    for (int i = 0; i < PROD_NUM; i++) {
        pthread_cancel(prod_pid[i]);
        //sem_post(&sem_full);
        pthread_join(prod_pid[i], NULL);       // waiting a thread exits
        fprintf(stdout, "The producer thread #%d has exited.\n", i);
    }

    for (int i = 0; i < CONS_NUM; i++) {
        pthread_cancel(cons_pid[i]);
        //sem_post(&sem_empty);
        pthread_join(cons_pid[i], NULL);       // waiting a thread exits
        fprintf(stdout, "The consumer thread #%d has exited.\n", i);
    }

    sem_destroy(&sem_mutex);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);

    exit(EXIT_SUCCESS);
}
/******************************************************************************
 * Function: void *producer(void *pno)
 * Description: The thread program for producers
 * 
 * Arguments:   pro     just a index # of producer's thrads
 ******************************************************************************/
void *producer(void *pno) {   
    int item;
    int no = *(int *)pno;           // The index # of producer threads
    while (1) {
        item = rand();              // Produce an random item
        sem_wait(&sem_mutex);
        if (sem_trywait(&sem_full) == -1) {
            printf("%sProducer #%d: The buffer is full.\n%s", FONT_RED, no, FONT_DEFAULT);
            //sem_wait(&sem_full);
        } else {
            buffer[in] = item;
            printf("Producer #%d: Insert Item %d at %d\n", no, buffer[in], in);
            in = (in + 1) % BUF_SIZE;
            sem_post(&sem_empty);
        }
        sem_post(&sem_mutex);
        sleep(rand() % prod_rate);
    }
}
/******************************************************************************
 * Function: void *consumer(void *pno)
 * Description: The thread program for consumers
 * 
 * Arguments:   pro     just a index # of conmuser's thrads
 ******************************************************************************/
void *consumer(void *cno) {
    int no = *(int *)cno;           // index number of consumer threads

    while (1) {
        sem_wait(&sem_mutex);
        if (sem_trywait(&sem_empty) == -1) {
            printf("%sConsumer #%d: The buffer is empty.\n%s", FONT_YELLOW, no, FONT_DEFAULT);
            //sem_wait(&sem_empty);
        } else {
            int item = buffer[out];
            printf("Consumer #%d: Remove Item %d from %d\n", no, item, out);
            out = (out + 1) % BUF_SIZE;
            sem_post(&sem_full);
        }
        sem_post(&sem_mutex);
        sleep(rand() % cons_rate);
    }
}

void usage(char *prog) {
    printf("Usage: %s <production rate> <consumption rate>\n", prog);
    printf("          Large number indicates high rate.\n\n");
}
// main process
int main(int argc, char **argv) {   
    if ((argc > 1) && ((strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0))) {
        usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if ((argc > 1) && (atoi(argv[1]) > 0) && (atoi(argv[1]) < MAX_RATE)) {
        prod_rate = MAX_RATE - (uint)atoi(argv[1]);     // actual production rate
    } 

    if ((argc > 2) && (atoi(argv[2]) > 0) && (atoi(argv[2]) < MAX_RATE)) {
        cons_rate = MAX_RATE - (uint)atoi(argv[2]);     // actual consumption rate
    } 

    srand((uint)time(NULL));        // random seed

    // initial semaphores
    sem_init(&sem_empty, 0, 0);
    sem_init(&sem_full, 0, BUF_SIZE);
    sem_init(&sem_mutex, 0, 1);

    int a[5] = {1, 2, 3, 4, 5};     // Just used for numbering the producer and consumer

    signal(SIGINT, sigint);         // Catch ctrl+c 

    for (int i = 0; i < PROD_NUM; i++) {
        // Create threads for producers
        pthread_create(&prod_pid[i], NULL, (void *)producer, (void *)&a[i]);
    }
    for (int i = 0; i < CONS_NUM; i++) {
        // Create threads for consumers
        pthread_create(&cons_pid[i], NULL, (void *)consumer, (void *)&a[i]);
    }

    // never reachs here
    for (int i = 0; i < PROD_NUM; i++) {
        pthread_join(prod_pid[i], NULL);
    }
    for (int i = 0; i < CONS_NUM; i++) {
        pthread_join(cons_pid[i], NULL);
    }

    sem_destroy(&sem_mutex);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);

    exit(EXIT_SUCCESS);
}
