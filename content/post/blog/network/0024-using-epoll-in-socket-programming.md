---
title: "使用epoll()进行socket编程处理多客户连接的TCP服务器实例"
date: 2024-03-10T16:43:29+08:00
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
  - socket
  - epoll

draft: false
#references: 
# - [LINUX – IO MULTIPLEXING – SELECT VS POLL VS EPOLL](https://devarea.com/linux-io-multiplexing-select-vs-poll-vs-epoll/)
# - [epoll() Tutorial – epoll() In 3 Easy Steps!](https://suchprogramming.com/epoll-in-3-easy-steps/)
# - [Using epoll() For Asynchronous Network Programming](https://kovyrin.net/2006/04/13/epoll-asynchronous-network-programming/)
# - [c-example](https://github.com/millken/c-example/blob/master/epoll-example.c)
# - [Epoll is fundamentally broken](https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/)
# - [The method to epoll’s madness](https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642)

postid: 180024
---


在网络编程中，当需要使用单线程处理多客户端的连接时，常使用select()或者poll()来处理，但是当并发数量非常大时，select()和poll()的性能并不好，epoll()的性能大大好于select()和poll()，在编写大并发的服务器软件时，epoll()应该是首选的方案，本文介绍epoll()在网络编程中的使用方法，本文提供了一个具体的实例，并附有完整的源代码，本文实例在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0。
<!--more-->


## 1 基本概念
* 在[『网络编程专栏』][net_category]中，有两篇文章都涉及到了使用 `select()` 处理多个 `socket` 连接：
    - [《使用select实现的UDP/TCP组合服务器》][article01]
    - [《TCP服务器如何使用select处理多客户连接》][article02]
* 在[『网络编程专栏』][net_category]中，有一篇文章都涉及到了使用 `poll()` 处理多个 `socket` 连接：
    - [《使用poll()代替select()处理多客户连接的TCP服务器实例》][article03]
* `poll()` 和 `select()` 的编程方法非常相似，但 epoll 有较大区别；
* epoll 完成与 `poll()` 相似的工作：监视多个文件描述符看它们是否可以进行 I/O 操作；
* epoll 的核心概念就是 epoll 实例，从用户空间的角度看，一个 epll 实例就是内核中的一个数据结构，可以被看成是下列两个列表的容器；
    - **Interest List**：(有时也被称为 epoll set)进程向 epoll 登记的需要被监视的文件描述符集；
    - **Ready List**：可以无阻塞地进行 I/O 操作的文件描述符集，Ready List 是 Interest List 的一个子集，内核实时地把可以进行 I/O 操作的文件描述符从 Interest List 填充到 Ready List；
* 使用 epoll 的过程就是将要监视的文件描述符向 epoll 登记进入 Interest List，然后从 Ready List 中处理那些可以进行 I/O 操作的文件描述符；
* 使用 epoll 有三个基本的函数，后面会详细介绍这三个函数的使用方法：
    - `epoll_create1()` - 用于建立一个 epoll 实例；
    - `epoll_ctl()` - 用于向 epoll 实例的 Interest List 中添加要监视的文件描述符，或者修改/删除 Interest List 中的文件描述符；
    - `epoll_wait()` - 用于监视已经登记的文件描述符集，当有一个或多个被监视的文件描述符可以进行 I/O 操作时返回；
* 在调用 `epoll_wait()` 后，有两种触发方式可以使 `epoll_wait()` 返回，边沿触发(Edge-Triggered)和电平触发(Level-Triggered)，这两个词是从电子电路中引申过来的，熟悉电子电路的或者做嵌入式编程的读者应该对此有些了解；
* 可以用一个例子来说明这两种触发方式的不同，假设在下列条件下，看看边沿触发和电平触发有什么不同：
    1. 将一个管道(pipe)读出端的文件描述符 rfd 登记到 epoll 实例上进行监视；
    2. 在管道的写入端写入 2kb 的数据；
    3. 调用 `epoll_wait()` 会返回文件描述符 rfd，表示在 rfd 上有数据可以读取；
    4. 从 rfd 中读取 1kb 的数据；
    5. 再次调用 `epoll_wait()`；
