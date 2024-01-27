/*
 * File: sendmsg-recvmsg.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Example of IPC using UNIX domain socket(sendmsg()/recvmsg()).
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall sendmsg-recvmsg.c -o sendmsg-recvmsg
 * Usage: $ ./sendmsg-recvmsg
 *
 * Example source code for article 《IPC之九：使用UNIX Domain Socket进行进程间通信的实例》
 * 
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
//#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <sys/select.h>
#include <sys/wait.h>

#define SERVER_PATH         "/tmp/af_unix_server"
#define CLIENT_PATH         "/tmp/af_unix_client_"
#define BUFFER_LENGTH       128
#define FALSE               0

#define CLIENT_NUM          3

#define MSG_FROM_CLIENT     "Hello server. This message is from the client process."
#define MSG_FROM_SERVER     "Hi client. This message is from the server process."

#define FONT_RED            "\033[31m"
#define FONT_YELLOW         "\033[33m"
#define FONT_DEFAULT        "\033[0m"


/**************************************************************************
 * Function: void server(int seq)
 * Description: Server process. It creates a UNIX socket and then waiting
 *              for clients connecting. Receives the message from clients
 *              and sends another message to clients.
 * 
 * Arguments:   seq     a sequence number. no meaning
 * Return: none
 **************************************************************************/
void server(int seq) {
    int unix_sock = -1;
    int rc;
    int i;
    socklen_t length;
    char buffer[BUFFER_LENGTH];
    struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;

    struct msghdr msg = {0};
    struct iovec io = {
        .iov_base = buffer,
        .iov_len = BUFFER_LENGTH
    };

    unlink(SERVER_PATH);
    do {
        // 1. create a UNIX socket
        //==========================
        unix_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (unix_sock < 0) {
            perror("server: socket()");
            break;
        }

        // 2. bind the UNIX socket to a file path
        //========================================
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SERVER_PATH);

        rc = bind(unix_sock, (struct sockaddr *)&server_addr, SUN_LEN(&server_addr));
        if (rc < 0) {
            perror("server: bind()");
            break;
        }

        // 3. Receive data from clients
        length = sizeof(struct sockaddr_un);
        memset(&client_addr, 0, length);
        for (i = 0; i < CLIENT_NUM; ++i) {
            msg.msg_name = &client_addr;
            msg.msg_namelen = length;

            rc = recvmsg(unix_sock, &msg, 0);
            //rc = recvfrom(unix_sock, buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&client_addr, &length);
            if (rc < 0) {
                perror("server - recemsg()");
            } else {
                buffer[rc] = 0;
                printf("%sserver%s - Received a message from client: %s\n", FONT_RED, FONT_DEFAULT, buffer);

                //printf("Client address length: %d - %ld - %ld\n", length, sizeof(struct sockaddr_un), SUN_LEN(&client_addr));
                //printf("%d, %s\n", client_addr.sun_family, client_addr.sun_path);

                strcpy(buffer, MSG_FROM_SERVER);
                io.iov_base = buffer;
                io.iov_len = strlen(buffer);
                msg.msg_iov = &io;
                msg.msg_iovlen = 1;
                msg.msg_control = NULL;
                msg.msg_controllen = 0;
                rc = sendmsg(unix_sock, &msg, 0);
                //rc = sendto(unix_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr, SUN_LEN(&client_addr));
                if (rc < 0) {
                    perror("server - sendmsg()");
                } else {
                    printf("%sServer%s - sent a message to client.\n", FONT_RED, FONT_DEFAULT);
                }
            }
        }
    } while (FALSE);

    sleep(2);
    if (unix_sock != -1) close(unix_sock);

    unlink(SERVER_PATH);

    return;
}
/*************************************************************************
 * Function: void client(int seq)
 * Description: Client process. It connects to server and then sends 
 *              a message to server. Closes the socket after it gets
 *              the response from the server.
 * Arguments:   seq     a sequence number. no meaning.
 * Return: none
 *************************************************************************/
