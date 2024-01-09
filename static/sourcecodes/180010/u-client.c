/*
 * File: udp-client.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * TCP client for a combinatorial TCP and UDP server
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall udp-client.c -o udp-client
 * Usage: $ ./udp-client
 * 
 * Example source code for article 《使用select实现的UDP/TCP组合服务器》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVER_IP   "192.168.2.112"
#define PORT        5000
#define BUF_SIZE    1024

int main() {
    int sockfd;
    char buffer[BUF_SIZE];
    char *message = "Hello Server";
    struct sockaddr_in server_addr;
    int n, len;

    // Step 1: Creating socket file descriptor
    //=========================================
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket creation failed");
        exit(0);
    }

    // Step 2: Send a message to the server
    //======================================
    memset(&server_addr, 0, sizeof(server_addr));

    // Filling server information
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    // send hello message to server
    sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));

    // Step 3: Wait until a response from the server is received
    //===========================================================
    // receive server's response
    len = sizeof(struct sockaddr_in);
    n = 0;
    memset(buffer, 0, BUF_SIZE);
    n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, (socklen_t *)&len);
    buffer[n] = '\0';
    printf("Message from server: %s\n", buffer);

    // Step 4: Close socket descriptor and exit
    //==========================================
    close(sockfd);
    return 0;
}
