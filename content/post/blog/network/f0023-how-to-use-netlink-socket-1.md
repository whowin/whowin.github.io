---
title: "如何使用netlink socket(上)"
date: 2023-07-08T16:43:29+08:00
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
  - 网络编程
  - netlink
draft: true
#references: 
# - [Why and How to Use Netlink Socket](https://github.com/mwarning/netlink-examples/blob/master/articles/Why_and_How_to_Use_Netlink_Socket.md)
#     - 本文原型
# - [Netlink Examples](https://github.com/mwarning/netlink-examples)

postid: 180023
---



<!--more-->

## 1 前言
* Linux 的内核开发和维护是一件相当复杂的事情，通常只有那些基础的代码和对系统性能起关键作用的代码才会被放在内核中，其它的代码，比如 GUI、管理程序和普通的控制程序都会作为应用程序放在用户控件中；
* 把同一个功能的程序分别放在内核空间和用户空间中，这种情况在 *Linux* 中非常常见；那么内核代码和用户空间代码是如何相互通信的呢？
* 内核和用户空间使用 IPC(Inter-Process Communication) 进行通信，通常认为有 4 中 IPC 方法用于内核进程和用户进程之间通信：
    1. 系统调用
    2. ioctl
    3. proc 文件系统
    4. netlink

* 本文将主要讨论 ```netlink socket```，并介绍了它作为一种 IPC 方法的优势。

## 2 netlink介绍
* 用户进程之间或应用程序与远程服务器进行 TCP/IP 通信时，常会用到 socket，为了与 `netlink socket` 区别，我们把这种 socket 称为 `TCP/IP socket`；使用 `netlink socket` 与使用 `TCP/IP socket` 非常相似，所以，有过 `TCP/IP socket` 编程经验的程序员很容易掌握；
* `netlink socket` 是一种特殊的IPC，用于在内核进程和用户进程之间传输信息；
* 用户进程可以使用标准 `socket API` 实现与内核模块中的特定 API 之间的**全双工**通信；
* 在建立 `TCP/IP socket` 时使用的地址族为 `AF_INET`，与之类似，在建立 `netlink socket` 使用的地址族为 `AF_NETLINK`；
* `netlink socket` 有很多不同的类型，通常每种类型对应着不同的内核模块中 API，在建立 `netlink socket` 时用不同的协议类型来表示，这些协议类型定义在内核头文件 `linux/netlink.h` 中。
* 以下是常用的 `netlink socket` 的协议类型：
    - **NETLINK_ROUTING**: 用户空间路由节点之间的通信通道，<!--如：`BGP(Border Gateway Protocol)、OSPF(Open Shortest Path First Interior Gateway Protocol)、RIP(Routing Information Protocol)` 和内核报文转发模块，-->用户空间的*路由守护进程*通过这种协议类型更新内核路由表；
    - **NETLINK_FIREWALL**：接收 IPv4 防火墙代码发送的报文；
    - **NETLINK_NFLOG**：用户空间 iptable 管理工具和内核空间 Netfilter 模块的通信通道；
    - **NETLINK ARPD**：用于从用户空间管理 arp 表；
* 这些功能没有使用系统调用、ioctls 或者 proc 文件系统，而使用 netlink 的原因如下：
    - 为一个新功能添加系统调用、ioctl 或者 proc 文件是一件非常复杂的工作，有污染内核和破坏系统稳定性的风险；
    - `netlink socket` 相对就简单的多，只需要在 `netlink.h` 中添加一个常量作为协议类型，然后内核模块和应用程序之间就可以使用 `socket` 风格的 API 进行通信了；




* `netlink` 是异步的；与其它 `socket` 一样，`netlink` 使用消息队列来避免突发的大量消息造成的拥堵；
    - 发送 `netlink` 消息的系统调用将消息放到接收方的 `netlink` 队列中，然后调用接收方的接收处理程序；
    - 接收处理程序根据上下文决定是立即处理消息还是把消息留在队列中稍后再做处理；
