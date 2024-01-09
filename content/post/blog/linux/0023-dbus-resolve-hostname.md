---
title: "IPC之十三：使用libdbus通过D-Bus请求系统调用实现域名解析的实例"
date: 2023-12-10T16:43:29+08:00
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


postid: 100023
---

前面两篇有关 D-Bus 的文章介绍了使用 libdbus 库进行进程间的方法调用和信号的传输，实际上 D-Bus 的更强大的地方是其建立了与大量系统服务之间建立了有效的对话规范，使得应用程序可以使用标准的方式调用系统服务的方法，访问系统服务中的一些开放的属性，本文将使用 libdbus 库调用系统服务中的方法从而实现域名解析，本文给出了实现该功能的实例，附有完整的源代码；本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文不适合 Linux 编程的初学者阅读。

<!--more-->

## 1 基本概念
* 阅读本文需要了解一些关于 D-Bus 的相关知识，以及 libdbus 库的基本使用方法，请阅读文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 和 [《IPC之十二：使用libdbus在D-Bus上异步发送/接收信号的实例》][article12]
* 上述两篇文章中，一篇文章介绍了服务端如何使用 libdbus 库在 D-Bus 上提供方法调用的服务，以及客户端如何向服务端请求调用方法的服务，另一篇文章中介绍了客户端如何使用 libdbus 库在 D-Bus 下异步接收信号，以及服务端如何发送信号；
* 其实，D-Bus 的一个重要功能是建立了客户端向系统服务请求服务的规范，使客户端可以借助系统服务提供的 API 快速完成某些功能；
* 本文将依托系统服务 `systemd-resolved` 提供的 API，使用 libdbus 库通过 D-Bus 调用 API，实现域名解析；
* 在终端上使用 `systemctl` 可以列出当前内存中的所有 systemd 服务，使用 `systemctl list-units systemd-resolved` 可以看到 `systemd-resolved` 服务当前的状态；

    ![Screenshot of systemctl][img01]

* 通过 D-Bus 对一个系统服务中的一个方法进行调用与在应用层面对一个服务中的方法进行调用基本相同，请重点参考文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11]，这里简单回顾以下；
* 首先要调用一个服务中的方法，要了解以下一些信息：
    1. 总线名称
    2. 对象路径
    3. 接口名称
    4. 接口下的方法名称
    5. 调用接口的输入参数
    6. 调用完成后返回的参数

* 在下一节介绍具体调用方法时，将一一找到上面这六条信息的答案；
* 在文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 的实例中，我们用的是会话总线(Session Bus)，现在要调用系统服务，显然是要使用系统总线(System Bus)了，这是其中一点小区别；
* 在文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 的实例中，我们使用 `dbus_message_append_args()` 向消息里添加参数，使用 `dbus_message_get_args()` 从回复消息里获取参数，在这个系统调用里，调用时的输入参数比较简单，我们仍然还可以使用 `dbus_message_append_args()`，但调用完成后的返回参数比较复杂，我们必须要引入一个迭代器(iterator)，这也是一个较大的区别，后面会有介绍；

## 2 systemd-resolved 介绍
* systemd-resolved 在 D-Bus 上提供了一组用于解析 DNS 记录的 API，如下：
    1. `ResolveHostname()` 用于解析主机名以获取其 IP 地址；
    2. `ResolveAddress()` 用于反向操作，获取 IP 地址的主机名；
    3. `ResolveService()` 用于解析 DNS-SD(DNS Service Discovery) 或 SRV 服务
    4. `ResolveRecord()` 用于解析任意资源记录
* 关于这些 API 的说明，可以点击这里 [org.freedesktop.resolve1 APIs][systemd-resolved_api]
* 其实 glibc 中也是有类似的功能调用的，比如：getaddrinfo() 和 getnameinfo()；
* 本文中，将使用 `systemd-resolved` 的 `ResolveHostname()` 调用，获取一个主机名的 IP 地址；
* 先来看一下 `ResolveHostname` 方法在 [官方文档][systemd-resolved_api] 上的原始定义：
    ```
    node /org/freedesktop/resolve1 {
        interface org.freedesktop.resolve1.Manager {
            methods:
            ResolveHostname(in  i ifindex,
                            in  s name,
                            in  i family,
                            in  t flags,
                            out a(iiay) addresses,
                            out s canonical,
                            out t flags);
            ......
        }
    }
    ```
