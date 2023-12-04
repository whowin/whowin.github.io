---
title: "IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例"
date: 2023-12-01T16:43:29+08:00
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
  - libdbus
draft: false
#references: 
# - [What Is Inter-Process Communication In Linux?](https://www.scaler.com/topics/ipc-in-linux/)
#   - 里面列举了大量的IPC方法
# - [DBUS介绍与Linux C实例](https://blog.csdn.net/ZengYinning/article/details/128188252)
# - [D-Bus官网](https://www.freedesktop.org/wiki/Software/dbus/)
# - [D-Bus low-level public API](https://dbus.freedesktop.org/doc/api/html/group__DBus.html)
# ----------------------------------------------------------------------------------------------
# - [Error reporting](https://dbus.freedesktop.org/doc/api/html/group__DBusErrors.html)
#   - void dbus_error_init (DBusError *error), 
#   - void dbus_error_free (DBusError *error),
#   - dbus_bool_t dbus_error_is_set(const DBusError *error)
# - [Message bus APIs](https://dbus.freedesktop.org/doc/api/html/group__DBusBus.html)
#   - DBusConnection *dbus_bus_get(DBusBusType type, DBusError *error)
#   - int dbus_bus_request_name(DBusConnection *connection, const char *name, unsigned int flags, DBusError *error)
# - [DBusConnection](https://dbus.freedesktop.org/doc/api/html/group__DBusConnection.html)
#   - dbus_bool_t dbus_connection_read_write_dispatch(DBusConnection *connection, int timeout_milliseconds)
#   - DBusMessage *dbus_connection_pop_message(DBusConnection *connection)
#   - void dbus_connection_flush(DBusConnection *connection)
#   - dbus_bool_t dbus_connection_send(DBusConnection *connection, DBusMessage *message, dbus_uint32_t *serial)
#   - void dbus_connection_unref(DBusConnection *connection)
# - [DBusMessage](https://dbus.freedesktop.org/doc/api/html/group__DBusMessage.html)
#   - dbus_bool_t dbus_message_is_method_call(DBusMessage *message, const char *iface, const char *method)
#   - dbus_bool_t dbus_message_get_args(DBusMessage *message, DBusError *error, int first_arg_type,...)
#   - DBusMessage *dbus_message_new_method_return(DBusMessage *method_call)
#   - void dbus_message_iter_init_append(DBusMessage *message, DBusMessageIter *iter)
#   - dbus_bool_t dbus_message_has_path(DBusMessage *message, const char *path)
#   - dbus_bool_t dbus_message_iter_append_basic(DBusMessageIter *iter, int type, const void *value)
# --------------------------------------------------------------------------------------------------------
# - [Dbus组成和原理](https://www.cnblogs.com/klb561/p/9058282.html)
# - [D-Bus实例介绍](http://just4coding.com/2018/07/31/dbus/)
# - [Using the DBUS C API](http://www.matthew.ath.cx/articles/dbus)
#   - 里面有例子
# - [D-Bus Tutorial](https://www.softprayog.in/programming/d-bus-tutorial)
# - [D-Bus integration in Emacs](https://www.gnu.org/software/emacs/manual/html_node/dbus/index.html)
# - [D-Bus and Bluez](https://ukbaz.github.io/howto/python_gio_1.html)
# - [D-Bus : Transmit a Data Array in Simple and Useful Form](http://gaiger-programming.blogspot.com/2015/08/d-bus-simple-and-useful-example-to-send.html)
#   - 里面有一个很不错的例子

postid: 100021
---


IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本 IPC 系列文章的前十篇介绍了几乎所有的常用的 IPC 方法，每种方法都给出了具体实例，本文介绍在 Linux 下 IPC 的另外一种实现，D-Bus，D-Bus 是一种用于进程间通信的消息总线系统，它提供了一个可靠且灵活的机制，使得不同进程之间能够相互通信，本文给出了使用 D-Bus 进行基本 IPC 的实例，并附有完整的源代码；本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文不适合 Linux 编程的初学者阅读。

<!--more-->

## 1 D-Bus 概述
* 本文并不打算详细描述 D-Bus 的概念以及数据结构和调用方法，力求以实用为主，通过实例介绍 D-bus 的基本应用；
* D-Bus 是Linux 和类 unix 系统下一种进程间通信机制；
* D-Bus 和其它常用的 IPC 方法不同，它采用总线的形式实现进程间的通信；
* D-Bus 的基本原理并不复杂，见下图：

    ![D-Bus Overview][img01]

