---
title: "IPC之四：使用 POSIX 消息队列进行进程间通信的实例"
date: 2023-08-20T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
  - "C Language"
tags:
  - Linux
  - 进程间通信
  - 消息队列
  - "Message queues"
draft: false
#references: 
# - [mq_overview(7) — Linux manual page](https://man7.org/linux/man-pages/man7/mq_overview.7.html)
#     - POSIX 消息队列
# - [Implementation of Message Queues in the Linux Kernel](https://www.baeldung.com/linux/kernel-message-queues)
# - [POSIX message queues in Linux](https://www.softprayog.in/programming/interprocess-communication-using-posix-message-queues-in-linux)
# - [Message Passing With Message Queues](https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/MQueues.html)
# - [POSIX MESSAGE QUEUES](https://man7.org/tlpi/download/TLPI-52-POSIX_Message_Queues.pdf)
#       - PDF文件
#       - https://man7.org/tlpi/download/
#       - https://man7.org/tlpi/code/online/dist/lib/tlpi_hdr.h
#       - https://man7.org/tlpi/code/online/book/lib/get_num.h
# - [POSIX.4 Message Queues](https://users.pja.edu.pl/~jms/qnx/help/watcom/clibref/mq_overview.html)
#       - 参考其中的实例
# - `man sigevent` - 有关异步通知机制
# - [Linux的sigevent结构----mq_notify()实例](https://blog.csdn.net/qq_35976351/article/details/87024570)
#

postid: 100014
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍消息队列(Message Queues)，消息队列可以完成同一台计算机上的进程之间的通信，相比较管道，消息队列要复杂一些，但使用起来更加灵活和方便，Linux 既支持 UNIX SYSTEM V 的消息队列，也支持 POSIX 的消息队列，本文针对 POSIX 消息队列，POSIX 标准引入了一个简单的基于文件的接口，使应用程序可以轻松地与消息队列进行交互；本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文适合 Linux 编程的初学者阅读。

<!--more-->

## 1 POSIX 消息队列概述
* 文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]介绍了消息队列的基本概念及 UNIX System V 消息队列的编程方法，阅读本文前可以先阅读这篇文章；
* `man mq_overview` 可以从在线手册中了解 POSIX 消息队列的更详细的信息；
* Linux 内核 2.6.6 以后支持 POSIX 消息队列，glibc 2.3.4 以后开始 支持 POSIX 消息队列；
    - 查看 Linux 内核版本号：`uname -srm`
    - 查看 glibc 版本号：`ldd --version`
* 以下如无特别说明，消息队列均指 POSIX 消息队列；
* POSIX 消息队列提供了一组用于操作消息队列的 API，这些调用均是以 mq_ 开头：
    1. 打开/创建一个 POSIX 消息队列
        ```C
        mqd_t mq_open(const char *name, int oflag);
        mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
        ```
    2. 关闭一个消息队列
        ```C
        int mq_close(mqd_t mqdes);
        ```
    3. 获取/设置一个消息队列的属性
        ```C
        int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
        int mq_setattr(mqd_t mqdes, const struct mq_attr *newattr, struct mq_attr *oldattr);
        ```
    4. 向消息队列发送一条指定优先级的消息
        ```C
        int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
        ```
    5. 从消息队列中接收一条消息，并获得其优先级
        ```C
        ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
        ```
    6. 删除一个消息队列
        ```C
        int mq_unlink(const char *name);
        ```
    7. 尝试向消息队列中发送一条消息，设定阻塞时间，超时则返回
        ```C
        int mq_timedsend(mqd_t mqdes, 
                         const char *msg_ptr, size_t msg_len, 
                         unsigned int msg_prio, 
                         const struct timespec *abs_timeout);
        ```
    8. 尝试从消息队列中接收一条消息，设定阻塞时间，超时则返回
        ```C
        ssize_t mq_timedreceive(mqd_t mqdes, 
                                char *msg_ptr, size_t msg_len, 
                                unsigned int *msg_prio, 
                                const struct timespec *abs_timeout);
        ```
    9. 当有消息进入消息队列时，产生异步通知
        ```C
        int mq_notify(mqd_t mqdes, const struct sigevent *notification);
        ```

