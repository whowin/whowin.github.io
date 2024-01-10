---
title: "IPC之十二：使用libdbus在D-Bus上异步发送/接收信号的实例"
date: 2023-12-06T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories: 
  - "IPC"
  - "Linux"
  - "C Language"
tags:
  - Linux
  - 进程间通信
  - "D-Bus"
  - libdbus
  - IPC
draft: false
#references: 

# - [D-Bus : Transmit a Data Array in Simple and Useful Form](http://gaiger-programming.blogspot.com/2015/08/d-bus-simple-and-useful-example-to-send.html)
#   - 里面有一个很不错异步处理信号的例子
# - [Writing Resolver Clients](https://wiki.freedesktop.org/www/Software/systemd/writing-resolver-clients/)
#   - 通过 systemd-resolved 的总线 API 查找主机名和 DNS 记录
# - [The new sd-bus API of systemd](https://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html)
#   - 里面关于D-Bus的概念说的非常不错
# - [Using the DBUS C API](https://www.matthew.ath.cx/misc/dbus)
#   - 里面有个例子可以参考
# - [Implementing and using D-Bus signals](http://maemo.org/maemo_training_material/maemo4.x/html/maemo_Platform_Development_Chinook/Chapter_04_Implementing_and_using_DBus_signals.html)

postid: 190022
---


IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本 IPC 系列文章的前十篇介绍了几乎所有的常用的 IPC 方法，每种方法都给出了具体实例，前面的文章里介绍了 D-Bus 的基本概念以及调用远程方法的实例，本文介绍 D-Bus 的异步处理机制，以及信号处理的基本方法，本文给出了异步处理 D-Bus 的实例，附有完整的源代码；本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文不适合 Linux 编程的初学者阅读。

<!--more-->

## 1 D-Bus 的信号(Signal)
* 在阅读本文之前，建议阅读关于 D-Bus 的另一篇文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11]
* 在文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中，介绍了服务端如何在 D-Bus 上提供方法调用服务以及客户端如何向服务端请求一个方法调用服务；
* 通过 D-Bus 向服务端请求方法调用服务，仅仅是 D-Bus 一半的功能，D-Bus 还支持异步的广播通信方法，这种机制称为信号(Signal)，当服务端需要向大量接收者发送通知时，该机制非常有用；
* 举例来说，如果系统正在关闭、网络连接中断以及类似的系统范围内的情况，相关系统服务进程应该广播一个通知，使对这些服务有需求的进程能够及时做出反应，这样一种方式，使得接收信号的进程无需轮询服务状态；
* D-Bus 的信号(Signal)与调用方法(Method Call)有许多类似的地方，这里简要回顾一下在上一篇文章中讨论的调用方法的概念：
    1. 服务程序连接 D-Bus(dbus-daemon)，获得一个连接，D-Bus 随机给这个连接分配一个唯一名称；
    2. 为该连接绑定一个固定的名称(Bus Name)，以方便客户端访问这个连接，总线名称通常以反向域名的形式命名；
    3. 在该连接下可以建立一个或多个对象(Object)，对象名称以路径(类似文件系统路径)表示；
    4. 在每个对象上可以建立一个或者多个接口(Interface)，接口的名称也是使用反向域名的命名方式；
    5. 每个接口下可以有一个或者多个方法(Method)；
    6. 客户端需要请求服务端的某个方法时，需要知道总线名称、对象路径、接口名称以及方法名称，并将调用参数传送给服务端；
* 信号(Signal)也是建立在一个接口下，一个接口下不仅可以有一个或者多个方法，还可以有一个或者多个信号，
    - 1 -- 4 同上；
    5. 每个接口下可以有一个或者多个信号(Signal)，信号的命名与调用方法一样，没有很多规则，比如：sig_demo
    6. 客户端想要收到某个信号时，需要向总线注册，告知总线感兴趣的信号(包括：对象路径、接口名称和信号名称)，只能收到向总线注册过的信号；

* 所以其实一个接口下可以有若干个方法和信号，除此之外，接口下还可以有若干个属性(Properties)，方法、信号、属性组合在一起构成一个接口；
* 本文仅讨论接口，有关属性的事情，以后的文章中再讨论；