* 需要相互进行通信的进程都连接到 D-Bus 上，然后通过总线的消息转发实现进程间的消息传递；
* 当 Process B 要向 Process C 发送消息时，Process B 首先要把消息发送到 D-Bus 上，D-Bus 再把消息转发到 Process C 上；
* D-Bus 总线并不是像 PCI 总线那样的一条物理总线，而是一条逻辑总线；
* D-Bus 其实就是一个特殊的应用程序，它接受来自多个其他应用程序的连接，并在各个应用程序之间转发消息，从而实现进程间通信；
* 这个特殊的应用程序又被称为 dbus-daemon，dbus-daemon 在 Ubuntu(我这里是 Ubuntu 20.04) 下是默认安装的；
* 在 Ubuntu 下，dbus-daemon 开机是默认启动的，使用 `ps aux|grep dbus-daemon` 可以看到正在运行的 `dbus-daemon`；
* 可以使用 `man dbus-daemon` 在线手册了解 dbus-daemon 的使用方法；
* [D-Bus 官网](dbus_webpage)：https://www.freedesktop.org/wiki/Software/dbus/
* D-Bus 的最底层是 D-Bus 总线协议，在 [D-Bus 规范][dbus_specification] 中有详细描述；

* D-Bus 具有分层架构：
    - libdbus 库，是 D-Bus 总线协议的实现，它为两个进程之间的通信提供了 C 语言接口；
    - 消息总线守护进程(dbus-daemon)，dbus-daemon 是使用 libdbus 构建的，允许多个进程链接到 dbus_darmon，dbus-daemon 可以将消息从一个进程转发到多个其他进程；
    - 基于特定应用程序框架的包装库，例如 libdbus-glib 和 libdbus-qt，这些包装库简化了 D-Bus 编程的细节，是大多数程序员使用的 API，libdbus 的设计初衷主要是作为这些包装库的低级后端，其大部分 API 对包装库实现有用，对直接客户显得并不是很友好；

* libdbus 只支持一对一的连接，就像网络原始套接字(raw socket)一样；
* libdbus 的连接是基于消息的，换句话说就是在 dbus 的连接上只能发送消息，不能像 TCP 通信那样发送字节流；
* libdbus 连接上发送的消息有标识消息类型的报头和包含数据的正文，libdbus 还抽象了实际使用的传输方式(socket 等)，并可以处理诸如身份验证之类的细节。
* 尽管 libdbus 对直接用户并不友好，但是如果仅仅是要完成进程之间的通信，也并没有多少难度，本文的实例将使用 libdbus 库实现进程间通信；
* libdbus 库的数据结构和调用方法均有一定的复杂性，本文不会介绍 libdbus 所有的数据结构和函数，仅讨论与实例相关的部分；
* libdbus 库的 API 调用手册：[D-Bus low-level public API](libdbus_api)
* 本文将从一个具体的实例开始介绍如何使用 libdbus 完成进程间通信。

## 2 D-Bus 基本概念
* 这一节可能会比较晦涩，所以我们会增加一些代码片段以便可以直接地感受到其具体的使用方法，不明白的地方不必深究，与下一节一起看可能就明白了；
* 这一节中代码片段中涉及到的函数调用后面会有详细介绍，这里不必深究；
* D-Bus 总线分为两种类型：**系统总线**(System Bus)和**会话总线**(Session Bus)
    - **系统总线**(System Bus)：用于系统应用程序和用户进程之间通信；
    - **会话总线**(Session Bus)：用于桌面环境中应用程序之间交换数据；
* 当两个用户进程进行通信时，通常应该选择会话总线；
* D-Bus 的基本概念是基于 Client/Server 模式的，服务端程序连接到 D-Bus 上，建立一个新的连接并成为该连接的拥有者，向客户端提供服务，客户端连接到总线并向服务端请求服务；
* 下面代码使用 libdbus 库建立一个会话总线的连接：
    ```C
    ......
    DBusError dbus_error;
    DBusConnection *conn;

    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
    ......
    ```
* 前面提到过，D-Bus 总线不是物理总线，而是逻辑总线，实际上就是一个特殊的应用程序，被成为 dbus-daemon，连接到 D-Bus 上与连接到 dbus-daemon 上是相同的含义；
* 当一个应用进程连接到 D-Bus(dbus-daemon) 上时，dbus-daemon 会为每个新连接随机分配一个总线名称，这个过程与 DNS 的动态域名解析类似；
    - 随机分配的总线名称与 DNS 为新连接随机动态分配的 IP 地址类似，D-Bus 总线名称是以 ":" 开头的一组由 "." 分隔的数字组成，比如：`:1.14`
    - 总线名称是唯一性的标识，在整个 D-Bus 的生命周期内都不会改变，即便该连接已经关闭，并且有新的连接产生时，也不会占用已经分配过的总线名称；