* 使用 POSIX 消息队列 API 的程序在编译时必须加 -lrt 选项，以链接到实时库 librt；
* 笔者认为与 System V 消息队列相比，POSIX 提供的消息队列的接口更加简洁，使用起来更加方便。

## 2 消息队列的名称及虚拟文件系统
* System V 消息队列是使用 key(使用 ftok() 获取)来标识的，是没有名称的，但是，POSIX 消息队列是需要命名的，使用具有唯一性的消息队列名称来标识一个消息队列；
* 消息队列的名称是一个以 "/" 开头的字符串，而且名称中不允许再出现第二个 "/"，这个名称字符串以 NULL 结尾；
* 任何一个知道消息队列名称且具有适当权限的进程都可以向队列中发送或从队列中接收消息，当然也可以执行其它操作；
* POSIX 的消息队列实际上是建在一个虚拟文件系统下的，这个虚拟文件系统被挂载在 /dev/queue 下，在 Ubuntu 下，默认是自动挂载的；
* 可以使用 `ls /dev/mqueue` 命令列出已有的消息队列，也可以用 `rm /dev/mqueue/<mq_name>` 命令删除一个已有的消息队列；
* 使用 `cat /dev/mqueue/<mq_name>` 可以列出指定消息队列的状态；

* 如果在你的系统上没有这个虚拟文件系统，可以用下面的命令建立一个：
    ```bash
    # mkdir /dev/mqueue
    # mount -t mqueue none /dev/mqueue
    ```


## 3 消息队列的创建/打开和关闭
* 在使用一个消息队列之前需要打开(或创建)消息队列，使用 mq_open()
    ```C
    #include <fcntl.h>           /* For O_* constants */
    #include <sys/stat.h>        /* For mode constants */
    #include <mqueue.h>

    mqd_t mq_open(const char *name, int oflag);
    mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
    ```
* `mq_open()` 创建一个新的消息队列，或者打开一个已经存在的消息队列，调用成功则返回消息队列的描述符，调用失败返回 -1，且 errno 中为错误代码；
* **name** - 消息队列的名称，一个以 "/" 开头的字符串，是这个消息队列的唯一标识；
* **oflag** - 控制操作的一些标志，可以有以下选项，这些选项可以以 or 的方式进行组合：
    - **O_RDONLY** - 打开消息队列仅用于接收消息
    - **O_WRONLY** - 打开消息队列仅用于发送消息
    - **O_RDWR** - 打开消息队列用于发送/接收消息
    - **O_CLOEXEC** - 在执行 exec() 时，关闭此消息队列的描述符，以防止父进程将打开的消息队列的描述符泄露给子进程；
    - **O_CREAT** - 如果消息队列不存在，则建立一个新的消息队列，返回其描述符；如果消息队列存在，则返回消息队列的描述符；
    - **O_EXCL** - 如果同时设置了 O_CREAT，则当打开的消息队列已经存在时，返回调用失败，errno=EEXIST
    - **O_NONBLOCK** - 以非阻塞方式打开消息队列
* 如果在 oflag 中设置了 O_CREAT，则必须增加另外两个参数：`mode` 和 `attr`
* 关于 oflag 中的 O_EXCL：
    - 当 `O_CREAT | O_EXCL` 时：消息队列不存在则创建一个新的消息队列(与 O_CREAT 一样)，返回其描述符；如果消息队列存在则返回 -1，`errno=EEXIST(File exists)`
    - 当仅有 `O_EXCL` 时：如果消息队列已经存在，则返回其描述符(与 O_CREAT 一样)，如果消息队列不存在，则返回-1，`errno=ENOENT(No such file or directory)`
* **mode** - 新创建的消息队列的读/写权限，与文件的读/写权限的表达方式一致，使用八进制数，比如：0660、0666，实际的读/写权限还要受到 umask 的影响，最终的结果是 `mode & ~umask`；
    > 可以使用 shell 命令 `umask` 查看当前的 `umask`
