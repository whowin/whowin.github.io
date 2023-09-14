---
title: "IPC之一：使用匿名管道进行父子进程间通信的例子"
date: 2023-07-28T16:43:29+08:00
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
  - pipe
  - 匿名管道
draft: false
#references: 
# - [C Program to Demonstrate fork() and pipe()](https://www.geeksforgeeks.org/c-program-demonstrate-fork-and-pipe/)
# - [Half-duplex UNIX Pipes](https://tldp.org/LDP/lpg/node9.html)
# - [Linux Interprocess Communications](https://tldp.org/LDP/lpg/node7.html)
# - [sending commands to a child process through pipe/dup2 in C](https://stackoverflow.com/questions/11689421/sending-commands-to-a-child-process-through-pipe-dup2-in-c)

postid: 100010
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍匿名管道(又称管道、半双工管道)，尽管很多人在编程中使用过管道，但一些特殊的用法还是鲜有文章涉及，本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文适合 Linux 编程的初学者阅读

<!--more-->

## 1 概述
* IPC(Inter-Process Communication) - 进程间通信，提供了各种进程间通信的方法；
* 在 Linux C 编程中，IPC 通常有如下几种方式
    1. 半双工管道(Unix Pipe)，简称**管道**，又称为匿名管道
    2. FIFOs - 又称为命名管道
    3. 消息队列(Message Queues)
    4. 信号量集(Semaphore Sets)
    5. 共享内存(Shared Memory Segments)
    6. 网络 socket(AF_INET Address Family)
    7. 全双工管道(AF_UNIX Address family)
* 本文主要介绍匿名管道(Unix Pipe)的应用场景及使用方法，并给出多个附有完整源代码的实例；
* 管道又被称为 **匿名管道**，是相对于命名管道而言的；
* 匿名管道的通信模式是半双工的，所谓半双工指的是在管道中数据流是单方向的，当 A 进程和 B 进程之间使用管道进行通信时，数据要么从 A 发向 B，要么从 B 发向 A，在一个管道上，不能既有 A 向 B 的数据流，又有 B 向 A 的数据流；
* 管道还有一个特性就是只能在有亲缘关系的进程间传递消息，换句话说，只有当两个进程有相同的祖先时，才有可能使用管道进行通信。

## 2. 管道的基本概念
* 简单地说，管道是一种将一个进程的输出连接到另一个进程的输入的方法；
* 管道是最古老的 IPC 工具，从最早的 UNIX 操作系统开始就存在了；它们提供了进程间单向通信的方法(因此称为半双工)；
* 实际上，管道的这个特性广泛应用在 Linux 的命令行上，比如下面的命令：
    ```bash
    ls | sort | lp
    ```
* 这条命令实际上就建立了一个管道，将 `ls` 的输出作为 `sort` 的输入，将 `sort` 的输出作为 `lp` 的输入；数据在匿名管道中运行，看上去，数据在管道中从左向右单方向流动；
* 管道是在 `Linux` 内核中实现的，很多程序员在 `shell` 脚本编程中都会频繁使用管道，但很少有人会去想管道在 `Linux` 内核中是如何实现的；
    > 当一个进程创建管道时，内核会创建两个文件描述符(`fd[0]` 和 `fd[1]`)供管道使用；一个描述符(`fd[1]`)用于将数据写入管道，另一个描述符(`fd[0]`)用于从管道中读取数据；此时，管道的实际用处不大，因为创建管道的进程只能使用管道与自身进行通信，毫无意义；
* 下图展示了一个进程创建管道后，进程与内核的关系：

    ![process and kernel][img01]

* 从上图中，可以看出以下几点：
    - 文件描述符是如何连接在一起的；进程通过文件描述符(`fd[1]`)向管道写入数据，也能够从文件描述符(`fd[0]`)从管道中读取该数据；
    - 通过管道传输数据时，数据是通过内核流动的；在 `Linux` 下，管道在内核内部使用 `inode` 表示，`innode` 驻留在内核中，并不属于一个物理文件系统。
