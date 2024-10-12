---
title: "使用poll()代替select()处理多客户连接的TCP服务器实例"
date: 2024-02-27T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Network"
  - "Linux"
  - "C Language"
tags:
  - Linux
  - 网络编程
  - select
  - poll

draft: false
#references: 
# - [LINUX – IO MULTIPLEXING – SELECT VS POLL VS EPOLL](https://devarea.com/linux-io-multiplexing-select-vs-poll-vs-epoll/)
# - [Using poll() instead of select()](https://www.ibm.com/docs/en/i/7.1?topic=designs-using-poll-instead-select)
# - [poll and select](https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch05s03.html)
# - [How to handle the Linux socket revents POLLERR, POLLHUP and POLLNVAL?](https://stackoverflow.com/questions/24791625/how-to-handle-the-linux-socket-revents-pollerr-pollhup-and-pollnval)
# - [poll vs select vs event-based](https://daniel.haxx.se/docs/poll-vs-select.html)
# - [The problem with select() vs. poll()](https://beesbuzz.biz/code/5739-The-problem-with-select-vs-poll)

postid: 180021
---


在网络编程中，使用 select() 处理多客户端的连接是非常常用的方法，select() 是一个非常古老的方法，在大量连接下会显得效率不高，而且其对描述符的数值还有一些限制，Linux内核从 2.1.13 版以后提供了 poll() 替代 select()，本文介绍 poll() 在网络编程中的使用方法，并着重介绍 poll() 在编程行与 select() 的区别，旨在帮助熟悉 select() 编程的程序员可以很容易地使用 poll() 编程，本文提供了一个具体的实例，并附有完整的源代码，本文实例在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0。
<!--more-->


## 1 前言
* 在[『网络编程专栏』][net_category]中，有两篇文章都涉及到了使用 `select()` 处理多个 `socket` 连接：
    - [《使用select实现的UDP/TCP组合服务器》][article01]
    - [《TCP服务器如何使用select处理多客户连接》][article02]
* Linux 上的另一个系统调用 `poll()`，可以和 `select()` 完成相同的工作，而且通常认为 `poll()` 的效率要高于 `select()`；
* 似乎 `select()` 要比 `poll()` 更普及一些，这可能是因为在 Linux 内核 2.0 版以前是不支持 `poll()` 的，只有 `select()`，直到 `2.1.13` 版后才开始既支持 `select()` 也支持 `poll()`；
* 另一个导致 `select()` 更加普及的原因可能是大多数的应用程序需要同时处理的 `I/O` 数量并不是很多，使得对性能的要求不高，其实在大量的 `I/O` 处理上，`poll()` 和 `select()` 在性能上的差别还是挺大的；
* 本文假定读者已经对 socket 编程和 `select()` 函数有基本的理解，有关这方面的知识请自行参考前面提到的两篇文章，本文不再讨论；
* `select()` 和 `poll()` 并不适用于普通文件(指文件系统上的文件)，一个普通文件将永远处于可读或者可写的状态，不管是使用 `select()` 还是 `poll()`，都会一直返回；
* 通常情况下，`select()` 和 `poll()` 用于 `socket`、管道等，尽管我们在 `D-Bus` 的文章中也使用了 `select()`，但实际上 `D-Bus` 使用的是 `socket` 或者管道，`D-Bus` 只是将其抽象化了；
* `select()` 和 `poll()` 实际上完成的功能非常近似，使用上的差异也不大；
* 本文讨论的重点是使用 `poll()` 替代以前使用 `select()` 的程序，会介绍 `poll()` 的使用方法，以及 `poll()` 和 `select()` 在编程上的区别，并最终实现一个使用 `poll()` 的 TCP 服务器；
* 尽管普遍认为 `epoll()` 在处理多连接方面表现更加优异，但 `epoll()` 的编程方式与 `select()` 和 `poll()` 有较大区别，所以本文不会讨论 `epoll()` 相关的编程方法；

## 2 poll() 的基本使用方法
* poll() 函数的输入参数中有一个结构数组，结构中包含有一个描述符字段，poll() 函数等待在这些描述符上，当结构数组中的任何一个或多个描述符上有事件发生时，该函数将返回；
* poll() 函数的定义如下：
    ```C
    #include <poll.h>

    int poll(struct pollfd *fds, nfds_t nfds, int timeout);
    ```
* `struct pollfd` 的定义

    ```C
    struct pollfd
    {
        int fd;                 /* File descriptor to poll.  */
        short int events;       /* Types of events poller cares about.  */
        short int revents;      /* Types of events that actually occurred.  */
    };

    ```
