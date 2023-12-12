---
title: "使用C语言实现服务器/客户端的UDP通信"
date: 2023-01-05T16:43:29+08:00
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
draft: false
#references: 
# - [C语言socket编程](https://blog.csdn.net/Ga4ra/article/details/84283005)
# - [C语言UDP编程流程](https://www.cnblogs.com/lbao/p/16340968.html)
# - [Programming UDP sockets in C on Linux – Client and Server example](https://www.binarytides.com/programming-udp-sockets-c-linux/)
# - [Programming with UDP sockets](https://people.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html)
# - [UDP Server-Client implementation in C](https://www.geeksforgeeks.org/udp-server-client-implementation-c/)
postid: 180013
---

本文旨在使用简单的例子说明编写一个服务器/客户端模式的UDP通信程序的步骤，并通过一个实例给出了程序的基本框架，使读者可以在这个框架下经过扩展构建出更加复杂的服务，本文适合网络通信的初学者阅读；本文的程序在ubuntu 20.04中编译运行成功，gcc版本号为：9.4.0
<!--more-->

## 1. 前言
> 当两台主机间需要通信时，TCP和UDP是两种最常用的传输层协议，TCP是一种面向连接的传输协议，常用于对传输可靠性要求比较高的场合，比如传输文件；而UDP是一种无连接的通信方式，用于传输一些要求速度快，但对可靠性要求不高的场合，比如实时视频的传输；

> 所谓面向连接指的是在进行传输数据之前要确保进行通信的两台主机已经建立起了连接，比如A机和B机进行TCP通信，A发起通信时要首先连接B机，连接建立起来以后才能够进行数据传输(发送和接收数据)，如果无法建立连接(比如B机没有开机)则不能进行数据传输；TCP协议有完善的错误检查和错误恢复的能力，能够保证数据完好无损地传输到目的地；

> 所谓面向无连接指的是在传输数据之前无须在两台进行通信的主机之间建立连接，直接发送数据即可，带来的问题是如果需要通信的两台主机如果其中有一台没有连接在网络上，那么发送的数据肯定是不能到达目的地的，同样，UDP协议没有完善的纠错机制，所以如果传输过程中出现错误，某个数据包会被丢弃，导致数据没有到达目的地或者到达目的地的数据不完整；

> 听上去使用UDP协议进行数据传输非常不可靠，似乎这种传输没有什么意义，其实不然，尽管UDP传输不能保证可靠，但其占用的资源相比TCP要少一些，所以其传输效率要高于TCP协议，况且在网络状况良好的情况下，绝大多数的UDP数据包是可以顺利到达目的地的，有一些应用场景，比如实时语音的传输，就非常适合使用UDP传输，传输过程中即便出现一些丢失、损坏等无法到达的情况，实际感受无非是听到的声音有断续或者杂音，并不会影响后面的语音，但是传输效率高可以使实时语音的延时要小一些；

> 基于TCP协议的一次通信需要三个socket，服务器端建立一个server_sock监听一个特定端口，客户端建立一个client_sock向服务器发起连接请求，服务器端接受连接并生成一个新的connect_sock与该客户端进行通信；基于UDP协议的通信省去了客户端向服务器端发起连接请求和服务器端接受连接的步骤，收到UDP报文后直接从UDP报头中获取发送端的IP地址和端口号，并将回应直接发送给发送端。

## 2. 服务器/客户端UDP通信的基本流程
* **服务端流程**
  1. 创建一个UDP Socket；
    ```C
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ```
  2. 将socket绑定到服务器地址上；
    ```C
    #define PORT        8080
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    ```
  3. 等待从客户端发送的数据到达；
    ```C
    #define BUF_SIZE    1024

    struct sockaddr_in client_addr;
    char buffer[BUF_SIZE];

    socklen_t len = sizeof(client_addr);  // len is value/result
    memset(&client_addr, 0, len);
    memset(buffer, 0, BUF_SIZE);
    recvfrom(sockfd, (char *)buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &len); 
    ```
  4. 处理收到的数据并向客户端发送回复；
    ```c
    char *hello_str = "Hello from server";
    sendto(sockfd, (const char *)hello_str, strlen(hello_str), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len); 
    ```
  5. 返回步骤3。

* **客户端流程**
  1. 创建一个UDP Socket；
    ```c
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ```
  2. 向服务器发送消息；
    ```C
    #define SERVER_IP   "192.168.2.112"
    #define SERVER_PORT 8080

    struct sockaddr_in server_addr;
    char *hello = "Hello from client";

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    int n;
    socklen_t len;
    sendto(sockfd, (const char *)hello, strlen(hello), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    ```
  3. 等待接收服务器的响应；
    ```C
    #define BUF_SIZE    1024

    char buffer[BUF_SIZE];
    recvfrom(sockfd, (char *)buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *)&server_addr, &len);
    ```
  4. 处理收到的服务器端响应，如有必要，返回第2步。
  5. 关闭Socket并退出。

