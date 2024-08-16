---
title: "IPC之十五：使用libdbus通过D-Bus请求系统调用实现任意DNS记录解析的实例"
date: 2023-12-28T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "IPC"
  - "Linux"
  - "C Language"
  - "Network"
tags:
  - Linux
  - 进程间通信
  - "D-Bus"
  - libdbus
  - IPC
  - 域名解析
  - "systemd-resolved"
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


postid: 190025
---


关于D-Bus的文章中曾介绍了如何通过D-Bus调用系统服务从而实现解析出一个域名的IP地址的过程，本文我们继续调用系统调用来实现解析任意DNS记录，系统调用的方法与前一篇文章类似，只是方法名称和调用参数以及返回参数不同，本文将详细介绍systemd-resolved服务中的ResolveRecord方法，同前面几篇关于D-BUS的文章相同，本文将使用 libdbus 库实现系统服务的调用，本文给出了实现解析任意DNS记录的实例，附有完整的源代码；本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文不适合 Linux 编程的初学者阅读。

<!--more-->

## 1 基本概念
* 在阅读本文之前，建议阅读 [《IPC之十三：使用libdbus通过D-Bus请求系统调用实现域名解析的实例》][article13] 和文章 [《用C语言实现的一个DNS客户端》][article51]；
* 前一篇参考文章里介绍了如何使用 libdbus 库调用系统调用的概念和方法，本文的调用的系统调用与该文中一样，均是 `systemd-resoled`，只是请求的方法不同，所以，调用参数和返回参数均不同，但很多概念是一样的；
* 后一篇参考文章里介绍了 DNS 的基本概念，了解 DNS 基本概念是理解本文的基础，否则可能云里雾里搞不明白；
* 要通过 D-Bus 调用一个系统服务中的方法，要了解以下一些信息：
    1. 总线名称
    2. 对象路径
    3. 接口名称
    4. 接口下的方法名称
    5. 调用接口的输入参数
    6. 调用完成后返回的参数
