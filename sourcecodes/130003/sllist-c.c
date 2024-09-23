/*
 * File: sllist-c.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Implementation of singly linked list in C.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g sllist-c.c -o sllist-c
 * Usage: $ ./sllist-c
 * 
 * Example source code for article 《如何使用GLib的单向链表GList》
 *
 */
#include <stdio.h>
#include <stdlib.h>

// Define a structure of the node in linked list
typedef struct Node {
    int data;
    struct Node *next;
} Node;
/**********************************************************************
 * Function: Node *create_node(int data)
 * Description: Create a new node创建一个新节点，返回节点指针
 * parameter:   data        node data
 * Return:      the pointer of the new node
 **********************************************************************/
Node *create_node(int data) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}
/*********************************************************************
 * Function: Node *search_node(Node *head, int index)
 * Description: Search for a node by index number
 * Parameters:  head        head pointer of the list
 *              index       node index to search for
 * Return:      NULL        no nodes found
 *              others      pointer to the node found
 *********************************************************************/
Node *search_node(Node *head, int index) {
    Node *temp = head;
    int i = 0;
    while (i < index && temp != NULL) {
        temp = temp->next;
        ++i;
    }
    return temp;
}
/*********************************************************************
 * Function: void append_node(Node **head, int data)
 * Description: Append a new node at the end of the  list
 * Parameters:  head        head pointer of the list
 *              data        node data
 * Return:      NONE
 *********************************************************************/
void append_node(Node **head, int data) {
    Node *new_node = create_node(data);
    if (*head == NULL) {
        *head = new_node;
    } else {
        Node* temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}
/***********************************************************************
 * Function: void insert_node(Node **head, int index, int data)
 * Description: Insert a new node after the specified node in the llist
 * Paameters:   head        head pointer of the list
 *              index       index number of the specified node
 *              data        node data
 * Return:      NONE
 ***********************************************************************/
void insert_node(Node **head, int index, int data) {
    Node *new_node = create_node(data);
    if (*head == NULL) {
        *head = new_node;
    } else {
        Node *temp = search_node(*head, index);
        if (temp == NULL) {
            append_node(head, data);
        } else {
            Node *next = search_node(*head, index + 1);
            new_node->next = next;
            temp->next = new_node;
        }
    }
}
/*******************************************************************
 * Function: int remove_node(Node **head, int index)
 * Description: Remove the node with specified index number
 * Parameters:  head        head pointer of the list
 *              index       index of the node to be removed
 * Return:      1           removed successfully
 *              0           removal failed
 *******************************************************************/
int remove_node(Node **head, int index) {
    Node *prev, *temp, *next;
    if (head == NULL) return 0;
    if (index == 0) {
        temp = *head;
        *head = (*head)->next;
        free(temp);
        return 1;
    }
    temp = search_node(*head, index);
    if (temp == NULL) return 0;
    prev = search_node(*head, index - 1);
    if (prev == NULL) return 0;
    next = search_node(*head, index +1);
    if (next == NULL) {
        prev->next = NULL;
    } else {
        prev->next = next;
    }
    free(temp);
    return 1;
}
/******************************************************************
 * Function: void print_list(Node *head)
 * Description: Traverse the list
 * Parameter:   head        head pointer of the list
 ******************************************************************/
void print_list(Node *head) {
    Node* temp = head;
    while (temp != NULL) {
        printf("%d -> ", temp->data);
        temp = temp->next;
    }
    printf("NULL\n");
}
/******************************************************************
 * Function: void free_list(Node* head)
 * Description: Release the memory of the linked list
 * Parameter:   head        head pointer of the list
 ******************************************************************/
void free_list(Node* head) {
    Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

int main() {
    Node *head = NULL;
    printf("Append 4 nodes, the data are 1, 2, 3, 5.\n");
    append_node(&head, 1);
    append_node(&head, 2);
    append_node(&head, 3);
    append_node(&head, 5);
    print_list(head);

    printf("Insert a new node after node with the index 2.\n");
    insert_node(&head, 2, 4);
    print_list(head);

    printf("Remove node with the index 2.\n");
    remove_node(&head, 2);
    print_list(head);

    free_list(head);            // Free all nodes
    return 0;
}