* 这样建立的管道毫无用处，一个进程要自言自语，没有必要建立一个管道；但是，如果创建管道的进程再 `fork` 出一个子进程，由于子进程会从父进程继承管道的描述符，这样父子进程之间有可以通过这个管道进行通信了；
* 下图描述了父进程、子进程和内核的关系

    ![parent-child processes and kernel][img02]

* 从上图中，我们可以看到，父进程和子进程都可以访问管道的两个文件描述符，但是很显然，如果父进程和子进程同时向 `fd[1]` 写入数据，一定会造成混乱，而且如果父、子进程均向 `fd[1]` 写入数据，当从 `fd[0]` 读出数据时，并无法区分读到的数据是那个进程写入的；所以必须要做出抉择，这个建立的管道的数据是向那个方向流动，从父进程流向子进程？还是从子进程流向父进程？两个进程必须达成一致，否则会出现混乱；
* 为了讨论方便，我们假定子进程要做一些事务，然后把结果通过管道发送给父进程，如下面图示：

    ![Data flows from child to parent process][img03]

* 至此，管道已经建立完毕，下面就是如何使用管道；前面提到过，管道的文件描述符使用 `inode`，所以可以使用低级文件 I/O 的系统调用来直接访问管道；
* 向管道中写入数据，使用 `write()` 系统调用；从管道中读出数据，使用 `read()` 系统调用；
* **特别提醒**：系统调用 `lseek()` 不能在管道中使用。

## 3 如何用C语言创建管道
* 使用 `pipe()` 系统调用可以创建一个管道，这个调用需要一个由两个整数组成的数组作为参数，调用成功后，该数组将包含管道的两个文件描述符；
* 系统调用：pipe()
    ```
    原型：#include <unistd.h>
         int pipe(int fd[2]);
    返回：调用成功返回 0
         调用失败返回 -1
         error = EMFILE (no free descriptors)
                 EMFILE (system file table is full)
                 EFAULT (fd array is not valid)

    备注: fd[0] 用于从管道中读取数据, fd[1] 用于向管道中写入数据
    ```
* 调用成功后，不仅两个管道描述符被建立，而且处于打开状态，可以直接进行读、写操作；
* 再次重申，所有通过管道传输的数据都要通过内核；下面是使用 `pipe()` 建立管道的代码：
    ```C
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/types.h>

    main() {
        int     fd[2];
        
        pipe(fd);
        .
        .
    }
    ```
* 前面说过，这样建立的管道毫无用处，进程自言自语并不需要使用管道；要使管道有意义，在建立管道后要 `fork()` 一个子进程；
    ```C
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/types.h>

    main() {
        int   fd[2];
        pid_t childpid;

        pipe(fd);

        if ((childpid = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        .
        .
    }
    ```
* 如果父进程要从子进程接收数据，父进程应关闭向管道写入的描述符 `fd[1]`，而子进程应该关闭从管道读出的描述符 `fd[0]`；如果父进程要向子进程发送数据，则父进程应关闭从管道读出的描述符 `fd[0]`，而子进程应该关闭向管道写入的描述符 `fd[1]`；
* 由于管道描述符在父进程和子进程之间是共享的，所以我们要确保关闭掉我们不需要的管道末端，从技术上讲，如果不需要的管道末端没有关闭，则永远不会返回 `EOF`；
* 下面代码假定父进程要从子进程接收数据：
    ```C
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/types.h>

    main() {
        int   fd[2];
        pid_t childpid;

        pipe(fd);

        if ((childpid = fork()) == -1) {
            perror("fork");
            exit(1);
        }

        if (childpid == 0) {
            /* Child process closes up input side of pipe */
            close(fd[0]);
        } else {
            /* Parent process closes up output side of pipe */
            close(fd[1]);
        }
        .
        .
    }
    ```
* 如前所述，建立了管道以后，就可以像对待普通文件描述符一样对待管道描述符；
* 源程序：[pipe.c][src01](**点击文件名下载源程序**)演示了子进程向父进程发送信息：Hello, world!