* 根据 [org.freedesktop.resolve1 APIs][systemd-resolved_api] 的说明：
    1. 服务名称(总线名称)：`org.freedesktop.resolve1`
    2. 对象路径：`/org/freedesktop/resolve1`
    3. 接口名称：`org.freedesktop.resolve1.Manager`
    4. 方法名称：`ResolveHostname`

* 关于方法调用的部分涉及到 D-Bus 的类型系统(Type System)，这部分在 [D-Bus Specification][dbus_specification] 中有详细的描述，这里把一些关键信息列出来

    > 普通数据类型

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

    > 字符串类型

    |Conventional name|ASCII type-code|Validity constraints|
    |:----------------|:-------------:|:-------------------|
    |STRING|s (115)|No extra constraints|
    |OBJECT_PATH|o (111)|Must be a syntactically valid object path|
    |SIGNATURE|g (103)|Zero or more single complete types|

* 结构 struct 的定义
    - 和上面的基本类型一样，struct 也以一个类型代码，为 'r'，但在实际表达上通常用括号 "( )" 表达；
    - `(ii)` 表示有两个整数类型的结构；
        ```C
        struct {
            int32_t a;
            int32_t b;
        }
        ```
    - 在结构中还可以嵌套另一个结构，比如："(i(ii))" 表示：
        ```C
        struct {
            int32_t a;
            struct {
                int32_t b;
                int32_t c;
            } d;
        }
        ```
    - 尽管结构在表达上并不使用其类型代码 'r'，但是在编程中，当你要获取一个数据的类型，而这个数据正好是一个结构时，函数会返回 'r'；

* 数组 array 的定义
    - 数组的类型代码为 'a'，`ai` 表示一个整数数组，相当于 `int a[]`
    - 也可以定义一个结构数组，比如：`a(ii)` 表示一个有两个整数的结构数组，相当于：
        ```C
        struct {
            int32_t a;
            int32_t b;
        } c[];
        ```

* 有了上面的介绍，应该可以看懂方法的定义了：
    ```
    ResolveHostname(in  i ifindex,
                    in  s name,
                    in  i family,
                    in  t flags,
                    out a(iiay) addresses,
                    out s canonical,
                    out t flags);
    ```
    - 'in' 表示调用该方法时的输入参数，'out' 表示该方法返回的参数；
    - 所以这个方法调用时有四个输入参数，有三个返回参数，下面对输入和输出参数作出详细说明；
* 调用参数：

    |序号|字段名称|字段类型|说明|
    |:--:|:-----|:----:|:---|
    |1|ifindex|int32_t|网络接口索引号|
    |2|name|char *|主机名|
    |3|family|int32_t|地址族|
    |4|flags|uint64_t|标志|

    - ifindex：网络接口索引号，如果不知道使用那个网络接口，可以填 0，表示任何接口都可以，函数 `if_indextoname()` 可以把网络接口索引号转换成网络接口名称，`if_nametoindex()` 可以把网络接口名称转换成网络接口索引号，比如网络接口 "eth0"，可以用 `if_nametoindex("eth0")` 将其转为网络接口索引号；
    - name：主机名，就是常说的域名，比如：`whowin.cn`、`www.baidu.com`、`google.com` 等；
    - family：地址族，常用的就两个：`AF_INET` 和 `AF_INET6`，前者表示 IPv4，后者表示 IPv6
    - 如果不确定主机名有 IPv4 或者 IPv6 地址，可以将参数 family 设为 `AF_UNSPEC`，表示既可以是 IPv4 也可以是 IPv6，但是这个选项不会既有 IPv4 地址又有 IPv6 地址，它首先会尝试查找 IPv4 地址，如果找到，则不再查找 IPv6 地址，如果没有找到，则尝试查找 IPv6 地址；
    - flags：标志，通常设置为 0 即可，如果希望了解这个标志的详情，可以在 [systemd-resolve 官方文档][systemd-resolved_api] 中找到，但在一般的应用中，可以不考虑这个参数；

