/*
 * File: tcp-server.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Normal TCP server program under C/S architecture
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall tcp-server.c -o tcp-server
 * Usage: $ ./tcp-server
 * 
 * Example source code for article 《使用C语言实现服务器/客户端的TCP通信》
 * https://whowin.gitee.io/post/blog/network/0012-tcp-server-client-implementation-in-c/
 *
 */
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF_SIZE        80
#define PORT            8080

int main() {
    int sockfd, connfd;
    socklen_t len;
    struct sockaddr_in server_addr, client_addr;

    char buff[BUF_SIZE];
    int n;

    // Step 1: Create TCP socket
    //===========================
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed...\n");
        exit(EXIT_FAILURE);
    } else printf("Socket successfully created..\n");

    // Step 2: Bind socket to server address
    //=======================================
    bzero(&server_addr, sizeof(server_addr));
    // assign IP, PORT
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr))) != 0) {
        printf("Socket bind failed...\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    } else printf("Socket successfully binded..\n");

    while (1) {
        // Step 3: Listen on the socket and wait for the client asking the server to make a connection
        if ((listen(sockfd, 5)) != 0) {
            printf("Listen failed...\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else printf("Server listening..\n\n");
        
        // Step 4: Accept the connection request from the client and form a new socket
        //=============================================================================
        len = sizeof(client_addr);

        // Accept the data packet from client and verification
        //connfd = accept(sockfd, (struct sockaddr *)&client_addr, &len);
        connfd = accept(sockfd, NULL, NULL);
        if (connfd < 0) {
            printf("Server accept failed...\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else printf("Server accept the client...\n");

        // infinite loop for chat
        while (1) {
            // Step 5: Wait for the message from the client on the new socket
            //================================================================
            bzero(buff, BUF_SIZE);
            // read the message from client and copy it in buffer
            read(connfd, buff, sizeof(buff));
            // Step 6: Process the data and send a reply to the client
            //=========================================================
            // print buffer which contains the client contents
            printf("Client: %s\nEnter the message: ", buff);
            bzero(buff, BUF_SIZE);
            n = 0;
            // get repply from key board. end with return.
            while ((buff[n++] = getchar()) != '\n' && n < BUF_SIZE)
                ;

            // and send that buffer to client
            write(connfd, buff, sizeof(buff));

            // if msg contains "Exit" then server exit and chat ended.
            if (strncmp("exit", buff, 4) == 0) {
                printf("Server Exit...\n");
                break;
            }
        }
        // Step 7: Close the socket
        //==========================
        close(connfd);
        // Step 8: Go back step 3
        //========================
    }

    // After chatting close the socket
    close(sockfd);
}