* 发送信号通常是服务端的事情，信号通常是以广播的方式发出，而只有订阅了这个消息的客户端才能收到消息，实际上，信号也是可以点对点发送的(仅发给指定客户端)，这个以后讨论，先讨论通常的广播信号；
* 服务端发送信号的步骤
    1. 使用 `dbus_bus_get()` 连接到 D-Bus，获得一个连接 DBusConnection；
    2. 使用 `dbus_message_new_signal()` 为构建信号初始化一个 DBusMessge；
    3. 使用 `dbus_message_append_args()` 将信号参数添加到信号的 DBusMessage 中；
    4. 使用 `dbus_connection_send()` 将信号放入发送队列；
    5. 使用 `dbus_connection_flush()` 将发送队列的消息全部发送出去；
    6. 使用 `dbus_message_unref()` 释放信号的 DBusMessage；

* 整个过程与在文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中描述的客户端向服务端请求一个服务的过程高度相似，但要简单一些：
    1. 请求服务时是客户端向服务端发出请求，而发送信号时是服务端发送一个广播消息；
    2. 请求服务时，服务端通常要在连接上绑定一个公用的名称(总线名称)，客户端在请求服务时必须要指定这个总线名称，否则 D-Bus 不知道你要向哪个应用程序请求服务，发送信号时，服务端不一定需要在连接上绑定名称，因为通常客户端只需要接收信号，不需要向服务端发送任何消息；
    3. 请求服务时，客户端在发出请求后通常需要服务端的回复，发送信号时没有任何回复消息；

* 客户端要接收到信号，需要订阅指定的信号，D-Bus 只会把你订阅的信号推送过来；
* 使用 `dbus_bus_add_match()` 订阅信号：
    ```C
    void dbus_bus_add_match(DBusConnection *connection,
                            const char *rule,
                            DBusError *error);
    ```
    - connection 是使用 `dbus_bus_get()` 获得的连接；
    - error 已经在很多函数调用中出现过，不多说了；
    - 这个 rule 参数是这个函数的灵魂，这是一个字符串，这个字符串定义了一个规则，告诉 D-Bus 我要订阅符合这个规则的信号；
    - 这个规则使用 `key/value` 的形式描述，可以有多个 `key/value` 对用于描述多个条件，每个 `key/value` 对用 "," 分隔；
    - 举个 rule 的例子：`"type='signal',sender='cn.whowin.dbus', path='/cn/whowin/dbus', interface='cn.whowin.dbus_iface',member='notify'"`
    - 在这个例子中，`type='signal'` 表示消息类型为信号，sender 是发送方的总线名称，path 是发送方的对象路径，interface 是发送方的接口名称，member 是信号名称，D-Bus 会把符合这些条件的信号推送到订阅的进程中；
    - 在描述规则时不用把条件写的这么全，比如：`"type='signal',sender='cn.whowin.dbus',path='/cn/whowin/dbus'"`，则从 `cn.whowin.dbus` 的对象 `/cn/whowin/dbus` 发出的消息都可以收到；
    - 在这个调用中，如果 error 参数为 NULL，则调用 `dbus_bus_add_match()` 后会立即返回，不会产生阻塞，但是订阅不会生效，需要执行 `dbus_connection_flush(conn)` 后订阅才会生效，而且如果发生了错误程序也是无法知晓的，所以，不建议这样做；
    - 目前 rule 可以使用的 `key/value` 对中的 key 可以为：type, sender, interface, member, path, destination；
    - destination 指的是目的地址，比如：`:275.6`，在广播信号中通常用不上；
    - 下面这段代码订阅了一个信号：

    ```C
    DBusError dbus_error;
    DBusConnection *conn;

    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    dbus_bus_add_match(conn, "type='signal',path='/cn/whowin/dbus/signal',interface='cn.whowin.dbus_iface'", &dbus_error);
    ......    
    ```
* 如果有必要，你可以使用 `dbus_bus_add_match()` 订阅多个信号。

## 2 libdbus 的异步处理机制
* 客户端并不知道什么时候会有信号发出来，所以为了能及时收到信号必须不断轮询，像这样：
    ```C
    DBusConnection *conn;
    DBusMessage *message;
    ......
    while (dbus_connection_read_write_dispatch(conn, -1)) {
        // loop
        message = dbus_connection_pop_message(conn);
        if (message == NULL) {
            usleep(10000);
            continue;
        }
        if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
            usleep(10000);
            continue;
        }
        ......
    }    
    ```
