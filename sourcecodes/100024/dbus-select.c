/*
 * File: dbus-select.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Example of using libdbus to receive D-Bus messages through select()
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g dbus-select.c -o dbus-select `pkg-config --libs --cflags dbus-1`
 * Usage: $ ./dbus-select
 *
 * Example source code for article 《IPC之十四：使用libdbus通过select()接收D-Bus消息的实例》
 * https://whowin.gitee.io/post/blog/linux/0024-select-recv-message/
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/select.h>

#include <dbus/dbus.h>

#define CLIENT_NUM              3

#define SERVER_BUS_NAME         "cn.whowin.dbus"
#define OBJECT_PATH_NAME        "/cn/whowin/dbus"
#define INTERFACE_NAME          "cn.whowin.dbus_demo"
#define METHOD_NAME_YES         "yes"
#define METHOD_NAME_NO          "no"
#define METHOD_NAME_QUIT        "quit"

#define BUF_SIZE                (128)
#define REPLY_TIMEOUT           (500)
#define SELECT_TIMEOUT_MS       (5500)

#define SELECT_RECV_SUCCESS     0
#define SELECT_RECV_FAIL        1
#define SELECT_RECV_QUIT        2

struct watch_struct {
    DBusWatch *watched_watch;
    int watched_rd_fd;
    int watched_wr_fd;
};

/********************************************************************************
 * Function: void print_dbus_error(DBusError *dbus_error_p, char *str)
 * Description: Print the error message when involking dbus functions
 ********************************************************************************/
void print_dbus_error(DBusError *dbus_error_p, char *str) {
    printf("%s: %s\n", str, dbus_error_p->message);
    dbus_error_free(dbus_error_p);
}

/*******************************************************************************
 * Function: static dbus_bool_t add_watch(DBusWatch *w, void *data)
 * Description: Called when libdbus needs a new watch to be monitored 
 *              by the main loop.
 *******************************************************************************/
static dbus_bool_t add_watch(DBusWatch *w, void *data) {
    if (!dbus_watch_get_enabled(w)) {               // Returns whether a watch is enabled or not.
        return TRUE;
    }

    struct watch_struct *watch_fds = (struct watch_struct *)data;
    int fd = dbus_watch_get_unix_fd(w);             // Returns a UNIX file descriptor to be watched, 
                                                    // which may be a pipe, socket, or other type of descriptor.
    unsigned int flags = dbus_watch_get_flags(w);   // Gets flags from DBusWatchFlags indicating what conditions 
                                                    // should be monitored on the file descriptor.
    if (flags & DBUS_WATCH_READABLE) {              // the watch is readable
        watch_fds->watched_rd_fd = fd;
    }
    if (flags & DBUS_WATCH_WRITABLE) {              // the watch is writable
        watch_fds->watched_wr_fd = fd;
    }
    watch_fds->watched_watch = w;

    printf("ADD_WATCH: add a dbus watch. fd=%d watch=%p rd_fd=%d wr_fd=%d\n", 
           fd, w, watch_fds->watched_rd_fd, watch_fds->watched_wr_fd);
    return TRUE;
}
/*******************************************************************************
 * Function: static void remove_watch(DBusWatch *w, void *data)
 * Description: Called when libdbus no longer needs a watch to be monitored 
 *              by the main loop.
 *******************************************************************************/
static void remove_watch(DBusWatch *w, void *data) {
    struct watch_struct *watch_fds = (struct watch_struct *)data;

    watch_fds->watched_watch = NULL;
    watch_fds->watched_rd_fd = 0;
    watch_fds->watched_wr_fd = 0;
    printf("REMOVE_WATCH: remove dbus watch watch=%p\n", w);
}
/**********************************************************************************
 * Function: static void toggle_watch(DBusWatch *w, void *data)
 * Description: Called when dbus_watch_get_enabled() may return a different value 
 *              than it did before.
 **********************************************************************************/
static void toggle_watch(DBusWatch *w, void *data) {
    printf("TOGGLE_WATCH: toggle dbus watch watch=%p\n", w);
    if (dbus_watch_get_enabled(w)) {    // Returns whether a watch is enabled or not.
        add_watch(w, data);
    } else {
        remove_watch(w, data);
    }
}
/********************************************************************************
 * Function: void print_message_info(DBusMessage *msg)
 * Description: print the object path, interface and member of msg
 *              try to get a string argument and print it
 ********************************************************************************/
