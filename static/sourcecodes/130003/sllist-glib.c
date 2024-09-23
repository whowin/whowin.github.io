/*
 * File: sllist-glib.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Implementation of singly linked list with glib in C.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g sllist-glib.c -o sllist-glib `pkg-config --cflags glib-2.0` 
 * Usage: $ ./sllist-glib
 * 
 * Example source code for article 《如何使用GLib的单向链表GList》
 *
 */
#include <stdio.h>
#include <glib.h>

/*********************************************************************************
 * Function: void print_node(gpointer data, gpointer user_data)
 * Description: Print the data of a node, it will be called by foreach function
 * Parameter:   data        data of a node
 *              user_data   user's data
 ********************************************************************************/
void print_node(gpointer data, gpointer user_data) {
    printf("%d -> ", GPOINTER_TO_INT(data));
}
/******************************************************************
 * Function: void print_list(GSList *list)
 * Description: Traverse the list
 * Parameter:   list        head pointer of the list
 ******************************************************************/
void print_list(GSList *list) {
    g_slist_foreach(list, &print_node, NULL);
    printf("NULL\n");
}

int main() {
    GSList *list = NULL;

    printf("Append 4 nodes, the data are 1, 2, 3, 5.\n");
    list = g_slist_append(list, GINT_TO_POINTER(1));
    list = g_slist_append(list, GINT_TO_POINTER(2));
    list = g_slist_append(list, GINT_TO_POINTER(3));
    list = g_slist_append(list, GINT_TO_POINTER(5));
    print_list(list);

    printf("Insert a new node after node with the data 3.\n");
    list = g_slist_insert(list, GINT_TO_POINTER(4), 3);
    print_list(list);

    printf("Remove node with the data 3.\n");
    list = g_slist_remove(list, GINT_TO_POINTER(3));
    print_list(list);

    // Free the list
    g_slist_free(list);

    return 0;
}
