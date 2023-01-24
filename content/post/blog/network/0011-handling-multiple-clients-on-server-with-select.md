---
title: "TCP服务器如何使用select处理多客户连接"
date: 2023-01-09T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
  - "C Language"
  - "Network"
tags:
  - Linux
  - Socket
  - 网络编程
  - select
draft: false
#references: 
# - [Socket Programming in C/C++: Handling multiple clients on server without multi threading](https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/)
postid: 180011
---

本文用一个简化的实例说明如何在一个TCP服务器程序中，使用select处理同时出现的多个客户连接，文章给出了程序源代码，本文假定读者已经具备了基本的socket编程知识，熟悉基本的服务器/客户端模型架构。
<!--more-->

## 1. 基本思路
* TCP服务器端程序，对于每一个连接请求，可以使用多线程的方式为每一个连接启动一个线程处理该连接的通信，但使用多线程的方式，通常认为有如下缺点：
  1. 多线程编程和调试相对都比较难，而且有时会出现无法预知的问题；
  2. 上下文切换的开销较大；
  3. 对于巨大量的连接，可扩展性不足；
  4. 可能发生死锁。
* 使用select处理多连接的基本思路
  1. 建立一个用于侦听的socket，叫做master_socket；
  2. 建立一个sockets数组，用于存储已经与master_socket建立连接的socket，叫做client_socket，初始化时全部清0，数组的长度即为程序允许的最大连接数；
  3. 绑定服务器地址并在master_socket上启动侦听；
  4. 将master_socket、client_socket中不为0的项加入到readfds中，启动select；
  5. 有活动socket返回时，如果是master_socket则调用accept接受连接，生成新的socket并加入到client_socket中，发送欢迎信息后回到步骤4；
  6. 如果不是master_socket，为client_socket中的一员，则调用read从socket中读出数据，处理并做出回应，回到步骤4；
  7. 如果从client_socket读出数据长度为0，表示socket已经关闭，关闭socket，并从client_socket中清除该socket，回到步骤4；
  8. 如果从client_socket读出数据长度小于0，如果errno=EINTR，则直接返回步骤4；
  9. 如果从client_socket读出数据长度小于0，如果errno不是EINTR，则关闭socket，并从client_socket中清除该socket，回到步骤4；

## 2. 主要函数、宏和数据结构
* 请参考[《使用select实现的UDP/TCP组合服务器》][article1]、[《使用C语言实现服务器/客户端的UDP通信》][article2]和[《使用C语言实现服务器/客户端的TCP通信》][article3]

## 3. 实例
* 该实例最多可以同时处理30个连接，理论上可以更多，与机器的资源有关；
* 该实例收到客户端的信息后没有做处理，将收到的信息发回给了客户端；
* 服务器源程序：select_server.c
  ```
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <string.h>
  #include <errno.h>

  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>

  #define PORT            8888
  #define MAX_SOCKETS     30
  #define MAX_CLIENTS     MAX_SOCKETS

  int main(int argc, char *argv[]) {
      int master_socket, new_socket, client_socket[MAX_SOCKETS];
      int activity, i, nread, client_count;
      int max_sd;
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
          max_sd = master_socket;

          client_count = 0;
          // add client sockets to set
          for (i = 0 ; i < MAX_CLIENTS; i++) {
              // if valid socket descriptor then add to read list
              if (client_socket[i] > 0) {
                  FD_SET(client_socket[i], &readfds);
                  client_count++;
              }

              //highest file descriptor number, need it for the select function
              if (client_socket[i] > max_sd) max_sd = client_socket[i];
          }

          // wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
          activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

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
  ```
* 编译
  ```
  gcc -Wall select_server.c -o select_server
  ```
* 运行
  ```
  ./select_server
  ```
* 测试，使用nc模拟客户端，有关nc命令的使用方法，可以参考另一篇文章[《如何在Linux命令行下发送和接收UDP数据包》][article4]
  - 服务器ip：192.168.2.114
  - 在另一台机器(或者多台机器)开三个终端窗口，分别输入下面命令：
    ```
    nc -n 192.168.2.114 8888

    hello from client 1
    ```
    ```
    nc -n 192.168.2.114 8888

    hello from client 2
    ```
    ```
    nc -n 192.168.2.114 8888

    hello from client 3
    ```
  - 分别在第2、第3、第1终端窗口中按下ctrl+c退出nc命令
  - 服务器端运行截屏

    ![Screenshot of server][img01]

  ------------
  - 客户端第一个窗口运行截屏

    ![Screenshot of client][img02]

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[img01]:/images/180011/screenshot_of_server.png
[img02]:/images/180011/screenshot_of_client_1.png


[article1]:../0010-tcp-and-udp-server-using-select/
[article2]:../0013-udp-server-client-implementation-in-c/
[article3]:../0012-tcp-server-client-implementation-in-c/
[article4]:../0005-send-udp-via-linux-cli/