* **attr** - 指定该消息队列的最大消息数和一条消息的最大字节数；
    ```C
    struct mq_attr {
        long mq_flags;       /* Flags (ignored for mq_open()) */
        long mq_maxmsg;      /* Max. # of messages on queue */
        long mq_msgsize;     /* Max. message size (bytes) */
        long mq_curmsgs;     /* # of messages currently in queue
                                (ignored for mq_open()) */
    };
    ```
    - 在调用 `mq_open()` 时，这个结构中只有 mq_maxmsg(最大消息数)和 mq_msgsize(消息的最大长度)可以设置；
    - 如果 attr 为 NULL，则使用默认属性来创建消息队列，这些默认值可以在 proc 文件系统中找到
    - 使用命令 `cat /proc/sys/fs/mqueue/msg_max` 查看默认的最大消息数；
    - 使用命令 `cat /proc/sys/fs/mqueue/msgsize_max` 查看默认的一条消息的最大长度。

* 消息队列使用完毕后，需要关闭该消息队列
    ```C
    #include <mqueue.h>

    int mq_close(mqd_t mqdes);
    ```
* mqdes 就是使用 mq_open() 打开消息队列时返回的描述符。   
* **源程序**：[mq-create.c][src02](**点击文件名下载源程序**)是一个简单的命令行工具，用于创建一个新的消息队列；
* 编译：`gcc -Wall mq-create.c -o mq-create -lrt`
* 运行：`./mq-create -cx -m 8 -s 512 /test_mq_name 0660`
* 在使用该程序时需要在命令行输入一些参数：
    - **-c** 表示使用 O_CREATE 标志，**-x** 表示使用 O_EXCL 标志；
    - **-m** 表示设置消息队列的最大消息数(mq_maxmsg)，在 -m 选项后必须跟一个数值，表示队列允许的最大消息数；
    - **-s** 表示设置每条消息的最大长度(mq_msgsize)，-m 选项后必须跟一个数值，表示每条消息的最大长度；
    - 这几个选项中，**-c** 和 **-x** 至少要有一个，也可以两个都有，写成 **-cx** 或者 **-c -x** 都可以；
    - **-m** 和 **-s** 可有可无，如果没有，默认的最大消息数为 10，每条消息的默认最大长度为 256 字节；
    - 在这几个选项后面是消息队列的名称，注意，消息队列的名称必须以 `"/"` 开头，而且后面的字符串中不能再有 `"/"`；
    - 在名称后面可以跟一个消息队列的读写权限，也可以没有，如果要设置消息队列的权限，这里要使用八进制，比如 `0666`、`0644` 等；
    - 如果没有设置权限，默认权限为 `0660`；
* 下面是这个程序的运行截屏：

    ![screenshot of mq-create command][img02]

* 请注意，尽管我们设置了消息队列的读/写权限是 `0666`，但最后显示的消息队列的权限却是 `0664`，这是因为 umask 为 `0002`。

----
## 4 获取/设置消息队列的属性
* 在介绍 mq_open() 时，已经接触到了消息队列的属性结构，如下：
    ```C
    struct mq_attr {
        long mq_flags;       /* Flags: 0 or O_NONBLOCK */
        long mq_maxmsg;      /* Max. # of messages on queue */
        long mq_msgsize;     /* Max. message size (bytes) */
        long mq_curmsgs;     /* # of messages currently in queue */
    };
    ```
* 其中的 `mq_maxmsg` 和 `mq_msgsize` 前面已经介绍过其含义，另外的两项：
    - **mq_flags**：如果在调用 mq_open() 时设置了 O_NONBLOCK，则在获取属性时，该字段为 O_NONBLOCK，否则该字段为 0；
    - **mq_currmsgs**：在这个消息队列中目前有多少条消息；
* 可以使用 `mq_getattr()` 来获取一个消息队列的属性；
* 可以使用 `mq_setattr()` 来设置消息队列的属性，但实际作用有限；
    ```C
    #include <mqueue.h>

    int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
    int mq_setattr(mqd_t mqdes, const struct mq_attr *newattr, struct mq_attr *oldattr);
    ```
