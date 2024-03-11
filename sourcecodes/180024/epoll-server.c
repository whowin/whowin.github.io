/*
 * File: epoll-server.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * TCP servers that handle multi-client connections using epoll.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g epoll-server.c -o epoll-server
 * Usage: $ ./epoll-server
 * 
 * Example source code for article 《使用epoll()进行socket编程处理多客户连接的TCP服务器实例》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <sys/epoll.h>

#define PORT                8888
#define SERVER_IP           "127.0.0.1"
#define MAX_CONNECTIONS     (3)                 // MAX. connections 
#define MAX_CLIENTS         MAX_CONNECTIONS     // max. numbers of client processes
#define BUF_SIZE            (1024)              // buffer size

#define MAX_EVENTS          (3 * MAX_CONNECTIONS)
#define EPOLL_TIMEOUT_MS    (1 * 60 * 1000)     // 1 minute, timeout for poll()

int listen_sock;                            // Socket descriptor for listening
int client_socks[MAX_CLIENTS + 1] = {0};
//struct pollfd fds[MAX_CONNECTIONS + 1];     // put decriptors and events into struct array for poll()
struct epoll_event *events;
const char *message = "ECHO Daemon v1.0 \n\n";  // greeting message for new connection

/***************************************************************************
 * Function: int add_fd_to_interest_list(int epfd, int fd)
 * Descripton: Add a descriptor into fds
 ***************************************************************************/
int add_fd_to_interest_list(int epfd, int fd) {
    struct epoll_event event;
    int i;

    memset(&event, 0 , sizeof(struct epoll_event));
    // Set up the structure epoll_event
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    // Add a new socket to the interest list
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("server - EPOLL_CTL_ADD failed");
        return -1;
    }

    for (i = 0; i < MAX_CLIENTS + 1; ++i) {
        if (client_socks[i] == 0) client_socks[i] = fd;
    }

    return 0;
}

int remove_fd_from_interest_list(int epfd, int fd) {
    int i;

    for (i = 0; i < MAX_CLIENTS + 1; ++i) {
        if (client_socks[i] == fd) {
            client_socks[i] = 0;
            break;
        }
    }

    /*
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("server -EPOLL_CTL_DEL failed");
        return -1;
    }
    */
    return 0;
}
/****************************************************************
 * Function: void sigint(int signum)
 * Description: ctrl+c handler
 ****************************************************************/
void sigint(int signum) {
    int i;
    printf("\nServer terminated by user.\n");

    for (i = 0; i < MAX_CLIENTS + 1; ++i) {
        if (client_socks[i] > 0) {
            close(client_socks[i]);
        }
    }
    close(listen_sock);
    free(events);
    exit(EXIT_FAILURE);
}
/*******************************************************************
 * Function: int server()
 * Description: Program for server process
 *******************************************************************/
