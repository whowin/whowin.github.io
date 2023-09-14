---
title: "IPC之二：使用命名管道(FIFO)进行进程间通信的例子"
date: 2023-08-02T16:43:29+08:00
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
  - FIFO
  - 命名管道
draft: false
#references: 
# - [Named Pipe or FIFO with example C program](https://www.geeksforgeeks.org/named-pipe-fifo-example-c-program/)
# - [Named Pipes (FIFOs - First In First Out)](https://tldp.org/LDP/lpg/node15.html)
# - [Linux Interprocess Communications](https://tldp.org/LDP/lpg/node7.html)
# - [Inter process communication using named pipes(FIFO) in linux](https://tuxthink.blogspot.com/2012/02/inter-process-communication-using-named.html)
# - [Cool way of sharing your Linux terminal session with named pipe/FIFO](https://devdojo.com/bobbyiliev/cool-way-of-sharing-your-linux-terminal-session-with-named-pipefifo)


postid: 100011
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍命名管道(FIFO)，命名管道可以完成同一台计算机上的进程之间的通信，本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文适合 Linux 编程的初学者阅读。

<!--more-->

## 1 命名管道(FIFO)的基本概念
* FIFO(First-In First-Out)，在 Linux 文件系统中是一种特殊的文件，相对于匿名管道，FIFO 又被称为命名管道；
* 在我的另一篇文章[《IPC之一：使用匿名管道进行父子进程间通信的例子》][article01]中，介绍了管道(又称匿名管道)的基本概念；
* 匿名管道和命名管道本质上是一种东西，在内核中都是一个 `inode`，只是因为匿名管道在物理文件系统中没有文件故而仅能通过进程间的继承来传递这个 `inode` 的索引号；而命名管道因为有了物理文件系统中的文件，通过这个文件可以找到内核中相应的 `inode`，故而对该文件有读写权限的进程均可以使用这个管道；
* 命名管道的工作方式与匿名管道非常相似，但有一些明显的差异：
    1. 命名管道作为一种特殊文件存在于文件系统中，匿名管道仅存在于内核中，不存在于文件系统中；
    2. 不同祖先的进程可以通过命名管道交换数据，而匿名管道只能在有共同祖先的进程间进行通信；
    3. 命名管道的生命周期可以大于使用它的进程的生命周期，也就是说，当使用命名管道交换数据的所有进程都终止后，命名管道仍然可以保留在文件系统中供以后使用；匿名管道则不同，当使用匿名管道的进程全部终止后，匿名管道也会消亡。

* 当进程通过命名管道交换数据时，内核会在内部传递所有数据，而不将其写入文件系统；因此，表示一个命名管道的特殊文件在文件系统上没有内容；文件系统中表示命名管道的文件名仅充当参考点，以便进程可以使用文件系统中的文件名找到内核中的 `inode`，从而访问管道。
* 每个 FIFO 特殊文件代表着一个命名管道，由内核进行维护，必须先打开 FIFO 两端(读和写)，然后才能传递数据；一般情况下，打开 FIFO 会产生阻塞，直到另一端也打开为止，后面我们会专门讨论 FIFO 的阻塞问题；
* 进程也可以以非阻塞模式打开 FIFO；
    - 在使用非阻塞方式打开 FIFO 时，即便没有进程打开 FIFO 的写入端，一个进程以只读方式打开 FIFO 也会成功；
    - 即便是以非阻塞方式打开 FIFO，如果没有进程打开 FIFO 的读出端，以只写方式打开 FIFO 也会失败，产生错误：No such device or address
* 以读写方式打开 FIFO 不管是在阻塞还是非阻塞方式下都会成功，其返回的文件描述符既是写入端又是读出端，以这种方式打开的 FIFO，既可以做写入端使用，也可以做读出端使用，但通常只能使用其一端(要么写入要么读出)。

## 2 在 shell 下建立一个命名管道(FIFO)
* 有多种方法可以创建命名管道，其中下面两种方法可以直接在 shell 中创建命名管道；
* **使用 `mknod` 命令建立命名管道**
    ```bash
    mknod myfifo p
    ```
    - `mknod` 命令用于创建一个特殊文件，命令最后的参数 "p"，表示建立一个 FIFO 特殊文件，这个参数还可以是 `b、c、u`，分别表示其它的特殊文件；
    - 可以使用在线手册 `man mknod` 了解 `mknod` 命令的详细信息；
    - 使用 `ls -l myfifo` 可以查看我们刚刚创建的 FIFO，文件权限最前面的 "p"，表示这个文件是个命名管道 FIFO；

        ![Screenshot for ls fifo][img01]

    - 使用 `mknod` 创建 FIFO 时，可以使用参数 `-m 权限` 来设置权限，比如：`mknod -m 0644 myfifo p` 命令，创建的 FIFO 只有拥有者可读写，其它用户只读；

