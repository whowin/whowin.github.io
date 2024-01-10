/*
 * File: dbus-objects.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Example of using libdbus to request service from the client to the server.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g dbus-objects.c -o dbus-objects `pkg-config --libs --cflags dbus-1`
 * Usage: $ ./dbus-objects
 *
 * Example source code for article 《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/wait.h>

#include <dbus/dbus.h>

#define CLIENT_NUM              3

#define SERVER_BUS_NAME         "cn.whowin.dbus"
#define OBJECT_PATH_NAME_YES    "/cn/whowin/dbus/yes"
#define OBJECT_PATH_NAME_NO     "/cn/whowin/dbus/no"
#define OBJECT_PATH_NAME_QUIT   "/cn/whowin/dbus/quit"
#define INTERFACE_NAME          "cn.whowin.dbus_demo"
#define METHOD_NAME             "hello"

#define BUF_SIZE                128
#define REPLY_TIMEOUT           500

/********************************************************************************
 * Function: void print_dbus_error(DBusError *dbus_error_p, char *str)
 * Description: Print the error message when involking dbus functions
 ********************************************************************************/
void print_dbus_error(DBusError *dbus_error_p, char *str) {
    printf("%s: %s\n", str, dbus_error_p->message);
    dbus_error_free(dbus_error_p);
}
/********************************************************************************
 * Function: void dbus_server()
 * Decription: D-Bus server process
 ********************************************************************************/
void dbus_server() {
    DBusError dbus_error;
    DBusConnection *conn;
    int ret;
    int exit_count = 0;

    // Step 1: get dbus connection
    //==============================
    dbus_error_init(&dbus_error);                           // Initializes a DBusError structure.
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);     // Connects to the bus daemon and registers the client with it.
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "Server: dbus_bus_get");
        exit(EXIT_FAILURE);
    }

    // Step 2: ask bus to assign the given name
    //==========================================
    ret = dbus_bus_request_name(conn, 
                                SERVER_BUS_NAME, 
                                DBUS_NAME_FLAG_DO_NOT_QUEUE,    // must be primary owner
                                &dbus_error);
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "Server: dbus_bus_request_name");
        exit(EXIT_FAILURE);
    }
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        printf("Server: not primary owner, ret = %d\n", ret);
        exit(EXIT_FAILURE);
    }

    bool loop = TRUE;
    // Loop when something can be read or writen
    while (dbus_connection_read_write_dispatch(conn, -1)) {
        // Step 3: Read the message
        //===========================
        DBusMessage *message;
        message = dbus_connection_pop_message(conn);
        if (message == NULL) {
            //printf("Server: Did not get message\n");
            continue;
        }

        ret = dbus_message_get_type(message);
        if (ret != DBUS_MESSAGE_TYPE_METHOD_CALL) {
            // Wrong message type
            //printf("Server: The msg type is %d\n", ret);
            dbus_message_unref(message);
            continue;
        }

        if (!dbus_message_is_method_call(message, INTERFACE_NAME, METHOD_NAME)) {
            // Wrong interface or methed
            print_dbus_error(&dbus_error, "Server: dbus_message_is_method_call");
            dbus_message_unref(message);
            continue;
        }

        char *s;
        // Gets arguments from a message given a variable argument list.
        ret = dbus_message_get_args(message,
                                    &dbus_error,
                                    DBUS_TYPE_STRING, 
                                    &s, 
                                    DBUS_TYPE_INVALID);
        if (!ret) {
            print_dbus_error(&dbus_error, "Server: dbus_message_get_args");
            dbus_message_unref(message);
            continue;
        }

        printf("Server received: %s\n", s);

        // Step 4: Reply the message
        //============================
        if (dbus_message_get_no_reply(message)) continue;
        DBusMessage *reply;
        char answer[BUF_SIZE];
        // Constructs a message that is a reply to a method call.
        reply = dbus_message_new_method_return(message);
        if (reply == NULL) {
            printf("Server: Can't allocate the memory for the message.\n");
            exit(EXIT_FAILURE);
        }

        // make the string to reply
        if (dbus_message_has_path(message, OBJECT_PATH_NAME_YES)) {
            sprintf(answer, "Yes, %s", s);
        } else if (dbus_message_has_path(message, OBJECT_PATH_NAME_NO)) {
            sprintf(answer, "No, %s", s);
        } else if (dbus_message_has_path(message, OBJECT_PATH_NAME_QUIT)) {
            sprintf(answer, "Quit please, %s", s);
            if (++exit_count >= CLIENT_NUM) loop = FALSE;
        } else {
            sprintf(answer, "No object found");
        }

        char *ptr = answer;
        // Append a string to the message
        if (!dbus_message_append_args(reply, DBUS_TYPE_STRING, &ptr, DBUS_TYPE_INVALID)) {
            printf("Server: Can't append the string to the message.\n");
            exit(EXIT_FAILURE);
        }
        // Adds a message to the outgoing message queue.
        if (!dbus_connection_send(conn, reply, NULL)) {
            printf("Server: Can't add the messge to the queue.\n");
            exit(EXIT_FAILURE);
        }
        // Blocks until the outgoing message queue is empty.
        dbus_connection_flush(conn);
        // Decrements the reference count of a DBusConnection, 
        // and finalizes it if the count reaches zero.
        dbus_message_unref(reply);
        if (!loop) break;
    }

    return;
}
/********************************************************************************
 * Function: void dbus_client()
 * Decription: D-Bus client process
 ********************************************************************************/