void client(int seq) {
    int unix_sock = -1;
    int rc;
    char client_sock_path[64];
    //socklen_t length;
    char buffer[BUFFER_LENGTH];
    struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;

    struct msghdr msg = {0};
    struct iovec io = {
        .iov_base = buffer,
        .iov_len = BUFFER_LENGTH
    };

    sprintf(client_sock_path, CLIENT_PATH"%d", seq);
    unlink(client_sock_path);

    do {
        // 1. create a UNIX socket
        unix_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (unix_sock < 0) {
            perror("client: socket()");
            break;
        }

        // 2. set the server address to send/receive data 
        memset(&server_addr, 0, sizeof(struct sockaddr_un));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SERVER_PATH);

        // 3. bind the unix socket to a file path 
        memset(&client_addr, 0, sizeof(struct sockaddr_un));
        client_addr.sun_family = AF_UNIX;
        strcpy(client_addr.sun_path, client_sock_path);
        rc = bind(unix_sock, (struct sockaddr *)&client_addr, SUN_LEN(&client_addr));

        // 4. send a message to server
        sprintf(buffer, "clienr No.%d - %s", seq, MSG_FROM_CLIENT);
        io.iov_base = buffer;
        io.iov_len = strlen(buffer);
        msg.msg_name = &server_addr;
        msg.msg_namelen = SUN_LEN(&server_addr);
        msg.msg_iov = &io;
        msg.msg_iovlen = 1;

        rc = sendmsg(unix_sock, &msg, 0);
        //rc = sendto(unix_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, SUN_LEN(&server_addr));
        if (rc < 0) {
            perror("client - sendmsg()");
        } else {
            printf("%sclient No.%d%s - sent a message to the server.\n", FONT_YELLOW, seq, FONT_DEFAULT);
        }

        // 5. receive the response from server
        io.iov_base = buffer;
        io.iov_len = BUFFER_LENGTH;
        msg.msg_name = &server_addr;
        msg.msg_namelen = sizeof(struct sockaddr_un);
        msg.msg_iov = &io;
        msg.msg_iovlen = 1;
        rc = recvmsg(unix_sock, &msg, 0);
        //rc = recvfrom(unix_sock, buffer, BUFFER_LENGTH, 0, NULL, NULL);
        if (rc < 0) {
            perror("recvmsg()");
            break;
        }
        buffer[rc] = '\0';
        printf("%sClienr No.%d%s - Received a message from the server: %s\n", FONT_YELLOW, seq, FONT_DEFAULT, buffer);
    } while (FALSE);

    if (unix_sock != -1)
        close(unix_sock);
    unlink(client_sock_path);

    return;
}

int main() {
    pid_t pid[CLIENT_NUM + 1];
    int i = 0;

    srand((uint)time(NULL));

    // Fork server process and client processes
    for (i = 0; i < (CLIENT_NUM + 1); ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        sleep((int)(rand() % 2) + 1);
    }
    // the first child is server process, others are client processes
    if (i == 0) {
        // child process, for server
        printf("%d. Server process. PID=%d.\n", i + 1, getpid());
        server(i);        // i is the index # of shelfs
    } else if (i < (CLIENT_NUM + 1)) {
        // child process, for clients
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client(i);
    } else if (i == (CLIENT_NUM + 1)) {
        // Parent process, just waiting all children exit
        printf("\n=== Parent process. Waiting for child processes exiting ===\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < (CLIENT_NUM + 1); ++j) {
            pid_wait = wait(NULL);
            for (i = 0; i < (CLIENT_NUM + 1); ++i) {
                if (pid[i] == pid_wait) {
                    pid[i] = 0;
                    break;
                }
            }
            printf("Client exited. PID=%d.\n", pid_wait);
        }
        /*
        if (pid[0] > 0) {
            kill(pid[0], SIGINT);
            pid_wait = wait(NULL);
        } 
        */       
    }

    return EXIT_SUCCESS;

}