* 当使用电平触发(Level-Triggered)方式时，只要 rfd 中仍然还有数据没有读出，`epoll_wait()` 就会被触发返回，由于写入了 2kb 数据但只读出了 1kb，所以在第 5 步时，`epoll_wait()` 会返回 rfd 有数据可读；
* 当使用边沿触发(Edge-Triggered)方式时，只有当 rfd 从没有数据可读变为有数据可读时才会触发 `epoll_wait()` 返回，虽然读缓冲区中仍有 1kb 的数据没有被读出，但在第 5 步时 `epoll_wait()` 是不会返回的；
* 当使用电平触发方式时，epoll 实际上只是一个运行的比较快的 `poll()`，可以在任何使用 `poll()` 的地方使用电平触发方式的 epoll，epoll 真正的意义在于其边缘触发方式；
* 由于边沿触发方式的特点，`epoll_wait()` 被触发后必须将读缓冲区的数据全部读出，否则可能会有数据丢失，所以当使用边沿触发方式时，通常需要将文件描述符设置成非阻塞方式，然后循环读取，直至出现 EAGAIN 错误代码为止，如下
    ```C
    ......
    int done = 0;               // not done
    int nbytes = 0;             // how many bytes to read
    do {
        nbytes = recv(fd, buffer, sizeof(buffer), 0);
        if (nbytes > 0) {
            buffer[nbytes] = '\0';
            ...
            continue;
        } else if (rc == 0) {
            // the socket discinnected
            break;
        } else if (errno == EINTR) {
            // if errno==EINTR, it means socket is not closed, just because some network errors happened
            continue;
        } else if (errno == EAGAIN) {
            done = 1;
            break;
        } else {
            perror("recv() failed");
            break;
        }
    } while (1);
    ......
    ```
* 本文后面的实例中将演示边沿触发方式的具体编程方法；


## 2 epoll 的基本使用方法
* 前面提到过了使用 epoll 的三个基本函数，本节将着重介绍这些函数的使用方法及相关的数据结构；
* **epoll_create1()** - 创建一个 epoll 实例
    ```C
    #include <sys/epoll.h>

    int epoll_create(int size);
    int epoll_create1(int flags);
    ```
    - 这两个函数都是创建一个 epoll 实例，在 epoll_create() 中的参数 size 表示在这个 epoll 实例上所管理的最大描述符的数量，但从 Linux 2.6.8 以后，这个参数已经无效，但参数 size 必须是一个大于 0 的整数；
    - 实际上通常都是使用 epoll_create1() 来建立一个 epoll 实例，参数 flags 通常设为 0；
    - 调用成功，函数返回 epoll 实例的文件描述符，调用失败时返回 -1，errno 中是错误代码；