* 这两个函数在调用成功时返回 0，调用失败时返回 -1，errno 中为错误代码；
* 调用 `mq_setattr()` 时，将使用参数 newattr 中的值设置消息队列的属性，但实际上只能设置 mq_flags 字段，而且该字段只能设置为 O_NONBLOCK 或者 0，其它字段将被忽略；
* 在调用 `mq_setattr()` 时，如果参数 oldattr 不为 NULL，则在调用成功后，oldattr 中将返回消息队列的属性，但是返回的是调用 `mq_setattr()` 之前消息队列的属性；
* 所以，如果需要了解调用 `mq_setattr()` 之后消息队列的属性，在调用 `mq_setattr()` 之后，需要调用 `mq_getattr()`。
* **源程序**：[mq-attr.c][src01](**点击文件名下载源程序**)演示了 `mq_open()、mq_getattr()、mq_setattr() 和 mq_close()` 的使用方法；
* 编译：`gcc -Wall mq-attr.c -o mq-attr -lrt`
* 运行：`./mq-attr`
    - 该程序首先建立一个新的消息队列，消息队列的名称与当前进程 pid 相关，所以每次运行都会建立一个新的消息队列；
    - 建立消息队列时，以无阻塞方式(O_NONBLOCK)打开消息队列；
    - 使用 mq_getattr() 获取消息队列的属性，可以看到 mq_flags 字段是 O_NONBLOCK
    - 使用 mq_setattr() 设置消息队列属性时，mq_flags 字段会从 O_NONBLOCK 改为 0，但从 oldattr 返回的属性中，mq_flags 字段仍然为 O_NONBLOCK；
    - 使用 mq_getattr() 再次获取消息队列的属性，可以看到 mq_flags 字段已经变成了 0；
    - 最后使用 mq_close() 关闭该消息队列；
* 前面介绍过消息队列的虚拟文件系统，消息队列会被建立在虚拟文件系统上，在运行完 `./mq-attr` 后，我们可以使用 ls 命令看一下虚拟文件系统的目录；
    ```bash
    ls -l /dev/mqueue/ 
    ```
* 然后我们还可以尝试使用 rm 命令删除虚拟文件系统下的消息队列；
* 下面是运行结果的截屏：

    ![Screenshot of mq-attr][img01]

------
## 5 向消息队列中发送消息
* 使用 mq_send() 可以向一个已经打开的、有写权限的消息队列中发送消息；
    ```C
    #include <mqueue.h>
    int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
    ```
* **mqdes**：打开的消息队列的描述符；
* **msg_ptr**：指向要发送消息的指针；
* **msg_len**：发送消息的长度，这个长度必须小于消息队列属性中的 mq_msgsize 字段，可以是 0，发送一个空消息；
* **msg_prio**：发送消息的优先级，0 为最低优先级；
* 消息队列属性中的 mq_maxmsg 字段表示这个消息队列允许的最大消息数，当消息队列已经满时(消息数已经达到 mq_maxmsg 字段的值)：
    - 默认情况下，`mq_send()` 将会阻塞，直到消息队列中有空闲位置；
    - 当以非阻塞方式(设置 O_NONBLOCK)打开消息队列时，`mq_send()` 会立即返回一个错误，`errno=EAGAIN`
* 调用 `mq_send()` 成功时返回 0，出错时返回 -1，errno 中存放错误代码；

## 6 从消息队列中接收消息
* 使用 mq_receive() 可以从一个已经打开的、有读权限的消息队列中接收消息；
    ```C
    #include <mqueue.h>
    ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
    ```
* **mqdes**：打开的消息队列的描述符；
* **msg_ptr**：指向接收缓冲区的指针；
* **msg_len**：接收缓冲区的长度，msg_len 必须大于或等于消息队列属性中的 mq_msgmax 字段，否则接收出错，`errno=EMSGSIZE`；
* **msg_prio**：当 msg_prio 不为 NULL 时，仅接收优先级为 msg_prio 的消息，当 msg_prio 为 NULL 时，接收按照优先级排列的所有消息；
* 接收消息成功后，在消息队列中的原始消息将被删除；
* 0 为最低优先级，该调用首先接收最高优先级的消息，同等优先级下接收消息按照先进先出的原则；
* 调用成功，返回收到的字节数，调用失败，返回 -1，errno 中为错误代码；
* 如果消息队列为空：
    - 如果打开消息队列时设置了 O_NONBLOCK，则立即返回错误，`errno=EAGAIN`
    - 如果打开消息队列时没有设置 O_NONBLOCK，则产生阻塞，直至队列中有可接收的消息；

