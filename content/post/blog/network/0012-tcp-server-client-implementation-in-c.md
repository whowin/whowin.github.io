---
title: "使用C语言实现服务器/客户端的TCP通信"
date: 2023-01-06T16:43:29+08:00
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
  - TCP
draft: false
#references: 
# - [TCP Server-Client implementation in C](https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/)
postid: 180012
---

本文力求使用简单的描述说明一个服务器/客户端TCP通信的基本程序框架，使读者在这个框架下可以构建更加复杂的服务，文中给出了服务器端和客户端的实例源程序，本文适合网络编程的初学者阅读；本文的程序在ubuntu 20.04中编译运行成功，gcc版本号为：9.4.0
<!--more-->

## 1. 前言
> 当两台主机间需要通信时，TCP和UDP是两种最常用的传输层协议，TCP是一种面向连接的传输协议，常用于对传输可靠性要求比较高的场合，比如传输文件；而UDP是一种无连接的通信方式，用于传输一些要求速度快，但对可靠性要求不高的场合，比如实时视频的传输；

> 所谓面向连接指的是在进行传输数据之前要确保进行通信的两台主机已经建立起了连接，比如A机和B机进行TCP通信，A发起通信时要首先连接B机，连接建立起来以后才能够进行数据传输(发送和接收数据)，如果无法建立连接(比如B机没有开机)则不能进行数据传输；TCP协议有完善的错误检查和错误恢复的能力，能够保证数据完好无损地传输到目的地；

> 所谓面向无连接指的是在传输数据之前无须在两台进行通信的主机之间建立连接，直接发送数据即可，带来的问题是如果需要通信的两台主机如果其中有一台没有连接在网络上，那么发送的数据肯定是不能到达目的地的，同样，UDP协议没有完善的纠错机制，所以如果传输过程中出现错误，出错的数据包会被丢弃，导致数据没有到达目的地或者到达目的地的数据不完整；

> 相比较UDP通信，TCP通信对资源要求的要多一些，所以传输速度比起UDP就要慢一些，但其高可靠性的特点使得很多应用层的协议都是基于TCP协议的，比如：HTTP、HTTPS、FTP、SMTP、ssh等；

> 基于TCP协议的一次通信需要三个socket，服务器端建立一个server_sock监听一个特定端口，客户端建立一个client_sock向服务器发起连接请求，服务器端接受连接并生成一个新的connect_sock与该客户端进行通信。

## 2. 服务器/客户端TCP通信的基本流程
* **服务端流程**
  1. 使用socket()，创建一个TCP Socket；
    ```C
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ```
  2. 将socket设置为可重用的状态
    > 这个server只能通过ctrl+c退出，尽管程序中拦截了ctrl+c的信号，在退出之前关闭了打开的socket，以防万一，仍然将socket设置为可重用比较好，这样保证ctrl+c退出后可以马上再进入；

    ```C
    const int reuseaddr_flag = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseaddr_flag, sizeof(reuseaddr_flag));
    ```
  3. 使用bind()，将socket绑定到服务器地址上；
    ```C
    #define PORT            8080
    struct sockaddr server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(PORT);

    bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    ```
  4. 使用listen()，在绑定的服务器地址上监听，等待从客户端发送的连接请求；
    ```C
    listen(sockfd, 5);
    ```
  5. 拦截ctrl+c的信号，保证程序可以干净地退出
    ```C
    void sigint(int signum) {
        if (connfd > 0) close(connfd);
        if (sockfd > 0) close(sockfd);
        exit(EXIT_FAILURE);
    }
    ......
    signal(SIGINT, sigint);
    ```
  6. 使用accept()，接受从客户端发来的连接请求，建立连接，并生成一个面向该连接的socket；
    ```C
    int connfd = accept(sockfd, NULL, NULL);
    ```
  7. 使用read()；在新socket上接收从客户端发来的信息；
    ```C
    #define BUF_SIZE    128
    char buff[BUF_SIZE];

    bzero(buff, BUF_SIZE);
    read(connfd, buff, sizeof(buff));
    ```
  8. 向客户端发送(write)从键盘输入的信息，如有必要，回到步骤7；
    ```C
    bzero(buff, BUF_SIZE);
    int n = 0;
    while ((buff[n++] = getchar()) != '\n' && n < BUF_SIZE)
        ;
    write(connfd, buff, sizeof(buff));
    ```
  9. 关闭与客户端的连接；
    ```C
    close(connfd);
    ```
  10. 返回步骤6。
**********
* **客户端流程**
  1. 使用socket()，创建一个TCP Socket；
    ```C
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ```
  2. 使用connect()，向服务器发起连接请求；
    ```C
    #define SERVER_IP     192.168.2.112
    #define PORT          8080

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port        = htons(PORT);

    connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ```
  3. 使用write()，向服务器发送从键盘输入的消息；
    ```C
    #define BUF_SIZE    128
    char buff[BUF_SIZE];

    bzero(buff, sizeof(buff));
    int n = 0;
    // get the mesage from keyboard. end with return.
    while ((buff[n++] = getchar()) != '\n' && n < BUF_SIZE)
        ;
    write(sockfd, buff, sizeof(buff));
    ```
  4. 使用read()，等待接收服务器的回复；
    ```C
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    ```
  5. 处理收到的服务器端回复，如有必要，返回第3步。
  6. 关闭Socket并退出。
    ```C
    close(sockfd);
    ```