* **使用 `mkfifo` 命令建立命名管道**
    ```bash
    mkfifo myfifo
    ```
    - `mkfifo` 命令用于创建一个 FIFO 特殊文件；
    - 可以使用在线手册 `man mkfifo` 了解 `mkfifo` 命令的详细信息；
    - 使用 `mkfifo` 创建 FIFO 时，可以像 `mknod` 命令一样使用参数 `-m 权限` 来设置权限，比如：`mkfifo -m 0666 myfifo` 命令，创建一个 FIFO，所有用户可读写；

* 用 `mknod` 和 `mkfifo` 创建的 FIFO 可以用 `chmod` 修改权限
    ```bash
    mkfifo -m 0644 myfifo
    chmod a=rw myfifo
    ```
    - `mkfifo` 创建了一个 FIFO，`chmod` 将其权限改为所有用户可读写。

* 用 `mknod` 和 `mkfifo` 创建的 FIFO 可以用 `rm` 删除
    ```bash
    rm myfifo
    ```

## 3 使用C语言创建命名管道
* 使用系统调用 mknod() 创建命名管道
    ```C
    原型：#include <sys/types.h>
         #include <sys/stat.h>
         #include <fcntl.h>
         #include <unistd.h>

         int mknod(const char *pathname, mode_t mode, dev_t dev);
    说明：mknod() 可以在文件系统上建立一个文件节点(普通文件、特殊文件、命名管道)
    返回：调用成功返回 0
         调用失败返回 -1
         errno = EFAULT (pathname invalid)
                 EACCES (permission denied)
                 ENAMETOOLONG (pathname too long)
                 ENOENT (invalid pathname)
                 ENOTDIR (invalid pathname)
                 (其它错误码请查看 man 手册)
    ```
    - pathname 是文件路径不用多说
    - mode 是文件类型和文件权限，比如：`S_IFIFO | 0666` 表示建立一个 FIFO 特殊文件，权限为 `0666` (所有用户可读写)
    - dev 是设备号，只有当文件类型为 `S_IFCHR 或 S_IFBLK` 时，才有设备号，其他文件类型填 0 即可；
    - 在线手册 `man 2 mknod` 可以查询该系统调用的详细信息
    - 下面的调用创建了一个 FIFO 文件：`/tmp/myfifo`，权限为：`0666` (所有用户可读写)
        ```C
        mknod("/tmp/myfifo", S_IFIFO | 0666, 0);
        ```
    - 但实际上设置的文件权限还要受到 `umask` 的影响，最终的文件权限是 `0666 & ~umask`
    - 可以使用 shell 命令 `umask` 查看当前的 `umask`

        ![Screenshot of umask][img02]

    - 为了保证创建的 FIFO 文件的权限，可以在调用 `mknod()` 前临时删除 `umask`
        ```C
        umask(0);
        mknod("/tmp/myfifo", S_IFIFO|0666, 0);
        ```
* 使用 mknodat() 创建命名管道
    ```C
    原型：#include <fcntl.h>           /* Definition of AT_* constants */
         #include <sys/stat.h>

         int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev);
    ```
    - 与 `mknod()` 相比，这个系统调用多了一个参数 `dirfd`，`dirfd` 是文件系统中一个打开的目录的文件描述符；
    - 当 pathname 为绝对路径时，参数 `dirfd` 被忽略，该调用与 `mknod()` 完全一样；
    - 当 pathname 为相对路径时，它是相对于 `dirfd` 所引用的目录，而不是相对于调用进程的当前工作目录；
    - `mknod()` 中的 `pathname` 也是可以使用相对路径的，但是相对于调用进程时的当前目录；
    - 下面代码使用 `mknodat()` 创建了一个 FIFO 文件：
        ```C
        int dirfd = open("/tmp", O_RDONLY);
        mknodat(dirfd, "./myfifo", S_IFIFO|0666, 0); 
        ```