* 在调用 `poll()` 之前，要先初始化 `fds`；
    - `fds` 中的 fd 是要监视的文件描述符，可以是文件、`socket`、管道、设备等，比较常用的是在一个 TCP 服务器上监视侦听的 `socket` 以及与多个客户端之间的连接 `socket`；
    - `fds` 中的 events 是要在这个描述符上监视哪些事件，可监视的事件主要有：
        + `POLLIN` - 当 socket 上有数据可读时，触发该事件；
        + `POLLOUT` - 当向 socket 上写入数据不会产生阻塞时，触发该事件；
        + `POLLPRI` - 当 socket 上有紧急的数据需要读取时，触发该事件；
        + `POLLERR` - 当 socket 上产生一个异步错误时，触发该事件；
        + `POLLHUP` - 当 socket 连接已经断开时，触发该事件，该事件仅在输出时有效；
        + `POLLNVAL` - 无效的请求，通常表示描述符没有打开，触发该事件；
        + 可以使用 "或" 操作在一个描述符上同时监视多个事件，最常用的监视事件为 `PULLIN` 和 `POLLOUT`，同时监视这两个事件时可以表达为 `POLLIN | POLLOUT`；
    - `revents` 是实际发生的事件，当 `poll()` 返回时会填充该字段，程序通过这个字段可以判断在这个描述符上发生了什么事件； 
* `poll()` 函数的第二个参数 `nfds` 是 fds 数组中有效的条目的数量，比如结构数组的 fds 可以容纳的条目最大为 100 个，但只有前 10 个是我们准备监视的描述符，则 `nfds` 应该为 10；
* `poll()` 函数的第三个参数 `timeout` 是超时时间，调用 `poll()` 会进入阻塞，等待被监视的描述符产生事件，如果设置了 `timeout` 参数，则阻塞 `timeout` 时间后，即便没有产生事件，`poll()` 函数也会返回；
* `timeout` 的时间单位为毫秒；
    - 将 `timeout` 设为 -1 表示永久等待，直至有事件产生；
    - 将 `timeout` 设为 0，`poll()` 将立即返回，不管是否有事件产生；
* `poll()` 的返回值有三种情况：
    - 当有事件产生时，`poll()` 返回一个大于 0 的正整数，表示有多少个文件描述符上有事件产生，程序需要遍历 fds 数组，查看结构中的 revent 字段来判断那个文件描述符上产生了哪些事件；
    - 当没有事件产生，`poll()` 仅仅是因为超时返回时，`poll()` 返回 0；
    - 当出现错误时，`poll()` 返回 -1，此时，errno 中存放有错误代码，详情可以查看在线手册 `man 2 poll`；

* `poll()` 检测到 `socket` 有数据可读时，如果读出的数据长度为 0 时，认为该 `socket` 连接已经断开；
* poll() 的返回值有四种可能：
    - **0**：表示调用超时；
    - **-1**：表示调用失败，errno 中为错误代码；
    - **1**：表示在被监听的描述符中，只有一个描述符上有事件产生，等待处理；
    - **1+**：表示在被监听的描述符中，有多个描述符上有等待处理的事件，返回值为等待处理的描述符的数量；

## 3 poll() 和 select() 在编程上的区别
* 通常情况下，poll() 的代码会比 select() 简单一些；
* 先看一下 select() 编程的代码：
    ```C
    fd_set fds;
    FD_ZERO(fd_set);
    FD_SET(500, &fds);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (select(501, &fds, NULL, NULL, &tv)) {
        if (FD_ISSET(500, &fds)) {
            ... 
        }
    }
    ```
* 再看一下完成同样工作的 poll() 编程的代码：
    ```C
    struct pollfd pfd[10];
    pfd[0].fd = 500;
    pfd[0].events = POLLIN;
    if (poll(&pfd, 1, 5000)) {
        if (pfd.revents & POLLIN) {
            ... 
        }
    }
    ```