## 3. TCP编程常用的函数和数据结构
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

* **int listen(int sockfd, int backlog)**
  - 在一个已经绑定了地址的socket上侦听
  - 参数说明：
    1. sockfd：已经绑定好地址的socket文件描述符
    2. backlog：侦听队列的最大长度；也就是等待处理的连接请求的最大数量

* **int connect(int sockfd, (const struct sockaddr \*)addr, socklen_t addrlen)**
  - 创建与指定地址的连接；
  - 参数说明：
    1. sockfd：socket文件描述符
    2. addr：建立连接的地址结构，struct sockaddr结构在后面介绍
    3. addrlen：地址结构的大小

* **int accept(int sockfd, (struct sockaddr \*)addr, socklen_t \*addrlen)**
  - 在指定的socket上接受一个连接请求
  - 参数说明：
    1. sockfd：socket文件描述符
    2. addr：建立连接的地址结构，struct sockaddr结构在后面介绍
    3. addrlen：地址结构的大小，注意这里参数是一个指针，与connect()不同
  - 调用这个函数时，addr和addrlen在调用成功后将被填写好对端的地址和端口信息，所以在调用前最好将其清0；如果我们并不关心对端的地址信息，这两个参数其实也可以为NULL；

* **ssize_t read(int fd, void \*buf, size_t count)**
  - 从指定文件描述符中读出内容到缓冲区中
  - 参数说明：
    1. fd：文件描述符(本文中为socket文件描述符)
    2. buf：存放读出内容的缓冲区
    3. count：buf的大小

* **ssize_t write(int fd, (const void \*)buf, size_t count)**
  - 将缓冲区内容写入指定文件描述符中
  - 参数说明：
    1. fd：文件描述符(本文中为socket文件描述符)
    2. buf：存放写入内容的缓冲区
    3. count：需要写入内容的长度

* **int close(int fd);**
  - 关闭一个文件描述符
  - 参数说明：
    1. fd：文件描述符(本文中是一个socket文件描述符)