* 下面的代码片段使用 libdbus 库查看一个连接的总线名称；
    ```C
    ......
    DBusError dbus_error;
    DBusConnection *conn;
    const char *unique_bus_name;

    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    unique_bus_name = dbus_bus_get_unique_name(conn);
    if (unique_bus_name) printf("The bus name is %s\n", unique_bus_name);
    ......
    ```

* 但是其它应用程序并不知道这个随机分配的总线名称，所以很难通过总线名称访问一个连接下的对象，通常要指定一个固定的名称与这个连接绑定，与动态 DNS 解析中将域名解析到一个动态 IP 上相似；
* 这个固定的名称通常使用反向域名的命名方式，例如：cn.whowin.dbus，应用程序可以通过这个固定的名称请求服务端的服务；
* 下面的代码片段使用 libdbus 库为一个连接指定一个固定的名称；
    ```C
    DBusError dbus_error;
    DBusConnection *conn;

    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    const char *server_bus_name = "cn.whowin.dbus"
    dbus_bus_request_name(conn, server_bus_name, DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);
    ......
    ```
* D-Bus 的服务采用了分层结构，以便客户端可以明确地请求服务；
    - 一个连接下可以有一个或多个对象(Object)；
    - 每个对象下可以有一个或多个接口(Interface)；
    - 每个接口下可以有一个或多个调用方法(Method Call)或者信号(Signal)；
* 对象(Object)的命名方法为对象路径，一个对象路径看上去与 Linux 的文件路径很像，比如：`/cn/whowin/dbus/object` 表示一个 D-Bus 对象；
    - 尽管看上去与文件路径很像，但是跟文件没有半毛钱关系，不会在文件系统上产生实际的物理文件，只是一个对象名称而已；
* 接口(Interface)的命名方法通常也是采用反向域名的方式，比如：`cn.whowin.dbus_iface` 表示一个接口；
* 接口下可以定义若干个调用方法(Method Call)和信号(Signal)，D-Bus 规范目前仅定义了这两种服务；
    - 调用方法需要定义一个方法名称及调用该方法的参数(参数数量和参数类型)，供客户端调用时使用；
    - 信号是一种消息通知，比如新设备上线通知等，客户端订阅感兴趣的信号后，就会从服务端收到相应的信号；
* 当客户端要在 D-Bus 上请求服务端的服务时，需要了解下面四点：
    1. 服务是在系统总线(System Bus)上，还是在会话总线(Session Bus)上；
    2. 提供服务的总线名称
    3. 调用方法所属的对象路径
    4. 调用方法或信号的接口
* 当客户端向服务端请求一个调用方法时，可以要求回复或者不回复，当需要回复时，会收到来自服务端的回复信息；
* D-Bus 有一个自省机制(D-Bus introspection mechanism)，当客户端进程并不了解服务端的对象、接口以及接口下定义的调用方法和信号时，可以通过这个自省机制获得信息；


## 3 D-Bus 编程实例
* **源程序**：[dbus-objects.c][src01](**点击文件名下载源程序**)演示了如何使用 libdbus 库实现多个客户端进程向服务端进程请求服务的通信过程；
* 该程序是个多进程的程序，建立了一个服务端进程，运行函数：`dbus_server()`，建立多个客户端进程，运行函数：`dbus_clien()`，运行起来以后，相当于运行着一个服务器端程序和若干个客户端程序；
* 客户端进程的数量由常量 `CLIENT_NUM` 控制，默认情况下是 3 个客户端进程；
* 服务端进程提供了三种调用方法的服务，其接口名称和方法名称一样，但分别部署在三个对象上：
    1. 对象路径：/cn/whowin/bus/yes，接口：cn.whowin.bus_demo，方法：hello
        > 将客户端请求中的参数(一个字符串)，前面加上 "yes" 返回；

    2. 对象路径：/cn/whowin/bus/no，接口：cn.whowin.bus_demo，方法：hello
        > 将客户端请求中的参数(一个字符串)，前面加上 "no" 返回；

    3. 对象路径：/cn/whowin/bus/quit，接口：cn.whowin.bus_demo，方法：hello
        > 将客户端请求中的参数(一个字符串)，前面加上 "quit" 返回；

