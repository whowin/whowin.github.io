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

独立的 TCP 服务器和UDP服务器，可以找到很多例子，但如果一个服务希望在同一个端口上既提供 TCP 服务，也提供 UDP 服务，写两个服务端显然不是一个好的办法，也不利于以后的维护，本文将把UDP服务器和 TCP 服务器合并成一个服务器，该服务器既可以提供 UDP 服务也可以提供 TCP 服务，本文将给出完整的源代码，阅读本文需要掌握基本的 socket 编程方法，本文对初学者难度不大。
<!--more-->

## 1. 基本流程
* 本示例一共有三个程序，```tcp/udp``` 服务器：```tu-server.c```，```tcp``` 客户端：```t-client.c``` 和 ```udp``` 客户端： ```u-client.c```
* 服务器端程序的基本思路是：在程序中为 tcp 服务和 udp 服务各建立一个 socket，将这两个 socket 放入 readfds 中，并将参数传递给 ```select()```，当 readfds 中(也就是 tcp 或者 udp socket)的某一个有数据发过来(udp)或者有客户端连接请求时，```select()``` 将返回，程序判断是哪个 socket 需要处理然后根据需要进入 TCP 处理程序或者 UDP 处理程序处理 socket 事件；
* 本例中，服务器端做了简单化处理，收到客户端信息后，并不作处理，对 TCP 客户端，回应 "Hello TCP Client"，对UDP客户端，则回应 "Hello UDP Client"；
* 服务器端程序流程
  1. **建立一个用于侦听TCP连接请求的TCP socket**
    ```C
    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    ```
  2. **建立一个用于接收UDP数据的UDP socket**
    ```C
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    ```
  3. **将这两个socket均绑定到服务器的地址上**
    ```C
    #define PORT            5000

    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    bind(tcp_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    bind(udp_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    ```
  4. **在TCP socket上侦听**
    ```C
    listen(tcp_fd, 5);
    ```
  5. **将TCP socket和UDP socket均加入到一个空的描述符集中**
    ```C
    fd_set rset;

    FD_ZERO(&rset);
    FD_SET(tcp_fd, &rset);
    FD_SET(udp_fd, &rset);
    ```
  6. **调用select()直至其中一个socket有可读数据**
    ```C
    int max_fd = (tcp_fd > udp_fd) ? tcp_fd : udp_fd + 1;
    select(max_fd, &rset, NULL, NULL, NULL);
    ```
    
  7. **处理TCP客户端发出的请求**
    > 如果是 TCP 客户端发出请求，则接受客户端的连接请求，接收客户端发来的信息，然后回应 "Hello TCP Client"，然后退出，回到步骤 5；

    ```C
    #define BUF_SIZE        1024

    struct sockaddr_in client_addr;
    char buffer[BUF_SIZE];
    socklen_t len;
    ssize_t n;
    char *tcp_msg = "Hello TCP Client";

    socklen_t len = sizeof(client_addr);
    int conn_fd = accept(tcp_fd, (struct sockaddr*)&client_addr, &len);
    if (conn_fd > 0) {
        bzero(buffer, sizeof(buffer));
        n = 0;
        n = read(conn_fd, buffer, sizeof(buffer));
        if (n > 0) {
            buffer[n] = '\0';
            write(conn_fd, tcp_msg, strlen(tcp_msg));
        }
        close(conn_fd);
    }
    ```
  8. **处理UDP客户端发来的消息**
    > 如果是 UDP 客户端发来消息，则接收客户端发来的信息，然后回应 "Hello UDP Client"，回到步骤5

    ```C
    #define BUF_SIZE        1024

    struct sockaddr_in client_addr;
    char buffer[BUF_SIZE];
    socklen_t len;
    ssize_t n;
    char *udp_msg = "Hello UDP Client";

    socklen_t len = sizeof(client_addr);
    bzero(buffer, sizeof(buffer));
    n = 0;
    n = recvfrom(udp_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &len);
    if (n > 0) {
        buffer[n] = '\0';
        sendto(udp_fd, udp_msg, strlen(udp_msg), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
    ```

