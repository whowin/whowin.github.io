---
title: "使用select实现的UDP/TCP组合服务器"
date: 2023-01-07T16:43:29+08:00
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
  - udp
  - tcp
draft: false
#references: 
# - [Socket Programming in C/C++](https://www.geeksforgeeks.org/socket-programming-cc/)
# - [Computer Network Tutorials](https://www.geeksforgeeks.org/computer-network-tutorials/)
# - [Socket中SO_REUSEADDR详解](https://blog.csdn.net/u010144805/article/details/78579528)
# - [TCP and UDP server using select](https://www.geeksforgeeks.org/tcp-and-udp-server-using-select/)
# - [select详解](https://blog.csdn.net/sjp11/article/details/126312199)
postid: 180010
---

在以前的文章中，我们给出过UDP服务器和TCP服务器的例子，本文将把UDP服务器和TCP服务器合并成一个服务器，该服务器既可以提供UDP服务也可以提供TCP服务，本文将给出完整的源代码。
<!--more-->

## 1. 基本流程
* 本示例一共有三个程序，tcp/udp服务器：tuserver.c，tcp客户端：tclient.c和udp客户端uclient.c
* 服务器端程序的基本思路是：在程序中为tcp服务和udp服务各建立一个socket，将这两个socket放入readfds中，并将参数传递给select()，当readfds中(也就是tcp或者udp socket)的某一个有数据发过来(udp)或者有客户端连接请求(时)，select()将返回，程序判断是哪个socket需要处理然后根据需要进入TCP处理程序或者UDP处理程序处理socket事件；
* 本例中，服务器端做了简单化处理，收到客户端信息后，并不作处理，对TCP客户端，回应"Hello TCP Client"，对UDP客户端，则回应"Hello UDP Client"；
* 服务器端程序流程
  1. 建立一个用于侦听TCP连接请求的TCP socket
  2. 建立一个用于接收UDP数据的UDP socket
  3. 将这两个socket均绑定到服务器的地址上
  4. 在TCP socket上侦听
  5. 将TCP socket和UDP socket均加入到一个空的描述符集中
  6. 调用select()直至其中一个socket有可读数据
  7. 如果是TCP客户端发出请求，则接受客户端的连接请求，接收客户端发来的信息，然后回应"Hello TCP Client"，然后退出，回到步骤5；
  8. 如果是UDP客户端发来消息，则接收客户端发来的信息，然后回应"Hello UDP Client"，回到步骤5

* tcp客户端程序流程
  1. 建立一个TCP socket
  2. 向服务器发出连接请求，等待服务器接受
  3. 向服务器发送信息，并等待服务器的回应
  4. 接收到服务器回应
  5. 关闭socket，退出

* udp客户端程序流程
  1. 建立一个UDP socket
  2. 向服务器发送信息，并等待服务器回应
  3. 收到服务器回应
  4. 关闭socket，退出

## 主要函数、宏和数据结构
* **select()函数**
  - select()函数用于监视文件描述符的变化情况——可读、可写或是异常。
  - 函数定义
    ```
    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
    ```
  - 参数说明
    1. nfds：最大的文件描述符加1
    2. readfds：等待可读事件的文件描述符集合，如果不关心读事件，可设置为NULL；
    3. writefds：等待可写事件(缓冲区中是否有空间)的文件描述符集合，如果不关心写事件，可设置为NULL；
    4. exceptfds：当相应的文件描述符发生异常时，失败的文件描述符将被放进exceptfds中，如果不关心异常事件，可设置为NULL；
    5. timeout：等待select返回的事件；如果timeout=NULL，则一直等待，直至select返回；如果timeout=固定值，则等待固定时间后返回；如果timeout=0，则立即返回；

* **struct timeval结构**
  - 该结构用于指定select函数的超时时间
  - 定义
    ```
    struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* microseconds */
    };
    ```
  - 如果希望select()等待5秒后返回，则要设置struct timeval timeout={5, 0};

