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

TCP是一种面向连接的通信方式，一个TCP服务器难免会遇到同时处理多个用户的连接请求的问题，本文用一个简化的实例说明如何在一个TCP服务器程序中，使用select处理同时出现的多个客户连接，文章给出了程序源代码，阅读本文应该具备了基本的socket编程知识，熟悉基本的服务器/客户端模型架构；本文对初学者难度不大。
<!--more-->

## 1. 基本思路
* TCP服务器端程序，对于每一个连接请求，可以使用多线程的方式为每一个连接启动一个线程处理该连接的通信，但使用多线程的方式，通常认为有如下缺点：
  1. 多线程编程和调试相对都比较难，而且有时会出现无法预知的问题；
  2. 上下文切换的开销较大；
  3. 对于巨大量的连接，可扩展性不足；
  4. 可能发生死锁。
* 使用select处理多连接的基本思路
  1. 建立一个用于侦听的socket，叫做master_socket；
    ```C
    int master_socket = socket(AF_INET , SOCK_STREAM , 0);
    ```
  2. 建立一个sockets数组，用于存储已经与master_socket建立连接的socket，叫做client_socket，初始化时全部清0，数组的长度即为程序允许的最大连接数；
    ```C
    #define MAX_CLIENTS     30

    int client_socket[MAX_CLIENTS];
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }
    ```
  3. 绑定服务器地址并在master_socket上启动侦听；
    ```C
    #define PORT          8888
    struct sockaddr_in address;

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);
    bind(master_socket, (struct sockaddr *)&address, sizeof(address));
    ```
  4. 在master_socket上侦听
    ```C
    listen(master_socket, 3);
    ```
  5. 将master_socket、client_socket中不为0的项加入到readfds中，启动select；
    ```C
    fd_set readfds;
    int max_fd, client_count;

    FD_ZERO(&readfds);
    FD_SET(master_socket, &readfds);
    max_fd = master_socket;

    client_count = 0;
    for (i = 0 ; i < MAX_CLIENTS; i++) {
        if (client_socket[i] > 0) {
            FD_SET(client_socket[i], &readfds);
            client_count++;
        }
        if (client_socket[i] > max_fd) max_fd = client_socket[i];
    }
    activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
    ```
  6. 有活动socket返回时，如果是master_socket则调用accept接受连接，生成新的socket并加入到client_socket中，发送欢迎信息后回到步骤4；
    ```C
    int new_socket;
    char *message = "ECHO Daemon v1.0 \n\n";

    addrlen = sizeof(address);
    if (FD_ISSET(master_socket, &readfds)) {
        new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        send(new_socket, message, strlen(message), 0);
        if (client_count < MAX_CLIENTS) {
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    client_count++;
                    break;
                }
            }
        } else {
            close(new_socket);
        }
    }
    ```
  7. 如果不是master_socket，为client_socket中的一员，则调用read从socket中读出数据，处理并做出回应，回到步骤4；
    ```C
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_socket[i] == 0) continue;

        if (FD_ISSET(client_socket[i], &readfds)) {
            if ((nread = read(client_socket[i], buffer, 1024)) > 0) {
                buffer[nread] = '\0';
                send(client_socket[i], buffer, strlen(buffer), 0 );
            }
        }
    }

    ```
  8. 如果从client_socket读出数据长度为0，表示socket已经关闭，关闭socket，并从client_socket中清除该socket，回到步骤4；
    ```C
    if (nread == 0) {
        close(client_socket[i]);
        client_socket[i] = 0;
    }
    ```
  9. 如果从client_socket读出数据长度小于0，如果errno=EINTR，则直接返回步骤4；
  10. 如果从client_socket读出数据长度小于0，如果errno不是EINTR，则关闭socket，并从client_socket中清除该socket，回到步骤4；
    ```C
    if (errno != EINTR) {
        close(client_socket[i]);
        client_socket[i] = 0;
    }
    ```

## 2. 主要函数、宏和数据结构
* 请参考[《使用select实现的UDP/TCP组合服务器》][article1]、[《使用C语言实现服务器/客户端的UDP通信》][article2]和[《使用C语言实现服务器/客户端的TCP通信》][article3]

## 3. 实例
* 该实例最多可以同时处理30个连接，理论上可以更多，与机器的资源有关；
* 该实例收到客户端的信息后没有做处理，将收到的信息发回给了客户端；
* 服务器源程序：[select-server.c][src01](**点击文件名下载源程序**)
* 编译
  ```bash
  gcc -Wall select-server.c -o select-server
  ```
* 运行
  ```bash
  ./select-server
  ```
* 测试，使用nc模拟客户端，有关nc命令的使用方法，可以参考另一篇文章[《如何在Linux命令行下发送和接收UDP数据包》][article4]
  - 服务器ip：192.168.2.114
  - 在另一台机器(或者多台机器)开三个终端窗口，分别输入下面命令：
    ```bash
    nc -n 192.168.2.114 8888

    hello from client 1
    ```
    ```bash
    nc -n 192.168.2.114 8888

    hello from client 2
    ```
    ```bash
    nc -n 192.168.2.114 8888

    hello from client 3
    ```
  - 分别在第2、第3、第1终端窗口中按下ctrl+c退出nc命令
  - 服务器端运行截屏

    ![Screenshot of server][img01]

  ------------
  - 客户端第一个窗口运行截屏

    ![Screenshot of client][img02]

## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://whowin.gitee.io/sourcecodes/180011/select-server.c

[img01]:https://whowin.gitee.io/images/180011/screenshot_of_server.png
[img02]:https://whowin.gitee.io/images/180011/screenshot_of_client_1.png

<!--gitee
[article1]:https://whowin.gitee.io/post/blog/network/0010-tcp-and-udp-server-using-select/
[article2]:https://whowin.gitee.io/post/blog/network/0013-udp-server-client-implementation-in-c/
[article3]:https://whowin.gitee.io/post/blog/network/0012-tcp-server-client-implementation-in-c/
[article4]:https://whowin.gitee.io/post/blog/network/0005-send-udp-via-linux-cli/
-->
[article1]:https://blog.csdn.net/whowin/article/details/129410476
[article2]:https://blog.csdn.net/whowin/article/details/129728570
[article3]:https://blog.csdn.net/whowin/article/details/129688443
[article4]:https://blog.csdn.net/whowin/article/details/128890866