* tcp 客户端程序流程
  1. **建立一个TCP socket**
    ```C
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ```
  2. **向服务器发出连接请求，等待服务器接受**
    ```C
    #define SERVER_IP   "192.168.2.112"
    #define PORT        5000

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ```
  3. **向服务器发送信息，并等待服务器的回应**
    ```C
    char *message = "Hello Server";
    write(sockfd, message, strlen(message));
    ```

  4. **接收到服务器回应**
    ```C
    #define BUF_SIZE    1024

    char buffer[BUF_SIZE];
    int n = 0
    memset(buffer, 0, sizeof(buffer));
    n = read(sockfd, buffer, sizeof(buffer));
    buffer[n] = '\0';
    printf("Message from server: %s\n", buffer);
    ```
  5. 关闭socket，退出
    ```C
    close(sockfd);
    ```

* udp客户端程序流程
  1. **建立一个UDP socket**
    ```C
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ```
  2. **向服务器发送信息，并等待服务器回应**
    ```C
    #define SERVER_IP   "192.168.2.112"
    #define PORT        5000

    char *message = "Hello Server";
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    // Filling server information
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    // send hello message to server
    sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    ```

  3. **收到服务器回应**
    ```C
    #define BUF_SIZE    1024

    char buffer[BUF_SIZE];
    int len = sizeof(struct sockaddr_in);
    int n = 0;
    memset(buffer, 0, BUF_SIZE);
    n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, (socklen_t *)&len);
    buffer[n] = '\0';
    printf("Message from server: %s\n", buffer);
    ```
  4. **关闭socket，退出**
    ```C
    close(sockfd);
    ```

## 2. 主要函数、宏和数据结构
* **select()函数**
  - ```select()``` 函数用于监视文件描述符的变化情况——可读、可写或是异常。
  - 函数定义
    ```C
    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
    ```
  - 参数说明
    1. nfds：最大的文件描述符加1
    2. readfds：等待可读事件的文件描述符集合，如果不关心读事件，可设置为NULL；
    3. writefds：等待可写事件(缓冲区中是否有空间)的文件描述符集合，如果不关心写事件，可设置为NULL；
    4. exceptfds：当相应的文件描述符发生异常时，失败的文件描述符将被放进exceptfds中，如果不关心异常事件，可设置为NULL；
    5. timeout：等待select返回的事件；如果timeout=NULL，则一直等待，直至select返回；如果timeout=固定值，则等待固定时间后返回；如果timeout=0，则立即返回；

* **struct timeval结构**
  - 该结构用于指定 select 函数的超时时间
  - 定义
    ```C
    struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* microseconds */
    };
    ```
  - 如果希望 ```select()``` 等待5秒后返回，则要设置 ```struct timeval timeout={5, 0}```;

* **fd_set**
  - 文件描述符集，该结构定义在头文件 ```sys/select.h``` 中
  - 本质上，fd_set 是一个 ```long int``` 的数组，其中的每一位表示一个文件描述符，在 ```x86-64``` 中，```long int``` 长度为 8 个字节，64 位，所以 ```fd_set[0]``` 可以表示文件描述符 ```fd=0-63```，```fd_set[1]``` 可以表示文件描述符 ```fd=64-127```；
  - fd_set是一个文件描述符的集合，当fd_set中的某一位为1，表示这个集合中包含有这个fd

* **宏FD_ZERO**
  - 该宏定义在头文件 ```sys/select.h``` 中
  - 该宏可以将一个 fd_set 全部清空，下面的例子将 fds 清空
    ```C
    fd_set fds;
    FD_ZERO(fds);
    ```

* **宏FD_SET**
  - 该宏定义在头文件 ```sys/select.h``` 中
  - 将指定的文件描述符 fd 加入到某一个文件描述符集 fd_set 中，下面的例子将文件描述符 fd 加入到文件描述符集 fds 中
    ```C
    fd_set fds;
    FD_SET(FD, fds);
    ```

