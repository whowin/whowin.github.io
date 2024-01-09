/*
 * File: tcp-udp-server.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * TCP client for a combinatorial TCP and UDP server
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall tcp-udp-server.c -o tcp-udp-server
 * Usage: $ ./tcp-udp-server
 * 
 * Example source code for article 《使用select实现的UDP/TCP组合服务器》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT            5000
#define BUF_SIZE        1024
#define MAXFD(x, y)     (x>y)?x:y

int main() {
    int tcp_fd, conn_fd, udp_fd;
    struct sockaddr_in server_addr, client_addr;

    fd_set rset;
    int nready, max_fd;

    char buffer[BUF_SIZE];
    ssize_t n;
    socklen_t len;

    char *tcp_msg = "Hello TCP Client";
    char *udp_msg = "Hello UDP Client";
    void sig_chld(int);

    // Step 1: Create a TCP socket
    //=============================
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Step 2: Create an UDP socket
    //==============================
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    // Step 3: Bind both sockets to the server address.
    //=================================================
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // binding server addr structure to listenfd
    bind(tcp_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // binding server addr structure to udp sockfd
    bind(udp_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    // Step 4: Listen on the TCP socket
    //==================================
    listen(tcp_fd, 5);

    // get maxfd
    max_fd = MAXFD(tcp_fd, udp_fd) + 1;
    while (1) {
        // Step 5: Put TCP socket and UDP socket descriptors into descriptor set
        //=======================================================================
        // clear the descriptor set
        FD_ZERO(&rset);

        // set listenfd and udpfd in readset
        FD_SET(tcp_fd, &rset);
        FD_SET(udp_fd, &rset);
        // Step 6: Call select and get the ready descriptor(TCP or UDP)
        //==============================================================
        // select the ready descriptor
        nready = select(max_fd, &rset, NULL, NULL, NULL);

        if (nready < 0) {
            printf("select() failed....\n");
            close(tcp_fd);
            close(udp_fd);
            return -1;
        } else if (nready > 0) {
            if (FD_ISSET(tcp_fd, &rset)) {
                // Step 7: if tcp socket is readable then handle it by accepting the connection
                //==============================================================================
                len = sizeof(client_addr);
                conn_fd = accept(tcp_fd, (struct sockaddr*)&client_addr, &len);
                if (conn_fd > 0) {
                    bzero(buffer, sizeof(buffer));
                    n = 0;
                    n = read(conn_fd, buffer, sizeof(buffer));
                    if (n > 0) {
                        buffer[n] = '\0';
                        printf("Message From TCP client: %s\n", buffer);
                        write(conn_fd, tcp_msg, strlen(tcp_msg));
                        sleep(1);
                    }
                    close(conn_fd);
                }
            } else if (FD_ISSET(udp_fd, &rset)) {
                // Step 8: if udp socket is readable receive the message.
                //=======================================================
                len = sizeof(client_addr);
                bzero(buffer, sizeof(buffer));
                n = 0;
                n = recvfrom(udp_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &len);
                if (n > 0) {
                    buffer[n] = '\0';
                    printf("\nMessage from UDP client: %s\n", buffer);
                    sendto(udp_fd, udp_msg, strlen(udp_msg), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                }
            }
        }
        // Go back step 5
    } // end of while
    return 0;
}