## 7 删除消息队列
* 使用 mq_unlink() 可以删除一个指定的消息队列名称，消息队列名称将被立即删除，但是消息队列并不一定立即销毁，而是要等到所有已经打开了消息队列的进程都关闭消息队列后才被销毁；
    ```C
    #include <mqueue.h>
    int mq_unlink(const char *name);    
    ```
* name 为消息队列名称，记着其起始字符必须是 `"/"`；
* 源程序：[mq-unlink.c][src03](**点击文件名下载源程序**)是一个简单的命令行工具，用于删除一个消息队列的名称；
* 编译：`gcc -Wall mq-unlink.c -o mq-unlink -lrt`
* 运行：`./mq-unlink mq-name`
* 下面是运行截图：

    ![screenshot of mq-unlink][img03]

-----
## 8 消息队列的一个完整实例
* 这个实例分为服务器端和客户端两部分，演示了服务器端使用消息队列接收多个客户端进程发送的消息，并根据客户端发送的消息建立与客户端之间的独立的消息队列通道，并向客户端发送消息；
    - 服务器端首先创建一个消息队列 `/posix-mq-server`，用于接收客户端程序发来的消息，然后等待接收来自客户端的消息；
    - 客户端程序首先创建 5 个子进程，每个子进程都运行相同的程序；
    - 客户端子进程创建一个与自身进程号相关的消息队列 `posix-mq-client-(PID)`，用于接收服务器端发送过来的信息；
    - 客户端子进程首先打开消息队列 `/posix-mq-server`，然后将客户端消息队列名称 `posix-mq-client-(PID)` 发送给服务器；
    - 服务器端收到客户端子进程发送过来的消息队列名称后，打开该消息队列，并将一个序列号发送给客户端；
    - 客户端子进程收到服务器端发送的序列号并显示出来；
    - 每个客户端子进程循环向服务端发送 5 条消息，并接收 5 次回应，每次发送消息间隔一个不大于 6 秒的随机数；
    - 客户端父进程等待 5 个子进程全部运行完毕后退出；
    - 服务器端程序只能用 ctrl+c 退出，所以程序中截获了 ctrl+c 信号，并在处理程序中对已经打开的消息队列进行了处理。
* **服务器端源程序**：[mq-server.c][src04](**点击文件名下载源程序**)
* **客户端源程序**：[mq-client.c][src05](**点击文件名下载源程序**)
* **包含文件**：[mq-const.h][src07](**点击文件名下载源程序**)
* 编译：
    ```bash
    gcc -Wall mq-server.c -o mq-server -lrt
    gcc -Wall mq-client.c -o mq-client -lrt
    ```
* 这个实例的运行需要两个终端窗口，一个窗口中运行 `./mq-server`，另一个窗口中运行 `./mq-client`；
* 下面是运行录屏，左边时服务端窗口，右边是客户端窗口：

    ![Recording screen for running example][img04]

-----
## 9 发送/接收消息时设置阻塞时间
* 当打开消息队列时没有设置 O_NONBLOCK 标志时，POSIX 消息队列还提供了两个可以设置阻塞时间的发送和接收调用；
* 当消息队列已满时，如果没有设置 O_NONBLOCK，则调用 mq_send() 发送消息时会产生阻塞，此时可以考虑使用 mq_timedsend() 发送消息；
* 如果消息队列为空，而且没有设置 O_NONBLOCK，当使用 mq_receive() 从接收消息时会产生阻塞，此时可以考虑使用 mq_timedreceive() 接收消息；
    ```C
    #include <time.h>
    #include <mqueue.h>

    int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio,
                     const struct timespec *abs_timeout);

    ssize_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio,
                            const struct timespec *abs_timeout);
    ```