* D-Bus 的类型系统(Type System)简单回顾，这部分在 [D-Bus Specification][dbus_specification] 中有详细的描述，这里把一些关键信息列出来

    - 普通数据类型

        |Conventional name|ASCII type-code|Encoding|
        |:----------------|:-------------:|:-------|
        |BYTE|y (121)|Unsigned 8-bit integer|
        |BOOLEAN|b (98)|Boolean value: 0 is false, 1 is true, any other value allowed by the marshalling format is invalid|
        |INT16|n (110)|Signed (two's complement) 16-bit integer|
        |UINT16|q (113)|Unsigned 16-bit integer|
        |INT32|i (105)|Signed (two's complement) 32-bit integer|
        |UINT32|u (117)|Unsigned 32-bit integer|
        |INT64|x (120)|Signed (two's complement) 64-bit integer (mnemonic: x and t are the first characters in "sixty" not already used for something more common)|
        |UINT64|t (116)|Unsigned 64-bit integer|
        |DOUBLE|d (100)|IEEE 754 double-precision floating point|
        |UNIX_FD|h (104)|Unsigned 32-bit integer representing an index into an out-of-band array of file descriptors, transferred via some platform-specific mechanism (mnemonic: h for handle)|

    - 字符串类型

        |Conventional name|ASCII type-code|Validity constraints|
        |:----------------|:-------------:|:-------------------|
        |STRING|s (115)|No extra constraints|
        |OBJECT_PATH|o (111)|Must be a syntactically valid object path|
        |SIGNATURE|g (103)|Zero or more single complete types|

    - struct 的类型代码为 'r'，但在实际表达上通常用括号 "( )" 表达，比如：`(ii)` 表示有两个整数类型的结构，在结构中还可以嵌套另一个结构，比如："(i(ii))"；
    - 数组的类型代码为 'a'，`ai` 表示一个整数数组，相当于 `int a[]`，也可以定义一个结构数组，比如：`a(ii)` 表示一个有两个整数的结构数组；

* DNS 服务遵循 [RFC-1035][rfc1035] 所描述的规范，其返回的数据被称为 RR(Resource Record)，其格式为：

    ```plaintext
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                                               /
    /                      NAME                     /
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TTL                      |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   RDLENGTH                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
    /                     RDATA                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ```
* RR 中所有表示域名的地方均以"标签(label)"方式表示，每个标签的最大长度为 63 个字符，域名的最大长度为 255 个字符；
* 一个标签的第一个字符的低 6 位表示这个标签的长度，后面紧跟相应数量的字符的 ASCII，一个长度为 0 的标签作为一个域名的结束，简单的说就是遇到一个 `"\0"` 表示一个域名结束；
* 一个域名可以由多个标签组成，标签之间以字符 `"."` 分割；
* RR 所有数字变量的字节序均为 "Big Endian"，而 X86 架构的字节序为 "Little Endian"，所以在读取数字变量时需要做一下转换。
---------
## 2 systemd-resolved 系统服务的 ResolveRecord 方法
* systemd-resolved 在 D-Bus 上提供了一组用于解析 DNS 记录的 API，如下：
    1. `ResolveHostname()` 用于解析主机名以获取其 IP 地址；
    2. `ResolveAddress()` 用于反向操作，获取 IP 地址的主机名；
    3. `ResolveService()` 用于解析 DNS-SD(DNS Service Discovery) 或 SRV 服务
    4. `ResolveRecord()` 用于解析任意资源记录(RR)
* 关于这些 API 的说明，可以点击这里 [org.freedesktop.resolve1 APIs][systemd-resolved_api]
* 其实 glibc 中也是有类似的功能调用的，比如：getaddrinfo() 和 getnameinfo()；
* 本文中，将使用 `systemd-resolved` 的 `ResolveRecord()` 调用，解析域名的 A 记录、CNAME 记录和 MX 记录；
* 在 [systemd-resolved 官方文档][systemd-resolved_api] 中对 ResolveRecord 方法做了如下定义：

    ```
    node /org/freedesktop/resolve1 {
        interface org.freedesktop.resolve1.Manager {
            methods:
                ......
                ResolveRecord(in  i ifindex,
                              in  s name,
                              in  q class,
                              in  q type,
                              in  t flags,
                              out a(iqqay) records,
                              out t flags);
                ......
        }
    }
    ```
* 根据 [org.freedesktop.resolve1 APIs][systemd-resolved_api] 的说明：
    1. 服务名称(总线名称)：`org.freedesktop.resolve1`
    2. 对象路径：`/org/freedesktop/resolve1`
    3. 接口名称：`org.freedesktop.resolve1.Manager`
    4. 方法名称：`ResolveRecord`
    5. 输入参数 5 个，分别为：

        |序号|数据类型|名称|说明|
        |:--:|:----:|:--|:---|
        |1|int32_t|ifindex|网络接口索引号，0-任意接口|
        |2|char * |name|要查询的域名|
        |3|uint16_t|class|要查询地址类型，1-IN(internet)|
        |4|uint16_t|type|要查询的记录类型，1-A，5-CNAME,15-MX|
        |5|uint64_t|flags|标志位，置 0 即可|

    6. 输出参数 2 个：
        + 结构数组，参考下面结构

            |序号|数据类型|名称|说明|
            |:--:|:----:|:--|:---|
            |1|int32_t|ifindex|实际使用的网络接口索引号|
            |2|uint16_t|class|记录地址类型，与输入参数一致|
            |3|uint16_t|type|记录类型，与输入参数一致|
            |4|char array|rrdata|RR 记录，符合 RFC-1035 描述的 RR 的格式|

        + 64 位无符号整数(uint64_t)，标志位，应该为 1，表示使用的是经典单播 DNS 协议进行的 DNS 查询，详情请参考 [org.freedesktop.resolve1 APIs][systemd-resolved_api]；

* 关于输出参数的 rrdata 字段，遵循 [RFC-1035][rfc1035] 中 `3.3` 节的描述，在本文上一节中有简单介绍，在文章 [《用C语言实现的一个DNS客户端》][article51] 中有比较详细的介绍和实例源程序；
* RR 记录中有如下字段：NAME、TYPE、CLASS、RDLENGTH 和 RDATA，在本文上一节中有简要说明；

    - 当 type 为 A 时，RDATA 为一个 IP 地址；
    - 当 type 为 CNAME 时，RDATA 为主机域名，使用标签(label)方式记录域名；
    - 当 type 为 MX 时，RDATA 为邮件交换服务器的域名，使用标签(label)方式记录域名；
    - 本文仅支持这三种记录类型；
--------
## 3 使用 D-Bus 查找 DNS 记录实例
* **源程序**：[dbus-dns-record.c][src01] (**点击文件名下载源程序，建议使用UTF-8字符集**)演示了使用 libdbus 通过 D-Bus 请求系统调用实现查找 DNS 任意记录的方法；
* 源程序中有较为详细的注释，这里就不做太多解释了
* 编译：```gcc -Wall -g dbus-dns-record.c -o dbus-dns-record `pkg-config --libs --cflags dbus-1` ```
* 有关 `pkg-config --libs --cflags dbus-1` 可以参阅文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中的简要说明；
* 运行：`./dbus-dns-record <domain name> [A/CNAME/MX]`，两个参数，第一个参数是要查询的域名，第二个参数是要查询的记录类型，本程序仅支持三种记录类型：A 记录、CNAME 记录和 MX 记录；

    ```./dbus-dns-record baidu.com MX```

* 运行截图：

    ![Screenshot of dbus-dns-record][img01]







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


[article51]: https://whowin.gitee.io/post/blog/network/0019-dns-client-in-c/

<!-- CSDN
[article51]: https://blog.csdn.net/whowin/article/details/130181333

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
[article14]: https://blog.csdn.net/whowin/article/details/135493350
[article15]: https://blog.csdn.net/whowin/article/details/135493382
-->

[rfc1035]: https://www.ietf.org/rfc/rfc1035.txt


[dbus_webpage]: https://www.freedesktop.org/wiki/Software/dbus/
[libdbus_api]: https://dbus.freedesktop.org/doc/api/html/group__DBus.html
[dbus_specification]: https://dbus.freedesktop.org/doc/dbus-specification.html
[systemd-resolved_api]: https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.resolve1.html


[src01]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190025/dbus-dns-record.c

[img01]: https://whowin.gitee.io/images/190025/screenshot-of-dbus-dns-record.png

<!-- CSDN
[img01]: https://img-blog.csdnimg.cn/img_convert/d8138da7294ab4f3f75f5da6302f4f4d.png
-->