## 4 在管道上使用 dup()
* 大多数已有的 `Linux` 命令或者自己编写的程序，其默认的输入设备往往是 `STDIN`，而输出设备是 `STDOUT`，当我们希望在程序中用某个 `Linux` 命令处理数据时，往往不太好获得命令的输出，或者不好把数据传送给这个程序，这时候管道可以发挥作用；
* 比如 `Linux` 命令 `sort`，在没有其它参数时，其默认的输入设备就是 `STDIN`，当我们在程序中希望使用 `sort` 处理一组数据时，我们可以设法把 `STDIN` 连接到管道的输出端，这样，我们向管道中的一端写入数据时，管道的另一端已经启动的 `sort` 就可以从 `STDIN` 读到数据并进行处理；
* 系统调用 `dup()` 和 `dup2()` 可以帮助我们实现这个想法；先看一下这两个系统调用的说明；
* 系统调用: dup();
    ```plain
    原型：#include <unistd.h>
         int dup(int oldfd);
    说明：dup() 系统调用创建文件描述符 oldfd 的副本，使用编号最小的未使用的文件描述符作为新描述符。
    返回：调用成功则返回新描述符
         调用失败则返回 -1
         errno = EBADF (oldfd is not a valid descriptor)       
                 EBADF (newfd is out of range)
                 EMFILE (too many descriptors for the process) 

    备注：oldfd 不会被关闭，新描述符和 oldfd 都可以使用。
    ```
* 系统调用：dup2();
    ```plain
    原型：#include <unistd.h>
         int dup2(int oldfd, int newfd);
    说明：dup2() 系统调用与 dup() 相似，创建文件描述符 oldfd 的副本，但它不使用编号最小的未使用文件描述符，而是使用 newfd 中指定的文件描述符；
         如果文件描述符 newfd 先前已打开，该调用会首先将其关闭然后再使用。
    返回：调用成功则返回新描述符
         调用失败则返回 -1
         errno = EBADF (oldfd is not a valid descriptor)
                 EBADF (newfd is out of range)
                 EMFILE (too many descriptors for the process)

    备注：使用 dup2()，oldfd 会被关闭
    ```
* 在子进程中，使用 `dup2()` 将管道的输出(`fd[0]`)复制到 `STDIN` 上，并关闭 `STDIN`，然后用 `exec()` 启动 `sort` 时，当 `sort` 从 `STDIN` 读入数据时，实际上是从管道中读出数据，当我们从父进程向管道中写入数据时，这个数据将被 `sort` 读取并处理；
* 为了搞清楚这种用法，请自行学习 Linux 命令 **sort**，可以用在线手册 `man sort` 了解该命令的详细信息；
* 下图或许可以更直观地描述这种使用方法：

    ![pipe with dup()][img04]

* 源程序：[pipe-dup-stdin.c][src02](**点击文件名下载源程序**)演示了在管道中使用 `dup2()` 将 fd[0] 复制到 `STDIN` 的方法：
    > 子进程中把 fd[0] 复制到 `STDIN`，然后启动 `sort`，父进程向管道中写入若干个单词，每个单词以 `\n` 结尾，`sort` 从 `STDIN` 读入数据，实际上是从管道中读入数据，所以 `sort` 程序会对这些单词进行排序，并把结果写入文件 `sort.log` 中，程序运行完毕后，使用 `cat sort.log` 可以看到经过排序的单词；

* 同样道理，也可以在子进程中把管道的输入端(`fd[1]`)复制到 `STDOUT` 上，这样，当子进程中启动的程序向 `STDOUT` 输出时，实际上是在向管道上写入数据；
* 源程序：[pipe-dup-stdout.c][src03](**点击文件名下载源程序**)演示了在管道中使用 `dup2()` 将 fd[1] 复制到 `STDOUT` 的方法：
    > 子进程中把 `fd[1]` 复制到 `STDOUT`，然后启动 `uname`，`uname -r` 会输出一个字符串到 `STDOUT`，实际上是写入到了管道中，父进程从管道中收到了这个字符串并显示出来；

    > 子进程中把管道的输入端复制到 `STDOUT` 后，在子进程中启动任何程序，在主进程中通过读取管道都可以轻易地获得这个程序的输出，比如我们要知道当前系统的是不是 64 位系统，那我们在子进程中启动命令 `uname -m`，如果主进程在管道上读出的内容是 `x86_64`，则系统无疑是64位的。