* 与 mq_send() 和 mq_receive() 相比，这两个函数仅仅是多了参数 `const struct timespec *abs_timeout`，这个是用来指定阻塞时间的；
    ```C
    struct timespec {
        time_t tv_sec;        /* seconds */
        long   tv_nsec;       /* nanoseconds */
    };
    ```
* 特别要注意的时，abs_timeout 并不是一个时长，而是一个绝对时间，其值表示从 `1970-01-01 00:00:00+0000 (UTC)` 开始，到函数返回的秒(tv_sec)和纳秒(tv_nsec)的数值；
* 在具体实践中，通常是使用 clock_gettime() 函数获取当前时间到 `struct timespec` 结构中，然后在 `struct timespec` 结构中加上希望阻塞的时长，再去调用 mq_timedsend() 或者 mq_timedreceive()；
* 下面这段代码片段演示了 mq_timedreceive() 的基本用法：
    ```C
    struct   timespec tm;

    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_sec += 1;
    if (0 > mq_timedreceive(fd, buf, 4096, NULL,  &tm)) {
        ...
    }
    ```

## 10 消息队列的异步通知机制
* Linux 的异步通知机制，不仅仅可以用于消息队列，也可以用于各类事件的通知，在线手册 `man sigevent` 可以了解更详细的信息；
* 本文仅介绍用于消息队列的异步通知机制；
* 当消息队列中有新消息时，异步通知机制可以按照设置启动一个线程，并在这个线程中执行指定的函数，并按照设置传递参数到这个被执行的函数；
* 这些设置包含在 `struct sigevent` 中：
    ```C
    struct sigevent {
        int          sigev_notify;  /* Notification method */
        int          sigev_signo;   /* Notification signal */
        union sigval sigev_value;   /* Data passed with
                                        notification */
        void       (*sigev_notify_function) (union sigval);
                        /* Function used for thread
                            notification (SIGEV_THREAD) */
        void        *sigev_notify_attributes;
                        /* Attributes for notification thread
                            (SIGEV_THREAD) */
        pid_t        sigev_notify_thread_id;
                        /* ID of thread to signal (SIGEV_THREAD_ID) */
    };
    ```
* 看上去还是挺复杂的一个结构，但其实解决完 `sigev_notify` 字段后，看上去就不复杂了；
* `sigev_notify` 字段设置如何执行通知，该字段可以有以下四个值之一：
    - **SIGEV_NONE**：不作任何通知；
    - **SIGEV_SIGNAL**：向进程发出一个信号，这个信号由 `sigev_signo` 字段指定，进程中的信号处理程序负责处理这个通知；
    - **SIGEV_THREAD**：建立一个新线程，并运行由 `sigev_notify_function` 指定的函数，`sigev_value` 字段将作为参数传递给这个函数；
    - **SIGEV_THREAD_ID**：目前仅用于 POSIX 定时器，需要设置 `sigev_notify_thread_id` 字段；
* 这里面只有当 `sigev_notify` 为 **SIGEV_THREAD** 时与消息队列有关，此时，只有 `sigev_notify_function`、`sigev_value` 和 `sigev_notify_attributes` 三个字段与本文有关；
* 在具体实践中，通常并不需要设置线程属性，所以 `sigev_notify_attributes` 也不需要设置；
* 当我们需要设置一个消息队列的异步通知时，仅需要填写 `struct sigevent` 中的三个字段：
    - `sigev_notify = SIGEV_THREAD`
    - `sigev_notify_function` 字段填处理新消息的函数指针；
    - `sigev_value` 字段设置为传递给处理函数的参数，通常为已经打开的消息队列的描述符；
* 看看如何把消息队列的描述符传递给处理函数，因为传递参数的字段是一个 `union sigval`，所以要先看 `union sigval` 的定义：
    ```C
    union sigval {          /* Data passed with notification */
        int     sival_int;      /* Integer value */
        void   *sival_ptr;      /* Pointer value */
    };
    ```
