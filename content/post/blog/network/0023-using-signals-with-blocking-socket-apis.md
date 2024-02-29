---
title: "使用signal中止阻塞的socket函数的应用实例"
date: 2024-01-25T16:43:29+08:00
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
  - accept
  - signal

draft: false
#references: 
# - [Example: Using signals with blocking socket APIs](https://www.ibm.com/docs/en/i/7.1?topic=designs-example-using-signals-blocking-socket-apis)

postid: 180023
---

在 socket 编程中，有一些函数是阻塞的，为了使程序高效运行，有一些办法可以把这些阻塞函数变成非阻塞的，本文介绍一种使用定时器信号中断阻塞函数的方法，同时介绍了一些信号处理和定时器设置的编程方法，本文附有完整实例的源代码，本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文不适合 Linux 编程的初学者阅读。

<!--more-->

## 1 前言
* 在 socket 编程中，阻塞还是不阻塞是经常要考虑的问题，`accept()`、`recv()` 等一些函数都是阻塞函数，阻塞函数有时会给程序带来麻烦；
* 使用 `select()` 或者 `poll()` 监视 `socket` 描述符可以有效地避免诸如 `accept()`、`recv()` 等函数的阻塞带来的麻烦；
* 下面这段代码是使用 select() 避免阻塞的示例：
    ```C
    int sockfd = socket(AF_INET, SOCK_STREAM , 0);
    ......
    fd_set fds;
    FD_ZERO(fd_set);
    FD_SET(sockfd, &fds);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (select(sockfd + 1, &fds, NULL, NULL, &tv)) {
        if (FD_ISSET(sockfd, &fds)) {
            ......
        }
    }
    ```
* 下面这段代码是使用 poll() 避免阻塞的示例：
    ```C
    int sockfd = socket(AF_INET, SOCK_STREAM , 0);
    ......
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 5000)) {
        if (pfd.revents & POLLIN) {
            ...... 
        }
    }
    ```
* 使用 `ioctl()` 将一个 socket 设置为非阻塞模式也是解决 socket 函数阻塞的方法之一；
* 下面代码使用 ioctl() 将 socket 设置为非阻塞模式：
    ```C
    int sockfd = socket(AF_INET, SOCK_STREAM , 0);
    int on = 1;
    ioctl(sockfd, FIONBIO, (char *)&on);
    ......
    ```
* 下面这段代码使用 fcntl() 将 socket 设置为非阻塞模式，与 ioctl() 是等效的：
    ```C
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    ......
    ```
* 如果将 `socket` 设置为非阻塞模式，`socket` 阻塞函数将立即返回，给出一个错误代码 `EAGAIN`，所以代码要写成下面这样：
    ```C
    int sockfd = socket(AF_INET, SOCK_STREAM , 0);
    int on = 1;
    ioctl(sockfd, FIONBIO, (char *)&on);
    ......
    int rc = 0;
    do {
        rc = accept(sockfd, NULL, NULL);
        usleep(100 * 1000);             // sleep 100 ms
    } while (rc == EAGAIN || rc == EINTR);
    ......
    ```
* 本文讨论使用信号(signal)避免 socket 阻塞函数产生阻塞的方法。

## 2 使用 signal 中止 socket 阻塞函数
* 实际上 socket 阻塞函数除了在非阻塞模式下会立即返回外，一旦当前进程收到信号(任何信号)时也会返回；
    - 在非阻塞模式下，socket 阻塞函数返回值为 -1 时，其 errno=EAGAIN;
    - 因为收到信号而中止的 socket 阻塞函数返回值为 -1， errno=EINTR；
* 基于此，可以设置一个定时器，Linux 的定时器会发出一个 SIGALRM 信号，该信号显然可以中止 socket 阻塞函数的阻塞状态；
* 设置定时器通常有两种方法，一种是使用 `alarm()`，另一种是使用 `setitimer()`；
* 下面代码使用 setitimer() 设置一个 5 秒的定时器：
    ```C
    struct itimerval new_value;
    new_value.it_value.tv_sec = 5;
    new_value.it_value.tv_usec = 0;
    new_value.it_interval.tv_sec = 5;
    new_value.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &new_value, NULL);
    ```
* 有关 `setitimer()` 的详细信息，可以查看在线手册 `man setitimer`，这里仅做简单介绍；
* `setitimer()` 的定义：
    ```C
    #include <sys/time.h>

    int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
    ```
* 其中 `struct itimeval` 的定义如下：
    ```C
    struct itimerval {
        struct timeval it_interval; /* Interval for periodic timer */
        struct timeval it_value;    /* Time until next expiration */
    };

    struct timeval {
        time_t      tv_sec;         /* seconds */
        suseconds_t tv_usec;        /* microseconds */
    };
    ```
* 调用 `setitimer()` 时，参数 which 表示计时方式，有三个可选值：
    - **ITIMER_REAL**：以实际时钟计时，计时器时间到产生 SIGALRM 信号；
    - **ITIMER_VIRTUAL**：以进程消耗的用户模式下 CPU 时间计时，计时器时间到产生一个 SIGVTALRM 信号；
    - **ITIMER_PROF**：以进程消耗的总 CPU 时间计时，计时器到时时产生一个 SIGPROF 信号；
* 调用 `setitimer()` 时，参数 `new_value` 用于设置定时器时间：
    - `new_value.it_value` 中有两个字段，如果两个字段均为 0，表示取消定时器，如果两个字段中有一个不为 0，则认为是设置了一个时间间隔；
    - `new_value.it_interval` 用于指定计时器的新间隔，当 `new_value.it_interval` 中的两个字段均为 0 时，表示这个计时器是单次的，其中有一个字段不为 0，则将被作为一个新的时间间隔在下次被指定；