## 3. UDP编程常用函数和数据结构
* **int socket(int domain, int type, int protocol)**
  - 建立一个没有绑定地址的socket，返回socket文件描述符
  - 参数说明：
    1. domain: 协议族；IPv4为AF_INET，IPv6为AF_INET6
    2. type：socket的传输方式；TCP为SOCK_STREAM，UDP为SOCK_DGRAM
    3. protocol：指定socket使用的协议；通常情况下，一个协议族只支持一种协议，所以通常将protocol置为0，让系统选择匹配的协议；

* **int bind(int sockfd, (const struct sockaddr \*)addr, socklen_t addrlen)**
  - 为未绑定地址的socket分配地址
  - 参数说明
    1. sockfd：要绑定的socket的文件描述符
    2. addr：绑定的地址(后面会介绍struct sockaddr)
    3. addrlen：addr结构的大小

* **ssize_t sendto(int sockfd, (const void \*)buf, size_t len, int flags, (const struct sockaddr \*)dest_addr, socklen_t addrlen)**
  - 在socket上发送消息；
  - 参数说明
    1. sockfd：socket文件描述符
    2. buf：发送数据的缓冲区，buf中存放有要发送的数据
    3. len：要发送数据的长度
    4. flags：标志位，每一位代表一种标志，通常情况下可以设为0
    5. dest_addr：目的地址，需要在发送数据前填好
    6. addrlen：dest_addr结构的大小

* **ssize_t recvfrom(int sockfd, (void \*)buf, size_t len, int flags, (struct sockaddr \*)src_addr, (socklen_t \*)addrlen)**
  - 在socket上接收数据
  - 参数说明：
    1. sockfd：socket文件描述符
    2. buf：接收数据的缓冲区，收到的数据将放到buf中
    3. len：接收缓冲区的最大长度
    4. flags：标志位，每一位代表一种标志，通常情况下可以设为0
    5. src_addr：源地址，收到数据时会填入数据的来源地址
    6. addrlen：src_addr结构的大小，注意这里传递的是指针，而不是数字本身，和sendto()不同。

* 结构**struct sockaddr** - 定义在bits/socket.h
  ```C
  struct sockaddr {
      __SOCKADDR_COMMON (sa_);	/* Common data: address family and length.  */
      char sa_data[14];		/* Address data.  */
  };
  ```
  > 这个结构用做bind、recvfrom、sendto等函数的参数，指明地址信息，但实际编程中并不直接针对此数据结构操作，因为针对不同的协议族，地址信息是不同的，比如对于IPv4(AF_INET)，使用(struct sockaddr_in)，这个结构和sockaddr是等价的，但是把sockaddr中的sa_data部分做了更明确的定义；当协议族为IPv6(AF_INET6)时，使用(struct sockaddr_in6)来表示IPv6的地址；

  > 在使用这个结构作为函数参数时，通常需要传递地址结构的指针，而且还需要传递这个地址结构的长度，比如sendto()函数的定义为：ssize_t sendto(int sockfd, (const void *)buf, size_t len, int flags, (const struct sockaddr *)dest_addr, socklen_t addrlen)；其中最后一个参数addrlen就是地址结构dest_addr的长度，这是因为对不同的协议族，使用的地址结构不同，这个地址结构的长度也是不同的，比如IPv4使用的地址结构(struct sockaddr_in)和IPv6使用的地址结构(struct sockaddr_in6)的长度就不同。