* 使用系统调用 mkfifo() 创建命名管道
    ```C
    原型：#include <sys/types.h>
         #include <sys/stat.h>

         int mkfifo(const char *pathname, mode_t mode);    
    ```
    - 毫无疑问，`mkfifo()` 就是专门用来创建 FIFO 文件的，`pathname` 为 FIFO 文件路径，`mode` 为文件权限，当然这个权限还要受到 `umask` 的影响，请参考前面的说明。

* 使用系统调用 mkfifoat() 创建命名管道
    ```C
    #include <fcntl.h>           /* Definition of AT_* constants */
    #include <sys/stat.h>

    int mkfifoat(int dirfd, const char *pathname, mode_t mode);    
    ```
    - `mkfifoat()` 与 `mkfifo()` 的关系与 `mknodat()` 与 `mknod()` 的关系一样，请参考 `mknodat()` 的说明

## 4 命名管道(FIFO)的实例
* 在命名管道上的 I/O 操作与匿名管道是一样的，使用 `read()` 从管道中读出数据，使用 `write()` 向管道中写入数据，不能使用 `lseek()` 等；
* 有一个明显的区别是在对命名管道进行 I/O 操作之前，必须用 `open()` 打开 FIFO 文件，这在匿名管道上是不需要的，因为匿名管道仅存在于内核中，并不存在于物理的文件系统中，而命名管道是存在于物理文件系统中的；
* 由于 FIFO 文件存在于文件系统中，所以，对命名管道也可以使用流式文件的 I/O 操作，即：
    - `fopen()` 打开 FIFO 文件
    - 使用 `fgetc()/fputc()、fgets()/fputs()` 进行读写操作；或者
    - 使用 `fscanf()/fprintf()` 进行读写操作；或者
    - 使用 `fread()/fwrite()` 进行读写操作；
    - `fclose()` 关闭 FIFO 文件
* 下面是一个使用 FIFO 传递消息的实例，分为两个独立的程序
    - 服务端源程序：[fifo-server.c][src01](**点击文件名下载源程序**)，从 FIFO 上接收数据
    - 客户端源程序：[fifo-client.c][src02](**点击文件名下载源程序**)，向 FIFO 中写入消息
    - 这个实例中使用 `fopen()` 打开 FIFO，客户端使用 `fputs()` 向 FIFO 中写入数据，服务端使用 `fgets()` 从 FIFO 中读出数据，使用 `fclose()` 关闭 FIFO；
    - 也可以使用 `open()/write()/read()/close()` 这组系统调用来进行 FIFO 的操作；
    - 这个实例使用 `mknod()` 创建 FIFO，当然也可以使用 `mkfifo()` 创建 FIFO；
    - 其实这个实例中也可以不使用 `fifo-client` 这个程序，我们可以在 shell 下使用 `echo "Hello World!">myfifo` 来代替 `fifo-client` 完成向 FIFO 发送信息的功能；
    - `fifo-client` 程序演示了使用流式文件向 FIFO 中写入数据的方法，特别要注意的是程序最后的 `fflush()`，在本例中，`fflush()` 并不是必须的，因为紧接着就使用了 `fclose()`，但在很多情况下，如果不使用 `fflush()`，数据会被缓存起来，不会立即发送出去。 

* FIFO 这种特殊文件有些地方确实特殊：
    - 从 FIFO 中读数据的进程在打开 FIFO 时会阻塞，直到有一个向这个 FIFO 中写数据的进程打开 FIFO；
    - 向 FIFO 中写数据的进程在打开 FIFO 时会阻塞，直到有一个从这个 FIFO 中读数据的进程打开 FIFO;
    - 进程从 FIFO 中读数据时会阻塞，直到有进程向 FIFO 中写入数据；
    - 向一个没有读出端的 FIFO 中写入数据时，会产生错误；
        > 这种情况通常发生在 FIFO 两端正常打开后，读出端关闭，导致管道没有读出端

* 编译运行
    - 编译
        ```bash
        gcc -Wall fifo-server.c -o fifo-server
        gcc -Wall fifo-client.c -o fifo-client
        ```
    - 这个实例需要在两个终端中运行，一个终端中运行 `./fifo-server`；另一个终端中运行 `./fifo-client "Hello world!"`
    - 初次运行时，要先运行 `./fifo-server` 再运行 `./fifo-client`，因为 FIFO 文件是在 `fifo-server` 程序中创建的；
    - `fifo-server` 不会自行退出，需要在键盘上输入 `ctrl+c` 才能退出；
    - `fifo-client` 可以在 shell 下使用命令：`echo "Hello world!">myfifo` 来代替。