## 5 使用管道的简单方法
* 上面介绍的在程序中使用管道获取一个外部程序的输出(或者向一个外部程序输入数据)的方法看上去不仅繁琐，而且绕的弯也比较多，其实使用管道还有更为简单的方法；
* 使用标准库函数 `popen()` 可以很容易地使用管道；
    ```plain
    库函数：popen();

    原型：#include <stdio.h>
         FILE *popen (char *command, char *type);
    说明：popen() 函数通过创建一个管道，调用 fork 产生一个子进程，执行 shell 运行命令来开启一个进程。
    返回：调用成功则返回一个标准 I/O 流
         调用 fork() 或 pipe() 失败则返回 NULL
    ```
* 该标准库函数通过内部调用 `pipe()` 创建匿名管道，然后 `fork()` 一个子进程，执行 `shell`，并在 `shell` 中执行 "command" 参数；数据流的方向由第二个参数 `type` 确定，`type` 可以是 "r" 或 "w"，表示读或写，不可能两者兼而有之！在 `Linux` 下，管道将以 `type` 参数的第一个字符指定的模式打开，如果您将 `type` 设置为 "rw"，该函数会以 "r" (读)模式打开管道。
* 与直接使用 `pipe()` 系统调用相比，这个库函数为我们做了很多繁琐的工作，但却让我们失去了对整个过程的精细控制；
* 该函数直接使用了 Bourne shell(bash), 所以在 `command` 参数中可以使用 `shell` 元字符以及元字符扩展(包括通配符)；
* 使用 `popen()` 创建的管道必须使用 `pclose()` 关闭；`popen()/pclose()` 与标准文件流I/O函数 `fopen()/fclose()` 非常相似。
    ```plain
    库函数：pclose();

    原型：#include <stdio.h>
         int pclose(FILE *stream);
    说明：pclose()函数等待相关进程终止，并返回由 wait4() 返回的命令退出状态。
    返回：返回 wait4() 调用的退出状态码
         如果 stream 不合法，或者 wait() 执行失败，则返回 -1

    备注：等待管道进程退出，然后关闭文件 I/O 流
    ```
* `pclose()` 函数对由 `popen()` 派生的进程执行 `wait4()`，当 `wait4()` 返回时，它会销毁管道和文件流；
* 源程序：[pipe-popen.c][src04](**点击文件名下载源程序**)完成与前面的例子 [pipe-dup-stdin.c][src02] 一样的功能，但看上去要简单的多；

* 由于 `popen()` 使用 `shell` 来执行命令，因此 `shell` 扩展字符和元字符都可以使用，此外，使用 popen() 打开管道时，可以使用一些高级技术来执行命令，例如重定向，甚至输出管道；以下调用示例分别使用了扩展字符、重定向和输出管道：
    ```C
    popen("ls ~scottb", "r");
    popen("sort > /tmp/foo", "w");
    popen("sort | uniq | more", "w");
    ```

* 源程序：[pipe-popen2.c][src05](**点击文件名下载源程序**)打开了两个管道，一个用于 `ls` 命令，另一个用于 `sort` 命令；
* 下面这个例子试图编写一个通用的管道程序，源程序文件：[pipe-popen3.c][src06](**点击文件名下载源程序**)
    - 使用方法为：`./pipe-popen3 [command] [filename]`
    - 该程序会首先打开文件 `filename`
    - 然后使用 `popen()` 以写方式打开 `command` 管道
    - 从 `filename` 中读出内容并写入管道
    - 可以尝试用以下方式测试这个例子
        ```bash
        ./pipe-popen3 sort pipe-popen3.c
        ./pipe-popen3 cat pipe-popen3.c
        ./pipe-popen3 more pipe-popen3.c
        ./pipe-popen3 cat pipe-popen3.c | grep main
        ```

