---
title: "IPC之十四：使用libdbus通过select()接收D-Bus消息的实例"
date: 2023-12-25T16:43:29+08:00
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
  - "D-Bus"
  - IPC
  - select
  - DBusWatch
draft: false
#references: 

# - [Writing Resolver Clients](https://wiki.freedesktop.org/www/Software/systemd/writing-resolver-clients/)
#   - 通过 systemd-resolved 的总线 API 查找主机名和 DNS 记录
# - [org.freedesktop.resolve1系统服务文档](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.resolve1.html)
# - [D-Bus low-level public API](https://dbus.freedesktop.org/doc/api/html/group__DBus.html)
# - [DBusWatch and DBusTimeout examples](https://stackoverflow.com/questions/9378593/dbuswatch-and-dbustimeout-examples)
#   - D-Bus异步接收消息的例子
# - [Using the DBUS C API](https://www.matthew.ath.cx/misc/dbus)
#   - 有四个例子：receiving signals, sending a signal, listenning for method calls and calling the method.
# - [DebuggingDBus](https://wiki.ubuntu.com/DebuggingDBus)
#   - 不是编程，只是将如何用 dbus-monitor 去监视 D-Bus
# - [DBusWatch and DBusTimeout examples](https://stackoverflow.com/questions/9378593/dbuswatch-and-dbustimeout-examples)
#   - 里面的例子起了重要作用


postid: 100024
---


在『进程间通信』系列文章中前面已经有三篇关于D-Bus的文章，本文继续讨论D-Bus；libdbus抽象了实现IPC时实际使用的方式(管道、socket等)，libdbus允许在一个D-Bus连接上添加一个watch，通过watch对实际IPC中使用的文件描述符进行监视，本文讨论了如何在D-Bus连接中添加watch，如何使用在socket编程中常用的select从D-Bus返回的文件描述符中接收到D-Bus消息，本文给出了具体实例，附有完整的源代码；本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文不适合 Linux 编程的初学者阅读。

<!--more-->

## 1 基本概念
* 阅读本文前，建议先阅读前面的关于 D-Bus 的三篇文章：[《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11]、[《IPC之十二：使用libdbus在D-Bus上异步发送/接收信号的实例》][article12] 和 [《IPC之十三：使用libdbus通过D-Bus请求系统调用实现域名解析的实例》][article13]
* 在文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中介绍了 D-Bus 的一些基本概念，介绍了 libdbus 库的一些基本 API，在实例中使用 libdbus 库的 APIs 实现了客户进程向服务进程请求服务的过程；
* 在文章 [《IPC之十二：使用libdbus在D-Bus上异步发送/接收信号的实例》][article12] 中，我们介绍了 D-Bus 中信号的概念，在实例中使用 libdbus 库的 match 订阅了信号，并使用过滤器(filter)异步接收信号；
* 在文章 [《IPC之十三：使用libdbus通过D-Bus请求系统调用实现域名解析的实例》][article13] 中，我们介绍了如何通过 D-Bus 调用系统服务，有很多系统服务都有 D-Bus 接口，透过这个接口，应用程序可以调用系统服务，从而快速地实现一些复杂的功能；
* 在『进程间通信』系列文章中的前十篇文章中，介绍了许多 IPC 的方法，有管道、共享内存、UNIX DOMAIN SOCKET 等，其实 D-Bus 在实现 IPC 时也是用其中的一种方法实现的，但是 libdbus 将具体的实现抽象化了，所以，我们并不知道具体用的那种方法；
* D-Bus 通过 `dbus-daemon` 来管理总线，服务进程通过一种传统 IPC 方法与 `dbus-daemon` 进行通信，客户进程可能通过另一种 IPC 方式与 `dbus-daemon` 进行通信，`dbus-daemon` 对客户进程与服务进程之间的消息进行转发；
* 实际上，我们也确实无需知道具体的实现方法，在 Linux 系统下，有所谓"一切皆文件"的原则，不管是管道还是 socket，最终返回给应用程序的都是一个文件描述符(File descriptor)；
* 在进行 socket 编程时，服务端在处理多个连接请求时，常常会使用 select()，将正在侦听的多个 socket 的文件描述符放到一个'集'中，当'集'中的某个文件描述符有可读数据时，select() 就会返回，然后应用程序就可以对相应的文件描述符做出处理；
* 如果在 D-Bus 编程中也能通过 select() 来处理消息的话，将给编程带来很大方便，当应用程序中要处理 D-Bus 消息、socket 通信以及文件读写时，使用 select() 可以合并处理，这将大大提高程序的效率；