* `fifo-server` 的运行截图：

    ![Screen of fifo-server][img03]

---------
## 5 命名管道(FIFO)的阻塞机制
* 打开 FIFO 有三种方式：只读方式、只写方式和读写方式
* 使用阻塞方式(没有 O_NONBLOCK 标志)打开 FIFO
    - 以只读方式(O_RDONLY)打开 FIFO 的读出端会发生阻塞，直至 FIFO 写入端被其它进程打开；
    - 以只写方式(O_WRONLY)打开 FIFO 的写入端会发生阻塞，直至 FIFO 读出端被其它进程打开；
    - 以读写方式(O_RDWR)方式打开 FIFO 会直接返回成功，因为打开的 FIFO 描述符既可作读出端，也可以作写入端，相当于打开了 FIFO 的两端；
    - 以只读方式(O_RDONLY)成功打开的 FIFO，在读出数据时：
        + 如果写入端打开，则发生阻塞，直到写入端将数据写入 FIFO 后，返回读出的数据长度(字符数)；
        + 如果写入端关闭，则立即返回 EOF(读出数据长度为 0)；
    - 以只写方式(O_WRONLY)成功打开的 FIFO，向 FIFO 写入数据时：
        + 如果读出端打开，则会直接返回成功，读出端可以正常读出数据；
        + 如果读出端关闭，则出现错误，并产生 SIGPIPE 信号。

* 使用非阻塞方式(设置 O_NONBLOCK 标志)打开 FIFO
    - 以只读方式(O_RDONLY)打开 FIFO 时不会阻塞，返回成功；
    - 以只写方式(O_WRONLY)打开 FIFO 时不会阻塞：
        + 如果 FIFO 的读出端没有打开，直接返回失败；
        + 如果 FIFO 的读出端已经打开，直接返回成功
    - 从以只读方式(O_RDONLY)打开的 FIFO 中读取数据不会阻塞：
        + FIFO 的写入端已打开，但 FIFO 中没有数据时，会产生错误：Resource temporarily unavailable
        + FIFO 的写入端已打开，且 FIFO 中有数据时，返回读出数据的长度(字节数)；
        + 当 FIFO 写入端关闭时，返回 EOF(读出数据长度为 0)。
    - 向以只写方式(O_WRONLY)打开的 FIFO 写入数据时不会阻塞，立即返回：
        + 当 FIFO 读出端关闭时，写入数据时出错，并产生 SIGPIPE 信号；
        + 当 FIFO 读出端打开时，写入成功，写入的数据可以被读出端正常读出。

* 这种教条式的说明其实很枯燥，建议读者**编写一些测试程序**来体会上面的“教条”。

## 6 SIGPIPE 信号和 EOF
* 在上一节的“教条”中，我们提出过两个要点：
    1. 当一个向 FIFO 写入数据的进程将写入端关闭时，正在等待从 FIFO 中读取数据进程会收到一个 EOF；
    2. 当一个从 FIFO 读出数据的进程将读出端关闭时，向 FIFO 中写入数据的进程在写入数据时系统会发出一个 SIGPIPE 信号。
* 这一节专门讨论与 FIFO 有关的 SIGPIPE 信号和 EOF(End Of File)；

* 在 Linux 的信号集有一个 SIGPIPE 信号是和 FIFO 相关的；
* 只有在向 FIFO 写入数据时才会产生 SIGPIPE 信号；也就是说，虽然读出端关闭，但如果不向 FIFO 中写入数据，也不会产生 SIGPIPE 信号；
* 写入数据时，`write()` 也会返回错误，所以，通常情况下并不一定要捕捉 SIGPIPE 信号；
* 源程序：[fifo-sigpipe.c][src03](**点击文件名下载源程序**)演示了在向 FIFO 写入数据时，如何截获 SIGPIPE 信号；
    - 编译：`gcc -Wall fifo-sigpipe.c -o fifo-sigpipe`
    - 运行这个程序需要两个终端窗口，在第一个终端上运行：`./fifo-sigpipe`，程序运行后将被阻塞在 `open()` 上，在第二个终端上运行：`cat myfifo`，来模拟从 FIFO 中读取数据；
    - 我们可以看到第一个终端中由 `fifo-sigpipe` 每 5 秒一次发出的信息可以在第二个终端上收到并显示出来，此时我们在第二个窗口中输入 `ctrl+c` 终止 `cat` 的运行，则在第一个窗口中可以看到截获到的 `SIGPIPE` 信号；
    - `./fifo-sigpipe` 是 FIFO 的写入端，`cat fifo-sigpipe` 是 FIFO 的读出端，终止读出端，然后再次运行 `cat fifo-sigpipe`，可以继续收到 `fifo-sigpipe` 发出的信息。