* 返回参数：

    |序号|字段名称|字段类型|说明|
    |:--:|:-----|:----:|:---|
    |1|addresses|数组|地址|
    |2|caonical|char *|规范的主机名|
    |3|flags|uint64_t|标志|

    - 返回参数的第一项要复杂一些，我们先说后面两项；
    - canonical：规范的主机名，这个主机名和调用时的主机名可能是一样的，也有可能不同；
    - flags：这个标志与调用时的标志相似，可以不用管它，或者在 [systemd-resolve 官方文档][systemd-resolved_api] 中查找其详细说明；
    - 回过头来说返回参数的第一项，当我们解析一个域名时，这个域名可能有不止一个 IP 地址，所以，解析结果是一个数组，数组中的每一项代表一个解析结果；
    - 这个数组时这样定义的 `a(iiay)`，这意味着这是一个结构数组，结构是由两个整数和一个数组组成，这个数组是一个 `uint8_t` 的数组，整个下来应该是下面这样：

        ```C
        struct address {
            int32_t ifindex;
            int32_t sa_family;
            uint8_t ip[INET6_ADDRSTRLEN];
        }

        struct address *addresses;
        ```
    - ifindex：接口的索引号，这个和输入参数的接口索引号是一个意思；
    - sa_family：表示当前地址的类型，`AF_INET` 表示是 IPv4 地址，`AF_INET6` 表示是 IPv6 地址；
    - 其中，数组 ip 的长度取决于 `sa_family` 的值，当 `sa_family` 为 `AF_INET` 时，数组 ip 的长度为 4，当 `sa_family` 为 `AF_INET6` 时，数组 ip 的长度为 16；

* 至此，对这个系统调用的方法已经介绍完了，剩下的就是实践了。

## 3 libdbus 中相关函数介绍
* 实例中的大部分函数都已经在文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 和 [《IPC之十二：使用libdbus在D-Bus上异步发送/接收信号的实例》][article12] 中做了介绍，再次建议在阅读本文之前阅读上述两篇文章；
* 这里仅介绍在上述两篇文章中没有介绍的函数；

### 函数 `dbus_message_iter_init()`：在一个消息上初始化一个迭代器

```C
dbus_bool_t dbus_message_iter_init(DBusMessage *message,
                                    DBusMessageIter *iter)
```

* 迭代器的概念多出现在面向对象的编程语言中，比如 C++、JAVA，用于遍历一个对象，这里的迭代器用于遍历整个参数；
* 在前两篇文章中，介绍过函数 `dbus_message_get_args()`，当返回参数中没有数组和结构这种复杂类型，都是基本数据类型，比如：各种整数、字符串等都可以使用这个函数；
* 甚至于返回的数据为一个简单的数组，比如：`int32_t a[5]` 这样的数组也是可以用 `dbus_message_get_args()` 获取返回参数的；
* 但是 `ResolveHostname` 方法返回的数组中不仅有数组，还是一个结构数组，结构数组中还嵌套有另一个数组，这个过于复杂，创建一个迭代器来获取参数是个好的选择；
* `dbus_message_iter_init()` 在消息 message 上创建一个迭代器 iter，创建成功返回 TRUE，否则返回 FALSE；

### 函数 `dbus_message_iter_recurse()`：在迭代器上初始化一个子迭代器；

```C
void dbus_message_iter_recurse(DBusMessageIter *iter,
                                DBusMessageIter *sub)
```

* 当消息参数中有数组或者结构时，需要使用迭代器去遍历参数中的各项，所以要在这个消息上建立一个迭代器；
* 当我们遍历参数中的各项时，如果又遇到数组或者结构，就需要在这个迭代器下建立一个子迭代器，用于遍历这个数组或者结构，以此类推；
* `dbus_message_iter_recurse()` 在迭代器 iter 指向的数组或结构上建立一个子迭代器 sub，用于遍历这个数组或者结构

### 函数 `dbus_message_iter_get_arg_type()`：获取迭代器指向的参数的参数类型；

```C
int dbus_message_iter_get_arg_type(DBusMessageIter *iter);
```

* 该函数可以获取迭代器指向的参数的数据类型，通常，如果数据类型为基本类型(不是数组、结构之类)，则可以用 `dbus_message_iter_get_basic()` 获取参数的值，如果数据类型是数组或者结构之类的类型，则不得不建立一个子迭代器去遍历其中的字段；
* 该函数返回数据类型，这些类型定义在头文件 `<dbus/dbus-protocol.h>` 中，是一系列的以 `"DBUS_TYPE_"` 开头的宏定义，一般一种类型有两个定义，一个是 `int` 的，还有一个是 `char *` 的，用于不同的场景，比如：32位整数的类型定义如下：
    ```C
    /** Type code marking a 32-bit signed integer */
    #define DBUS_TYPE_INT32                 ((int) 'i')
    /** #DBUS_TYPE_INT32 as a string literal instead of a int literal */
    #define DBUS_TYPE_INT32_AS_STRING       "i"
    ```