## 2 在D-Bus中与 watch 相关的函数介绍
* 添加 watch 的目的就是为了监视实际实现 IPC 的文件描述符，watch 添加成功才可以获取实际使用的文件描述符，从而实现在文件描述符一级的监视；
* libdbus 库的 API 调用手册：[D-Bus low-level public API](libdbus_api)
* 本节并不会介绍所有与 watch 相关的函数，仅介绍在本文实例中用到的函数；
* 使用 `dbus_bus_get()` 连接到 dbus-daemon，使用 `dbus_bus_request_name()` 获取总线名称都在以前的文章中做过介绍；
* <font size=5>函数 `dbus_connection_set_watch_functions()`</font> 
    - 设置 watch 相关的函数

    ```C
    dbus_bool_t dbus_connection_set_watch_functions(DBusConnection *connection,
                                                    DBusAddWatchFunction add_function,
                                                    DBusRemoveWatchFunction remove_function,
                                                    DBusWatchToggledFunction toggled_function,
                                                    void *data,
                                                    DBusFreeFunction free_data_function)	    
    ```

    - connection：通过 `dbus_bus_get()` 获取的连接
    - add_function：添加 watch 时执行的函数
    - remove_function：删除 watch 时执行的函数
    - toggled_function：切换 watch 时执行的函数
    - data：传递给 `add_function()`、`remove_function()` 和 `toggled_function()` 的用户参数；
    - free_data_function：释放 data 的函数；
    -------
* <font size=5>函数 `DBusAddWatchFunction add_function`</font>
    - **`DBusAddWatchFunction`** 的定义为：`typedef dbus_bool_t(* DBusAddWatchFunction) (DBusWatch *watch, void *data)`
    - 所以 `add_function()` 函数的格式应该为：

        ```C
        dbus_bool_t add_function(DBusWatch *watch, void *data) {
            ........
        }
        ```
    - 当调用 `dbus_connection_set_watch_functions()` 时，`add_function()` 将被自动调用，`watch` 将由系统传递给函数，`data` 是传递给 `add_function()` 的用户数据，在调用 `dbus_connection_set_watch_functions()` 时由参数 `data` 指定；
    - watch 的数据类型是 DBusWatch，实际上就是一个结构，C 语言不是一个面向对象的语言，当要实现对象时，通常都是使用数据结构，一般情况下，无需了解其数据结构的具体情况；
    - 系统传入的 `watch` 要保存好，因为在应用程序的其它地方可能需要它；
    - 用户传入的 `data` 可以是任何变量或者结构，在实例中会有具体的做法；
    - 在 `add_function()` 中通常需要判断 watch 是否被启用，然后通过调用 `dbus_watch_get_unix_fd()` 获取被监视的文件描述符；
    - `add_function()` 返回 FALSE 时表示内存不够，所以通常情况下即便出现错误也不要返回 FALSE；
    --------
* <font size=5>函数 `DBusRemoveWatchFunction remove_function`</font>
    - **`DBusRemoveWatchFunction`** 的定义为：`typedef void(* DBusRemoveWatchFunction) (DBusWatch *watch, void *data)`
    - 所以 `remove_function()` 函数的格式应该为：

        ```C
        void remove_function(DBusWatch *watch, void *data) {
            ........
        }
        ```
    - 在一个 watch 被禁用时，通常需要调用 `remove_function()`，一般情况下，当一个 watch 被禁用时系统会调用 `toggled_function()`，然后在 `toggled_function()` 中调用 `remove_funcion()`；
    - data 参数与 `add_function()` 中的 data 参数一样，在设置时指定；
    -------
