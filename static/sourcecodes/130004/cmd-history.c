/*
 * File: cmd-history.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Implementation of singly linked list with glib in C.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g cmd-history.c -o cmd-history `pkg-config --cflags --libs glib-2.0` 
 * Usage: $ ./cmd-history
 * 
 * Example source code for article 《双向链表及如何使用GLib的GList实现双向链表》
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <glib.h>

#define CLEAR_LINE              "\033[2K"
#define KEY_ESC                 27
#define KEY_SQUARE_BRACKET      '['
#define KEY_A                   'A'
#define KEY_B                   'B'

#define UP_ARROW_KEY            -1
#define DOWN_ARROW_KEY          1
#define NONE_KEY                0

typedef struct {
    GList *head;                // head pointer of the queue
    GList *tail;                // tail pointer of the queue
    GList *current;             // current pointer
    gint size;                  // queue size
} Queue;

/*********************************************************************************
 * Function: void enable_raw_mode()
 * Description: Set the terminal to raw mode(noncanonical mode)
 * Parameter:   NONE
 * Return:      NONE
 ********************************************************************************/
void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);    // turn off echo and canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/*********************************************************************************
 * Function: void disable_raw_mode()
 * Description: Set the terminal to canonical mode
 * Parameter:   NONE
 * Return:      NONE
 ********************************************************************************/
void disable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);     // turn on echo and canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/*********************************************************************************
 * Function: Queue *create_queue()
 * Description: Create a new queue
 * Parameter:   NONE
 * Return:      queue pointer
 ********************************************************************************/
Queue *create_queue() {
    Queue *queue = malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->current = NULL;
    queue->size = 0;
    return queue;
}
/*********************************************************************************
 * Function: void enqueue(Queue *queue, gint data)
 * Description: Enqueue operation(Add a node at the head)
 * Parameter:   queue       queue pointer
 *              data        data of the new node
 * Return:      NONE
 ********************************************************************************/
void enqueue(Queue *queue, GString *data) {
    queue->head = g_list_prepend(queue->head, data);
    if (queue->tail == NULL) {
        queue->tail = queue->head;
    }
    queue->size++;
    queue->current = queue->head;
}
/***********************************************************************************
 * Function: gint dequeue(Queue *queue)
 * Description: Dequeue operation(get a node from current and then update current)
 * Parameter:   queue       queue pointer
 *              direction   0  - don't update current pointer
 *                          1  - forward(current pointer moves towards head)
 *                          -1 - backward(current pointer moves towards tail)
 * Return:      >=0         data got from the queue
 *              -1          the queue is empty
 **********************************************************************************/
GString *dequeue(Queue *queue, gint direction) {
    if (queue->current == NULL) {
        //printf("Queue is empty!\n");
        return NULL;                  // the queue is empty
    }
    GString *data;
    if (direction == 1) {
        if (queue->current == queue->head) {
            return NULL;
        }
        queue->current = queue->current->prev;
    } else if (direction == -1) {
        if (queue->current == queue->tail) {
            return NULL;
        }
        queue->current = queue->current->next;
    }
    data = queue->current->data;
    return data;
}
/*********************************************************************************
 * Function: gint queue_size(Queue *queue)
 * Description: Get the size of queue
 * Parameter:   queue       queue pointer
 * Return:      the size ofthe queue
 ********************************************************************************/
gint queue_size(Queue *queue) {
    return queue->size;
}
/*********************************************************************************
 * Function: void destroy_queue(Queue *queue)
 * Description: Destroy the queue
 * Parameter:   queue       queue pointer
 * Return:      NONE
 ********************************************************************************/
void destroy_queue(Queue *queue) {
    queue->current = NULL;
    while (queue->head != NULL) {
        g_string_free(queue->head->data, TRUE);
        queue->head = g_list_delete_link(queue->head, queue->head);
    }
    free(queue);
}

int main() {
    const gchar *num_str[] = {"1st", "2nd", "3rd"};
    Queue *queue = create_queue();          // Create a new queue

    gchar *cmd_buf = NULL;
    gsize buf_len = 0;
    gssize cmd_len = 0;
    do {
        if (queue_size(queue) < 3) {
            printf("Please enter the %s Linux command('e' to exit): ", num_str[queue_size(queue)]);
        } else {
            printf("Please enter the %dth Linux command('e' to exit): ", queue_size(queue) + 1);
        }
        
        cmd_len = getline(&cmd_buf, &buf_len, stdin);   // Read a line from keyboard
        if (cmd_buf[cmd_len - 1] == '\n') cmd_buf[cmd_len -1] = '\0'; 
        GString *cmd = g_string_new(cmd_buf);
        enqueue(queue, cmd);                            // add a command to queue
    } while (strcmp(cmd_buf, "e") != 0);

    printf("\nA total of %d commands were entered.\n", queue_size(queue));
    printf("The last command entered is now displayed. \n");
    printf("Press the up and down arrow keys to view the previous and next command entered.\n");
    printf("Press 'q' to exit.\n\n");

    gint direction = NONE_KEY;
    gchar ch = '\0';
    enable_raw_mode();      // Set the terminal to noncanonical mode
    do {
        GString *cmd = dequeue(queue, direction);       // get a command from queue, update the current pointer
        if (cmd != NULL) {
            printf("%s\r%s", CLEAR_LINE, cmd->str);     // Clear the current line
            fflush(stdout);                             // forces writing the buffered data to terminal
        }
        read(STDIN_FILENO, &ch, 1);                     // Read a charcter from keyboard
        direction = 0;
        switch (ch) {
            case KEY_ESC:
                read(STDIN_FILENO, &ch, 1);
                if (ch == '[') {
                    read(STDIN_FILENO, &ch, 1);
                    if (ch == 'A') {
                        // up arrow
                        direction = UP_ARROW_KEY;
                    } else if (ch == 'B') {
                        // down arrow
                        direction = DOWN_ARROW_KEY;
                    }
                }
                break;

            default:
                break;

        }
    } while (ch != 'q');
    printf("\n");
    disable_raw_mode();

    destroy_queue(queue);
    return 0;
}