* 编译：```gcc -Wall -g dbus-objects.c -o dbus-objects `pkg-config --libs --cflags dbus-1` ```
    - 其中：`pkg-config --libs --cflags dbus-1` 是一个命令行命令，可以帮助你获取正确的编译选项；
    - `pkg-config` 依赖 `.pc` 文件，libdbus 的 .pc 文件位于路径 `/usr/lib/x86_64-linux-gnu/pkgconfig` 下(`ubuntu 20.04`)；
    - 可以在终端上运行 `pkg-config --libs --cflags dbus-1` 看一下输出的是什么，对理解 `pkg-cofig` 会有帮助：

        ![Screenshot of pkg-config][img03]

    - 有关 `pkg-config` 不是本文的讨论内容，请自行查找相关资料或查阅在线手册 `man pkg-config`
* 运行：`./dbus-objects`
* 运行截图：

    ![Screenshot of dbus-objects][img02]

-------------
* 同样是提供三个调用方法的服务，也可以部署在一个对象上，在一个接口上提供三个方法，如下：
    1. 对象路径：/cn/whowin/dbus，接口：cn.whowin.bus_demo，方法：yes
        > 将客户端请求中的参数(一个字符串)，前面加上 "yes" 返回；

    2. 对象路径：/cn/whowin/dbus，接口：cn.whowin.bus_demo，方法：no
        > 将客户端请求中的参数(一个字符串)，前面加上 "no" 返回；

    3. 对象路径：/cn/whowin/bus，接口：cn.whowin.bus_demo，方法：quit
        > 将客户端请求中的参数(一个字符串)，前面加上 "quit" 返回；

* **源程序**：[dbus-methods.c][src02](**点击文件名下载源程序**)与 dbus-objects.c 完成一样的任务，但是是在一个对象下的三个调用方法；
* 编译：```gcc -Wall -g dbus-methods.c -o dbus-methods `pkg-config --libs --cflags dbus-1` ```
* 运行：`./dbus-objects`
* 运行截图：

    ![Screenshot of dbus-methods][img04]


## 4 libdbus 数据结构及函数调用介绍
* 有了前面两个范例，再回过头来介绍与 libdbus 相关的数据结构及函数调用，本文仅就在两个范例中相关的结构和函数作出说明；
* 点击 [libdbus 库的 API 文档][libdbus_api] 可以查看 libdbus 库的在线文档；

* 使用 libdbus 实现服务端的步骤：
    1. 使用 `dbus_bus_get()` 连接到 D-Bus；
    2. 使用 `dbus_bus_request_name()` 为这个连接指定一个唯一的名称，以便客户端可以访问这个连接；
    3. 使用 `dbus_connection_pop_message()` 接收来自客户端的请求消息；
    4. 使用 `dbus_message_has_path()`、`dbus_message_is_method_call()` 判断消息的对象、接口和方法是否正确；
    5. 使用 `dbus_message_get_args()` 获取消息中的请求参数，并作出适当处理；
    6. 使用 `dbus_message_new_method_return()` 初始化一个用于向客户端回复的消息；
    7. 使用 `dbus_message_append_args()` 将回复内容添加到回复消息中；
    8. 使用 `dbus_connection_send()` 将回复消息加入到发送队列中；
    9. 使用 `dbus_connection_flush()` 将消息队列中消息发送出去；

* 关于接收信息
    - 服务端使用 `dbus_connection_pop_message()` 接收客户端请求，通常情况下，程序需要一个循环不断地调用这个函数，就像下面的代码，但是这种方式显然是效率低下的；
        ```C
        ......
        while (1) {
            DBusMwssage *message;
            message = dbus_connection_pop_message(conn);
            if (message == NULL) {
                usleep(1000000);
                continue;
            }
            // Process the message
            ......
        }
        ......
        ```
    - libdbus 给出了另外一种方案，即使用 `dbus_connection_read_write_dispatch()`，当消息队列中有消息到来或者可以发送消息时，这个函数会返回 TRUE，否则一直阻塞；
    - 使用这个函数就可以像下面这样接收消息：
        ```C
        ......
        while (dbus_connection_read_write_dispatch(conn, -1)) {
            DBusMessage *message;
            message = dbus_connection_pop_message(conn);
            ......
        }
        ......
        ```