int server() {
    int new_sock, rc, on = 1, i;
    int epfd;
    bool loop = true;

    struct sockaddr_in address;
    int addrlen;

    char buffer[BUF_SIZE];      // data buffer of 1K

    //struct epoll_event event;

    // Step 1: create a listening socket
    //====================================
    if ((listen_sock = socket(AF_INET, SOCK_STREAM , 0)) == 0) {
        perror("server - socket() failed");
        exit(EXIT_FAILURE);
    }
    // Step 2: Allow socket descriptor to be reuseable
    //==================================================
    rc = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    if (rc < 0) {
        perror("server - setsockopt() failed");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }
    // Step 3: Set socket to be nonblocking
    //======================================
    rc = ioctl(listen_sock, FIONBIO, (char *)&on);
    if (rc < 0) {
        perror("server - ioctl() failed");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }
    // Step 4: bind the listen_sock to server address
    //=================================================
    // server address information
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);
    addrlen = sizeof(address);

    // bind the socket to server address, port 8888
    if (bind(listen_sock, (struct sockaddr *)&address, addrlen) < 0) {
        perror("server - bind() failed");
        exit(EXIT_FAILURE);
    }
    // Step 5: start listening
    //=========================
    printf("server - Listening on port %d ...... \n", PORT);
    // try to specify maximum of 30 pending connections for the listenning socket
    if (listen(listen_sock, MAX_CONNECTIONS) < 0) {
        perror("server - listen() failed");
        exit(EXIT_FAILURE);
    }
    // Step 6: Create an epoll instance
    //====================================
    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("server - epoll_create1() failed");
        exit(EXIT_FAILURE);
    }
    // Step 7: Add listening socket to the interest list
    //===================================================
    if (add_fd_to_interest_list(epfd, listen_sock) == -1) {
        return EXIT_FAILURE;
    }
    // Step 8: Alocate memory for events
    //====================================
    events = calloc(MAX_EVENTS, sizeof(struct epoll_event));
    memset(client_socks, 0, sizeof(int) * (MAX_CLIENTS + 1));

    // Step 9: Catch INT handler for ctrl+c
    //=======================================
    signal(SIGINT, sigint);

    do {
        // Step 10: Call epoll_wait() and wait 1 minute for it to complete
        //==================================================================
        printf("server - Waiting on epoll()...\n");
        rc = epoll_wait(epfd, events, MAX_EVENTS, EPOLL_TIMEOUT_MS);
        if (rc < 0) {
            perror("server - epoll() failed");
            break;
        }
        if (rc == 0) {
            printf("server - epoll() timed out.\n");
            continue;;
        }

        // Step 11: Process readable descriptors
        //=========================================
        for (i = 0; i < rc; ++i) {
            if ((events[i].events & EPOLLIN) != EPOLLIN) {
                printf("server - Error. events = 0x%02x\n", events[i].events);
                loop = false;
                break;
            }

            if (events[i].data.fd == listen_sock) {
                // listening descriptor is readable.
                if ((new_sock = accept(listen_sock, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                    perror("server - accept() failed");
                    loop = false;
                    break;
                }

                // inform user of socket number - used in send and receive commands
                printf("server - New connection, socket fd is %d, ip is: %s, port: %d\n" , 
                        new_sock, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                // send greeting message for new connection
                if (send(new_sock, message, strlen(message), 0) != strlen(message)) {
                    perror("server - sending welcome message failed");
                } else {
                    printf("server - Welcome message sent successfully\n");
                }
                // set the socket to NON-BLOCKING
                if (ioctl(new_sock, FIONBIO, (char *)&on) < 0) {
                    perror("server - ioctl() failed");
                    close(new_sock);
                    exit(EXIT_FAILURE);
                }
                // Add the socket to interest list
                if (add_fd_to_interest_list(epfd, new_sock) == -1) {
                    loop = 0;
                    break;
                };
                continue;
            }

            // client descriptor is readable.
            int done = 0;               // not done
            int nbytes = 0;             // how many bytes to read
            do {
                nbytes = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                if (nbytes > 0) {
                    // Step 6: read the date from client socket, then send it back
                    //=============================================================
                    buffer[nbytes] = '\0';
                    printf("server - Read the message from socket %d: %s", events[i].data.fd, buffer);
                    printf("\nserver - Send the message back.\n\n");
                    nbytes = send(events[i].data.fd, buffer, strlen(buffer), 0 );
                    if (nbytes != strlen(buffer)) {
                        perror("server - send()-2 failed");
                    }
                    continue;
                } else if (rc == 0) {
                    // Step 7: socket disconnected, get its details and print
                    //========================================================
                    getpeername(events[i].data.fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("server - disconnected, socket: %d, ip: %s, port: %d\n",
                            events[i].data.fd, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    break;
                } else if (errno == EINTR) {
                    // if errno==EINTR, it means socket is not closed, just because some network errors happened
                    continue;
                } else if (errno == EAGAIN) {
                    done = 1;
                    break;
                } else {
                    perror("server - recv() failed");
                    break;
                }
            } while (1);
            // Step 9: socket was closed unexpectedly
            //========================================
            if (!done) {
                close(events[i].data.fd);
                remove_fd_from_interest_list(epfd, events[i].data.fd);
            }
        }
    } while (loop);

    return 0;
}
/***************************************************************
 * Function: int client()
 * Description: Program for client process
****************************************************************/
int client() {
    int sockfd, n, flags;
    char buffer[BUF_SIZE];
    struct sockaddr_in server_addr;

    // Step 1: Creating socket file descriptor
    //========================================
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client - socket()");
        exit(EXIT_FAILURE);
    }

    // Step 2: Establish a connection with the server.
    //=================================================
    memset(&server_addr, 0, sizeof(server_addr));

    // Filling server information
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    printf("client(%d)- Connecting to server.\n", getpid());
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("client - connect()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 3: Set the socket non-block
    //====================================
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("client - fcntl() faild");
        exit(EXIT_FAILURE);
    }
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // Step 4: When the connection is accepted write a message to a server
    //=====================================================================
    sleep(rand() % 5);
    sprintf(buffer, "Hello srver. This is client %d.", getpid());
    n = write(sockfd, buffer, strlen(buffer));
    if (n != strlen(buffer)) {
        perror("client - write() failed");
        close(sockfd);
        return EXIT_FAILURE;
    }
    printf("client(%d) - sends message to server successfully.\n", getpid());

    // Step 5: Read the response of the Server
    //==========================================
    n = 0;
    memset(buffer, 0, sizeof(buffer));

    do {
        n = read(sockfd, buffer, sizeof(buffer));
        if (n > 0) {
            buffer[n] = '\0';
            //printf("n=%d, strlen(buffer)=%ld\n", n, strlen(buffer));
            printf("client(%d) - Message from server: %s\n", getpid(), buffer);
            if (strcmp(buffer, message) != 0) {
                break;
            }
        } else if (n == 0) {
            printf("client(%d) - socket closed.\n", getpid());
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // means there is no data to read
                //printf("errno = EAGAIN or EWOULDBLOCK\n");
                usleep(100 * 1000);
                continue;;
            }
            
            perror("client - read() failed");
            break;
        }
    } while (1);

    // Step 6: Close socket descriptor and exit
    //==========================================
    close(sockfd);
    return EXIT_SUCCESS;
}

// main program
int main() {
    int pid[MAX_CLIENTS + 1];
    int i = 0;

    srand(time(NULL));
    // 1. Fork server process and client processes
    //=============================================
    for (i = 0; i < (MAX_CLIENTS + 1); ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        sleep(rand() % 5);
    }
 
    if (i == 0) {
        // 2. the first child is server process
        //======================================
        printf("%d. Server process. PID=%d.\n", i + 1, getpid());
        server();
    } else if (i <= MAX_CLIENTS) {
        // 3. next CLIENT_NUM children are client processes
        //==================================================
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client();
    } else if (i > MAX_CLIENTS) {
        // 4. the last child is parent process
        //======================================
        printf("\n=== Parent process. Waiting for child processes exiting ===\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < (MAX_CLIENTS); ++j) {
            pid_wait = wait(NULL);
            printf("Client exited. PID=%d.\n", pid_wait);
        }
        kill(pid[0], SIGINT);
    }

    return EXIT_SUCCESS;

}