* 函数 `dbus_connection_read_write_dispatch()` 在文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中做过介绍；
* 显然，这样的编程模式并不高效，尤其是当程序不仅仅是要接收信号，还有其他工作要做时，这种程序架构就显得更加不可接受；
* 实际上，libdbus 还提供了另外一种异步接收信息的方式，像下面这样的代码：
    ```C
    DBusHandlerResult signal_filter(DBusConnection *connection, DBusMessage *message, void *usr_data) {
        DBusError dbus_error;
        
        dbus_error_init(&dbus_error);
        if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
            printf("Client: This is not a signal.\n");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        ......
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    int main() {
        DBusError dbus_error;
        DBusConnection *conn;

        dbus_error_init(&dbus_error);
        conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

        dbus_connection_add_filter(conn, signal_filter, NULL, NULL);
        dbus_bus_add_match(conn, "type='signal',path='/cn/whowin/dbus/signal',interface='cn.whowin.dbus_iface'", &dbus_error);
        
        while (dbus_connection_read_write_dispatch(conn, -1)) {
            /* loop */
            ......
        }
        
        ......
        return;
    }
    ```
    - 在主程序中，首先使用 `dbus_connection_add_filter()` 添加了一个过滤器(Filter)，然后再用 `dbus_bus_add_match()` 订阅感兴趣的信号；
    - 在下面的 while 循环中，并不需要去接收消息，当订阅的信号到来时，会直接调用过滤器，在过滤器里处理收到的信号即可；
    - 所以，上面这段程序实际上是在函数 `signal_filter()` 中处理信号；

* 函数 `dbus_connection_add_filter()` 原型
    ```C
    dbus_bool_t dbus_connection_add_filter(DBusConnection *connection,
                                           DBusHandleMessageFunction function,
                                           void *user_data,
                                           DBusFreeFunction free_data_function 
    )
    ```
    - connection 为通过 `dbus_bus_get()` 获得的连接；
    - function 为过滤器调用的函数；
    - user_data 为传递给 function 的参数；
    - free_data_function 为释放 user_data 需要调用的函数；
* `DBusHandleMessageFunction` 的定义
    ```C
    typedef DBusHandlerResult(* DBusHandleMessageFunction)(DBusConnection *connection,
                                                           DBusMessage *message,
                                                           void *user_data);    
    ```
    - 所以在 `dbus_connection_add_filter()` 中，function 是一个函数指针，该函数将接收三个参数，第一个是从 `dbus_bus_get()` 获得的连接，第二个参数是一个消息结构 `DBusMessage`，表示收到的消息，第三个参数是用户数据，在使用 `dbus_connection_add_filter()` 添加过滤器时设置；
    - 当一个过滤器函数被调用时，收到的消息已经在 message 中了；
    - 这个过滤器函数的返回值是 DBusHandlerResult，这是一个枚举类型，其值有三个：`DBUS_HANDLER_RESULT_HANDLED`、`DBUS_HANDLER_RESULT_NOT_YET_HANDLED` 和 `DBUS_HANDLER_RESULT_NEED_MEMORY`；
    - `DBUS_HANDLER_RESULT_HANDLED` 表示该过滤器函数已经获得了一个有效消息并进行了处理，该消息无需再交给其他过滤器处理；
    - `DBUS_HANDLER_RESULT_NOT_YET_HANDLED` 表示该过滤器没有处理该消息，如果有其他过滤器，可以把该消息交给其他过滤器处理；
    - `DBUS_HANDLER_RESULT_NEED_MEMORY` 通常用不上；

* 调用过滤器函数是由 libdbus 实现的，应该是在调用 `dbus_connection_read_write_dispatch()` 时，当有可读消息时，调用过滤器函数；
* 调用过滤器函数后的返回值并不会返回到应用程序中，但是对其它过滤器可能会产生影响，当系统内有多个过滤器时，当前过滤器返回 
    1. `DBUS_HANDLER_RESULT_HANDLED` 意味着已经处理好了这个消息，不必再使用其它过滤器处理该消息；
    2. `DBUS_HANDLER_RESULT_NOT_YET_HANDLED` 意味着这个消息没有在当前过滤器中被处理，如果有其它过滤器，应该尝试使用其它过滤器处理；

* 所以，过滤器函数的返回一定要正确，否则会有消息丢失；
* 如果有必要，你可以添加多个过滤器，去处理不同的消息；
* 过滤器的概念，其实也不仅仅可以用在接收信号上，也可以用在调用方法上；
* 尽管我们向 D-Bus 订阅了我们感兴趣的信号，但其实有时也会一些不符合订阅条件的信号到来，所以，在程序中还是要做一些判断，以确保收到的是我们期望的信号，如果不是，返回 `DBUS_HANDLER_RESULT_NOT_YET_HANDLED`，让其它过滤器去处理。