* **epoll_ctl()** - epoll 文件描述符的控制接口
    ```C
    #include <sys/epoll.h>

    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    ```
    - 这个函数用于向 epoll 实例的 Interest List 中添加、修改和删除文件描述符，具体操作取决于参数 op；
    - **epfd** 为 epoll_create1() 返回的 epoll 实例的文件描述符
    - 当 op 为 **EPOLL_CTL_ADD** 时，表示要添加一个文件描述符 fd 进入 epoll 实例的 Interest List 中；
    - 当 op 为 **EPOLL_CTL_MOD** 时，表示要修改一个已经在 Interest List 中的文件描述符 fd；
    - 当 op 为 **EPOLL_CTL_DEL** 时，表示要将一个已经在 Interest List 中的文件描述符 fd 从 Intersst List 中删除；
    - 参数 **fd** 为想要操作的文件描述符；
    - 参数 **event** 在添加和修改时是有意义的，在删除时可以设置为 NULL；
    - **struct epoll_event** 的定义如下：
        ```C
        typedef union epoll_data {
            void        *ptr;
            int          fd;
            uint32_t     u32;
            uint64_t     u64;
        } epoll_data_t;

        struct epoll_event {
            uint32_t     events;      /* Epoll events */
            epoll_data_t data;        /* User data variable */
        };
        ```
    - struct epoll_event 中的 events 是一个位掩码，由以下零个或多个可用事件类型组合而成(这里仅列出常用的几个)：
        + **EPOLLIN**：相应的文件描述符上有数据可读；
        + **EPOLLOUT**：相应的文件描述符上可以进行写操作；
        + **EPOLLET**：使用边沿触发方式；
    - 下面代码将一个文件描述符 fd 加入到 epoll 实例 epfd 的 Interest List 中，使用边沿触发方式，当可以进行读操作时触发 epoll_wait() 返回：
        ```C
        ......
        int epfd = epoll_create1(0);
        ...
        struct epoll_event event;

        memset(&event, 0 , sizeof(struct epoll_event));
        // Set up the structure epoll_event
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLET;
        // Add a new descriptor to the interest list
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
            perror("EPOLL_CTL_ADD failed");
        }
        ......

        ```
    - 该函数调用成功时返回 0，失败时返回 -1，errno 中为错误代码；

* **epoll_wait()** - 等待 epoll 文件描述符上的 I/O 事件
    ```C
    #include <sys/epoll.h>

    int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    ```
    - 调用 `epoll_wait()` 后，当 epoll 实例中被监视的文件描述符有事件产生或者超时时间 timeout 到，该函数将返回；
    - 参数 **epfd** 为使用 `epoll_create1()` 返回的 epoll 实例的文件描述符；
    - 参数 **events** 中将返回所有有事件产生的 fd，`events->data.fd` 为产生事件的文件描述符，`events->events` 为实际产生的事件(位掩码)；
    - 参数 **maxevent** 为返回事件的最大值，必须大于 0，`epoll_wait()` 在参数 events 中返回的事件不会大于 maxevents；
    - 参数 **timeout** 为超时时间，单位为毫秒，`epoll_wait()` 等待 timeout 时长后不论是否有事件产生都会返回，将 timeout 设为 -1，`epoll_wait()` 将一直等待直至有事件产生，将 timeout 设为 0，`epoll_sait()` 将立即返回，不论是否有事件产生；
    - `epoll_wait()` 调用成功时，返回一个正整数，表示在参数 events 中有多少个事件；
    - `epoll_wait()` 因超时返回时，将返回 0；
    - `epoll_wait()` 调用失败将返回 -1，errno 中为错误代码；
    - `epoll_wait()` 可以被信号打断，此时，错误代码为 EINTR，通常情况下如果 errno 为 EINTR 时可以重新调用 `epoll_wait()`；