* 值得一提的是，定义了一个 `DBUS_TYPE_INVALID`，其值为 `((int) '\0')`，这并不是一个实际的数据类型，这个定义用于标识一组参数的结束，当向一个消息中添加参数时，最后要添加一个 `DBUS_TYPE_INVALID`，当我们获取参数时，当遇到 `DBUS_TYPE_INVALID` 时，表示参数遍历完成；

### 函数 `dbus_message_iter_get_basic()`：从消息迭代器读取基本类型值

```C
void dbus_message_iter_get_basic(DBusMessageIter *iter,
                                 void *value)	
```
* `value` 参数应该是存储返回值的位置的地址，因此，对于 `int32` 来说，它应该是 `dbus_int32_t *`，对于 `string` 来说，它应该是 `const char **`，返回值是引用值，不要使用 `free()` 释放；
* 要注意的是，调用本函数前，一定要使用 `dbus_message_iter_get_arg_type()` 确定数据类型，确保 value 的数据类型是正确的，否则可能程序会崩溃，比如，返回值是一个字符串，而 `value` 的数据类型是 `int32`，大概率程序会崩溃；

### 函数 `dbus_message_iter_next()`：将迭代器移动到下一个字段;
```C
dbus_bool_t dbus_message_iter_next(DBusMessageIter *iter);
```
- 使用迭代器遍历数组的时候，每处理完一个字段，都要使用这个函数使迭代器的指针指向下一个字段；
- 如果后面已经没有字段，该函数返回 FALSE，指针移动成功则返回 TRUE。

### 使用迭代器获取参数的步骤
    ```C
    DBusMessage *message;
    ......
    DBusMessageIter iter;
    dbus_message_iter_init(message, &iter);
    char arg_str[1024];
    uint64_t arg_uint64;
    do {
        int current_type = dbus_message_iter_get_arg_type(&iter);
        if (current_type == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &arg_str);
            goto NEXT_ITEM;
        }
        if (current_type == DBUS_TYPE_UINT64) {
            dbus_message_iter_get_basic(&iter, &arg_uint64);
            goto NEXT_ITEM;
        }
        ......
        if (current_type == DBUS_TYPE_ARRAY) {
            DBusMessageIter array_iter;
            dbus_message_iter_recurse(&iter, &array_iter);
            do {
                ......
            } while (dbus_message_iter_next(&array_iter));
        }
        if (current_type == DBUS_TYPE_STRUCT) {
            DBusMessageIter struct_iter;
            dbus_message_iter_recurse(&iter, &struct_iter);
            ......
            dbus_message_iter_next(&struct_iter);
            ......
            dbus_message_iter_next(&struct_iter);
            ......
        }
        ......
    } while (dbus_message_iter_next(&iter));
    ```

## 4 域名解析实例
* **源程序**：[dbus-hostname.c][src01](**点击文件名下载源程序，建议使用UTF-8字符集**)演示了使用 libdbus 请求系统服务 `systemd-resolved` 实现域名解析的过程；
* 编译：```gcc -Wall -g dbus-hostname.c -o dbus-hostname `pkg-config --libs --cflags dbus-1` ```
* 有关 `pkg-config --libs --cflags dbus-1` 可以参阅文章 [《IPC之十一：使用D-Bus实现客户端向服务端请求服务的实例》][article11] 中的简要说明；
* 该程序的使用方法：`./dbus-hostname <主机名> [地址族]`，主机名一定要有，地址族可以是 `AF_INET` 或者 `AF_INET6`，前者仅解析 IPv4 的 IP 地址，后者仅解析 IPv6 的 IP 地址，如果地址族没有填，则地址族设置为 `AF_UNSPEC`，其含义文中有介绍； 
* 运行：`./dbus-hostname baidu.com` 或者 `./dbus-hostname baidu.com AF_INET6`
* 运行截图：

    ![Screenshot of running dbus-hostname][img02]




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

[src01]: https://whowin.gitee.io/sourcecodes/100023/dbus-hostname.c


[img01]: https://whowin.gitee.io/images/100023/screenshot-of-systemctl.png
[img02]: https://whowin.gitee.io/images/100023/screen-of-dbus-hostname.png


