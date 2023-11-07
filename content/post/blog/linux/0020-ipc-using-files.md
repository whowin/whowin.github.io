---
title: "IPC之十：使用共享文件进行进程间通信的实例"
date: 2023-11-06T16:43:29+08:00
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
  - 进程间通信
  - files
  - IPC
draft: false
#references: 
# - [Inter-process communication: files](https://dev.to/leandronsp/inter-process-communication-files-1m34)
# - [The Uses of the Exec Command in Shell Script](https://www.baeldung.com/linux/exec-command-in-shell-script)
#       - 里面对使用 exec 命令打开和关闭文件描述符有介绍
# - [Inter-process communication in Linux: Shared storage](https://opensource.com/article/19/4/interprocess-communication-linux-storage)
# - `man fcntl` : 里面包含了文件锁的使用方法
# - [Inter-process communication](https://en.wikipedia.org/wiki/Inter-process_communication)
#       - 里面列出了所有 ipc 方法

postid: 100020
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，常用的 IPC 方式有管道、消息队列、共享内存等，但其实使用广大程序员都熟悉的文件也是可以完成 IPC 的，本文介绍如何使用共享文件实现进程间通信，本文给出了具体的实例，并附有完整的源代码；本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文的实例中涉及多进程编程、文件锁等概念，所以对 Linux 编程的初学者有一些难度，但对于了解 Linux 下共享文件，特别是文件锁的应用，将是非常难得的。

<!--more-->

## 1 使用共享文件实现IPC的基本概念
* 文件操作是一个程序员的必备技能，相比较 IPC 的各种方法(比如：管道、消息队列、共享内存等)，程序员显然更熟悉文件的操作；
* 那么，能不能使用文件实现进程间通信呢？答案时肯定的，多个进程共享一个文件同样可以完成进程间通信；
* 首先描述一个场景：
    - `Server/Client` 模式，一个服务端进程，三个客户端进程；
    - 进程间通信时，以每个进程的 PID 作为通信地址的唯一标识
    - 客户端只与服务端进程进行通信，客户端进程之间不进行通信；
* 使用共享文件实现 IPC，其实就是发送方将消息写入文件，接收方再从相同的文件中读出，看起来十分简单，但在多进程环境中，并不像看起来的那么简单；
* 使用共享文件进行 IPC 时，有两个比较麻烦的地方，一个是文件指针，另一个是文件锁机制；
* 先说文件指针问题：
    - 当一个文件被打开时，其文件指针的偏移为 0，当读出 10 个字节时，其文件指针偏移将增加 10；
    - 写入文件时，会从当前文件指针处写入文件，当写入 10 个字节后，其文件指针偏移将增加 10；
    - 一般读出需要从文件头顺序读取，但是写入需要向文件的尾部写入，所以如果一个进程中对同一个共享文件既有读操作又有写操作时，文件指针将比较混乱；
    - 这种混乱还表现在可能还有其它进程对共享文件进行写操作，导致你期望的文件指针与实际有所不同；
    - 为了避免这种文件指针的混乱，通常在一个进程中对同一个共享文件仅做读操作或者仅做写操作；
    - 对于我们上面描述的 IPC 场景，服务端需要接收客户端的消息并做出回应，通常我们要使用两个共享文件，一个文件服务端仅做读操作，客户端仅做写操作，用于客户端向服务端传递消息，另一个文件服务端仅做写操作，客户端仅做读操作，用于服务端向客户端传递消息；
* 再说文件锁机制：
    - 当多个进程同时对一个文件进行写操作时，很明显是会有冲突的，假定进程 1 要写入 100 个字节，进程 2 要写入 50 个字节，可能进程 1 写入完 30 个字节时，产生了进程调度，使进程 2 开始向文件写入数据，从而导致写入数据的混乱；
    - 当一个进程对文件进行写入操作时，如果有另一个进程正在读数据，也是有冲突的，假定写进程要写入 100 个字节，写入 30 个字节时，产生进程调度，读进程开始读文件，读出了刚刚写入的 30 个字节，而这 30 个字节是要写入的 100 个字节中的一部分，是不完整的数据；
    - 所以，当一个进程对一个共享文件进行写操作时，需要独占该文件，也就是同时不能有其它进程对该文件进行读写操作；
    - 当一个进程对一个文件进行读操作时，当然不能允许有其它进程进行写操作，但可以允许其它进程进行读操作；
    - 这种对文件的占有机制又叫做文件锁机制，我们在下一节会做专门的介绍；

* 使用共享文件进行 IPC 并不是一种常用的方式，在编程实践中很少这样去做，其实际运行时是有真实的文件 I/O 发生的，也就是其通信过程会真实的写入到文件系统中，如果通信频繁、信息量大且持续时间长，有可能在磁盘上产生一个很大的物理文件；
* 很显然，使用共享文件进行 IPC 的运行效率也是不高的，但仍然不失为一种 IPC 方法，而且相关的编程实践对理解 Linux 的共享文件及文件锁机制将会非常有帮助。