* 因为消息队列的描述符就是个 int，所以，我们既可以使用 `sival_int` 来传递描述符，也可以把描述符的指针赋予 `sigval_ptr` 传递过去；
* 对 `struct sigevent` 设置完成后，调用 `mq_notify()` 就完成了消息队列异步通知的设置；
    ```C
    #include <mqueue.h>
    int mq_notify(mqd_t mqdes, const struct sigevent *sevp);
    ```
* `mq_notify()` 的调用仅有两个参数，一个是消息队列的描述符，另一个是前面介绍的 `struct sigevent`，所以没有更多可以解释的；
* `mq_notify()` 调用成功将返回 0，错误时返回 -1，errno 中为错误码；
* 关于消息队列的异步通知机制，还有以下几点要注意：
    - 当调用 `mq_notify()` 成功后，就会注册一个消息通知，当要取消这个消息通知时，只需要将 `sevp = NULL`，并再次调用 `mq_notify()` 即可；
    - 一个消息队列只能注册一个异步通知，当注册多个时，只有注册的第一个有效；
    - 只有消息队列为空，收到第一条消息时，才会收到通知，消息队列不空时，即便有消息进来也不会触发通知；
* 消息队列的异步处理机制使我们的主程序无需阻塞在消息队列上，而是专心处理其它事务，可以大大提高程序的运行效率；
* **源程序**：[mq-notify.c][src06](**点击文件名下载源程序**)演示了消息队列的异步通知的基本编程方法；
    - 该程序首先建立一个消息队列，然后设置消息队列异步通知；
    - 等待 1 秒钟后向消息队列中发送一条消息；
    - 触发异步通知，通知处理函数成功收到消息；
* 编译：`gcc -Wall mq-notify.c -o mq-notify -lrt`
* 运行：`./mq-notify`
* 运行截图：

    ![Screenshot of mq-notify][img05]

--------
## 11 System V 消息队列 VS POSIX 消息队列
* 文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]介绍了 UNIX System V 消息队列的编程方法；
* POSIX 消息队列的功能与 System V 消息队列类似，很显然调用方法有较大的不同；
* POSIX 消息队列和 System V 消息队列一样，都是在内核中实现的，所以，与 System V 消息队列类似对资源的占用有一定的限制：
    - msg_max：一个消息队列中消息数量的最大值；
    - msgsize_max：消息队列中每个消息的最大长度；
    - queues_max：系统中消息队列数量的最大值；
* 与 System V 消息队列一样，这些最大值可以在 proc 文件系统中找到
    ```bash
    $ cat /proc/sys/fs/mqueue/msg_max
    $ cat /proc/sys/fs/mqueue/msgsize_max
    $ cat /proc/sys/fs/mqueue/queues_max
    ```
* 与 System V 消息队列相比，POSIX 消息队列还是有一些优点的：
    - POSIX 消息队列只有在使用它的所有进程均关闭该消息队列后才能被完全销毁；
        > POSIX 消息队列的 mq_unlink() 虽然立即删除了消息队列的名称，但是并没有立即销毁这个消息队列，所有已经打开该消息队列的进程仍然可以继续使用，但是不会再有新进程打开这个消息队列，当所有进程均关闭了该消息队列后，消息队列自行销毁；

        > System V 消息队列则不同，任何有权限的进程均可以随意删除一个消息队列，可能会导致一个进程正在读/写消息队列时，该消息队列被其它进程删除，依赖该消息队列传递消息的进程将发生不可预知的后果。
    - POSIX 消息队列有一个异步通知机制，一个空消息队列中有消息进入时可以启动一个线程处理该消息；
        > System V 消息队列没有这个机制，所以，一个需要从消息队列中读取消息的进程必须定时去队列中尝试读取消息，或者索性让程序阻塞在读取消息的函数(msgrcv())上，显然，POSIX 消息队列的异步通知机制更优越一些。
    - POSIX 消息队列的消息引入了优先级，获取到的第一条消息总是最高优先级的消息；
        > System V 消息队列中没有消息优先级的概念，但是可以使用不同的消息类别模拟出类似优先级的效果，在同一个消息类别下，后进入队列的消息不可能比先进入队列的消息先被读取到。
    - 在打开 POSIX 消息队列时，应用程序可以通过传递参数指定队列的属性(例如：每条消息大小或队列容量等)；