void dbus_client(int num) {
    DBusError dbus_error;
    DBusConnection *conn;
    char input[64];

    sprintf(input, "client No.%d", num);
    // Step 1: get dbus connection
    //==============================
    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "Client: dbus_bus_get");
    }

    const char *objects[] = {
                OBJECT_PATH_NAME_YES, 
                OBJECT_PATH_NAME_NO, 
                OBJECT_PATH_NAME_QUIT, 
                NULL};
    int i = 0;
    do {
        // Step 2: Constructs a new message to invoke a method on a remote object.
        //========================================================================
        DBusMessage *request = dbus_message_new_method_call(SERVER_BUS_NAME, 
                                                            objects[i], 
                                                            INTERFACE_NAME, 
                                                            METHOD_NAME);
        if (request == NULL) {
            printf("Client(%d): Can't construct a new message.\n", num);
            exit(EXIT_FAILURE);
        }

        // Append a string to the message
        char *ptr = input;
        if (!dbus_message_append_args(request, DBUS_TYPE_STRING, &ptr, DBUS_TYPE_INVALID)) {
            printf("Client(%d): Can't append the string to the message.\n", num);
            exit(EXIT_FAILURE);
        }

        // Step 3: sends the message and gets the reply
        //==============================================
        DBusMessage *reply;
        reply = dbus_connection_send_with_reply_and_block(conn, 
                                                          request, 
                                                          REPLY_TIMEOUT,
                                                          &dbus_error);
        dbus_message_unref(request);
        if (dbus_error_is_set(&dbus_error)) {
            //print_dbus_error(&dbus_error, "Client: dbus_connection_send_with_reply_and_block");
            dbus_error_free(&dbus_error);
            continue;
        }

        char *s;
        // Gets arguments from a message given a variable argument list.
        if (dbus_message_get_args(reply, &dbus_error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
            if (dbus_error_is_set(&dbus_error)) {
                print_dbus_error(&dbus_error, "Client: dbus_message_get_args");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Client(%d): Can't get arguments from the reply\n", num);
        }

        int msg_type = dbus_message_get_type(reply);
        if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
            //printf("Client(%d) - Got a error message: %s\n", num, s);
            i--;
        } else if (msg_type == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
            printf("Client(%d) - Reply: %s\n", num, s);
        } else {
            printf("Client(%d) - Unknown: %s\n", num, s);
        }

        // Decrements the reference count of a DBusConnection, 
        // and finalizes it if the count reaches zero.
        dbus_message_unref(reply);
        i++;
    } while (objects[i]);

    return;
}

int main() {
    int pid[CLIENT_NUM + 1];
    int i = 0;

    // 1. Fork server process and client processes
    //=============================================
    for (i = 0; i < (CLIENT_NUM + 1); ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        if (i == 0) sleep(1);
    }
 
    if (i == 0) {
        // 2. the first child is server process
        //======================================
        printf("%d. Server process. PID=%d.\n", i + 1, getpid());
        dbus_server();
    } else if (i <= CLIENT_NUM) {
        // 3. next CLIENT_NUM children are client processes
        //==================================================
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        dbus_client(i);
    } else if (i > CLIENT_NUM) {
        // 4. the last child is parent process
        //======================================
        printf("\n=== Parent process. Waiting for child processes exiting ===\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < (CLIENT_NUM + 1); ++j) {
            pid_wait = wait(NULL);
            printf("Client exited. PID=%d.\n", pid_wait);
        }
    }

    return EXIT_SUCCESS;

}