* **宏FD_ISSET**
  - 该宏定义在头文件 ```sys/select.h``` 中
  - 检查一个文件描述符集 fds 中是否有文件描述符 fd，下面例子中检查文件描述符集 fds 中是否存在文件描述符 fd；
    ```C
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
* 本示例一共有三个程序，```tcp/udp``` 服务器：```tu-server.c```，tcp 客户端：```t-client.c``` 和 udp 客户端：```u-client.c```
* 本示例演示了如何使用 ```select``` 机制在一个服务器程序里既提供 TCP 服务，又提供 UDP 服务；有些服务(比如聊天)，可以既允许 UDP 接入，也允许 TCP 接入的，这种情况下，这样一种机制就显得比较实用；
* 服务器端程序：[tu-server.c][src01](**点击文件名下载源程序**)

* 服务器端程序的编译
  ```bash
  gcc -Wall tu-server.c -o tu-server
  ```

* 服务器端程序的测试
  - 在一台机器上启动服务器端程序
    ```bash
    ./tu-server
    ```
  - 假定服务器 IP 为 ```192.168.2.112```，在另一台机器上启动 nc 模拟客户端，测试 TCP
    ```bash
    nc -n 192.168.2.112 5000
    hello server
    ```
  - 退出 TCP 测试，重新启动 nc，测试 UDP
    ```bash
    nc -n -u 192.168.2.112 5000
    ```
  - 有关 nc 命令的使用方法，可以参考另一篇文章[《如何在Linux命令行下发送和接收UDP数据包》][article3]
  - 在服务器端的运行截屏

    ![screenshot of tcp/udp server test][img01]

  --------------
  - TCP 测试客户端的截屏

    ![screenshot of tcp client test][img02]

  --------------
  - UDP 测试客户端的截屏

    ![screenshot of udp client test][img03]

*************
* TCP 客户端程序：[t-client.c][src02](**点击文件名下载源程序**)

* UDP 客户端程序：[u-client.c][src03](**点击文件名下载源程序**)

* 客户端程序编译
  ```bash
  gcc -Wall t-client.c -o t-client
  gcc -Wall u-client.c -o u-client
  ```

* 程序运行
  - 在服务器上(```192.168.2.112```)上运行服务器端程序
    ```bash
    ./tu-server
    ```
  - 在另一台机器上运行客户端程序
    ```bash
    ./t-client
    ./u-client
    ```
  - 服务器端的运行截图

    ![Screenshot of server][img04]

  --------------
  - 客户端的运行截图
  
    ![Screenshot of client][img05]

## 4. 后记
* 服务器端对 TCP 连接的处理是在是太简陋了，因为 TCP 连接建立后，产生一个新的 socket，本例中为 conn_fd，通常的做法应该是把 conn_fd 也加入到 rset 中，这样就可以处理多个 TCP 连接了，同时在处理 TCP 连接时也不会让程序阻塞；
* 服务器端对 TCP 连接的处理，也可以使用多线程的方式，即 accept 一个连接请求后，生成新的 conn_fd，建立一个线程，专门处理这个 connection，也不失为一个办法，但相对要复杂一些。


## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180010/tu-server.c
[src02]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180010/t-client.c
[src03]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180010/u-client.c

[img01]:/images/180010/server_test.png
[img02]:/images/180010/tcpclient_test.png
[img03]:/images/180010/udpclient_test.png
[img04]:/images/180010/screenshot_of_server.png
[img05]:/images/180010/screenshot_of_client.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/c2e13118d7c5db84c26cd9014f8f4507.png
[img02]:https://img-blog.csdnimg.cn/img_convert/b3920d5f55520c342c90dd1f069b7abc.png
[img03]:https://img-blog.csdnimg.cn/img_convert/277d8cafee8c53cc294088f28dd8da49.png
[img04]:https://img-blog.csdnimg.cn/img_convert/27b8175c759294d053b07742e5992ee7.png
[img05]:https://img-blog.csdnimg.cn/img_convert/ee8a02c6cd3416aab83871306c410cc0.png
-->

<!--gitee
[article1]:https://whowin.gitee.io/post/blog/network/0013-udp-server-client-implementation-in-c/
[article2]:https://whowin.gitee.io/post/blog/network/0012-tcp-server-client-implementation-in-c/
[article3]:https://whowin.gitee.io/post/blog/network/0005-send-udp-via-linux-cli/
-->
[article1]:https://blog.csdn.net/whowin/article/details/129728570
[article2]:https://blog.csdn.net/whowin/article/details/129688443
[article3]:https://blog.csdn.net/whowin/article/details/128890866