* EOF 是读文件时常用的判断文件结束的方法，在 FIFO 上表示写入端被关闭：
    - 以阻塞方式打开 FIFO 进行读操作时，EOF 表示写入端已被关闭；
    - 以阻塞方式打开 FIFO 进行读操作时，如果写入端打开但 FIFO 中没有数据，则会产生阻塞；
    - 以非阻塞方式打开 FIFO 时，写入端关闭时，读出端会返回 EOF(读出数据长度为 0)；
    - 以非阻塞方式打开 FIFO 时，写入端打开但 FIFO 中没有数据时，读出端不会返回 EOF，会产生错误：Resource temporarily unavailable

* 源程序：[fifo-eof.c][src04](**点击文件名下载源程序**)演示了在从 FIFO 中读数据时，如何截获 EOF；
    - 编译：`gcc -Wall fifo-eof.c -o fifo-eof`
    - 运行这个程序需要两个终端窗口，在第一个终端上运行：`./fifo-eof`，程序运行后将被阻塞在 `open()` 上，在第二个终端上运行：`echo "Hello world">myfifo`，来模拟向 FIFO 写入数据；
    - 当第二个终端的命令启动后，第一个终端上的 `fifo-eof` 程序会收到第二个终端的命令发来的数据，然后，第二个终端命令执行完毕退出，相当于关闭了 FIFO 的写入端，此时，可以在第一个终端的 `fifo-eof` 程序上看到捕捉到的 EOF；
    - `./fifo-eof` 是 FIFO 的读出端，`echo "Hello world">myfifo` 是 FIFO 的写入端，终止写入端，然后再次运行 `echo "Hello world">myfifo`，`fifo-eof` 可以再次从 FIFO 中收到信息；
    - 不管在打开 FIFO 时是阻塞状态还是非阻塞状态，当 FIFO 的写入端关闭时，读出端都会收到 EOF。

## 7 使用流式文件操作 FIFO 的一些差异
* 使用流式文件的操作函数(`fopen()/fread()/fwrite()/fclose()`等)是可以操作 FIFO 的，这个在前面的实例中已经有演示，但是与低级文件I/O的系统调用比是有一些差异的，本节将讨论已经发现的一些差异；
* 使用流式文件的方式从 FIFO 中读出数据时，如果遇到 EOF，必须要关闭 FIFO，然后再次打开，才能继续从 FIFO 中读取数据；
    - [fifo-server.c][src01] 这个实例中，演示了流式文件方式从 FIFO 中读取数据时，遇到 EOF 需要再次打开 FIFO 的过程；
    - 读者可以尝试修改这个程序，在遇到 EOF 后不关闭 FIFO，继续读取数据，是读不出数据的；
    - 但是使用低级文件 I/O (`open()/read()/write()/close()`)操作 FIFO，在遇到 EOF 时，则不需要关闭 FIFO；
    - [fifo-eof.c][src04] 这个实例中，演示了使用低级文件 I/O 从 FIFO 中读取数据时，遇到 EOF 无需关闭 FIFO 的过程；
* 当使用流式文件的方式向 FIFO 中写入数据时，因为流式文件缓存的原因，写入的数据可能不会马上进入到 FIFO 中；
    - 如果希望写入的数据立即进入 FIFO 的话，应该在调用写入函数(`fputs()/fwrite()`等)之后，立即调用 `fflush()`，该函数的意义在于强制将缓冲区中的数据写入到指定的流式文件中；
    - 使用低级文件 I/O 向 FIFO 中写入数据则没有这个问题；