void print_message_info(DBusMessage *msg) {
    DBusError err;

    dbus_error_init(&err);

    printf("\tObject path: %s\n", dbus_message_get_path(msg));
    printf("\tInterface: %s\t\tMember: %s\n", dbus_message_get_interface(msg), dbus_message_get_member(msg));
    char *str;
    dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
    if (dbus_error_is_set(&err)) {
        print_dbus_error(&err, "\tdbus_message_get_args()");
    } else printf("\tArgument: %s\n", str);
}
/********************************************************************************
 * Function: static int select_recv(DBusConnection *conn)
 * Description: Receiving the message and sending a reply
 ********************************************************************************/
static int select_recv(DBusConnection *conn) {
    int ret = SELECT_RECV_FAIL;     // default fail
    int rc = 0;
    static int quit_count = 0;
    DBusError err;

    dbus_error_init(&err);

    // Step 01: Determine whether there is message in the queue
    //==========================================================
    DBusDispatchStatus dispatch_rc = dbus_connection_get_dispatch_status(conn);
    if (dispatch_rc != DBUS_DISPATCH_DATA_REMAINS) {
        printf("SELECT_RECV: there is no message in queue.\n");
    }
    while (dispatch_rc == DBUS_DISPATCH_DATA_REMAINS) {
        // Step 02: borrow the first message from queue
        //===============================================
        DBusMessage *msg = dbus_connection_borrow_message(conn);
        if (msg == NULL) {
            printf("SELECT_RECV: status is remains but no message borrowed. \n");
            break;
        }
        // Step 03: throw away messages without processing
        //=================================================
        int mtype = dbus_message_get_type(msg);
        if (mtype == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
            printf("SELECT_RECV: received METHOD_RETURN and throw away.\n");
            dbus_connection_steal_borrowed_message(conn, msg);
            dbus_message_unref(msg);
        } else if (mtype == DBUS_MESSAGE_TYPE_ERROR) {
            printf("SELECT_RECV: received ERROR and throw away. \n");
            dbus_connection_steal_borrowed_message(conn, msg);
            dbus_message_unref(msg);
        } else if (mtype == DBUS_MESSAGE_TYPE_SIGNAL) {
            printf("SELECT_RECV: received SIGNAL and throw away.\n");
            dbus_connection_steal_borrowed_message(conn, msg);
            //print_message_info(msg);
            dbus_message_unref(msg);
        } else if (mtype == DBUS_MESSAGE_TYPE_METHOD_CALL) {
            // Step 04: process METHOD_CALL message
            //======================================
            //printf("SELECT_RECV: received METHOD_CALL. \n");
            if (!dbus_message_has_path(msg, OBJECT_PATH_NAME)) {
                printf("SELECT_RECV: wrong object path. throw away.\n");
                dbus_message_unref(msg);
            } else {
                dbus_connection_steal_borrowed_message(conn, msg);
                //print_message_info(msg);

                // Gets arguments from a message given a variable argument list.
                char *s;
                rc = dbus_message_get_args(msg,
                                           &err,
                                           DBUS_TYPE_STRING, 
                                           &s, 
                                           DBUS_TYPE_INVALID);
                if (!rc) {
                    print_dbus_error(&err, "SELECT_RECV: dbus_message_get_args()");
                    dbus_message_unref(msg);
                    continue;
                }
                printf("SELECT_RECV: received '%s'\n", s);

                // Step 05: reply the message
                //============================
                // make the string for reply
                char answer[BUF_SIZE];
                if (dbus_message_is_method_call(msg, INTERFACE_NAME, METHOD_NAME_YES)) {
                    sprintf(answer, "Yes, %s", s);
                } else if (dbus_message_is_method_call(msg, INTERFACE_NAME, METHOD_NAME_NO)) {
                    sprintf(answer, "No, %s", s);
                } else if (dbus_message_is_method_call(msg, INTERFACE_NAME, METHOD_NAME_QUIT)) {
                    sprintf(answer, "Quit please, %s", s);
                    ++quit_count;
                } else {
                    sprintf(answer, "Wrong interface");
                }
                //printf("answer: %s\n", answer);

                DBusMessage *reply = NULL;
                do {
                    dbus_uint32_t serial = 111;
                    reply = dbus_message_new_method_return(msg);
                    char *ptr = answer;
                    if (!dbus_message_append_args(reply, 
                                                DBUS_TYPE_STRING,
                                                &ptr,
                                                DBUS_TYPE_INVALID)) {
                        printf("SELECT_RECV: Failed to dbus_message_append_args()\n");
                    }
                    if (!dbus_connection_send(conn, reply, &serial)) {
                        printf("SELECT_RECV: Out Of Memory on dbus_connection_send()\n");
                        break;
                    }
                    dbus_connection_flush(conn);
                } while (0);
                // Step 06: clean up messages
                //=============================
                if (reply != NULL) {
                    dbus_message_unref(reply); 
                }
                if (msg != NULL) {
                    dbus_message_unref(msg);
                }
                ret = SELECT_RECV_SUCCESS;      // success
            }
        } else {
            printf("SELECT_RECV: unknown msg type %d \n", mtype);
        }

        if (quit_count >= CLIENT_NUM) {
            ret = SELECT_RECV_QUIT;
            break;
        }
        dispatch_rc = dbus_connection_get_dispatch_status(conn);
    }  // while (...)
    return ret;
}