* **fd_set**
  - 文件描述符集，该结构定义在头文件sys/select.h中
  - 本质上，fd_set是一个long int的数组，其中的每一位表示一个文件描述符，在x86-64中，long int长度为8个字节，64位，所以fd_set[0]可以表示文件描述符fd=0~63，fd_set[1]可以表示文件描述符fd=64~127；
  - fd_set是一个文件描述符的集合，当fd_set中的某一位为1，表示这个集合中包含有这个fd

* **宏FD_ZERO**
  - 该宏定义在头文件sys/select.h中
  - 该宏可以将一个fd_set全部清空，下面的例子将fds清空
    ```
    fd_set fds;
    FD_SET(fds);
    ```

* **宏FD_SET**
  - 该宏定义在头文件sys/select.h中
  - 将指定的文件描述符fd加入到某一个文件描述符集fd_set中，下面的例子将文件描述符fd加入到文件描述符集fds中
    ```
    fd_set fds;
    FD_SET(FD, fds);
    ```

* **宏FD_ISSET**
  - 该宏定义在头文件sys/select.h中
  - 检查一个文件描述符集fds中是否有文件描述符fd，下面例子中检查文件描述符集fds中是否存在文件描述符fd
    ```
    fd_set fds
    ......
    if (FD_ISSET(fd, &fds)) {
        // fd is part of the set fds.
        some codes
    } else {
        // fd is not in the fds
        some codes
    }
    ```

* 其它函数和数据结构的介绍，请参考另两篇文章[《使用C语言实现服务器/客户端的UDP通信》][article1]和[《使用C语言实现服务器/客户端的TCP通信》][article2]

## 3. 实例
* 本示例一共有三个程序，tcp/udp服务器：tuserver.c，tcp客户端：tclient.c和udp客户端uclient.c
* 本示例演示了如何使用select机制在一个服务器程序里既提供TCP服务，又提供UDP服务；有些服务(比如聊天)，可以既允许UDP接入，也允许TCP接入的，这种情况下，这样一种机制就显得比较实用；
* 服务器端程序：tuserver.c
  ```
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
  ```

* 服务器端程序的编译
  ```
  gcc -Wall tuserver.c -o tuserver
  ```

* 服务器端程序的测试
  - 在一台机器上启动服务器端程序
    ```
    ./tuserver
    ```
  - 假定服务器IP为192.168.2.112，在另一台机器上启动nc模拟客户端，测试TCP
    ```
    nc -n 192.168.2.112 5000
    hello server
    ```
  - 退出TCP测试，重新启动nc，测试UDP
    ```
    nc -n -u 192.168.2.112 5000
    ```
  - 有关nc命令的使用方法，可以参考另一篇文章[《如何在Linux命令行下发送和接收UDP数据包》][article3]
  - 在服务器端的运行截屏

    ![screenshot of tcp/udp server test][img01]

  --------------
  - TCP测试客户端的截屏

    ![screenshot of tcp client test][img02]

  --------------
  - UDP测试客户端的截屏

    ![screenshot of udp client test][img03]

*************
* TCP客户端程序：tclient.c
  ```
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
  ```

* UDP客户端程序：uclient.c
  ```
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
  ```

* 客户端程序编译
  ```
  gcc -Wall tclient.c -o tclient
  gcc -Wall uclient.c -o uclient
  ```

* 程序运行
  - 在服务器上(192.168.2.112)上运行服务器端程序
    ```
    ./tuserver
    ```
  - 在另一台机器上运行客户端程序
    ```
    ./tclient
    ./uclient
    ```
  - 服务器端的运行截图

    ![Screenshot of server][img04]

  --------------
  - 客户端的运行截图
  
    ![Screenshot of client][img05]

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[img01]:/images/180010/server_test.png
[img02]:/images/180010/tcpclient_test.png
[img03]:/images/180010/udpclient_test.png
[img04]:/images/180010/screenshot_of_server.png
[img05]:/images/180010/screenshot_of_client.png


[article1]:../0013-udp-server-client-implementation-in-c/
[article2]:../0012-tcp-server-client-implementation-in-c/
[article3]:../0005-send-udp-via-linux-cli/
