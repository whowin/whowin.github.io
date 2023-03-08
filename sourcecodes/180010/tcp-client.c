/*
 * File: tcp-client.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * TCP client for a combinatorial TCP and UDP server
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall tcp-client.c -o tcp-client
 * Usage: $ ./tcp-client
 * 
 * Example source code for article 《使用select实现的UDP/TCP组合服务器》
 * https://whowin.gitee.io/post/blog/network/0010-tcp-and-udp-server-using-select/
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP   "192.168.2.112"
#define PORT        5000
#define BUF_SIZE    1024

int main() {
    int sockfd;
    char buffer[BUF_SIZE];
    int n;
    char *message = "Hello Server";
    struct sockaddr_in server_addr;

    // Step1: Creating socket file descriptor
    //========================================
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket creation failed");
        exit(0);
    }

    // Step 2: Establish a connection with the server.
    //=================================================
    memset(&server_addr, 0, sizeof(server_addr));

    // Filling server information
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("\n Error : Connect Failed \n");
        close(sockfd);
        exit(0);
    }

    // Step 3: When the connection is accepted write a message to a server
    //=====================================================================
    write(sockfd, message, strlen(message));

    // Step 4: Read the response of the Server
    //==========================================
    n = 0;
    memset(buffer, 0, sizeof(buffer));
    n = read(sockfd, buffer, sizeof(buffer));
    buffer[n] = '\0';
    printf("Message from server: %s\n", buffer);

    // Step 5: Close socket descriptor and exit
    //==========================================
    close(sockfd);
    return 0;
}