* <font size=5>函数 `DBusWatchToggledFunction toggled_function`</font>
    - **`DBusWatchToggledFunction`** 的定义为：`typedef void(* DBusWatchToggledFunction) (DBusWatch *watch, void *data)`
    - 所以 `toggled_function()` 函数的格式应该为：

        ```C
        void toggled_function(DBusWatch *watch, void *data) {
            ........
        }
        ```
    - 当 watch 被启用或者禁用时，系统通过调用 `toggled_function()` 来通知应用程序，系统将发生变化的 watch 作为参数传给 `toggled_function()`；
    - data 参数与 `add_function()` 中的 data 参数一样，在设置时指定；
    - 在 `toggled_function()` 中，通常需要通过 `dbus_watch_get_enabled()` 判断 watch 是启用还是禁用，如果是启用，调用 `add_function()`，如果是禁用则调用 `remove_function()`；
    -----
* <font size=5>函数 `dbus_watch_get_enabled()`</font>
   - 检查 watch 是否已启用

    ```C
    dbus_bool_t dbus_watch_get_enabled(DBusWatch *watch);	
    ```
    - 返回 TRUE 表示 watch 已经启用，FALSE 表示 watch 已经禁用；
    ---------
* <font size=5>函数 `dbus_watch_get_unix_fd()`</font>
    ```C
    int dbus_watch_get_unix_fd(DBusWatch *watch);	
    ```
    - 返回 watch 监视的文件描述符，这个描述符可能是管道、socket，也可能是其它类型的描述符；
    -------
* <font size=5>函数 `dbus_watch_get_flags()`</font>
    ```C
    unsigned int dbus_watch_get_flags(DBusWatch *watch);
    ```
    - 获取 watch 对文件描述符的监视内容；
    - 返回值可能有两个标志：`DBUS_WATCH_READABLE` 和 `DBUS_WATCH_WRITABLE`；
    - `DBUS_WATCH_READABLE` 表示对文件描述符的'读'进行监视；
    - `DBUS_WATCH_WRITABLE` 表示对文件描述符的'写'进行监视；
    - 默认 watch 会监视文件描述符的错误状态(DBUS_WATCH_ERROR)和挂起状态(DBUS_WATCH_HANGUP)，所以这两个标志不会在这个函数的返回值中出现；
    ---------
* <font size=5>函数 `dbus_watch_handle()`</font>
    ```C
    dbus_bool_t dbus_watch_handle(DBusWatch *watch,
                                  unsigned int flags);	
    ```
    - 当 watch 所监视的文件描述符有可读取的数据时，需要调用此函数通知 libdbus 库；
    - 当被监视的文件描述符中有数据可以读取时，调用函数 `dbus_watch_handle()` 将把数据从从文件描述符中读出，解析为 D-Bus 的消息，并把消息放入消息队列，然后才可以用 libdbus 库的 API 从队列中读出消息并进行处理，如果不调用 `dbus_watch_handle()`，则文件描述符上将一直处于有数据可读状态；
    - `dbus_watch_handle()` 返回为 FALSE 时表示执行失败，执行失败的原因只有一个，就是内存不够，如果忽略返回的 FALSE，通常不会有致命问题，直至有足够内存时，`dbus_watch_handle()` 便会返回 TRUE；
    - 参数 watch 为使用 `dbus_connection_set_watch_functions()` 设置的 watch，也就是系统在调用 `add_function()` 时传递过来的 watch；
    - 参数 flags 与函数 `dbus_watch_get_flags()` 返回的值含义相同，`DBUS_WATCH_READABLE` 表示文件描述符中有可读数据，`DBUS_WATCH_WRITABLE` 表示文件描述符中有可写数据；
    ---------