* 使用 libdbus 实现客户端的步骤：
    1. 使用 `dbus_bus_get()` 连接到 D-Bus，这里要使用服务端为连接指定的名称；
    2. 使用 `dbus_message_new_method_call()` 为向服务端发出的请求初始化一个方法调用消息；
    3. 使用 `dbus_message_append_args()` 将方法调用的参数添加到消息中；
    4. 使用 `dbus_connection_send_with_reply_and_block()` 发送请求消息并得到服务端的回复消息；
    5. 使用 `dbus_message_get_args()` 获取消息中的回复信息；

* 客户端向服务端发送请求时，可以要求服务端给予回复，此时要使用 `dbus_connection_send_with_reply_and_block()` 发送消息，而不能使用 `dbus_connection_send()`；

* 范例中主要涉及到了 libdbus 中的三个数据结构：
    1. DBusConnection：表达一个与 D-Bus 的连接；
    2. DBusMessage：表示一个消息；
    3. DBusError：表示一个错误信息；
* libdbus 进行了抽象，所以实际上在应用层面上根本不用理会 DBusConnection 和 DBusMessage 两个结构中的具体组成字段，对 DBusError 也仅需了解一、两个字段即可；

* 结构 **DBusError**
    - 表示错误信息的结构；
    - libdbus 库中的函数在调用出错时，如果调用参数中有 `DBusError *`，其错误信息会返回到这个结构中；
    - 这个结构中有意义的字段只有两个，一个是 `char *name`，表示错误名称，另一个是 `char *message`，表示错误信息；
    - 在本文的实例中，仅用到了 message 字段，用于显示错误信息；
    - 在使用这个结构之前，需要使用 `void dbus_error_init(DBusError *error)` 对数据结构进行初始化；
    - 使用函数 `dbus_bool_t dbus_error_is_set(const DBusError *error)` 检查在函数调用过程中是否出现了错误，如果出错，可以打印出 message 字段查看错误信息；
    - 如果出现错误，在处理完错误信息后要使用 `void dbus_error_free(DBusError *error)` 释放错误，或者使用 `dbus_error_init()` 重新初始化；
    - 下面代码片段演示了如何使用这个结构：
        ```C
        ......
        DBusError dbus_error;
        DBusConnection *conn;

        dbus_error_init(&dbus_error);
        conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
        if (dbus_error_is_set(&dbus_error)) {
            sprintf(stderr, "dbus_bus_get(): %s\n", dbus_error.message);
            dbus_error_free(&dbus_error);
            exit(1);
        }
        ......
        ```

* 函数 `dbus_bus_get()` 连接 `dbus-daemon`
    ```C
    DBusConnection *dbus_bus_get(DBusBusType type, 
                                 DBusError *error)
    ```

    - 连接到 dbus-daemon 守护进程，并向其注册；
    - type 为总线类型，可以是 `DBUS_BUS_SESSION`(会话总线) 或者 `DBUS_BUS_SYSTEM`(系统总线)，应用程序中通常选择 `DBUS_BUS_SESSION`；
    - 该函数在正常情况下会返回一个 `DBusConnection` 结构指针，在后面的接收消息和发送消息中均需要这个结构指针；
    - 不管是服务端还是客户端，在执行 D-Bus 相关操作前均需要首先连接到 dbus-darmon；