* 看上去显然使用 `poll()` 编程要简单一些；
* `select()` 在标识文件描述符时使用的位掩码，`bit 0` 表示描述符为 0，`bit 1` 表示描述符为 1，以此类推，当表示某个描述符的 bit 为 1 时，表示这个描述符需要被监视，如果我们仅需要监视数值为 500 的文件描述符，此时，`bit 0~499` 为 0，`bit 500` 为 1；
* 所以，在使用 `select()` 时，即便我们只需要监视描述符为 500 的 socket，实际上，仍要检查描述符 `0~499` 的位掩码，当所要监视的描述符值比较大时，运行效率肯定会受到影响；
* 在使用 `select()` 时，使用三个描述符集来监视读、写和意外事件，当一个描述符既需要监视读又需要监视写时，是需要在两个描述符集中设置相应的描述符的；
* 另外，使用 `select()` 监视描述符时，对描述符的最大值是有限制的，在 Linux 下允许的描述符的最大值为 1024，这一点显然也是 `select()` 的麻烦之处，当要监视的描述符的值大于 1024 时，将无法使用 `select()`；
* 使用 `poll()` 则不需要设置描述符集，而是需要为每一个需要监视的描述符建立一个结构：`struct pollfd`，这个结构中的 fd 字段指定要监视的描述符，events 字段可以设置多个要监视的事件，如果对一个描述符既要监视其"读"事件，也要监视其"写"事件，只需在这个 events 字段上设置两个事件即可，像下面这段代码
    ```C
    ......
    struct pollfd *fds;
    ...
    fds[i].fd = fd;
    fds[i].events = POLLIN | POLLOUT;
    ......
    ```
* 由此可见，使用 `poll()` 监视描述符不会有最大值不能超过 1024 的限制，在监视效率上也会比 `select()` 要高一些；
* 在 `select()` 编程中，通常每次调用 `select()` 之前需要重新设置描述符集，像下面代码，这段代码中 client_socket 数组中存放着连接到服务器的客户端的 socket，listen_socket 为服务器上正在侦听的 socket：
    ```C
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listening_socket, &readfds);
        max_fd = listening_socket;

        for (i = 0 ; i < MAX_CLIENTS; i++) {
            if (client_socket[i] > 0) {
                FD_SET(client_socket[i], &readfds);
            }
            if (client_socket[i] > max_fd) max_fd = client_socket[i];
        }
        rc = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        ......
    }
    ```
* 但其实 `poll()` 编程中也有类似的困扰，`poll()` 使用一个结构数组 `struct pollfd *` 来标识需要监视的描述符及其事件，但当一个客户连接中止时，尽管可以将结构数组中的 fd 字段置为 0，但 `poll()` 并不会自动地跳过 fd 字段为 0 的数组项，所以我们在启动 `poll()` 之前仍然有必要重新整理整个结构数组，以保证其中没有已经不再使用的描述符，像下面代码段，fds_size 为结构数组 fds 中的有效元素数，从中删除所有的 fd 字段为 0 的元素：
    ```C
    ......
    struct pollfd *fds;
    ...
    while ((fds_size > 0) && (fds[fds_size - 1].fd == 0)) fds_size--;
    if (fds_size > 1) {
        int i = fds_size - 1;
        while (i > 0) {
            if (fds[i - 1].fd == 0) {
                fds[i - 1].fd = fds[fds_size - 1].fd;
                fds[i - 1].events = fds[fds_size - 1].events;
                fds[i - 1].revents = 0;
                fds[fds_size - 1].fd = 0;
                while ((fds_size > 0) && (fds[fds_size - 1].fd == 0)) fds_size--;
                if (fds_size < 2) break;
            }
            i--;
        }
    }
    ......
    ```
* 在 `poll()` 编程中，当有事件产生时，需要遍历整个结构数组，检查每个数组结构中的 revents 字段，找到需要处理的描述符及其事件，这一点和 `select()` 编程类似，`select()` 返回后，需要检查所有被监控的描述符，以找到哪个描述符需要处理；


## 4 poll() 编程的基本步骤
1. 使用 `socket()` 建立需要侦听的 socket；
2. 使用 `setsockopt()` 设置 socket 为可重复使用；
3. 使用 `ioctl()` 设置 socket 为非阻塞；
4. 使用 `bind()` 绑定服务器的地址和端口；
5. 使用 `listen()` 开始侦听端口；
* 以上步骤和使用 `select()` 编程时一致的；
7. 初始化结构数组 `struct pollfd *`，将服务器侦听 socket 加入到数组中；
8. 启动 `poll()`；
    - 返回 0 表示调用超时，可以重新启动 `poll()`；
    - 返回 `<0` 表示 `poll()` 出错，errno 中为错误代码；
    - 返回 `>0` 表示有需要处理的 socket，进行处理；

    > 要处理的 socket 通常又分为两种，一种是正在侦听的 socket，如果有 `POLLIN` 事件表示有客户端发出了连接请求，使用 `accept()` 接受连接将产生一个新的 socket，这个新的 socket 要加入到结构数组 `struct pollfd *` 中，以便在 `poll()` 中可以被监视；另一类 socket 就是已经和服务器建立连接的一个或多个客户端的 socket，这类 socket 有 POLLIN 事件产生可能是有数据发送回来，也可能是因为连接中断，在调用 `recv()` 从 socket 中接收数据时，如果返回值 `>0` 表示确实有数据发送回来，要做出相应处理，如果返回值为 0 则表示这个连接已经中断，此时应该及时将该 socket 从结构数组 `struct pollfd *` 中清除，避免在调用 `poll()` 时给 `poll()` 增加额外负担；

