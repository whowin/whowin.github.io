/*
 * File: udp-client.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Normal UDP client program under C/S architecture
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall udp-client.c -o udp-client
 * Usage: $ ./udp-client
 * 
 * Example source code for article 《使用C语言实现服务器/客户端的UDP通信》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
    
#define SERVER_IP   "192.168.2.112"
#define SERVER_PORT 8080
#define BUF_SIZE    1024
    
int main() {
    int sockfd;
    char buffer[BUF_SIZE];
    char *hello = "Hello from client";
    struct sockaddr_in     server_addr;
    
    // Step 1: Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        return -1;
    }

    // Step 2: Send a message to the server.
    memset(&server_addr, 0, sizeof(server_addr));
    // Filling server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    int n;
    socklen_t len;

    //sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    sendto(sockfd, (const char *)hello, strlen(hello), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Hello message sent.\n");

    // Step 3: Wait until response from the server is received
    n = recvfrom(sockfd, (char *)buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *)&server_addr, &len);
    buffer[n] = '\0';

    // Step 4: Process reply
    printf("Server : %s\n", buffer);

    // Step 5: Close socket descriptor and exit
    close(sockfd);
    return 0;
}