* 函数 `dbus_bus_request_name()` 为连接指定名称
    ```C
    int dbus_bus_request_name(DBusConnection *connection,
                              const char *name,
                              unsigned int flags,
                              DBusError *error);    
    ```
    - 该函数为一个连接指定一个名称；
    - 当服务端连接上 dbus-daemon 后，dbus-daemon 会为这个连接随机分配一个名称，以 ":" 开头的一组数字，比如：`:1.14`
    - 客户端无法知晓这个分配的名称，所以也就无法通过这个名称向服务端请求服务，通常需要使用一个公共的名称与 dbus-daemon 分配的名称绑定，客户端可以使用这个公共名称向服务端发出请求；
    - 比如这个公共名称为：cn.whowin.dbus，服务端启动后与 dbus-daemon 建立连接，然后将 cn.whowin.dbus 与 dbus-daemon 分配的名称绑定，客户端启动后只需使用 cn.whowin.dbus 向服务端请求服务即可；
    - dbus-daemon 允许多个进程向其申请相同的总线名称，但同时只能有一个进程实际拥有这个名称，这个拥有者叫做**主要拥有者**(Primary Owener)，其他申请该名称的进程将进入队列等候，直到主要拥有者释放该名称，队列中的第一位申请者将获得该名称；
    - 参数 connection：需要指定名称的连接；
    - 参数 name：为该连接指定的名称；
    - 参数flags：这个标志可以有三个选项，
        1. `DBUS_NAME_FLAG_ALLOW_REPLACEMENT`：表示该进程在成为名称的主要拥有者后，如果其他申请该名称的进程在调用 `dbus_bus_request_neme()` 时使用了 `DBUS_NAME_FLAG_REPLACE_EXISTING` 标志，则新的申请名称的进程将从当前拥有者手中抢走该名称的拥有权，成为新的主要拥有者；
        2. `DBUS_NAME_FLAG_REPLACE_EXISTING`：见 `DBUS_NAME_FLAG_ALLOW_REPLACEMENT`；
        3. `DBUS_NAME_FLAG_DO_NOT_QUEUE`：如果在申请名称时使用了该标志，表示进程只想立即成为该名称的主要拥有者，并不想进入队列等待，如果不能立即拥有该名称，将返回一个错误；
        4. 如果没有指定标志(flags = 0)，则只有当所申请的名称没有拥有者时才会获得名称的所有权，并且其它进程无法使用 `DBUS_NAME_FLAG_REPLACE_EXISTING` 从其手中抢夺所有权；

    - 返回值：
        1. DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER：当前进程获得了该名称的主要拥有者；
        2. DBUS_REQUEST_NAME_REPLY_IN_QUEUE：当前进程没有获得名称的所有权，已经进入队列等待名称所有者放弃该名称；
        3. DBUS_REQUEST_NAME_REPLY_EXISTS：该名称已经被其它进程拥有，当前调用指定了 `DBUS_NAME_FLAG_DO_NOT_QUEUE` 或者该名称的拥有者没有设置为 `DBUS_NAME_FLAG_ALLOW_REPLACEMENT` 或者当前调用没有指定 `DBUS_NAME_FLAG_REPLACE_EXISTING`，这三种情况下会返回该值；
        4. DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER：当前进程在调用 `dbus_bus_request_name()` 之前已经是该名称的主要拥有者；

* 服务端接收客户端发送的服务请求
    - 使用函数 `dbus_connection_pop_message()` 和 `dbus_connection_read_write_dispatch()` 组合可以方便地接收到客户端发送的服务请求消息；
    - 服务端通常使用这两个函数读取客户端发送的请求，基本格式如下：

        ```C
        ......
        while (dbus_connection_read_write_dispatch(conn, -1)) {
            DBusMessage *message;
            message = dbus_connection_pop_message(conn);
            ......
        }
        ......        
        ```
    - `dbus_connection_read_write_dispatch()` 原型

        ```C
        dbus_bool_t dbus_connection_read_write_dispatch(DBusConnection *connection,
                                                        int timeout_milliseconds);
        ```
    - `dbus_connection_read_write_dispatch()` 监视连接上是否有可读消息或者可以发送消息，然后使用 `dbus_connection_pop_message()` 接收消息；
    - connection：使用 `dbus_bus_get()` 函数获取的连接；
    - timeout_milliseconds：`dbus_connection_read_write_dispatch()` 是一个阻塞函数，直至 `timeout_milliseconds` 指定的超时时间到，或者有可读/写消息，注意 `timeout_milliseconds` 的单位为毫秒(millisecond)，当 `timeout_milliseconds` 设为 -1 时，表示一直阻塞直至有可读/写消息；
    - `dbus_connection_pop_message()` 原型

        ```C
        DBusMessage *dbus_connection_pop_message(DBusConnection *connection);	
        ```
    - `dbus_connection_pop_message()` 函数从连接的消息队列中接收一条消息，参数 connection 是通过 `dbus_bus_get()` 获得的连接，返回一个 DBusMessage 结构的指针；
    - 如果消息队列中没有消息，`dbus_connection_pop_message()` 会返回一个 NULL；