* <font size=5>函数 `dbus_connection_get_dispatch_status()`</font>
    ```C
    DBusDispatchStatus dbus_connection_get_dispatch_status(DBusConnection *connection);	
    ```
    - 获取接收消息队列的当前状态；
    - 其返回值 DBusDispatchStatus 是一个枚举类型，其值有三个：
        1. `DBUS_DISPATCH_DATA_REMAINS`：表示接收消息队列中可能有消息；
        2. `DBUS_DISPATCH_COMPLETE`：表示接收消息队列为空；
        3. `DBUS_DISPATCH_NEED_MEMORY`：表示可能有消息，但如果没有更多内存，则无法确定是否有消息；
    - 当返回状态为 `DBUS_DISPATCH_DATA_REMAINS` 时，表示接收队列中有消息，或者正在将原始数据转换为 D-Bus 的消息，此时会返回 `DBUS_DISPATCH_DATA_REMAINS` 状态，但由于消息还没有解析完毕，所以队列中还没有完整的消息；
    ----------
* <font size=5>函数 `dbus_connection_borrow_message()`</font>
    ```C
    DBusMessage *dbus_connection_borrow_message(DBusConnection *connection);	
    ```
    - 这个函数很有意思，它从消息队列中"借用"一个消息，"借用"消息和读取消息的区别在于"借用"的消息不会从消息队列中删除，而读取的消息会从消息队列中删除；
    - 当一个消息被"借用"以后，这条消息尽管还在队列中，但会被锁定，其它进程(线程)将无法再读取该消息，所以"借用"的消息要么尽快"还"回来，要么尽快读取出来，否则会影响消息的处理；
    - 之所以要"借用"消息，是因为不能确定这条消息是否应该由当前程序处理，所以要"借用"出来做一下判断，如果不该当前程序处理，则立即"还"回去，否则应该立即将该消息读出来；
    --------
* <font size=5>函数 `dbus_connection_return_message()`</font>
    ```C
    void dbus_connection_return_message(DBusConnection *connection,
                                        DBusMessage *message);
    ```
    - 该函数将由函数 `dbus_connection_borrow_message()` "借用"的消息"还"回到消息队列中；
    - 参数 message 是要"还"回去的消息；

    --------
* <font size=5>函数 `dbus_connection_steal_borrowed_message()`</font>
    ```C
    void dbus_connection_steal_borrowed_message(DBusConnection *connection,
                                                DBusMessage *message);
    ```
    - 该函数将由函数 `dbus_connection_borrow_message()` "借用"的消息从消息队列中读取出来；
    - 参数 message 为要读出的消息，该函数执行完成后，该消息将从消息队列中被删除；
    --------
* 其它在实例中用到的函数已经在前面几篇关于 D-Bus 的文章中做过介绍，这里不再赘述；

## 3 使用 select() 处理 D-Bus 消息的步骤
* 使用 `dbus_bus_get()` 连接到 dbus-daemon，使用会话总线；
* 使用 `dbus_bus_request_name()` 设置总线名称，以便客户端可以访问到；
* 使用 `dbus_connection_set_watch_functions()` 添加一个 watch，考虑将一个结构传给设置的 watch 函数，用于存储 watch 的相关信息；
    ```C
    struct watch_struct {
        DBusWatch *watched_watch;   // Currently used watch
        int watched_rd_fd;          // Readable file descriptor being watched
        int watched_wr_fd;          // Writable file descriptor being watched
    }watch_fds;

    dbus_connection_set_watch_functions(conn, 
                                        add_watch, 
                                        remove_watch,
                                        toggle_watch,
                                        (void *)&watch_fds, NULL))
    ```