* 当使用流式文件的方式向 FIFO 中写入数据时，如果 FIFO 的读出端关闭，写入函数仍然会返回成功；
    - 尽管函数调用返回成功，但向 FIFO 写入的数据会被忽略，再次打开的读出端无法读出这些数据；
    - 和使用低级文件 I/O 写入 FIFO 时一样，会捕捉到一个 SIGPIPE 信号，所以使用流式文件的方式向 FIFO 写入数据时，要靠捕捉 SIGPIPE 信号来判断写入失败；
    - 使用低级文件 I/O 写入 FIFO 时，如果 FIFO 读出端已关闭，写入函数(`write()`)会报告错误，这使得我们可以考虑不去捕捉 SIGPIPE 信号。

## 8 后记
* 本文涉及的所有观点均编有测试程序进行测试，限于篇幅，不能都列在文章中；
* 以本文提供的四个范例已经足够衍生出很多的测试程序：
    - [fifo-server.c][src01] - 是使用流式文件的方式从 FIFO 中读数据的，可以尝试将 `fopen()/fclose()` 放在循环外面，可以验证使用流式文件的方式读取 FIFO 时遇到 EOF 需要重新打开 FIFO 的观点；
    - [fifo-server.c][src01] - 可以将这个程序改成使用低级文件 I/O 读取 FIFO 的方式，以比较流式文件的操作函数和低级文件 I/O 的操作函数之间的差异；
    - [fifo-client.c][src02] - 可以加一个循环，使程序每隔 5 秒向 FIFO 写入一次数据，同时去掉 `fflush()`，可以验证流式文件的缓存对 FIFO 写入的影响；
    - [fifo-client.c][src02] - 可以将其改成使用低级文件 I/O 写入 FIFO 的方式，以比较流式文件的操作函数和低级文件 I/O 的操作函数之间的差异；
    - [fifo-sigpipe.c][src03] - 可以将其改为使用流式文件方式向 FIFO 写入数据，验证一下在读出端关闭时，`fputs()` 仍然返回成功的情况，同时体会一下如何在流式文件方式下捕捉 SIGPIPE 信号；
* 本质上说，命名管道和匿名管道在内核中的实现基本是一样的，所以，命名管道其实也是半双工的，要建立一个全双工的命名管道只需建立两条管道即可；可以参考我的另一篇介绍匿名管道的文章[《IPC之一：使用匿名管道进行父子进程间通信的例子》][article01]
* 命名管道如果不从文件系统中删除，会一直存在下去，这点和匿名管道有很大的区别，删除一个命名管道和删除一个普通文件无异；
* 为了让所有进程都可以访问到建立的命名管道，通常会把一个命名管道建在 `/tmp/` 下，好处有两点：
    1. 这个目录的权限是 `0777`，所有的用户可进行读写；
    2. 这个目录是一个内存文件系统，并不在物理磁盘上，所以重新启动后你建立的命名管道也会消失，减少了维护。
* 与匿名管道一样，Linux 下命名管道的缓冲区长度也是有限制的，最大长度为 4096 字节，定义在头文件：`linux/limits.h` 中：
    ```C
    #define PIPE_BUF        4096	/* # bytes in atomic write to a pipe */
    ```
* 向 FIFO 中写入数据的长度不超过这个阈值，将是一个原子操作；
* 与匿名管道不同，由于命名管道在物理文件系统上建立了一个节点，所以其生命周期不会随着使用它的所有进程的终止而终止，只要命名管道没有被删除，可以一直使用下去。
* 与匿名管道一样，命名管道不仅可以传输字符串，也可以传输二进制数据的，也就是说，可以把一个结构完整第通过管道进行传输。

* 有关进程间通信(IPC)的的其它文章：
    - [IPC之一：使用匿名管道进行父子进程间通信的例子][article01]
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

[src01]: https://whowin.gitee.io/sourcecodes/100011/fifo-server.c
[src02]: https://whowin.gitee.io/sourcecodes/100011/fifo-client.c
[src03]: https://whowin.gitee.io/sourcecodes/100011/fifo-sigpipe.c
[src04]: https://whowin.gitee.io/sourcecodes/100011/fifo-eof.c


[img01]: https://whowin.gitee.io/images/100011/screenshot-ls-fifo.png
[img02]: https://whowin.gitee.io/images/100011/screenshot-of-umask.png
[img03]: https://whowin.gitee.io/images/100011/screenshot-fifo-server.png