## 2 文件锁及其操作
* `fcntl()` 函数可以对文件进行加锁操作；
* `fcntl()` 可以对一个文件描述符做很多操作，在此，我们仅介绍其符合 POSIX 标准部分，与文件“锁”相关的调用方法；
* 下面是 `fcntl()` 的调用方法：
    ```C
    #include <unistd.h>
    #include <fcntl.h>

    int fcntl(int fd, int cmd, ... /* arg */ );
    ```
* `fcntl()` 是一个不定参数的调用函数，但对于 POSIX 的文件锁而言，它只有三个参数：
    ```C
    int fcntl(int fd, int cmd, (struct flock *)lock);
    ```
* 在这个调用中，**fd** 是一个已经打开的文件描述符，**cmd** 是要执行的命令；
* POSIX 与文件锁相关的命令有三个：
    - **F_SETLK**：获取文件锁或者释放文件锁，如果文件锁已被其它进程占有会立即返回错误；
    - **F_SETLKW**：执行与 `F_SETLK` 相同的指令，但当文件锁被其它进程占有时，会产生阻塞，直到获得该文件锁；
    - **F_GETLK**：获取当前文件锁状态；

* 其中，`struct flock` 的定义如下：
    ```C
    struct flock {
        short l_type;   /* Type of lock: F_RDLCK, F_WRLCK, F_UNLCK */
        short l_whence; /* How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END */
        off_t l_start;  /* Starting offset for lock */
        off_t l_len;    /* Number of bytes to lock */
        pid_t l_pid;    /* Process holding the lock. */
    };
    ```
    - POSIX 文件锁可以分为读文件锁和写文件锁两种；
    - POSIX 规定文件锁可以仅锁定文件中的一部分，而不是锁定整个文件，`struct flock` 结构不仅定义了锁的类型，同时，`l_start` 和 `l_len` 两个字段还定义了文件中那一部分被这个文件锁锁定；
    - **l_type**：锁类型，F_RDLCK - 读文件锁，F_WRLCK - 写文件锁，F_UNLCK - 释放文件锁；
    - **l_start**、**l_len**：该文件锁仅锁定从偏移量 `l_start` 开始，长度为 `l_len` 字节的区域，`l_len` 为 0 表示从 `l_start` 开始到文件结束；
    - **l_whence**：`l_start` 偏移量计算的起始位置，可以有三个选项：
        + **SEEK_SET**：从文件的开始计算 `l_start` 的偏移量，此时 `l_start` 必须是一个正整数；
        + **SEEK_CUR**：从当前文件指针处计算 `l_start` 的偏移量，此时，`l_start` 可以为负整数，但不能跑到文件起始位置之前；
        + **SEEK_END**：从文件尾部计算 `l_start` 的偏移量，此时，`l_start` 为负整数或者 0；
    - **l_pid**：在调用 F_GETLK 获取当前文件锁状态时，如果文件锁被其它进程占用，该字段将返回占用文件锁的进程号；
* 在大多数的应用中，无需仅锁定文件的一部分，锁定整个文件即可，也就是 `l_wence=SEEK_SET; l_start=0; l_len=0`；
* 下面代码片段在文件 fd 上获取写文件锁：
    ```C
    ......
    struct flock lock;

    lock.l_tyepe = F_WRLCK;
    lock.l_wence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    fcntl(fd, F_SETLKW, &lock);
    ......
    ```
* 下面代码片段释放了一个文件锁：
    ```C
    ......
    struct flock lock;

    lock.l_tyepe = F_UNLCK;
    lock.l_wence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    fcntl(fd, F_SETLKW, &lock);
    ......
    ```
* 命令 `F_SETLK` 和 `F_SETLKW` 的唯一区别是一个不阻塞直接返回，另一个阻塞直到获得所请求的文件锁；

* `man fcntl` 可以查看该函数的在线手册；

## 3 实例
* 正如第 1 节中描述的场景一样，该实例建立一个服务端进程，三个客户端进程，模拟一个 `client/server` 架构的服务过程；
* 正如第 1 节介绍的一样，需要使用两个共享文件实现客户端进程与服务端进程之间的通信，从服务端进程看，一个文件用于服务端读取客户端的消息，另一个文件用于服务端向客户端发送消息；
* 两个共享文件由服务端进程建立，服务端进程要最先开始运行，否则客户端进程无法打开共享文件；
* 整个通信过程以每个进程的进程号作为唯一地址标识，当目的进程号为 0 时表示是一条广播消息，所有进程都要接收并处理；
* 客户端进程启动时，需要知道服务端进程的 PID 才可以与服务端进行通信，此时要发出一条广播消息，服务端进程收到后回应一条消息从而建立通信通道；
* 客户端在空闲时循环向服务端发送一个字符串，服务端在收到后回应一个确认消息，模拟一个服务端为客户端提供服务的过程；
* 服务端向多个客户端进程发送消息时使用同一个共享文件，所以每个客户端进程要具备过滤地址的功能，即：只保留发给自己的消息，丢弃发给其它客户端进程的消息；
* 因为多个客户端进程都要向同一个共享文件中写入数据(即向服务端发送消息)，每次写入时应该写在文件的尾部，但对每个进程而言，当前的文件指针不一定是在文件的尾部，所以在获取了文件写入锁以后，需要将文件指针移动的文件的尾部才能写入数据；
* 为了通信方便，在传送信息时，所有进程使用下面的统一结构：
    ```C
    struct ipc_msg {
        int len;            // total length including itself
        int src_pid;        // source PID
        int dest_pid;       // destination PID
        uint seq_num;       // sequence number of the current message
        ushort cmd;         // command code
        char msg[1];        // the auxiliary information
    };
    ```