## 3 epoll 进行 socket 编程的基本步骤
* 尽管 epoll 监视的事件是文件描述符的事件，但通常不会用在普通文件(指文件系统下的文件)，一个普通文件将永远处于可读或者可写的状态，epoll 更多地是用在 socket 编程上；
* epoll 进行 socket 编程的基本步骤：
    1. 使用 `socket()` 建立需要侦听的 socket；
    2. 使用 `setsockopt()` 设置 socket 为可重复使用；
    3. 使用 `ioctl()` 设置 socket 为非阻塞；
    4. 使用 `bind()` 绑定服务器的地址和端口；
    5. 使用 `listen()` 开始侦听端口；
    * 以上步骤和使用 `select()/poll()` 编程时是一致的；
    6. 使用 `epoll_create1()` 构建一个 epoll 实例 epfd； 
    7. 构建一个结构 `struct epoll_event ev`，将服务器侦听 socket 加入到加入到结构中，并设置 EPOLLIN 事件及边沿触发方式(EPOLLET)；
    8. 使用 `epoll_ctl()` 的 `EPOLL_CTL_ADD` 方法将侦听 socket 加入到 epoll 实例 epfd 的 Interest List 中；
    9. 启动 `epoll_wait()`；
        - 返回 0 表示调用超时，可以重新启动 `epoll_wait()`；
        - 返回 `<0` 表示 `epoll_wait()` 出错，errno 中为错误代码；
        - 返回 `>0` 表示有需要处理的 socket，进行处理；

        > 要处理的 socket 通常又分为两种，一种是正在侦听的 socket，如果有 `EPOLLIN` 事件表示有客户端发出了连接请求，使用 `accept()` 接受连接将产生一个新的 socket，这个新的 socket 要按照步骤 7、8 的方法加入到 epoll 实例的 Interest List 中，以便在 epoll 中可以被监视，因为我们使用的边沿触发方式，所以还要记得使用 `ioctl()` 将这个新的 socket 设置成非阻塞；
        
        > 另一类 socket 就是已经和服务器建立连接的一个或多个客户端的 socket，这类 socket 有 EPOLLIN 事件产生可能是有数据发送回来，也可能是因为连接中断，在调用 `recv()` 从 socket 中接收数据时，如果返回值 `>0` 表示确实有数据发送回来，要做出相应处理，如果返回值为 0 则表示这个连接已经中断，此时只需将该 socket 关闭即可，理论上说，当一个 socket 被关闭后，epoll 会自动地将该 socket 从 Interest List 中删除，所以通常我们不需要显式地使用 epoll_ctl() 的 EPOLL_CTL_DEL 方法从 epoll 实例的 Interest List 中删除这个 socket；

    10. 回到步骤 9，再次启动 `epoll_wait()`；

## 4 实例：一个使用 epoll() 的 TCP 服务器
* **源程序**：[epoll-server.c][src01](**点击文件名下载源程序，建议使用UTF-8字符集**)演示了使用 epoll 完成的一个 TCP 服务器；
* 编译：`gcc -Wall -g epoll-server.c -o epoll-server`
* 运行：`./epoll-server`
* 该程序是一个多进程程序，程序会建立一个服务端进程和若干个(默认为 3 个，由宏 MAX_CONNECTIONS 控制)客户端进程；
* 服务端进程侦听在端口 8888 上，等待客户端进程的连接；
* 启动 `epoll_wait()` 监视 socket；
* 服务端在接受客户端请求后，将新连接的 socket 加入到 epoll 实例中，并向客户端发送一条欢迎信息；
* 客户端在连接建立以后向服务端发送一条信息，服务端在收到客户端信息后会将该信息原封不动地发送回客户端；
* 客户端判断收到的信息与自己发出的信息一样后，主动关闭连接，然后退出进程；
* 服务端发现连接中断后，关闭该 socket，并使用 epoll_ctl() 的 EPOLL_CTL_DEL 方法从 epoll 中删除该失效 socket(这一步可以没有)，然后继续启动 `epoll_wait()` 监视 socket；
* 服务进程中拦截了 SIGINT 信号，这个信号可以使用 `ctrl + c` 产生，服务进程在收到这个信号后将退出进程；
* 主进程监视客户端进程的退出，当所有客户端进程都已退出后，向服务端进程发送 SIGINT 信号，使服务端进程退出，整个程序运行结束；
* 该程序的客户端进程的程序与文章[《使用poll()代替select()处理多客户连接的TCP服务器实例》][article03]中的客户端程序完全一样；
* 运行截图：

    ![Screenshot of epoll-server][img01]







## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[net_category]: https://blog.csdn.net/whowin/category_12180345.html
[article01]: https://blog.csdn.net/whowin/article/details/129410476
[article02]: https://blog.csdn.net/whowin/article/details/129685842
[article03]: https://blog.csdn.net/whowin/article/details/136454503


[src01]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180024/epoll-server.c

[img01]: https://whowin.gitee.io/images/180024/screenshot-of-epoll-server.png

<!-- CSDN
[img01]: https://img-blog.csdnimg.cn/img_convert/1d624912b6c5c7c6f4c61925006293e7.png
-->