9. 整理结构数组 `struct pollfd *`，然后再次启动 `poll()`；
    > 在处理 poll() 函数的返回结果时，当有新的客户端连接请求被接受时，会有新的 socket 需要添加到结构数组 `struct pollfd *` 中，当有客户端 socket 连接中断时，需要将这个已经失效的 socket 从结构数组 `struct pollfd *` 中删除，所以在处理完 poll() 的返回结果后需要对结构数组 `struct pollfd *` 进行整理，将新的 socket 添加进来，将失效的 socket 删除；不能简单地把结构数组 `struct pollfd *` 中的 fd 字段或者 events 字段置 0 来表示一个失效的 socket，poll() 并不会自动跳过这样标识的结构数组项；

## 5 实例：一个使用 poll() 的 TCP 服务器
* **源程序**：[poll-server.c][src01](**点击文件名下载源程序，建议使用UTF-8字符集**)演示了使用 poll() 完成的一个 TCP 服务器；
* 编译：`gcc -Wall -g poll-server.c -o poll-server`
* 运行：`./poll-server`
* 该程序是一个多进程程序，程序会建立一个服务端进程和若干个(默认为 3 个，由宏 MAX_CONNECTIONS 控制)客户端进程；
* 服务端进程侦听在端口 8888 上，等待客户端进程的连接；
* 启动 `poll()` 监视 socket；
* 服务端在接受客户端请求后，将新连接的 socket 加入到结构数组中，并向客户端发送一条欢迎信息；
* 客户端在连接建立以后向服务端发送一条信息，服务端在收到客户端信息后会将该信息原封不动地发送回客户端；
* 客户端判断收到的信息与自己发出的信息一样后，主动关闭连接，然后退出进程；
* 服务端发现连接中断后，将从结构数组中删除该失效 socket，然后继续启动 `poll()` 监视 socket；
* 服务进程中拦截了 SIGINT 信号，这个信号可以使用 `ctrl + c` 产生，服务进程在收到这个信号后将退出进程；
* 主进程监视客户端进程的退出，当所有客户端进程都已退出后，向服务端进程发送 SIGINT 信号，使服务端进程退出，整个程序运行结束。
* 运行截图：

    ![Screenshot of poll-server][img01]

## 6 结论
* `poll()` 和 `select()` 编程有很多相似的地方，除了 `select()` 使用描述符集而 `poll()` 使用结构数组外，其它方面非常相似，对熟悉 `select()` 编程的程序员而言，使用 `poll()` 替换 `select()` 编写服务端程序并不困难；
* 在调用 `select()` 之前，需要初始化描述符集，在调用 `poll()` 之前，需要初始化结构数组；
* 在 `select()` 返回后，需要使用 `FD_ISSET()` 遍历描述符集以判断哪个描述符有事件产生，在 `poll()` 返回后，需要遍历结构数组中的 revents 字段，以判断哪个描述符有事件产生；
* 在接受连接请求、接收、发送数据，判断错误以及连接是否中断方面，使用 `select()` 和 `poll()` 是一样的；
* 在处理完 select() 的返回后，需要重新初始化描述符集才可以再次调用 `select()`，在处理完 `poll()` 的返回后，需要重新整理结构数组后才可以再次调用 `poll()`。







## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[net_category]: https://blog.csdn.net/whowin/category_12180345.html

[article01]: /post/blog/network/0010-tcp-and-udp-server-using-select/
[article02]: /post/blog/network/0011-handling-multiple-clients-on-server-with-select/

<!--CSDN
[article01]: https://blog.csdn.net/whowin/article/details/129410476
[article02]: https://blog.csdn.net/whowin/article/details/129685842
-->

[src01]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180021/poll-server.c

[img01]: /images/180021/screenshot-of-poll-server.png

<!-- CSDN
[img01]: https://img-blog.csdnimg.cn/img_convert/77672b962e220c5185dc3a49f0123a4a.png
-->