* 为添加 watch 编写三个函数：`add_watch()`、`remove_watch()` 和 `toggle_watch()`；

    > `add_wath()` 中，首先使用 `dbus_watch_get_enabled()` 判断添加的 watch 是否已经启用，如未启用则无法继续，然后使用 `dbus_watch_get_unix_fd()` 获取被监视的文件描述符，使用 `dbus_watch_get_flags()` 判断被监视的这个描述符是可读还是可写，并保存好相关信息；

    ```C
    static dbus_bool_t add_watch(DBusWatch *w, void *data) {
        if (!dbus_watch_get_enabled(w)) {               // Returns whether a watch is enabled or not.
            return TRUE;
        }

        struct watch_struct *watch_fds = (struct watch_struct *)data;
        int fd = dbus_watch_get_unix_fd(w);             // Returns a UNIX file descriptor to be watched, 
                                                        // which may be a pipe, socket, or other type of descriptor.
        unsigned int flags = dbus_watch_get_flags(w);   // Gets flags from DBusWatchFlags indicating what conditions 
                                                        // should be monitored on the file descriptor.
        if (flags & DBUS_WATCH_READABLE) {              // the watch is readable
            watch_fds->watched_rd_fd = fd;
        }
        if (flags & DBUS_WATCH_WRITABLE) {              // the watch is writable
            watch_fds->watched_wr_fd = fd;
        }
        watch_fds->watched_watch = w;

        return TRUE;
    }
    ```

    > `remove_watch()` 的主要作用就是在禁用 watch 时可以释放在执行 `add_watch()` 时所占用的资源；

    ```C
    static void remove_watch(DBusWatch *w, void *data) {
        struct watch_struct *watch_fds = (struct watch_struct *)data;

        watch_fds->watched_watch = NULL;
        watch_fds->watched_rd_fd = 0;
        watch_fds->watched_wr_fd = 0;
    }
    ```

    > `toggle_watch()` 首先使用 `dbus_watch_get_enabled()` 判断系统传过来的 watch 时禁用还是启用状态，如果是禁用状态，调用 `remove_watch()`，如果是启用状态，调用 `add_watch()`；

    ```C
    static void toggle_watch(DBusWatch *w, void *data) {
        if (dbus_watch_get_enabled(w)) {
            add_watch(w, data);
        } else {
            remove_watch(w, data);
        }
    }
    ```
* 初始化 `select()` 的文件描述符集，设置 `select()` 超时时间，执行 `select()`；
    ```C
    fd_set readfds;            // Reading fd set, Writing fd set and ... fd set
    int max_fd = 0;
    int activity;

    FD_ZERO(&readfds);
    if ((watch_fds.watched_watch != NULL) && (watch_fds.watched_rd_fd > 0)) {
        FD_SET(watch_fds.watched_rd_fd, &readfds);      // set watched FD into fd set
        max_fd = watch_fds.watched_rd_fd;
    } else {
        exit(EXIT_FAILURE);
    }
    struct timeval timeoutval = {5, 0};     // timeout is 5 seconds
    activity = select(max_fd + 1, &readfds, NULL, NULL, &timeoutval);
    ```
* 文件描述符上有可读数据时，调用 `dbus_watch_handle()` 和消息接收函数 `select_recv()`
    ```C
    if (FD_ISSET(watch_fds.watched_rd_fd, &readfds)) {
        if (dbus_watch_handle(watch_fds.watched_watch, DBUS_WATCH_READABLE)) {
            select_recv(conn);
        }
    }
    ```
    - 调用 `dbus_watch_handle()` 的目的是把文件描述符上的数据读出并解析为 D-Bus 消息放入消息队列；
    - 如果 `dbus_watch_handle()` 返回 FALSE 表示需要更多的内存才能解析数据，此时文件描述符仍然处于有可读数据的状态，所以下次执行 `select()` 仍可以处理数据，不会有灾难后果，也可以在 `dbus_watch_handle()` 返回 FALSE 后做一个短延时(比如100ms)；

* 接收消息程序 `select_recv()`
    - 这个程序是需要自己编写的，其目的就是接收、处理消息并向客户端发送回复消息；
    - 首先使用 `dbus_connection_get_dispatch_status()` 检查状态，如果不是 `DBUS_DISPATCH_DATA_REMAINS` 表示消息队列中没有消息，则不能继续进行；
    - 这个接收函数的通常做法是先使用 `dbus_connection_borrow_message()` 借用消息，然后根据其消息类型、对象路径、接口名称等将消息分发给其它函数，需要丢弃的消息则将其读出直接丢弃；
    - 本文实例中仅留下了需要处理的消息，实际上丢弃了所有其它的消息；
    - 在这个函数中要处理到所有消息类型的消息，自行处理、分发到其他函数处理或者丢弃，不处理的话无法处理消息队列中的下一条消息；