* 判断客户端调用的对象、接口和方法
    - 实际编程实践中，不必关心 `dbus_connection_pop_message()` 返回的 DBusMessage 结构中的具体细节，有一系列的函数可以帮助你从这个结构中获取信息；
    - `int dbus_message_get_type(DBusMessage *message)` 可以获取 message 的类型，通常有四种类型：
        + DBUS_MESSAGE_TYPE_METHOD_CALL：该消息为一个方法调用，一般客户端会以这种类型的消息向服务端请求服务；
        + DBUS_MESSAGE_TYPE_METHOD_RETURN：该消息为一个方法调用产生的回复信息，客户端向服务端请求服务并要求得到回复时，客户端会收到服务端发来的这种类型的消息；
        + DBUS_MESSAGE_TYPE_ERROR：该 message 为一条错误信息；
        + DBUS_MESSAGE_TYPE_SIGNAL：该 message 是一个信号(Signal)，本文不讨论信号；
    - `dbus_bool_t dbus_message_has_path(DBusMessage *message, const char *path)` 可以检查消息中是否存在指定的对象路径，其中 `path` 参数为对象路径，比如：`/cn/whowin/dbus`；
    - `dbus_bool_t dbus_message_is_method_call(DBusMessage *message, const char *iface, const char *method)` 可以检查消息中是否与指定的接口和方法一致，其中 `iface` 是接口名称，比如 `cn.whowin.dbus_demo`，`method` 为方法名称；
    - 首先使用 `dbus_message_get_type()` 判断该消息是否为我们需要处理的消息类型，然后使用 `dbus_message_has_path()` 和 `dbus_message_is_method_call()` 判断客户请求的对象、接口和方法，从而提供准确的服务；

* 函数 `dbus_message_get_args()` 获取请求参数
    - 函数 `dbus_message_get_args()` 原型

        ```C
        dbus_bool_t dbus_message_get_args(DBusMessage *message,
                                          DBusError *error,
                                          int first_arg_type,
                                          ... 
                                          );
        ```
    - 我们通过前面介绍的几个函数已经可以很清楚地判断出客户端请求的对象、接口和方法，`dbus_message_get_args()` 函数的作用在于进一步获取这个请求的参数；
    - 一个请求的参数可以有多个，参数的数据类型也可以有很多种，本例中，调用参数中只有一个字符串，这里仅介绍获取字符串参数的方法；
    - 这个函数的前两个参数无需更多介绍，在前面的函数中已经多次出现；
    - `first_arg_type` 是参数类型，当参数为字符串时，这个值应为：`DBUS_TYPE_STRING`，还可以有：DBUS_TYPE_INT16、DBUS_TYPE_ARRAY 等，这里不多介绍，在头文件 `dbus/dbus-protocol.h` 中定义；
    - 下面的 `"..."` 表示允许放多个参数，用于从 message 中一次获取多个调用参数，我们这里仅介绍获取一个字符串参数的方法，以后的文章中会介绍更多；
    - 当仅获取一个字符串参数时，其实际调用方法如下：
        ```C
        char *str;
        dbus_bool_t dbus_message_get_args(DBusMessage *message,
                                          DBusError *error,
                                          DBUS_TYPE_STRING,
                                          &str,
                                          DBUS_TYPE_INVALID);
        ```
    - 所以这里需要我们关注的只有一个变量 str，注意这里传给函数的一个字符串指针的指针；
    - 很显然这个调用成功后，str 会指向一个字符串，但根据 D-Bus 的文档，返回到 str 中的类型为 `const char *`，不需要我们去释放相应的内存；
    - 这个函数在调用成功时，会返回 TRUE，失败时返回 FALSE，从 error 可以获取错误信息；
    - 最后那个 DBUS_TYPE_INVALID 是必需的；

* 初始化一个 DBusMessage 结构
    - 当需要发送消息时，需要首先构建一个 DBusMessage 结构，通常使用 libdbus 提供的函数来初始化；
    - libdbus 提供了几个函数用来在各种不同的情况下初始化 DBusMessage 结构，包括：方法调用(Method Call)、方法调用的回复(Method Call Return)和信号(Signal)等，本文仅涉及方法调用和方法调用回复；
    - 初始化一个方法调用的 DBusMessage 结构
        ```C
        DBusMessage *dbus_message_new_method_call(const char *destination, 
                                                  const char *path, 
                                                  const char *iface, 
                                                  const char *method);
        ```
      + 当初始化一个方法调用的 DBusMessage 时，意味着客户端准备向服务端请求一个服务，此时需要知道四个信息：服务端总线名称、请求的对象路径、接口名称以及方法名称；
      + destination：服务端的总线名称，比如：cn.whowin.dbus
      + path：服务端对象路径，比如：/cn/whowin/dbus
      + iface：接口名称，比如：cn.whowin.dbus_demo
      + method：请求的方法名称，比如：yes
    - 初始化一个方法调用回复的 DBusMessage 结构
        ```C
        DBusMessage *dbus_message_new_method_return (DBusMessage *method_call);
        ```
        + 当初始化一个方法调用回复的 DBusMessage 时，意味着服务端已经收到了一个客户端发来的方法调用的消息，经过处理后，准备将处理结果回复给客户端，此时，只需知道客户端发来的方法调用的 DBusMessage 即可，因为这个结构里有所有需要的信息；
        + method_call：客户端发送的方法调用消息；

