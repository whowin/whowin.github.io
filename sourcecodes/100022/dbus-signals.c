/*
 * File: dbus-signals.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Example of using libdbus to realize the client receiving server signals
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g dbus-signals.c -o dbus-signals `pkg-config --libs --cflags dbus-1`
 * Usage: $ ./dbus-signals
 *
 * Example source code for article 《IPC之十二：使用libdbus异步发送/接收信号的实例》
 * https://whowin.gitee.io/post/blog/linux/0022-dbus-asyn-process-signal/
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
#define OBJECT_PATH_NAME        "/cn/whowin/dbus/signal"
#define INTERFACE_NAME          "cn.whowin.dbus_iface"
#define SIGNAL_NMAE             "notify"
#define SIGNAL_ARG_START        "start"
#define SIGNAL_ARG_QUIT         "quit"

#define BUF_SIZE                128

/********************************************************************************
 * Function: void print_dbus_error(DBusError *dbus_error_p, char *str)
 * Description: Print the error message when involking dbus functions
 ********************************************************************************/
void print_dbus_error(DBusError *dbus_error_p, char *str) {
    printf("%s: %s\n", str, dbus_error_p->message);
    dbus_error_free(dbus_error_p);
}
/*******************************************************************************
 * Function: DBusHandlerResult signal_start(DBusConnection *connection, 
 *                                          DBusMessage *message, 
 *                                          void *usr_data)
 * Description: A filter, use for receiving signals that emit from server
 *******************************************************************************/
DBusHandlerResult signal_start(DBusConnection *connection, DBusMessage *message, void *usr_data) {
    DBusError dbus_error;
    dbus_bool_t bool_ret;
    
    dbus_error_init(&dbus_error);

    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
        printf("Client(%d): This is not a signal.\n", getpid());
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    if (!dbus_message_has_path(message, OBJECT_PATH_NAME)) {
        printf("Client(%d): Wrong object path(%s).\n", getpid(), dbus_message_get_path(message));
        /*
        printf("The interface is %s\n", dbus_message_get_interface(message));
        printf("The member is %s\n", dbus_message_get_member(message));
        char *arg;
        bool_ret =  dbus_message_get_args(message, 
                                        &dbus_error, 
                                        DBUS_TYPE_STRING,
                                        &arg,
                                        DBUS_TYPE_INVALID);
        printf("The argument is %s\n", arg);
        */
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        //return DBUS_HANDLER_RESULT_HANDLED;
    }
    if (!dbus_message_is_signal(message, INTERFACE_NAME, SIGNAL_NMAE)) {
        printf("Client(%d): Wrong interface.\n", getpid());
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    char *str;
    bool_ret =  dbus_message_get_args(message, 
                                      &dbus_error, 
                                      DBUS_TYPE_STRING,
                                      &str,
                                      DBUS_TYPE_INVALID);
    if (bool_ret == FALSE) {
        print_dbus_error(&dbus_error, "dbus_message_get_args()");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    printf("signal_start(%d) - received: %s\n", getpid(), str);
    if (strcmp(str, SIGNAL_ARG_START) != 0) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    return DBUS_HANDLER_RESULT_HANDLED;
}
/*******************************************************************************
 * Function: DBusHandlerResult signal_quit(DBusConnection *connection, 
 *                                         DBusMessage *message, 
 *                                         void *usr_data)
 * Description: A filter, use for receiving signals that emit from server
 *******************************************************************************/
DBusHandlerResult signal_quit(DBusConnection *connection, DBusMessage *message, void *usr_data) {
    DBusError dbus_error;
    dbus_bool_t bool_ret;
    bool *quit = (bool *)usr_data;
    
    dbus_error_init(&dbus_error);

    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
        printf("Client(%d): This is not a signal.\n", getpid());
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    if (!dbus_message_has_path(message, OBJECT_PATH_NAME)) {
        printf("Client(%d): Wrong object path(%s).\n", getpid(), dbus_message_get_path(message));
        /*
        printf("The interface is %s\n", dbus_message_get_interface(message));
        printf("The member is %s\n", dbus_message_get_member(message));
        char *arg;
        bool_ret =  dbus_message_get_args(message, 
                                        &dbus_error, 
                                        DBUS_TYPE_STRING,
                                        &arg,
                                        DBUS_TYPE_INVALID);
        printf("The argument is %s\n", arg);
        */
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        //return DBUS_HANDLER_RESULT_HANDLED;
    }
    if (!dbus_message_is_signal(message, INTERFACE_NAME, SIGNAL_NMAE)) {
        printf("Client(%d): Wrong interface.\n", getpid());
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    char *str;
    bool_ret =  dbus_message_get_args(message, 
                                      &dbus_error, 
                                      DBUS_TYPE_STRING,
                                      &str,
                                      DBUS_TYPE_INVALID);
    if (bool_ret == FALSE) {
        print_dbus_error(&dbus_error, "dbus_message_get_args()");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    printf("signal_quit(%d) - received: %s\n", getpid(), str);
    if (strcmp(str, SIGNAL_ARG_QUIT) != 0) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    *quit = TRUE;
    return DBUS_HANDLER_RESULT_HANDLED;
}
/********************************************************************************
 * Function: void dbus_client(int num)
 * Decription: D-Bus client process
 ********************************************************************************/
void dbus_client(int num) {
    static bool quit = FALSE;
    DBusError dbus_error;
    DBusConnection *conn;
    char match_rule[BUF_SIZE];

    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "dbus_bus_get()");
        return;
    }

    if (!dbus_connection_add_filter(conn, signal_quit, (void *)&quit, NULL)) {
        return;
    }
    if (!dbus_connection_add_filter(conn, signal_start, NULL, NULL)) {
        return;
    }

    sprintf(match_rule, "type='signal',path='%s',interface='%s'", OBJECT_PATH_NAME, INTERFACE_NAME);
    dbus_bus_add_match(conn, match_rule, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "dbus_bus_add_match()");
        return;
    }
    
    while (dbus_connection_read_write_dispatch(conn, -1)) {
        // loop
        if (quit) break;
    }
    
    return;
}
/***************************************************************************************
 * Function: int send_signal(DBusConnection *conn, char *arg)
 * Description: Emits a signal
 ***************************************************************************************/
int send_signal(DBusConnection *conn, char *arg) {
    DBusMessage *signal_msg;
    dbus_bool_t bool_ret;

    // Initial a message struct about signal
    signal_msg = dbus_message_new_signal(OBJECT_PATH_NAME, INTERFACE_NAME, SIGNAL_NMAE);

    // Append a string to signal
    char *str = arg;
    bool_ret = dbus_message_append_args(signal_msg, 
                                        DBUS_TYPE_STRING,
                                        &str,
                                        DBUS_TYPE_INVALID);
    if (bool_ret == false) {
        printf("Can't append arguments to the signal.\n");
        return -1;
    }

    // Sends the signal
    if (!dbus_connection_send(conn, signal_msg, NULL)) return -1;
    dbus_connection_flush(conn);
    dbus_message_unref(signal_msg);
   
    return 0;
}

/********************************************************************************
 * Function: void dbus_server(int num)
 * Decription: D-Bus server process
 ********************************************************************************/
void dbus_server() {
    DBusError dbus_error;
    DBusConnection *conn;

    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "dbus_bus_get()");
        return;
    }

    sleep(1);           // Waiting for client processes start
    send_signal(conn, SIGNAL_ARG_START);
    sleep(5);           // Just simulating doing something and then quit
    send_signal(conn, SIGNAL_ARG_QUIT);

    dbus_connection_unref(conn);
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