* System V 的消息队列使用 ID 来标识，消息队列本身没有名称；POSIX 消息队列需要为队列起一个名称，这在使用上会方便一些；
* 总体来看，个人认为 POSIX 消息队列更易于使用；

## 12 消息队列 VS 管道
* 文章[《IPC之一：使用匿名管道进行父子进程间通信的例子》][article01]介绍了匿名管道的编程方法；
* 文章[《IPC之二：使用命名管道(FIFO)进行进程间通信的例子》][article02]介绍了命名管道的编程方法；
* 尽管和消息队列比起来，管道显得简单而且功能较弱，但管道也是有其自身特点的；
* 进程在使用管道进行数据交换时，数据在管道中的流动是一种非结构化的字节流；而消息队列中的数据是一种结构化的消息；
	> 当在消息队列中检索到一条消息时，我们只能完整地接收这条消息，不可能只接收一部分，而把另一部分留在消息队列中；但是管道则不同，当一个进程向管道中写入 100 个字节的数据时，我们可以仅从管道中读取 40 个字节，而把其余的 60 个字节留在管道中；

    > 再举一个例子，假定有一个函数需要被执行 100 次，每次需要向服务端发送 1 字节的执行结果，如果使用消息队列，那么服务端将收到 100 条消息，需要接收 100 次才能获得所有的运行结果，但是如果使用管道，可以等执行完后一次性地从管道中接收 100 个字节的消息，一次读取获得所有的运行结果，所以，在某些场景下，管道也会有优势；

* 消息队列使用特殊标识符(描述符)，而不是完全的文件描述符，因此，消息队列需要一组专门的函数来发送和接收数据，而不能使用标准的文件操作函数；管道使用标准的文件描述符，所以可以使用标准的文件操作函数来进行读写；

* 消息队列具有关联的元数据，允许进程指定接收消息的顺序；也就是说，消息队列不保证先入先出；但是管道是完全的先入先出；
	> System V 消息队列的元数据中有消息类型，接收程序可以指定接收消息类型，从而过滤掉其它类型的已经在消息队列中的数据；但仍然可以保证在同样消息类型的条件下，做到先入先出；POSIX 消息队列的元数据中有消息优先级，接收程序收到的数据总是优先级高的消息在前面，仅可以保证相同优先级的消息是先入先出的。

* 消息队列具有内核级持久性，需要使用专门的函数或实用程序来删除，终止进程不会删除消息队列；匿名管道在创建它的进程终止时将自动被删除，删除一个命名管道与删除一个普通文件完全一样，不需要专门的函数。




-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[article01]: https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
[article02]: https://whowin.gitee.io/post/blog/linux/0011-ipc-examples-of-fifo/
[article03]: https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/

<!-- for CSDN
[article01]: https://blog.csdn.net/whowin/article/details/132171311
[article02]: https://blog.csdn.net/whowin/article/details/132171930
[article03]: https://blog.csdn.net/whowin/article/details/132172172
-->

[src01]: https://whowin.gitee.io/sourcecodes/100014/mq-attr.c
[src02]: https://whowin.gitee.io/sourcecodes/100014/mq-create.c
[src03]: https://whowin.gitee.io/sourcecodes/100014/mq-unlink.c
[src04]: https://whowin.gitee.io/sourcecodes/100014/mq-server.c
[src05]: https://whowin.gitee.io/sourcecodes/100014/mq-client.c
[src06]: https://whowin.gitee.io/sourcecodes/100014/mq-notify.c
[src07]: https://whowin.gitee.io/sourcecodes/100014/mq-const.h

[img01]: https://whowin.gitee.io/images/100014/screenshot-of-mqattr.png
[img02]: https://whowin.gitee.io/images/100014//screenshot-of-mq-create.png
[img03]: https://whowin.gitee.io/images/100014/screenshot-of-mq-unlink.png
[img04]: https://whowin.gitee.io/images/100014/mq-server-client.gif
[img05]: https://whowin.gitee.io/images/100014/screenshot-of-mq-notify.png