## 6 管道的原子操作
* 所谓“原子操作”，是指一个或一系列不可中断的操作，就是说一个原子操作一旦开始执行就不能被中断，直至执行完毕；
* POSIX 标准规定了管道上原子操作的最大缓冲区大小是 512 字节，定义在头文件：`bits/posix1_lim.h` 中：
    ```C
    #define _POSIX_PIPE_BUF         512
    ```
* 根据这一定义，如果一次写入/读出管道的操作大于 512 字节，操作将是非“原子操作”，也就是写入/读出的数据可能会被分割；
* 在 Linux 下，定义的管道上的原子操作的最大缓冲区大小为：4096 字节，定义在头文件：`linux/limits.h` 中：
    ```C
    #define PIPE_BUF        4096	/* # bytes in atomic write to a pipe */
    ```
* 显然，在我们目前的环境下，在管道上进行不大于 4096 字节的读/写操作是原子操作；
* 在多进程环境下，原子操作对管道的读/写操作非常重要，当一个进程写入管道的数据大于阈值时，其写入过程中间会中断，操作系统会产生进程调度，如果这时其它进程也向这个管道写入数据，那么写入管道的数据会产生混乱。

## 7 匿名管道的其它说明
* 尽管管道是半双工的，但是，打开两个管道，并在子进程中合理地重新分配描述符，可以构建出一个类似全双工的管道；
* `pipe()` 调用必须在 `fork()` 调用之前进行，否则描述符将不会被子进程继承；
* 使用匿名管道进行通信的进程都必须有一个共同的祖先，而且这个祖先必须是管道的创建者，由于管道位于内核中，不在管道创建者祖先中的进程都无法对其进行寻址，这与命名管道(FIFO)是不同；
* 由于使用管道的一些限制，在进程间进行通讯时，管道实际上并不是一个常用的方法，但是，如果需要使用已有的 Linux 命令处理数据，或者从 Linux 命令获得结果数据，管道不失为一个好的选择；
* 匿名管道的生命周期与创建它的进程的生命周期一致，当进程结束时，其创建的匿名管道也将被销毁；
* 尽管在本文的所有实例中，从管道传输的数据都是字符串，但管道是可以传输二进制数据的，也就是说，可以把一个结构完整第通过管道进行传输。
* 有关进程间通信(IPC)的的其它文章：
    - [IPC之二：使用命名管道(FIFO)进行进程间通信的例子][article02]
    - [IPC之三：使用 System V 消息队列进行进程间通信的实例][article03]
    - [IPC之四：使用 POSIX 消息队列进行进程间通信的实例][article04]


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[article01]: https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
[article02]: https://whowin.gitee.io/post/blog/linux/0011-ipc-examples-of-fifo/
[article03]: https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/
[article04]: https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/

<!-- for CSDN
[article01]: https://blog.csdn.net/whowin/article/details/132171311
[article02]: https://blog.csdn.net/whowin/article/details/132171930
[article03]: https://blog.csdn.net/whowin/article/details/132172172
[article04]: 
[article05]: 
-->

[src01]: https://whowin.gitee.io/sourcecodes/100010/pipe.c
[src02]: https://whowin.gitee.io/sourcecodes/100010/pipe-dup-stdin.c
[src03]: https://whowin.gitee.io/sourcecodes/100010/pipe-dup-stdout.c
[src04]: https://whowin.gitee.io/sourcecodes/100010/pipe-popen.c
[src05]: https://whowin.gitee.io/sourcecodes/100010/pipe-popen2.c
[src06]: https://whowin.gitee.io/sourcecodes/100010/pipe-popen3.c


[img01]: https://whowin.gitee.io/images/100010/pipe-process-kernel.png
[img02]: https://whowin.gitee.io/images/100010/pipe-parent-child-process-kernel.png
[img03]: https://whowin.gitee.io/images/100010/data-from-child-to-parent.png
[img04]: https://whowin.gitee.io/images/100010/pipe-with-dup.png