* 函数 `dbus_message_append_args()` 将调用参数添加到消息中
    - 在初始化 DBusMessage 结构时，已经设置了总线名称、对象路径、接口名称和方法名称，`dbus_message_append_args()` 的作用就是向这个消息中添加实际的调用参数；
    - `dbus_message_append_args()` 原型
        ```C
        dbus_bool_t dbus_message_append_args(DBusMessage *message, 
                                             int first_arg_type,
                                             ...);
        ```
    - 这个函数的调用方式与前面介绍的 `dbus_message_get_args()` 有些相似，本例中仅需要添加一个字符串类型的参数，其具体调用方法如下：
        ```C
        char *str_arg = "This is an argument";
        char *ptr = str_arg;
        dbus_bool_t dbus_message_append_args(DBusMessage *message, 
                                             DBUS_TYPE_STRING,
                                             &ptr,
                                             DBUS_TYPE_INVALID);
        ```
    - 请参考函数 `dbus_message_get_args()` 的介绍；
    - 该函数调用成功返回 TRUE，失败时返回 FALSE；

* 发送消息
    - 通常发送消息时可以使用 `dbus_connection_send()` 函数；
    - 当客户端向服务端发送请求并且希望服务端给予回复时，可以使用 `dbus_connection_send_with_reply_and_block()` 函数，这两个函数在调用方法上略有不同；
    - 函数 `dbus_connection_send()` 原型
        ```C
        dbus_bool_t dbus_connection_send(DBusConnection *connection, 
                                         DBusMessage *message, 
                                         dbus_uint32_t *serial);
        ```
        + 该函数在调用成功时返回 TRUE
        + 该函数仅仅将消息放入发送队列，然后就返回，并不保证消息已经发出，即便是连接已经断开，仍然可以将消息放入发送队列，所以该函数仍然会返回 TRUE；
        + 如果希望消息马上发出，应该在该调用后调用 `void dbus_connection_flush(DBusConnection *connection)` 该函数会将指定连接的发送队列中的消息全部发送出去再返回；
    - 函数 `dbus_connection_send_with_reply_and_block()` 原型
        ```C
        DBusMessage dbus_connection_send_with_reply_and_block(DBusConnection *connection,
                                                              DBusMessage *message,
                                                              int timeout_milliseconds,
                                                              DBusError *error)
        ```
        + 该函数会发送 message 消息，然后进入阻塞，等待服务端的回复消息，或者到达超时时间 `timeout_milliseconds`
        + 该消息成功获得服务端的返回消息后，返回回复消息的 DBusMessage 结构，如果超时或调用失败，则返回 NULL，从 error 中可以获得错误信息；
        + 注意超时时间单位为毫秒

* 至此，本文范例中涉及的 libdbus 的函数就介绍完了，原恒旭应该可以搞懂了。

## 5 其他
* 本文仅对 D-Bus 作了初步的介绍，其范例也是基本的使用方法，或许在后续的文章中会更多地介绍 D-Bus 的相关应用和编程方法；
* 很多系统服务均提供一系列的 D-Bus API，比如：域名解析、蓝牙等，这使得应用程序可以方便地调用系统服务的 API，给编写应用程序带来很多方便；
* D-Bus 也有一些命令行工具，通过命令行可以向 D-Bus 发送消息，查看 D-Bus 连接树等，这些将在后续的文章中介绍。








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

[dbus_webpage]: https://www.freedesktop.org/wiki/Software/dbus/
[libdbus_api]: https://dbus.freedesktop.org/doc/api/html/group__DBus.html
[dbus_specification]: https://dbus.freedesktop.org/doc/dbus-specification.html

[src01]: https://whowin.gitee.io/sourcecodes/100021/dbus-objects.c
[src02]: https://whowin.gitee.io/sourcecodes/100021/dbus-methods.c


[img01]: https://whowin.gitee.io/images/100021/d-bus.png
[img02]: https://whowin.gitee.io/images/100021/screenshot-of-dbus-objects.png
[img03]: https://whowin.gitee.io/images/100021/screenshot-of-pkg-config.png
[img04]: https://whowin.gitee.io/images/100021/screenshot-of-dbus-methods.png


