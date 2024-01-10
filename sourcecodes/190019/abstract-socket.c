/*
 * File: abstract-socket.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Example of IPC using AF_UNIX adress family with abstract linux namespace.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall abstract-socket.c -o abstract-socket
 * Usage: $ ./abstract-socket
 *
 * Example source code for article 《IPC之九：使用UNIX Domain Socket进行进程间通信的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define FALSE           0
#define BUFFER_SIZE     64
#define CMD_SIZE        64

#define SERVER_PATH     "#unix_socket.sock"
#define MSG_SERVER      "Hello from server"
#define MSG_CLIENT      "Hello from client"

// The server process
void server_process() {
    int sock_server = -1;
    int sock_client = -1;
    int rc;
    socklen_t length;
    char buffer[BUFFER_SIZE];
    struct sockaddr_un server_addr, client_addr;

    char cmd[CMD_SIZE];

    do {
        // 1. create the socket
        sock_server = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_server < 0) {
            perror("server - socket()");
            exit(0);
        }

        // 2. bind the socket to a abstract path
        memset(&server_addr, 0, sizeof(struct sockaddr_un));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SERVER_PATH);
        length = SUN_LEN(&server_addr);
        server_addr.sun_path[0] = '\0';

        rc = bind(sock_server, (struct sockaddr *)&server_addr, length);
        if (rc < 0) {
            perror("server - bind()");
            break;
        }

        // 3. listen on the socket
        if (listen(sock_server, 1) == -1) {
            perror("server - listen()");
            break;
        }

        sprintf(cmd, "lsof -p %d|grep '@'", getpid());
        printf("\n=== Executing command: %s ===\n", cmd);
        system(cmd);
        printf("================================\n\n");
        // 4. accept the client connecting request
        printf("waiting for connection\n");
        length = sizeof(struct sockaddr_un);
        sock_client = accept(sock_server, (struct sockaddr *)&client_addr, &length);
        if (sock_client < 0) {
            perror("server - accept()");
            break;
        }

        // 5. read and write data from/to client socket
        rc = read(sock_client, buffer, BUFFER_SIZE);
        if (rc < 0) {
            perror("server - read()");
            break;
        }
        buffer[rc] = '\0';
        printf("server: %s\n", buffer);

        strcpy(buffer, MSG_SERVER);
        rc = write(sock_client, buffer, strlen(buffer));
        if (rc < 0) {
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
void client_process () {
    int sock_server = -1;
    int rc;
    socklen_t length;
    char buffer[BUFFER_SIZE];
    struct sockaddr_un server_addr;

    do {
        // 1. create the socket
        sock_server = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_server < 0) {
            perror("client - socket()");
            break;
        }
        // 2. fill the structure and connect to the server path
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SERVER_PATH);
        length = SUN_LEN(&server_addr);
        server_addr.sun_path[0] = '\0';

        rc = connect(sock_server, (struct sockaddr *)&server_addr, length);
        if (rc < 0) {
            perror("client - connect()");
            break;
        }

        // 3. read and write data from/to server socket
        strcpy(buffer, MSG_CLIENT);
        rc = write(sock_server, buffer, strlen(buffer));
        if (rc < 0) {
            perror("client - write()");
            break;
        }

        rc = read(sock_server, buffer, BUFFER_SIZE);
        if (rc < 0) {
            perror("client - read()");
            break;
        }
        buffer[rc] = '\0';
        printf("client: %s\n", buffer);
    } while (FALSE);

    if (sock_server != -1) close(sock_server);

    return;
}

int main() {
    pid_t pid[2];
    int i = 0;

    srand((uint)time(NULL));

    // Fork a server process and a client process
    for (i = 0; i < 2; ++i) {
        sleep((int)(rand() % 2) + 1);
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
    }
    // the first child is server process, others are client processes
    if (i == 0) {
        // the first child process, for server
        printf("%d. Server process. PID=%d.\n", i + 1, getpid());
        server_process();
    } else if (i == 1) {
        // the second child process, for client
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client_process();
    } else {
        // the parent process, just waiting all children exit
        printf("\n=== Parent process. Waiting for child processes exiting ===\n\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < 2; ++j) {
            pid_wait = wait(NULL);
            printf("\nClient exited. PID=%d.\n", pid_wait);
        }
    }

    return EXIT_SUCCESS;
}