* 调用 `setitimer()` 时，参数 `old_value` 用于返回之前的设置值(实际就是 `getitime()` 返回的值)，可以设置为 NULL；
* 函数 `setitimer()` 调用成功时返回 0，失败时返回 -1，`errno` 中为错误代码；

* `alarm()` 的使用比较简单，定义如下：
    ```C
    #include <unistd.h>

    unsigned int alarm(unsigned int seconds);
    ```
* `alarm()` 设置的时间到时，将产生一个 SIGALRM 信号，`alarm()` 是一个单次的定时器，所以使用 `alarm()` 设置的定时器只会响应一次，如果需要重复定时，可以在 SIGALRM 信号处理程序中再次执行 `alarm()` 重新设置定时；
* `alarm()` 和 `setitimer()` 使用的是同一个定时器，所以，这两个函数相互间会互相影响，建议在同一个进程中，应避免使用两种方法设置定时器；
* `alarm()` 在设置定时器时只能设置到秒的精度，而且只能使用实际时钟，相比较而言，`setitimer()` 可以设置精度更高的定时器，而且计时方式也比较多样，但复杂度略高；
* 不管是 `alarm()` 还是 `setitimer()`，在计时时间到时都是发出一个信号，所以编写信号处理程序是使用定时器时必须要做的工作，需要使用 `signal()` 设置信号处理程序；
* 下面程序设置了 SIGALRM 信号的信号处理程序：
    ```C
    void signal_handler(int sig) {
        signal(sig, signal_handler);
        
        printf("Catch the signal: %d\n",sig);
        ......
    }
    
    int main() {
        ......
        signal(SIGALRM, signal_handler);
        ......
    }
    ```
* signal() 函数设置的信号处理程序在信号产生后会被重置为默认处理程序，如果需要下次产生信号时继续使用当前处理程序，需要在信号处理程序中执行 signal() 重新设置，就像上面程序演示的那样；
* 下面这段程序使用 alarm() 设置了一个 5 秒的定时器，每 5 秒会产生一个 SIGALRM 信号：
    ```C
    #define _POSIX_SOURCE
    ......
    #include <unistd.h>
    ......
    void signal_handler(int sig) {
        signal(sig, signal_handler);
        
        printf("Catch the signal: %d\n",sig);
        ......
        alarm(5);
    }
    
    int main() {
        ......
        signal(SIGALRM, signal_handler);
        alarm(5);

        while (loop) {
            ......
        }
        ......
    }
    ```
* 下面这段代码使用 setitimer() 设置了一个 5 秒的定时器，每 5 秒会产生一个 SIGALRM 信号：
    ```C
    #define _POSIX_SOURCE
    ......
    #include <sys/time.h>
    ......
    void signal_handler(int sig) {
        signal(sig, signal_handler);
        
        printf("Catch the signal: %d\n",sig);
        ......
    }
    
    int main() {
        ......
        signal(SIGALRM, signal_handler);

        struct itimerval new_value;
        new_value.it_value.tv_sec = 5;
        new_value.it_value.tv_usec = 0;
        new_value.it_interval.tv_sec = 5;
        new_value.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &new_value, NULL);

        while (loop) {
            ......
        }
        ......
    }
    ```
* 除了编程上的区别外，还要注意 `alarm()` 需要的头文件是 `<unistd.h>`，而 `setitimer()` 需要的头文件是 `<sys/time.h>`；
* 关于系统调用中的阻塞函数在进程收到信号后会被中止的相关信息可以参考在线手册 `man 7 signal`，其中 `<Interruption of system calls and library functions by signal handlers>` 一节中详细介绍了那些阻塞函数可以被信号中止；
* 另外，阻塞函数被信号中止的功能是 POSIX 标准中的一部分，并不是 libc 默认支持的，所以在程序的开头要加上 `#include _POSIX_SOURCE`。

## 3 范例
* **源程序**：[nonblock-signal.c][src01](**点击文件名下载源程序，建议使用UTF-8字符集**)演示了使用信号使 socket 编程里的阻塞函数 `accept()` 每隔 5 秒钟中止一次的过程；
* 该范例不仅仅是处理了 SIGALRM 信号，还处理了 SIGINT 和 SIGQUIT 信号，旨在说明不仅仅是定时器产生的 SIGALRM 信号会中止阻塞函数，任何信号都会使阻塞函数中止；
* SIGQUIT 信号可以使用键盘 `ctrl + \` 产生，SIGINT 信号就是 `ctrl + c`；
* 为了程序可以正常退出，程序对 SIGINT 信号做了计数，当按下 `ctrl + c` 四次时，程序会正常退出；
* 因为一个 socket 阻塞函数可以被任意信号打断，被打断的函数会返回一个 EINTR 错误，所以在进行 socket 编程时，一定要处理 EINTR；
* 程序使用 常量 `_ALARM_FUNC` 控制采用哪种方式设置定时器，当常量 `_ALARM_FUNC` 已定义时，使用 `alarm()` 设置定时器，否则使用 `setitimer()` 设置定时器；
* 编译：`gcc -Wall -g nonblock-signal.c -o nonblock-signal`
* 运行：`./nonblock-signal`
* 运行截图：

    ![GIF of running nonblock-signal][img01]








## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[net_category]: https://blog.csdn.net/whowin/category_12180345.html


[src01]: https://whowin.gitee.io/sourcecodes/100023/nonblock-signal.c

[img01]: https://whowin.gitee.io/images/180023/nonblock-signal.gif

