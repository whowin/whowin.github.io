/*
 * File: tcp-client.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Normal TCP client program under C/S architecture
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall tcp-client.c -o tcp-client
 * Usage: $ ./tcp-client
 * 
 * Example source code for article 《使用C语言实现服务器/客户端的TCP通信》
 * https://whowin.gitee.io/post/blog/network/0012-tcp-server-client-implementation-in-c/
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE        80
#define PORT            8080
#define SERVER_IP       "192.168.2.112"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buff[BUF_SIZE];
    int n;

    // Step 1: Create a TCP socket
    //=============================
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else printf("Socket successfully created..\n");

    // Step 2: Send a connection request to the server
    //=================================================
    bzero(&server_addr, sizeof(server_addr));
    // assign IP, PORT
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port        = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    } else printf("connected to the server..\n");

    while (1) {
        // Step 3: Send a message to the server
        //======================================
        bzero(buff, sizeof(buff));
        printf("Enter the message: ");
        n = 0;
        // get the mesage from keyboard. end with return.
        while ((buff[n++] = getchar()) != '\n' && n < BUF_SIZE)
            ;
        write(sockfd, buff, sizeof(buff));
    
        // Step 4: Wait for reply from server
        //====================================
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));

        // step 5: Process the reply
        //===========================
        printf("Server: %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
        // if necessary, go back to step 3
    }
    // Step 6: Close the socket and exit
    //===================================
    close(sockfd);
    return 0;
}