## 3 实例
* **源程序**：[dbus-select.c][src01] (**点击文件名下载源程序，建议使用UTF-8字符集**)演示了使用 libdbus 通过 `select()` 接收方法调用消息的过程；
* 该程序与文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中的实例 [dbus-methods.c][src02] 完成相同的任务，其中客户端进程的程序完全一样；
* 与 [dbus-methods.c][src02] 不同的是服务端进程，[dbu-select.c][src01] 在服务端进程为 D-Bus 连接添加了一个 watch，使用 `dbus_watch_get_unix_fd()` 从 watch 中获取实际使用的文件描述符；
* 当文件描述符上有数据可以读取时，调用接收程序读取消息并处理消息，然后向客户端发出回复消息；
* 该程序是个多进程程序，建立了一个服务端进程和若干个(默认为 3 个)客户端进程，服务端进程执行的函数为：`dbus-server()`，客户端进程执行的函数为：`dbus-client()`；
* 客户端进程向服务端进程的三个方法分别发出请求，服务端进程一一做出回复，客户端进程在调用完 `quit` 方法并收到服务端回复后主动结束进程；
* 服务端进程在收到所有客户端进程发来的对 `quit` 方法的请求消息并做出回复后主动退出进程；

* 编译：```gcc -Wall -g dbus-select.c -o dbus-select `pkg-config --libs --cflags dbus-1` ```
* 有关 `pkg-config --libs --cflags dbus-1` 可以参阅文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中的简要说明；
* 运行：`./dbus-select`
* 运行截图：

    ![Screenshot of dbus-select][img01]








## **欢迎订阅 [『进程间通信专栏』](https://blog.csdn.net/whowin/category_12404164.html)**



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
[article10]: https://whowin.gitee.io/post/blog/linux/0020-ipc-using-files/
[article11]: https://whowin.gitee.io/post/blog/linux/0021-ipc-using-dbus/
[article12]: https://whowin.gitee.io/post/blog/linux/0022-dbus-asyn-process-signal/
[article13]: https://whowin.gitee.io/post/blog/linux/0023-dbus-resolve-hostname/


<!-- for CSDN
[article01]: https://blog.csdn.net/whowin/article/details/132171311
[article02]: https://blog.csdn.net/whowin/article/details/132171930
[article03]: https://blog.csdn.net/whowin/article/details/132172172
[article04]: https://blog.csdn.net/whowin/article/details/134869490
[article05]: https://blog.csdn.net/whowin/article/details/134869636
[article06]: https://blog.csdn.net/whowin/article/details/134939609
[article07]: https://blog.csdn.net/whowin/article/details/135015196
[article08]: https://blog.csdn.net/whowin/article/details/135074991
[article09]: https://blog.csdn.net/whowin/article/details/135143545
[article10]: https://blog.csdn.net/whowin/article/details/135212050
[article11]: https://blog.csdn.net/whowin/article/details/135281195
[article12]: https://blog.csdn.net/whowin/article/details/135332257
[article13]: https://blog.csdn.net/whowin/article/details/135332658
-->


[dbus_webpage]: https://www.freedesktop.org/wiki/Software/dbus/
[libdbus_api]: https://dbus.freedesktop.org/doc/api/html/group__DBus.html
[dbus_specification]: https://dbus.freedesktop.org/doc/dbus-specification.html
[systemd-resolved_api]: https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.resolve1.html


[src01]: https://whowin.gitee.io/sourcecodes/100024/dbus-select.c
[src02]: https://whowin.gitee.io/sourcecodes/100021/dbus-methods.c

[img01]: https://whowin.gitee.io/images/100024/dbus-select.gif