* **len** 为整个信息的总长度，包括 len 字段自身，接收端首先接收该字段，然后确定该信息后面还需要读取的字节数，再一次性地读取完整个结构；
* **src_pid** 为发送该信息的进程 PID；
* **dest-pid** 为接收该信息的进程 PID，当该字段为 0 时，表示该信息为广播消息，所以，一个进程应该接收该字段为自身 PID 或者该字段为 0 的消息，并丢弃其它消息；
* **cmd** 表示该信息的含义，目前有五个可选值：
    1. CMD_SERVER_ONLINE - 表示服务端在线，客户端在启动后并不知道服务端进程的 PID，所以应该周期性地广播 CMD_SERVER_STATUS 消息，服务端进程收到该广播消息后，向相应的客户端进程发送 CMD_SERVER_ONLINE 消息，客户端收到该消息便可获知服务端进程的 PID，从而建立通信通道；
    2. CMD_SERVER_OFFLINE - 表示服务端离线，当服务端准备退出时，广播该信息，客户端在收到该消息时，应主动退出；
    3. CMD_SERVER_STATUS - 客户端进程启动后广播该信息，服务端进程收到该信息应回复 CMD_SERVER_ONLINE，从而使客户端获得服务端进程的 PID；
    4. CMD_STRING - 客户端在空闲时定期向服务端进程发送一个字符串，以模拟客户端进程向服务端进程请求服务的过程，发送此消息时，字符串应放在 msg 字段中，所以这个消息的长度是不定长的，在实际的应用中，这个字符串可以是一个 json 数据，可以实现复杂的服务请求；
    5. CMD_STRING_OK - 服务端在收到客户端进程发送的 CMD_STRING 消息后，回应一个 CMD_STRING_OK 消息，模拟对客户端请求服务的响应；
* 各个进程在向共享文件写入数据时，均要求以 `struct ipc_msg` 格式写入，分下面几个步骤完成：
    1. 为 `struct ipc_msg` 分配内存，如果有 `ipc_msg.msg` 字段，则分配的内存要包含 `ipc_msg.msg` 字符串的长度；
    2. 计算整个消息的长度，长度应包括 `ipc_msg.msg` 最后的 `\0` 字符，将消息长度填写到 `ipc_msg.len` 字段中；
    3. 将当前进程的 PID 写入到 `ipc_msg.src_pid` 字段；
    4. 将接收进程的 PID 写入到 `ipc_msg.dest_pid` 字段，如果是广播消息，该字段填 `BROADCAST_PROCESS_ID`；
    5. 将消息序列号写入到 `ipc_msg.seq_num` 字段，
    6. 根据情况填写 `ipc_msg.cmd` 字段；
    7. 如果有 `ipc_msg.msg`，将字符串写入 `ipc_msg.msg` 中；
    8. 将 `struct ipc_msg` 写入共享文件；
    9. 释放为 `struct ipc_msg` 分配的内存；
* 各进程在读入数据时，要遵循下面步骤：
    1. 首先读取一个 int，此为 `struct ipc_msg` 中的 len 字段，然后根据 len 字段的值读取剩余的数据；
    2. 检查 `dest_pid` 字段是否为自身的 PID 或者 `BROADCAST_PROCESS_ID`，否则丢弃该消息，转到步骤 1 读取下一个消息；
    3. 根据消息内容做出回应；

* **源程序**：[ipc-files.c][src01](**点击文件名下载源程序**)演示了如何使用共享文件实现进程间通信；
* 编译：`gcc -Wall -g ipc-files.c -o ipc-files`
* 运行：`./ipc-files`
* 运行动图：

    ![screenshot of running ipc-files][img01]






-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

[article01]: https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
[article02]: https://whowin.gitee.io/post/blog/linux/0011-ipc-examples-of-fifo/
[article03]: https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/
[article04]: https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/
[article05]: https://whowin.gitee.io/post/blog/linux/0015-systemv-semaphore-sets/
[article06]: https://whowin.gitee.io/post/blog/linux/0016-posix-semaphores/
[article07]: https://whowin.gitee.io/post/blog/linux/0017-systemv-shared-memory/
[article08]: https://whowin.gitee.io/post/blog/linux/0018-posix-shared-memory/
[article09]: https://whowin.gitee.io/post/blog/linux/0019-ipc-with-unix-domain-socket/

[src01]: /sourcecodes/100020/ipc-files.c

[img01]: /images/100020/ipc-files.gif
