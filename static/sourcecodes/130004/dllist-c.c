/*
 * File: dllist-c.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Implementation of doubly linked list in C.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g dllist-c.c -o dllist-c
 * Usage: $ ./dllist-c
 * 
 * Example source code for article 《双向链表及如何使用GLib的GList实现双向链表》
 *
 */
#include <stdio.h>
#include <stdlib.h>

// Define node structure of doubly linked list
struct Node {
    int data;
    struct Node *next;
    struct Node *prev;
};

/**************************************************************************
 * Function: struct Node *create_node(int data)
 * Description: Create a new node with the specified data
 * 
 * parameter:   data        new node data
 * Return:      pointer that point to the new node
 **************************************************************************/
struct Node *create_node(int data) {
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = NULL;
    return new_node;
}

/****************************************************************************
 * Function: void append(struct Node **head_ref, int new_data)
 * Description: Add a node at the end of the linked list
 * 
 * parameter:   head_ref    pointer to the start pointer of the linked list
 *              new_data    data of the new node
 * Return:      NONE
 ****************************************************************************/
void append(struct Node **head_ref, int new_data) {
    struct Node *new_node = create_node(new_data);
    if (*head_ref == NULL) {
        *head_ref = new_node;
        return;
    }

    struct Node *last = *head_ref;
    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_node;
    new_node->prev = last;
}
/****************************************************************************
 * Function: struct Node *get_node(struct Node *head, int node_index)
 * Description: Get the pointer to the node with the specified index number
 * 
 * parameter:   head        start pointer of the linked list
 *              node_index  node index number
 * Return:      pointer to the found node
 ****************************************************************************/
struct Node *get_node(struct Node *head, int node_index) {
    if (head == NULL) return NULL;
    if (node_index < 0) return NULL;

    struct Node *node = head;
    int i = 0;
    while (node->next != NULL) {
        if (i == node_index) return node;
        ++i;
        node = node->next;
    }
    return NULL;
}
/******************************************************************************
 * Function: void insert(struct Node **head_ref, int new_data, int node_index)
 * Description: Insert a node after the specified node
 * 
 * parameter:   head_ref    pointer to the start pointer of the linked list
 *              new_data    data of the new node
 *              node_index  node index number
 * Return:      0       Success
 *              -1      Failed
 ******************************************************************************/
int insert(struct Node **head_ref, int new_data, int node_index) {
    struct Node *node = get_node(*head_ref, node_index);
    if (node == NULL) return -1;

    struct Node *new_node = create_node(new_data);
    new_node->prev = node;
    new_node->next = node->next;
    node->next = new_node;

    return 0;
}
/******************************************************************************
 * Function: void remove_node(struct Node **head_ref, int node_index)
 * Description: Remove the specified node
 * 
 * parameter:   head_ref    pointer to the start pointer of the linked list
 *              node_index  node index number that will be removed
 * Return:      0       Success
 *              -1      Failed
 ******************************************************************************/
int remove_node(struct Node **head_ref, int node_index) {
    if (*head_ref == NULL) return -1;

    struct Node *node = get_node(*head_ref, node_index);
    if (node == NULL) return -1;

    if (*head_ref == node) {
        *head_ref = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    free(node);
    return 0;
}
/******************************************************************************
 * Function: void print_list(struct Node *head)
 * Description: Print the whole linked list
 * 
 * parameter:   head_ref    pointer to the start pointer of the linked list
 *              node_index  node index number that will be removed
 * Return:      0       Success
 *              -1      Failed
 ******************************************************************************/
void print_list(struct Node *head) {
    struct Node *node = head;
    //printf("Traversal in forward direction:\n");
    while (node != NULL) {
        printf("%d -> ", node->data);
        node = node->next;
    }
    printf("NULL\n");
}
/******************************************************************************
 * Function: void free_list(struct Node *head)
 * Description: Free memory of linked list
 * 
 * parameter:   head_ref    pointer to the start pointer of the linked list
 *              node_index  node index number that will be removed
 * Return:      0       Success
 *              -1      Failed
 ******************************************************************************/
// Free memory of linked list
void free_list(struct Node *head) {
    struct Node *node;
    while (node != NULL) {
        node = node->next;
        free(node->prev);
    }
}

int main() {
    struct Node *head = NULL;

    printf("Append 4 nodes, the data are 1, 2, 3, 5.\n");
    append(&head, 1);
    append(&head, 2);
    append(&head, 3);
    append(&head, 5);
    print_list(head);

    printf("Insert a new node after node with the index 2.\n");
    insert(&head, 4, 2);
    print_list(head);

    printf("Remove node with the index 2.\n");
    remove_node(&head, 2);
    print_list(head);

    free_list(head);            // Free all nodes
    return 0;
}