/********************************************************************************
 * Function: void dbus_server()
 * Decription: D-Bus server process
 ********************************************************************************/
void dbus_server() {
    DBusError dbus_error;
    DBusConnection *conn;
    static struct watch_struct watch_fds;
    int ret;

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

    // Step 03: Add a watch
    //========================
    watch_fds.watched_watch = NULL;
    watch_fds.watched_rd_fd = 0;
    watch_fds.watched_wr_fd = 0;
    if (!dbus_connection_set_watch_functions(conn, 
                                             add_watch, 
                                             remove_watch,
                                             toggle_watch,
                                             (void *)&watch_fds, NULL)) {
        printf("Server: failed to set watch.\n");
        return;
    }

    bool loop = TRUE;
    do {
        // Step 04: Initial fd sets
        //============================
        fd_set readfds;            // Reading fd set, Writing fd set and ... fd set
        // timeout for select
        struct timeval timeoutval = {
                        SELECT_TIMEOUT_MS / 1000, 
                        (SELECT_TIMEOUT_MS % 1000) * 1000
                        };
        int max_fd = 0;
        int activity;

        printf("\n");
    
        FD_ZERO(&readfds);
        if (watch_fds.watched_watch != NULL) {
            if (watch_fds.watched_rd_fd > 0) {
                FD_SET(watch_fds.watched_rd_fd, &readfds);      // set watched FD into fd set
                max_fd = watch_fds.watched_rd_fd;
            } else {
                printf("Server: there is no readable FD.\n");
                exit(EXIT_FAILURE);
            }
        }
        printf("Server: ready to select with max_fd %d...\n", max_fd);

        // Step 05: select()
        //=====================
        activity = select(max_fd + 1, &readfds, NULL, NULL, &timeoutval);
        if (activity < 0) {
            printf("Server: select error");
            break;
        }
        if (activity == 0) {
            printf(" SELECT returned 0 \n");
            continue;
        }
        // Step 06: read messge from watched fd
        //=======================================
        if (FD_ISSET(watch_fds.watched_rd_fd, &readfds)) {
            printf("Server: calling watch_handle\n");
            if (dbus_watch_handle(watch_fds.watched_watch, DBUS_WATCH_READABLE)) {
                printf("Server: calling select_recv\n");
                ret = select_recv(conn);
                printf("Server: done select_recv\n");
            }
            if (ret > 1) loop = FALSE;
        }
    } while (loop);

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

    const char *methods[] = {
                METHOD_NAME_YES, 
                METHOD_NAME_NO, 
                METHOD_NAME_QUIT, 
                NULL};
    int i = 0;
    do {
        // Step 2: Constructs a new message to invoke a method on a remote object.
        //========================================================================
        DBusMessage *request = dbus_message_new_method_call(SERVER_BUS_NAME, 
                                                            OBJECT_PATH_NAME, 
                                                            INTERFACE_NAME, 
                                                            methods[i]);
        if (request == NULL) {    //dbus_bool_t loop = TRUE;

            printf("Client(%d): Can't construct a new message.\n", num);
            exit(EXIT_FAILURE);
        }

        // append a string to the message
        char *ptr = input;
        if (!dbus_message_append_args(request, DBUS_TYPE_STRING, &ptr, DBUS_TYPE_INVALID)) {
            printf("Client(%d): Can't append the string to the message.\n", num);
            exit(EXIT_FAILURE);
        }

        // Step 3: sends the message and gets the reply
        //==============================================
        DBusMessage *reply;
        printf("Client(%d) send a message(%d).\n", num, i);
        reply = dbus_connection_send_with_reply_and_block(conn, 
                                                          request, 
                                                          REPLY_TIMEOUT,
                                                          &dbus_error);
        dbus_message_unref(request);
        if (dbus_error_is_set(&dbus_error)) {
            print_dbus_error(&dbus_error, "Client: dbus_connection_send_with_reply_and_block");
            sleep(1);
            //dbus_error_free(&dbus_error);
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

        dbus_message_unref(reply);
        i++;
        sleep(3);
    } while (methods[i]);

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