* 系统调用是同步处理的，这点与 netlink 不同，因此如果使用系统调用将消息传到内核，如果处理这个消息的时间比较长，则内核调度可能会受到影响，从而影响系统性能；
* 系统调用的代码是在编译时静态地链接到内核中的，因此是无法在系统启动时动态加载的；使用 netlink socket，netlink 应用程序可以做成可加载的内核模块，其与内核中的 netlink 核心程序，不存在编译时的依赖关系；所以相比系统调用，netlink 显得更加灵活；
* netlink socket 支持多播，这个特性是系统调用、ioctls和 proc 文件系统所没有的；一个进程可以多播一条消息到一个 netlink 组地址上，其他多个进程可以监听这个组地址，从而收到多播消息；对于从内核向用户空间进行事件分发而言，netlink 提供了一个近乎完美的机制。
* 系统调用和 ioctl 的会话只能由用户空间应用程序发起，所以可以认为这些 IPC 是简单的 IPC，当内核模块对用户空间应用程序有紧急消息要发送时，这些 IPC 是做不到的；为了做到这一点，应用程序需要不断地轮询内核，这样代价很高；netlink 允许内核发起会话，非常好地解决了这个问题，netlink 的这一特性被成为 **双工特性**；
* netlink socket 提供了一套 BSD socket 风格的 API，对开发者而言易于理解，与系统调用 api 和 ioctl 相比，其学习成本更低。

## 2 与 BSD Routing Socket 相关
* 在 BSD TCP/IP 栈中，有一个特殊的 socket 称为 routing socket，它的地址族是 AF_ROUTE，协议族是 PF_ROUTE，套接字类型是 SOCK_RAW，BSD中的 routing socket 用于进程在内核路由表中添加或删除路由；

* 在Linux中，routing socket 的等效功能由 netlink socket 协议类型 NETLINK_ROUTE 提供，netlink socket 提供了 BSD routing socket 的功能超集。

## 3 netlink socket APIs
* 用户空间应用程序可以使用标准的 ```socket api``` - ```socket()```、```sendmsg()```、```recvmsg()``` 和 ```close()``` 来访问 ```netlink socket```；
* 查阅 man 手册，可以查询这些 api 的详细定义(比如：```man sendmsg```)；本文仅讨论在 netlink socket 下，如何为这些 api 选择参数；使用 ```TCP/IP socket``` 编写过程序的程序员应该很熟悉这些 api；

* **socket()**
    - 使用 socket() 建立一个 socket：
        ```C
        int socket(int domain, int type, int protocol);
        ```
    - domain：AF_NETLINK(地址族)；
    - type：SOCK_RAW 或 SOCK_DGRAM，NETLINK 是面向数据报的服务；
    - protocol：协议类型，选择 socket 使用 netlink 的那个功能；常用的 netlink 协议类型有：NETLINK_ROUTE、NETLINK_FIREWALL、NETLINK_ARPD、NETLINK_ROUTE6 和 NETLINK_IP6_FW；如果需要，还可以添加自定义的 netlink 协议类型。
    - 每种netlink协议类型最多可以定义32个多播组，每个多播组用一个位掩码来表示，```1 << i```，其中 ```0 <= i <= 31```，当一组用户进程与内核进程协同实现同一个功能时，多播功能非常有用

* **bind()**
    ```C
    struct sockaddr_nl nladdr;

    bind(fd, (struct sockaddr *)&nladdr, sizeof(nladdr));
    ```
    - 与 ```TCP/IP socket``` 类似，```netlink bind()``` 将本地(源) socket 地址( *nladdr* )与打开的 socket( *fd* ) 关联起来；
    - netlink 地址结构如下所示，该结构定义在头文件 `linux/netlink.h` 中；
        ```
        struct sockaddr_nl
        {
            sa_family_t    nl_family;  /* AF_NETLINK   */
            unsigned short nl_pad;     /* zero         */
            __u32          nl_pid;     /* process pid */
            __u32          nl_groups;  /* mcast groups mask */
        } nladdr;
        ```
    - 
    - `struct sockaddr_nl` 作为 `bind()` 一起使用时，`sockaddr nl的nl pid字段可以用调用进程自己的pid填充。nl pid在这里用作这个netlink套接字的本地地址。应用程序负责选择一个唯一的32位整数来填充nl pid









## 4 从内核发送 netlink 消息



## 5 内核与应用程序之间的单播通信


## 6 内核与应用程序之间的多播通信




## 7 结论