* 结构**struct sockaddr** - 定义在bits/socket.h
  ```C
  struct sockaddr {
      __SOCKADDR_COMMON (sa_);  /* Common data: address family and length.  */
      char sa_data[14];         /* Address data.  */
  };
  ```
  > 通常情况下，做socket编程时，我们只会include <sys/socket.h>，sys/socket.h中会include <bits/socket.h>；在(struct sockaddr)结构中的宏__SOCKADDR_COMMON展开后就是(sa_family_t sa_family)，sa_family_t是一个类型定义，实际为：unsigned short，所以(struct sockaddr)的结构为：

  ```C
  struct sockaddr {
      sa_family_t sa_family;  /* Common data: address family and length.  */
      char sa_data[14];       /* Address data.  */
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
  > 当协议族为IPv4时，使用这个结构来指明地址信息，就本文而言，有两个地方需要用到这个结构：
  
    1. 服务器端程序在socket绑定服务器地址信息时，需要填写这个结构并作为参数传递给bind()函数；
    2. 客户端程序，在需要向服务器发起连接时，需要填写这个结构并作为参数传递给connect()函数；

  > 这个结构中只有三个字段，

    1. sin_family: 协议族，IPv4下填AF_INET
    2. sin_port: 端口号；存储为网络字节顺序，所以需要使用htons()转换一下，比如htons(8080)；
    3. sin_addr：这是一个结构(struct in_addr)，这个结构中只有一个字段s_addr，这是一个32位的IP地址，对于通常使用的字符串IP地址，需要用inet_addr()转换一下，见下面例子：

    ```C
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("192.168.1.10");
    ```

## 4. 服务器/客户端TCP通信的实例
* 这个实例是一个服务器和客户端聊天的程序，大致运行流程如下：
  1. 服务器端程序启动后侦听在8080端口上；
  2. 客户端程序启动后向服务器发起连接请求；
  3. 服务器端程序接受连接并建立一个新的socket；
  4. 客户端从键盘输入要发送到服务器端的信息，以回车结束，并将信息发到服务器；
  5. 服务器端收到客户端发来的信息，并要求键盘输入回应信息，以回车结束，并将信息发向客户端；
  6. 循环步骤4、5，直至服务器端在键盘输入"exit"并且向客户端发送"exit"；
  7. 服务器端在键盘输入"exit"并发往客户端后退出聊天，并继续侦听在8080端口上；
  8. 客户端在收到服务器端发来的"exit"后退出程序。
* 这两个程序表达了编写一个服务器/客户端TCP通信程序的基本框架，实际使用还需要添加许多代码；
* 这两个程序在ubuntu 20.04下编译运行成功；
* 服务器端程序：[tcp-server.c][src02](**点击文件名下载源程序**)

  > 服务器端程序在绑定地址时绑定的是服务器的地址，端口号是程序侦听的端口，INADDR_ANY这个宏在netinet/in.h中定义，实际上就是一个32位的0，对应的IP地址就是0.0.0.0，和inet_addr("0.0.0.0")是一样的，inet_addr()函数会把一个字符串形式的IP地址转换成一个网络字符顺序的32位的IP地址，这里将IP绑定为0.0.0.0的含义是本机的所有IP地址，一台机器有可能有多个网卡，比如有线网卡和无线网卡，那么这台机器就可能有两个IP地址，加上loopback，就有三个IP地址，假定这三个地址分别是：192.168.2.112(有线网卡)、192.168.2.113(无线网卡)和127.0.0.1(loopback)，如果这里设置成inet_addr("192.168.2.112")，则表示只接收发往目的地址是192.168.2.112这个IP的信息，也就是只接收从有线网卡收到的数据，大家可以试一下；如果绑定的IP地址不是本机的一个合法IP，在执行bind()时会出错；

  > 这个服务器端程序同时只能处理一个客户端的连接，尽管在调用listen(sockfd, 5)时允许连接队列里有5个未处理的连接，但实际处理中并不能同时处理多个连接；

  > 服务器端程序是没有退出出口的，退出服务器程序的唯一办法是ctrl+c；服务器端程序退出聊天有两种方式，一种是服务器端主动向客户端发送"exit"，第二种是当客户端发来"exit"时，回复"exit"，总之，服务器端向客户端发出"exit"后就可以退出；这个程序只有在退出当前聊天后才能处理下一个连接请求。

* 编译、运行和测试服务器端程序
  - 编译
    ```bash
    gcc -Wall tcp-server.c -o tcp-server
    ```
  - 运行服务器端程序
    ```bash
    ./tcp-server
    ```
  - 在另一台机器上的终端上运行下面指令模拟客户端，可以进入聊天模式，这里192.168.2.112为运行了tcp-server程序的服务器IP地址
    ```bash
    nc -n 192.168.2.112 8080
    ```
  - 有关nc命令的使用方法，可以参考另一篇文章[《如何在Linux命令行下发送和接收UDP数据包》][article1]
  - 服务器端程序只能用ctrl+c才能退出；
  - 服务器运行截图

    ![test tcperver with nc][img01]
  
  ------------
  - 模拟客户端运行截图

    ![test tcpserver with nc][img02]

****************
* 客户端程序：[tcp-client.c][src01](**点击文件名下载源程序**)

  > 客户端程序在发起连接时设置服务器地址，端口号是服务器端程序绑定的端口，192.168.2.112是服务器的IP地址，请根据自身的情况进行修改，IP和端口号必须和服务器一致，否则服务器无法收到信息；

  > 必须要首先运行服务器端程序，客户端程序才能运行起来；客户端程序只有收到服务器端发送过来的"exit"才能退出。

* 客户端程序的编译
  ```bash
  gcc -Wall tcp-client.c -o tcp-client
  ```
* 程序运行
  - 一台机器上运行服务端程序：tcpserver
    ```bash
    ./tcp-server
    ```
  - 在另一台机器上运行客户端程序：tcp-client
    ```bash
    ./tcp-client
    ```
  - 进入聊天模式，直至服务器端向客户端发送"exit"
  - 客户端程序的运行截图

    ![screenshot of udpclient][img03]

  -------------------
  - 服务器端程序的运行截图

    ![screenshot of udpserver][img04]

*************
## 5. 后记
* 从本文的例子看，TCP的服务器/客户端的通信框架并不复杂，但是本文的例子只是一个最基本的框架，并不适合在生产环境下运行；
* 本文实例中用于发送和接收数据使用了read()和write()两个函数，实际使用中还可以使用send()/recv()或者sendmsg()/recvmsg()等；
* 对于一个socket还有很多模式可以设置，通常使用fcntl()和setsockopt()进行设置，这些在本文中也没有讨论；
* 对于服务器端程序，一定会遇到多个客户端同时向服务器端发送连接请求的情况，有多种方法处理这种情况，比如：多线程、select、epoll等；


## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180012/tcp-client.c
[src02]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180012/tcp-server.c

[img01]:/images/180012/tcp_server_testing.png
[img02]:/images/180012/tcp_netcat_testing.png
[img03]:/images/180012/screenshot_tcpclient.png
[img04]:/images/180012/screenshot_tcpserver.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/180c46cf82c0610e6ad9774884ccdc38.png
[img02]:https://img-blog.csdnimg.cn/img_convert/22305de32989c318ca3384172bd88729.png
[img03]:https://img-blog.csdnimg.cn/img_convert/0f109838d422eee721d72d3cdb29ff0d.png
[img04]:https://img-blog.csdnimg.cn/img_convert/f3e99f1fc63fcce264d731731cee9c79.png
-->

[article1]:/post/blog/network/0005-send-udp-via-linux-cli/

<!--CSDN
[article1]:https://blog.csdn.net/whowin/article/details/128890866
-->
