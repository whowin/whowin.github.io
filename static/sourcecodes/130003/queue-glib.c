/*
 * File: qeue-glib.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Implementation of FIFO queue with glib in C.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g queue-glib.c -o queue-glib `pkg-config --cflags --libs glib-2.0` 
 * Usage: $ ./queue-glib
 * 
 * Example source code for article 《如何使用GLib的单向链表GList》
 *
 */
#include <stdio.h>
#include <glib.h>

#define QUEUE_MAX_LEN           10

typedef struct {
    GSList *head;               // head pointer of the queue
    GSList *tail;               // tail pointer of the queue
    gint size;                  // size of the queue
    gint max_size;              // max. size of the queue
} Queue;

/***************************************************************************
 * Function: Queue *queue_init(int maxn)
 * Description: Initialize the queue
 * Parameter:   maxn        Max. length of the queue
 * Return:  NONE
 **************************************************************************/
Queue *queue_init(int maxn) {
    Queue *queue = malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->max_size = maxn;
    return queue;
}
/*********************************************************************************
 * Function: void enqueue(Queue *queue, gint data)
 * Description: Enqueue operation(Add a node at the head)
 * Parameter:   queue       queue pointer
 *              data        data of the new node
 * Return:      NONE
 ********************************************************************************/
gboolean enqueue(Queue *queue, gint data) {
    if (queue->size >= queue->max_size) {
        return FALSE;
    }
    queue->head = g_slist_prepend(queue->head, GINT_TO_POINTER(data));
    if (queue->tail == NULL) {
        queue->tail = queue->head;
    }
    queue->size++;
    return TRUE;
}
/*********************************************************************************
 * Function: gint dequeue(Queue *queue)
 * Description: Dequeue operation(get a node from tail and then delete it)
 * Parameter:   queue       queue pointer
 * Return:      >=0         data got from the queue
 *              -1          the queue is empty
 ********************************************************************************/
gint dequeue(Queue *queue) {
    if (queue->tail == NULL) {
        return -1;                  // the queue is empty
    }
    gint data = GPOINTER_TO_INT(queue->tail->data);

    queue->head = g_slist_delete_link(queue->head, queue->tail);
    queue->tail = g_slist_last(queue->head);
    queue->size--;
    
    return data;
}

int main(int argc, char **argv) {
    guint64 len;
    if (argc >= 2) {
        len = g_ascii_strtoll(argv[1], NULL, 10);           // Convert string to int
        if (len <= 0 || len > (QUEUE_MAX_LEN * 10)) {
            len = QUEUE_MAX_LEN;
        }
    } else {
        len = QUEUE_MAX_LEN;
    }

    printf("Max. length of the queue is %ld.\n", len);
    Queue *queue = queue_init(len);            // Initialize the queue

    guint16 i;
    // append some data to the queue
    for (i = 0; i < (queue->max_size << 1); ++i) {
        if (enqueue(queue, (i + 1))) {
            printf("Put data %d into the queue.\n", i + 1);
        } else {
            printf("The queue is full.\n");
            break;
        }
    }
    // get some data from the queue
    for (i = 0; i < (queue->max_size << 1); ++i) {
        gint queue_data = dequeue(queue);
        if (queue_data >= 0) {
            printf("Get data %d from the queue.\n", queue_data);
        } else {
            printf("The queue is empty.\n");
            break;
        }
    }

    free(queue);

    return 0;
}

