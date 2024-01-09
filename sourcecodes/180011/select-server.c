/*
 * File: select-server.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * TCP servers that handle multi-client connections using select.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall select-server.c -o select-server
 * Usage: $ ./select-server
 * 
 * Example source code for article 《TCP服务器如何使用select处理多客户连接》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT            8888
#define MAX_SOCKETS     30
#define MAX_CLIENTS     MAX_SOCKETS

int main(int argc, char *argv[]) {
    int master_socket, new_socket, client_socket[MAX_SOCKETS];
    int activity, i, nread, client_count;
    int max_fd;
    struct sockaddr_in address;
    int addrlen;

    char buffer[1025];      // data buffer of 1K
    fd_set readfds;         // set of socket descriptors

    char *message = "ECHO Daemon v1.0 \n\n";    // greeting message for new connection

    // Step 1: create a master socket
    //================================
    if ((master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Step 2: initialise all client_socket[] to 0 so not checked
    //============================================================
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Step 3: bind the master_socket to server address and start listening
    //======================================================================
    // server address information
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    // bind the socket to server address, port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    // try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept the incoming connection
    addrlen = sizeof(address);
    printf("Waiting for connections ...\n");

    while (1) {
        // Step 4: add master socket and client sockets to readfds, then start select()
        //==============================================================================
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(master_socket, &readfds);
        max_fd = master_socket;

        client_count = 0;
        // add client sockets to set
        for (i = 0 ; i < MAX_CLIENTS; i++) {
            // if valid socket descriptor then add to read list
            if (client_socket[i] > 0) {
                FD_SET(client_socket[i], &readfds);
                client_count++;
            }

            //highest file descriptor number, need it for the select function
            if (client_socket[i] > max_fd) max_fd = client_socket[i];
        }

        // wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(master_socket, &readfds)) {
            // Step 5: something happened on the master socket, it means there is an incoming connection, 
            //         then accept the connection, form a new socket, send a greeting mesage,
            //         add the new socket to client sockets array
            //============================================================================================
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // inform user of socket number - used in send and receive commands
            printf("New connection, socket fd is %d, ip is: %s, port: %d\n" , 
                    new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // send new connection greeting message
            if (send(new_socket, message, strlen(message), 0) != strlen(message)) {
                perror("send");
            }

            printf("Welcome message sent successfully\n");

            // add new socket to array of sockets
            if (client_count < MAX_CLIENTS) {
                for (i = 0; i < MAX_CLIENTS; i++) {
                    //if position is empty
                    if (client_socket[i] == 0) {
                        client_socket[i] = new_socket;
                        printf("Adding socket %d to list of sockets as %d\n\n",new_socket, i);
                        client_count++;

                        break;
                    }
                }
            } else {
                printf("Maximum number of clients exceeded\n");
                close(new_socket);
            }
        }

        // else its some IO operation on some other socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_socket[i] == 0) continue;

            if (FD_ISSET(client_socket[i], &readfds)) {
                // Check if it was for closing, and also read the incoming message
                if ((nread = read(client_socket[i], buffer, 1024)) > 0) {
                    // Step 6: read the date from client socket, then send it back
                    //=============================================================
                    buffer[nread] = '\0';
                    printf("Read the message from socket %d: %s", client_socket[i], buffer);
                    printf("Send the message back.\n\n");
                    send(client_socket[i], buffer, strlen(buffer), 0 );
                } else if (nread == 0) {
                    // Step 7: socket disconnected, get his details and print
                    //========================================================
                    getpeername(client_socket[i], (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected, socket: %d, ip: %s, port: %d\n",
                            client_socket[i], inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(client_socket[i]);
                    client_socket[i] = 0;
                } else if (errno != EINTR) {
                    // Step 9: socket was closed unexpectedly
                    //========================================
                    // if errno==EINTR, it means socket is not closed, just because some network errors happened
                    close(client_socket[i]);
                    client_socket[i] = 0;
                }
                // Step 8: else error==EINTR, network error occorred
                //===================================================
            }
        }
    }

    return 0;
}
