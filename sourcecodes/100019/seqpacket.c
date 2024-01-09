/*
 * File: seqpacket.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Example of IPC using AF_UNIX adress family with SOCK_SEQPACKET.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall seqpacket.c -o seqpacket
 * Usage: $ ./seqpacket
 *
 * Example source code for article 《IPC之九：使用UNIX Domain Socket进行进程间通信的实例》
 * https://whowin.gitee.io/post/blog/linux/0019-ipc-with-unix-domain-socket/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define FALSE           0
#define BUFFER_SIZE     128

#define STREAM_SERVER_PATH          "#stream_unix_socket.sock"
#define SEQPACKET_SERVER_PATH       "#seqpacket_unix_socket.sock"
#define MSG_STREAM_SERVER           "Hello from SOCK_STREAM server"
#define MSG_SEQPACKET_SERVER        "Hello from SOCK_SEQPACKET server"
#define MSG_CLIENT                  "Hello from client"

// The server process
void server_process(int sock_type) {
    int sock_server = -1;
    int sock_client = -1;
    int rc;
    socklen_t length;
    char buffer[BUFFER_SIZE];
    struct sockaddr_un server_addr, client_addr;

    char server_name[32];
    int err_count = 0;

    if (sock_type == SOCK_STREAM) {
        strcpy(server_name, "The SOCK_STREAM server");
    } else {
        strcpy(server_name, "The SOCK_SEQPACKET server");
    }

    do {
        // 1. create the socket
        sock_server = socket(AF_UNIX, sock_type, 0);
        if (sock_server < 0) {
            puts(server_name);
            perror("server - socket()");
            exit(0);
        }

        // 2. bind the socket to a abstract path
        memset(&server_addr, 0, sizeof(struct sockaddr_un));
        server_addr.sun_family = AF_UNIX;
        if (sock_type == SOCK_STREAM) {
            strcpy(server_addr.sun_path, STREAM_SERVER_PATH);
        } else {
            strcpy(server_addr.sun_path, SEQPACKET_SERVER_PATH);
        }
        length = SUN_LEN(&server_addr);
        server_addr.sun_path[0] = '\0';

        rc = bind(sock_server, (struct sockaddr *)&server_addr, length);
        if (rc < 0) {
            puts(server_name);
            perror("server - bind()");
            break;
        }

        // 3. listen on the socket
        if (listen(sock_server, 1) == -1) {
            puts(server_name);
            perror("server - listen()");
            break;
        }

        // 4. accept the client connecting request
        printf("waiting for connection\n");
        length = sizeof(struct sockaddr_un);
        sock_client = accept(sock_server, (struct sockaddr *)&client_addr, &length);
        if (sock_client < 0) {
            puts(server_name);
            perror("server - accept()");
            break;
        }

        // 5. receive data from the client socket
        do {
            rc = recv(sock_client, buffer, BUFFER_SIZE, MSG_DONTWAIT);
            if (rc > 0) {
                buffer[rc] = '\0';
                if (sock_type == SOCK_STREAM) {
                    printf("SOCK_STREAM server: %s\n", buffer);
                } else {
                    printf("SOCK_SEQPACKET server: %s\n", buffer);
                }
                err_count = 0;
            } else if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (++err_count >= 5) {
                    break;
                }
                sleep(1);
            }
        } while (rc > 0 || (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)));

        // 6. send data to the client socket
        if (sock_type == SOCK_STREAM) {
            strcpy(buffer, MSG_STREAM_SERVER);
        } else {
            strcpy(buffer, MSG_SEQPACKET_SERVER);
        }
        rc = send(sock_client, buffer, strlen(buffer), 0);
        if (rc < 0) {
            puts(server_name);
            perror("server - write()");
            break;
        }
        sleep(1);
    } while (FALSE);

    if (sock_client) close(sock_client);
    if (sock_server) close(sock_server);

    return;
}

// The client process
void client_process(int sock_type) {
    int sock_server = -1;
    int rc;
    socklen_t length;
    char buffer[BUFFER_SIZE];
    struct sockaddr_un server_addr;
    char client_name[32];

    int i;

    if (sock_type == SOCK_STREAM) {
        strcpy(client_name, "The SOCK_STREAM client");
    } else {
        strcpy(client_name, "The SOCK_SEQPACKET client");
    }

    do {
        // 1. create the socket
        sock_server = socket(AF_UNIX, sock_type, 0);
        if (sock_server < 0) {
            puts(client_name);
            perror("client - socket()");
            break;
        }
        // 2. fill the structure and connect to the server path
        server_addr.sun_family = AF_UNIX;
        if (sock_type == SOCK_STREAM) {
            strcpy(server_addr.sun_path, STREAM_SERVER_PATH);
        } else {
            strcpy(server_addr.sun_path, SEQPACKET_SERVER_PATH);
        }
        length = SUN_LEN(&server_addr);
        server_addr.sun_path[0] = '\0';

        rc = connect(sock_server, (struct sockaddr *)&server_addr, length);
        if (rc < 0) {
            puts(client_name);
            perror("client - connect()");
            break;
        }

        // 3. send data to the server socket
        for (i = 0; i < 3; i++) {
            sprintf(buffer, "No.%d: %s   ", i + 1, MSG_CLIENT);
            rc = send(sock_server, buffer, strlen(buffer), 0);
        }

        // 4. receive response from the server socket
        rc = recv(sock_server, buffer, BUFFER_SIZE, 0);
        if (rc < 0) {
            puts(client_name);
            perror("client - read()");
            break;
        }
        buffer[rc] = '\0';
        if (sock_type == SOCK_STREAM) {
            printf("SOCK_STREAM client: %s\n", buffer);
        } else {
            printf("SOCK_SEQPACKET client: %s\n", buffer);
        }
    } while (FALSE);

    if (sock_server != -1) close(sock_server);

    return;
}

int main() {
    pid_t pid[3];
    int i = 0;

    srand((uint)time(NULL));

    // Fork two server processes and a client process
    for (i = 0; i < 3; ++i) {
        sleep((int)(rand() % 2) + 1);
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
    }
    if (i == 0) {
        // the first child process, for SOCK_STREAM server
        printf("%d. SOCK_STREAM server process. PID=%d.\n", i + 1, getpid());
        server_process(SOCK_STREAM);
    } else if (i == 1) {
        // the second child process, for SOCK_SEQ_PACKET server
        printf("%d. SOCK_SEQPACKET server process. PID=%d.\n", i + 1, getpid());
        server_process(SOCK_SEQPACKET);
    } else if (i == 2) {
        // the third child process, for client
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client_process(SOCK_STREAM);
        client_process(SOCK_SEQPACKET);
    } else {
        // the parent process, just waiting all children exit
        printf("\n=== Parent process. Waiting for child processes exiting ===\n\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < 3; ++j) {
            pid_wait = wait(NULL);
            if (pid_wait == pid[0]) {
                printf("\nThe SOCK_STREAM server process exited.\n");
            } else if (pid_wait == pid[1]) {
                printf("\nThe SOCK_SEQPACKET server process exited.\n");
            } else {
                printf("\nThe client process exited.\n");
            }
        }
    }

    return EXIT_SUCCESS;
}
