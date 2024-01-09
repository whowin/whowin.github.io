/*
 * File: udp-server.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Normal UDP server program under C/S architecture
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall udp-server.c -o udp-server
 * Usage: $ ./udp-server
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

#define PORT        8080
#define BUF_SIZE    1024

int main() {
    int sockfd;
    char buffer[BUF_SIZE];
    char *hello_str = "Hello from server";
    struct sockaddr_in server_addr, client_addr;

    // Step 1: Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    // Step 2: Bind the socket to the server address
    memset(&server_addr, 0, sizeof(server_addr));

    // Filling server information
    server_addr.sin_family      = AF_INET;      // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   // receive message from any IP address
    server_addr.sin_port        = htons(PORT);

    // call bind() function
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return -1;
    }

    while (1) {
        // Step 3: Wait until the datagram packet arrives from the client
        socklen_t len;
        int n;
        len = sizeof(client_addr);  // len is value/result
        memset(&client_addr, 0, len);
        memset(buffer, 0, BUF_SIZE);
        // wait for udp datagram arrives
        n = recvfrom(sockfd, (char *)buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &len); 
        buffer[n] = '\0';

        // Step 4: Process the datagram packet and send a reply to the client
        printf("Client IP: %s\n", inet_ntoa(client_addr.sin_addr));
        printf("Client port: %d\n", ntohs(client_addr.sin_port));
        printf("Client message: %s\n", buffer);
        // Send a reply to the client
        sendto(sockfd, (const char *)hello_str, strlen(hello_str), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len); 
        printf("Hello message sent.\n");

        // Step 5: Go back to Step 3.
    }

    return 0; 
}