* 结构**struct sockaddr_in** - 定义在netinet/in.h
  ```C
  struct sockaddr_in {
      __SOCKADDR_COMMON (sin_);
      in_port_t sin_port;           /* Port number.  */
      struct in_addr sin_addr;      /* Internet address.  */

      /* Pad to size of `struct sockaddr'.  */
      unsigned char sin_zero[sizeof (struct sockaddr)
          - __SOCKADDR_COMMON_SIZE
          - sizeof (in_port_t)
          - sizeof (struct in_addr)];
  };
  ```
  > 当协议族为IPv4时，使用这个结构来指明地址信息，对于普通的UDP编程而言，有下面几个地方会用到这个结构：

    1. 当一个socket需要绑定一个地址时，需要填写这个结构并作为参数传递给bind()函数；
    2. 当使用sendto()发送UDP报文时，需要使用这个结构来指定目的地址；
    3. 当使用recvfrom()接收报文时，需要把一个空的地址结构指针传递给recvfrom()函数，接收报文成功时，发送方的地址信息会自动填入这个地址结构中；

  > 这个结构中只有三个字段，

    1. sin_family: 协议族，IPv4下填AF_INET
    2. sin_port: 端口号；存储为网络字节顺序，所以需要使用htons()转换一下，比如htons(8080)；
    3. sin_addr：这是一个结构(struct in_addr)，这个结构中只有一个字段s_addr，这是一个32位的IP地址，对于通常使用的字符串IP地址，需要用inet_addr()转换一下，见下面例子：

    ```C
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("192.168.1.10");
    ```

## 4. 服务器/客户端UDP通信的实例
* 至此我们已经有足够的知识来编写一个服务器/客户端UDP通信的简单程序了，程序分为两部分：服务器端程序udpserver.c和客户端程序udpclient.c
* 这两个程序表达了编写一个服务器/客户端UDP通信程序的基本框架，实际使用还需要添加许多代码；
* 这两个程序在ubuntu 20.04下编译运行成功；
* 服务器端程序：[udp-server.c][src02](**点击文件名下载源程序**)
  > 服务器端程序在绑定地址时绑定的是服务器的地址，端口号是程序接收数据的端口，INADDR_ANY这个宏在netinet/in.h中定义，实际上就是一个32位的0，对应的IP地址就是0.0.0.0，和inet_addr("0.0.0.0")是一样的，inet_addr()函数会把一个字符串形式的IP地址转换成一个网络字符顺序的32位的IP地址，这里将IP绑定为0.0.0.0的含义是本机的所有IP地址，一台机器有可能有多个网卡，比如有线网卡和无线网卡，那么这台机器就可能有两个IP地址，加上loopback，就有三个IP地址，假定这三个地址分别是：192.168.2.112(有线网卡)、192.168.2.113(无线网卡)和127.0.0.1(loopback)，如果这里设置成inet_addr("192.168.2.112")，则表示只接收发往目的地址是192.168.2.112这个IP的信息，也就是只接收从有线网卡收到的数据，大家可以试一下；如果绑定的IP地址不是本机的一个合法IP，在执行bind()时会出错。

* 编译、运行和测试服务器端程序
  - 编译
    ```bash
    gcc -Wall udp-server.c -o udp-server
    ```
  - 运行服务器端程序
    ```bash
    ./udp-server
    ```
  - 在另一台机器上的终端上运行下面指令，则在运行了udpserver的终端上可以看到收到的信息：hello from netcat
    ```bash
    echo "hello from netcat">/dev/udp/192.168.2.114/8080
    ```
  - 有关在命令行下发送udp数据的方法，可以参考另一篇文章[《如何在Linux命令行下发送和接收UDP数据包》][article1]
  - 服务器端程序只能用ctrl+c才能退出；
  - 运行截图

    ![test udpserver with nc][img01]

****************
* 客户端程序：[udp-client.c][src01](**点击文件名下载源程序**)

  > 客户端程序的socket不需要绑定地址，但在发送时需要设置目的地址，目的地址是服务器的地址，端口号是服务器端程序绑定的端口，192.168.2.112是服务器的IP地址，请根据自身的情况进行修改，IP和端口号必须和服务器一致，否则服务器无法收到信息。

* 客户端程序的编译
  ```bash
  gcc -Wall udp-client.c -o udp-client
  ```
* 程序运行
  - 一台机器上运行服务端程序：udp-server
    ```bash
    ./udp-server
    ```
  - 在另一台机器上运行客户端程序：udp-client
    ```bash
    ./udp-client
    ```
  - 客户端程序的运行截图

    ![screenshot of udpclient][img02]

  -------------------
  - 服务器端程序的运行截图

    ![screenshot of udpserver][img03]

*********
## 5. 后记
* 从本文的例子看，UDP的服务器/客户端的通信框架非常简单，但是本文的例子只是一个最基本的框架，并不适合在生产环境下运行；
* 用于发送和接收数据的两个函数sento()和recvfrom()中有一个参数flags，本文并没有进行讨论，请参考其它文献；
* 对于一个socket还有很多模式可以设置，通常使用fcntl()和setsockopt()进行设置，这些在本文中也没有讨论；
* 对于服务器端程序，一定会遇到多个客户端同时向服务器端发送报文的情况，有多种方法处理这种情况，比如：多线程、select、epoll等；
* UDP通信也是可以像TCP通信那样，在传输数据前进行连接的，这一点，本文也没有进行讨论。


## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://whowin.gitee.io/sourcecodes/180013/udp-client.c
[src02]:https://whowin.gitee.io/sourcecodes/180013/udp-server.c
  
[img01]:https://whowin.gitee.io/images/180013/test_udpserver_with_nc.png
[img02]:https://whowin.gitee.io/images/180013//screenshot_of_udpclient.png
[img03]:https://whowin.gitee.io/images/180013/screenshot_of_udpserver.png

<!--gitee
[article1]:https://whowin.gitee.io/post/blog/network/0005-send-udp-via-linux-cli/
-->
[article1]:https://blog.csdn.net/whowin/article/details/128890866