## 3 使用 libdbus 异步接收信号的实例
* **源程序**：[dbus-signals.c][src01](**点击文件名下载源程序，建议使用UTF-8字符集**)演示了使用 libdbus 对信号进行发送和接收，以及如何异步接收信号；
* 该程序是一个多进程程序，建立了一个服务端进程和三个客户端进程；
* 服务端进程在启动后发送出一个内容为 "start" 的信号，暂停 5 秒后，再发出一个内容为 "quit" 的信号，然后退出进程；
* 服务端在发送信号时，其对象路径、接口名称和信号名称均相同；
* 客户端进程订阅了服务端的信号，并添加了两个过滤器，一个用于处理内容为 "start" 的信号，另一个用于处理内容为 "quit" 的信号，这里仅是为了演示多个过滤器的工作方式；
* 客户端检查收到的信号，如果其内容为 "quit"，则退出进程；
* 编译：```gcc -Wall -g dbus-signals.c -o dbus-signals `pkg-config --libs --cflags dbus-1` ```
* 有关 `pkg-config --libs --cflags dbus-1` 可以参阅文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中的简要说明；
* 运行：`./dbus-signals`
* 运行截图：

    ![Screenshot of running dbus-signals][img01]

* 程序运行后，客户端进程的两个过滤器都显示了 "Wrong object path" 的信息，这条信息是 D-Bus 为客户端连接分配了名称后发送过来的通知信号，虽然我们没有订阅，但 D-Bus 会强行推送过来；
* 这条通知信号在经过过滤器时，过滤器返回了 "DBUS_HANDLER_RESULT_NOT_YET_HANDLED"，因为这个返回值导致这个消息在经过第一个过滤器后还会再进入第二个过滤器进行处理，如果过滤器在遇到对象路径不对时返回 "DBUS_HANDLER_RESULT_HANDLED"，则这条消息不会再去第二个过滤器，读者可以尝试修改程序看看是不是这样；
* 如果你多次运行这个程序你会发现，信号总是首先到达 `signal_quit()` 过滤器，然后才到达 `signal_start()` 过滤器，这是因为我们先添加的 `signal_quit()` 过滤器，如果你改动一下程序，先添加 `signal_start()` 过滤器，再添加 `signal_quit()` 过滤器，你会看到信号到达的顺序也会发生变化。



## **欢迎订阅 [『进程间通信专栏』](https://blog.csdn.net/whowin/category_12404164.html)**



-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

[article01]: https://whowin.gitee.io/post/blog/ipc/0010-ipc-example-of-anonymous-pipe/
[article02]: https://whowin.gitee.io/post/blog/ipc/0011-ipc-examples-of-fifo/
[article03]: https://whowin.gitee.io/post/blog/ipc/0013-systemv-message-queue/
[article04]: https://whowin.gitee.io/post/blog/ipc/0014-posix-message-queue/
[article05]: https://whowin.gitee.io/post/blog/ipc/0015-systemv-semaphore-sets/
[article06]: https://whowin.gitee.io/post/blog/ipc/0016-posix-semaphores/
[article07]: https://whowin.gitee.io/post/blog/ipc/0017-systemv-shared-memory/
[article08]: https://whowin.gitee.io/post/blog/ipc/0018-posix-shared-memory/
[article09]: https://whowin.gitee.io/post/blog/ipc/0019-ipc-with-unix-domain-socket/
[article10]: https://whowin.gitee.io/post/blog/ipc/0020-ipc-using-files/
[article11]: https://whowin.gitee.io/post/blog/ipc/0021-ipc-using-dbus/
[article12]: https://whowin.gitee.io/post/blog/ipc/0022-dbus-asyn-process-signal/
[article13]: https://whowin.gitee.io/post/blog/ipc/0023-dbus-resolve-hostname/
[article14]: https://whowin.gitee.io/post/blog/ipc/0024-select-recv-message/
[article15]: https://whowin.gitee.io/post/blog/ipc/0025-resolve-arbitrary-dns-record/

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
-->


[dbus_webpage]: https://www.freedesktop.org/wiki/Software/dbus/
[libdbus_api]: https://dbus.freedesktop.org/doc/api/html/group__DBus.html
[dbus_specification]: https://dbus.freedesktop.org/doc/dbus-specification.html

[src01]: https://whowin.gitee.io/sourcecodes/190022/dbus-signals.c

[img01]:https://whowin.gitee.io/images/190022/screenshot-of-dbus-signals.png


