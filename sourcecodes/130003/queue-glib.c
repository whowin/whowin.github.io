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
 * To compile: $ gcc -Wall -g queue-glib.c -o queue-glib `pkg-config --cflags glib-2.0` 
 * Usage: $ ./queue-glib
 * 
 * Example source code for article 《如何使用GLib的单向链表GList》
 *
 */
#include <stdio.h>
#include <glib.h>

#define QUEUE_MAX_LEN           10
GSList *queue_head, *queue_tail;        // head and tail pointers of the queue
guint32 queue_max_len;                  // Max. length of the queue

/***************************************************************************
 * Function: void queue_init(int maxn)
 * Description: Initialize the queue
 * Parameter:   maxn        Max. length of the queue
 * Return:  NONE
 **************************************************************************/
void queue_init(int maxn) {
    queue_head = queue_tail = NULL;
    queue_max_len = maxn;
}
/**************************************************************************
 * Function: gboolean queue_put(gpointer data)
 * Description: Append a node with data to the queue
 * Parameter:   data        point to the data of node
 * Return:      TRUE        Success
 *              FALSE       The queue is full
 *************************************************************************/
gboolean queue_put(gpointer data) {
    guint queue_len = g_slist_length(queue_head);           // length of the queue
    if (queue_len >= queue_max_len) {
        // the queue is full
        return FALSE;
    } else {
        queue_head = g_slist_append(queue_head, data);      // append a node with data to the queue
        queue_tail = g_slist_last(queue_head);              // get the pointer of last node
    }
    return TRUE;
}
/************************************************************************
 * Function: gpointer queue_get()
 * Description: Get the data from header of the queue
 * Parameter:   not NULL    point to the data
 *              NULL        the queue is empty
 ***********************************************************************/
gpointer queue_get() {
    guint queue_len = g_slist_length(queue_head);           // length of the queue
    if (queue_len == 0) {
        // the queue is empty
        return NULL;
    }
    gpointer queue_data = queue_tail->data;                 // data pointer of the last node
    queue_head = g_slist_delete_link(queue_head, queue_tail);   // delete the last node
    if (queue_head == NULL) {
        queue_tail = queue_head;                            // the queue is empty
    } else {
        queue_tail = g_slist_last(queue_head);              // get the pointer of last node
    }
    return queue_data;
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
    queue_init(len);            // Initialize the queue

    guint16 i;
    // append some data to the queue
    for (i = 0; i < (queue_max_len << 1); ++i) {
        if (queue_put(GINT_TO_POINTER(i + 1))) {
            printf("Put data %d into the queue.\n", i + 1);
        } else {
            printf("The queue is full.\n");
            break;
        }
    }
    // get some data from the queue
    for (i = 0; i < (queue_max_len << 1); ++i) {
        gpointer queue_data = queue_get();
        if (queue_data != NULL) {
            printf("Get data %d from the queue.\n", GPOINTER_TO_INT(queue_data));
        } else {
            printf("The queue is empty.\n");
            break;
        }
    }

    return 0;
}

