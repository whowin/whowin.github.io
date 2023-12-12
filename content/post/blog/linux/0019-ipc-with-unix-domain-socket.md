---
title: "IPC之九：使用UNIX Domain Socket进行进程间通信的实例"
date: 2023-10-16T16:43:29+08:00
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
  - pipe
  - socket
  - "AF_UNIX"
draft: false
#references: 
# - [Using AF_UNIX address family](https://www.ibm.com/docs/en/i/7.2?topic=families-using-af-unix-address-family)
# - [UNIX domain sockets](https://www.ibm.com/docs/en/ztpf/1.1.0.15?topic=considerations-unix-domain-sockets)
# - [Linux Interprocess Communications](https://tldp.org/LDP/lpg/node7.html)
# - [Unix domain socket](https://en.wikipedia.org/wiki/Unix_domain_socket)
# - [The difference between af_inet and Af_unix domains in socket communication](https://topic.alibabacloud.com/a/the-difference-between-af_inet-and-af_unix-domains-in-socket-communication_1_11_30546555.html)
# - [socketpair的用法和理解](https://blog.csdn.net/weixin_40039738/article/details/81095013)
# - [Sockets and Network Connections](https://dwheeler.com/secure-programs/Secure-Programs-HOWTO/sockets.html)
# - [Sequenced Socket Packet code example](https://www.hitchhikersguidetolearning.com/2020/04/26/sequenced-socket-packet-code-example/)
# - [Sequenced Packets Over Ordinary TCP](https://urchin.earth.li/~twic/Sequenced_Packets_Over_Ordinary_TCP.html)
# - [Linux's abstract namespace for Unix domain sockets](https://utcc.utoronto.ca/~cks/space/blog/linux/SocketAbstractNamespace)
# - [Abstract Namespace Socket Connections](http://www.hitchhikersguidetolearning.com/2020/04/25/abstract-namespace-af_unix-stream-socket-code-example/)
#       - 里面有一个例子

postid: 100019
---

socket 编程是一种用于网络通信的编程方式，在 socket 的协议族中除了常用的 AF_INET、AF_RAW、AF_NETLINK等以外，还有一个专门用于 IPC 的协议族 AF_UNIX，IPC 是 Linux 编程中一个重要的概念，常用的 IPC 方式有管道、消息队列、共享内存等，本文主要介绍用于本地进程间通信的 UNIX Domain Socket，本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文的实例中涉及多进程编程等，本文对 Linux 编程的初学者有一些难度。

<!--more-->

## 1 UNIX Domain Socket 的基本概念
* 本文不再回顾有关 socket 编程的相关知识，但是阅读本文需要掌握基本的 socket 编程方法，这方面的资料比较丰富，请自行解决；
* 关于 UNIX Domain Socket 的在线文档：`man 7 unix`；
* 在使用 socket() 创建一个 socket 时，如果使用 AF_UNIX 协议族，而不是我们常用的 AF_INET 时，创建的 sockst 被称为 UNIX Domain Socket；
* AF_UNIX 是一个用来进行进程间通信的协议族，AF_LOCAL 和 AF_UNIX 是完全一样的；
* AF_UNIX 协议族和 AF_INET 的主要区别：
    - 当使用 AF_UNIX 时，一个进程发送的数据无需再经过协议栈，而是通过内核缓冲区将数据直接拷贝到另一个进程的缓冲区中；
    - 当使用 AF_UNIX 时，无需再绑定 IP 地址，发送和接收过程也与 IP 地址无关；
* 作为 IPC 方法，socket 的 AF_UNIX 家族并没有共享内存的效率高，我认为其存在的价值主要是它的使用方法几乎和 socket 网络编程方法无异，这无疑给那些熟悉 socket 编程的程序员带来了极大的方便；
* 使用 AF_UNIX 建立的 socket 被称为 UNIX Domain Socket，又名 UDS 或者 IPC socket，常用于互联网通信的，使用 AF_INET(AF_INET6) 建立的 socket 被称为 Internet Socket；
* 一个 Internet socket 需要绑定在一个 IP 地址和一个端口上，与 Internet Socket 不同，UNIX socket 必须绑定到一个文件路径上，在后面会详细说明；
* UNIX socket 也可以是匿名的，也就是无需绑定到文件路径上，没有名称，通常匿名的 UNIX socket 只能用于父子进程或者有同一个父进程的子进程之间通信；
* 以下如无特别说明，socket 特指 UNIX Domain Socket，而非 Internet Socket。

-------------
## 2 命名 UNIX Socket 的使用
* 创建 UNIX Socket
    ```C
    #include <sys/socket.h>
    #include <sys/un.h>

    unix_socket = socket(AF_UNIX, type, 0);
    ```
    - **type** 可以是：
        + **SOCK_STREAM**：面向连接的数据流传输；
        + **SOCK_DGRAM**：面向无连接的数据报传输；
        + **SOCK_SEQPACKET**：面向连接的顺序数据包传输，可以按照发送的顺序传递消息；

* 绑定一个文件路径
    - 一个命名的 UNIX domain socket 必须绑定到一个路径下；
    - 在 IPv4 下，`Internet Socket` 绑定 IP 地址和端口时，使用 `struct sockaddr_in`，在 Unix Socket 上绑定文件路径时需要使用结构 `struct sockaddr_un`
        ```C
        struct sockaddr_un {
            sa_family_t sun_family;               /* AF_UNIX */
            char        sun_path[108];            /* Pathname */
        };
        ```
        + sun_family 一定是 AF_UNIX
        + sun_path 指向一个文件路径，比如 `/tmp/unix_socket`

    - bind()
        ```C
        #include <sys/types.h>          /* See NOTES */
        #include <sys/socket.h>

        int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
        ```
        + **sockfd** 是使用 socket() 建立的 UNIX Socket；
        + **addr** 指向 `struct sockaddr_un` 的指针

* 下面代码片段创建了一个 socket 并绑定到了一个文件路径上

    ```C
    ......
    int unix_sock;
    struct sockaddr_un serveraddr;

    unix_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, "/tmp/unix_socket");

    bind(unix_sock, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
    ......
    ```
* 建议使用宏 `SUN_LEN(addr)` 来计算填好文件路径的 `struct sockaddr_un` 的长度，因为其中的 `sun_path` 字段是不定长的，这个宏可以帮助我们计算出正确的结果，宏 `SUN_LEN(addr)` 定义在头文件 `sys/un.h` 中；
* 当把一个 socket 绑定到一个文件上时，在文件系统上，这个文件会真实存在，但是这个文件不能使用 open() 打开，而只能使用 socket() 打开；
* 建议在使用完一个命名 UNIX domain socket 后，显式地使用 unlink() 或者 remove() 将其删除，避免其残留在文件系统中；
* 尽管我们的例子中使用的文件名在 `/tmp/` 目录下，但在实际应用中我们并不建议这样做，因为 `/tmp/` 目录是任何人都可以读写的，这可能会带来一些安全隐患，通常可以把这个文件建立在项目所在的目录下，相对比较安全；
* 当我们用 bind() 将一个文件绑定到 socket 上时，相应的文件将被建立，该文件的默认权限为所绑定的 socket 的权限，通常情况下使用 socket() 建立的 socket 的权限是 0777，可以使用 fstat 获得，但这时建立的 socket 文件的默认属性是 0775；
* 如果我们在 bind() 之前先使用 fchmod() 修改 socket 的权限，那么再使用 bind() 绑定一个文件时，其建立的文件的权限也会产生变化，下面这段代码可以演示这种权限的变化；
    ```C
    ......
    #define SOCK_NAME       "./unix_socket.sock"
    ......
    int sockfd;
    struct sockaddr_un addr;
    struct stat sock_stat;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    fchmod(sockfd, 0660);

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_NAME);
    bind(sockfd, (struct sockaddr *)&addr, SUN_LEN(&addr));

    system("ls -l unix_socket.sock");
    ......
    ```
* 这段代码在当前目录下生成的 socket 文件的权限为 0660，最后一行的 `system("ls -l")` 显示的文件清单将清楚地显示出来，你可以尝试一下，如果没有 fchmod() 这行代码，生成的文件的权限为 0775；
* 使用 `ls -l` 显示文件清单时，该文件的前面有一个 **s** 标志，表示这是一个 socket 文件；
* socket 以及 socket 文件是可以使用 stat()、fstat() 获取属性，同时可以使用 chmod()、fchmod() 来改变权限的；
* 改变一个 socket 文件权限的意义在于安全性，因为其它进程是需要通过 socket 文件来使用这个 UNIX domain socket 的，那么这个进程必须要有相应的权限才行；

* 可以使用 `read()/write()、send()/recv()、sendto()/recvfrom()、sendmsg()/recvmsg()` 来发送和接收数据，与 Internet Socket 下的使用方法无大的差异；
* `read()/write()`
    ```C
    #include <unistd.h>

    ssize_t read(int fd, void *buf, size_t count);
    ssize_t write(int fd, const void *buf, size_t count);
    ```
    - 这对函数是文件的读/写函数，因为 socket 返回的是标准的文件描述符，所以可以使用这两个函数进行接收和发送数据；
    - 这对函数通常仅用于面向连接的 socket，也就是通常不用于 SOCK_DGRAM 类型的 socket；
    - 这对函数是比较简单的，write() 基本不会出错；
    - 使用 read() 接收数据时可能会有阻塞问题，当使用 SOCK_STREAM 时，可能还有粘包问题，不过这些问题在 Internet Socket 的 TCP 编程中也是一样存在的，并不是 UNIX Socket 所特有的，如果有这方面的问题，请参考有关资料；

* `recv()/send()`

    ```C
    #include <sys/types.h>
    #include <sys/socket.h>

    ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    ```
    - 这对函数理论上既可以用于有连接的 socket(AF_STREAM)，也可以用于无连接的 socket(AF_DGRAM)，但通常仅用于面向连接的 socket，也就是通常不用于 SOCK_DGRAM 类型的 socket；
    - 与 read()/write() 相比，这对函数多了一个参数 flags
    - 在使用 send() 发送数据时，绝大多数情况下，flags 都可以设置为 0，最常用的 falgs 设置是 MSG_DONTWAIT，但对于向 UNIX Socket 发送数据而言，如果发送的数据不是很大，是不可能产生阻塞的，所以不需要设置 MSG_DONTWAIT；
    - 在使用 recv() 接收数据时，当 socket 上没有数据时，会产生阻塞，如果不希望阻塞，可以设置 flags 为 MSG_DONTWAIT，这样可以让程序有更好的适应性；
    - 在使用 recv() 接收数据时，与 Internet Socket 的 TCP 编程一样，可能出现"粘包"问题，请参考相关资料；

* `recvfrom()/sendto()`
    ```C
    #include <sys/types.h>
    #include <sys/socket.h>

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen);
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, 
                   const struct sockaddr *dest_addr, socklen_t addrlen);
    ```
    - 这对函数通常多用于面向无连接的 socket，也就是通常仅用于 SOCK_DGRAM 类型的 socket；
    - 在 `Internet Socket` 的 UDP 编程中，需要使用 `struct sockaddr_in` 设置对端的 IP 地址和端口，而 `UNIX Socket` 要使用 `struct sockaddr_un` 来设置对端的文件路径；
    - **源程序**：[sendto-recvfrom.c][src01](**点击文件名下载源程序**)演示了如何使用 sendto() 和 recvfrom() 进行面向报文的进程间通信；
    - 编译：`gcc -Wall -g sendto-recvfrom.c -o sendto-recvfrom`
    - 运行：`./sendto-recvfrom`
    - 运行截图：

        ![screenshot of running sendto-recvfrom][img01]

-------
* `recvmsg()/sendmsg()`
    ```C
    #include <sys/types.h>
    #include <sys/socket.h>

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
    ```
    - 这对函数在 socket 上接收/发送数据相对较为复杂，主要是 `struct msghdr` 结构比较复杂，这对函数既可以用于有连接的 socket(AF_STREAM)，也可以用于无连接的 socket(AF_DGRAM)，在实践中也确实如此；
    ```C
    struct msghdr {
        void         *msg_name;       /* Optional address */
        socklen_t     msg_namelen;    /* Size of address */
        struct iovec *msg_iov;        /* Scatter/gather array */
        size_t        msg_iovlen;     /* # elements in msg_iov */
        void         *msg_control;    /* Ancillary data, see below */
        size_t        msg_controllen; /* Ancillary data buffer len */
        int           msg_flags;      /* Flags (unused) */
    };

    struct iovec {
        void  *iov_base;    /* Starting address */
        size_t iov_len;     /* Number of bytes to transfer */
    };

    ```
    - 在多数的编程实践中，我们并不需要使用这对函数，使用 `recv()/send()`、`read()/write()` 或者 `sendto()/recvfrom()` 已经足够；
    - `struct msghdr` 看似有很多字段，其实也并不是很复杂；
        + 最后一个字段 `msg_flags` 在 sendmsg() 中没有使用，在 recvmsg() 调用返回时将被设置为某个标志，可以不用管它；
        + `msg_control` 和 `msg_controllen` 是用来传输控制信息的，如果我们不传输控制信息，这两个字段填 NULL 和 0 即可；
        + 最前面的两个字段 `msg_name` 和 `mag_namelen`，在无连接的 socket(SOCK_DGRAM) 中有意义，在 sendmsg() 中用于指定目的地址，在 recvmsg() 中用于填充发送方的源地址，如果我们使用有连接的 socket(SOCK_STREAM)，则这两个字段只需填 NULL 和 0；
        + `msg_iov` 是一个结构数组，而 `msg_iovlen` 为这个数组的元素个数，这是这个结构中的灵魂所在；
    - 关于这对函数的使用可以参考在线文档 `man sendmsg`、`man recvmsg` 和 `man writev`，本文并不打算详细描述这对函数的使用方法，但会给出一个具体实例；
    - **源程序**：[sendmsg-recvmsg.c][src02](**点击文件名下载源程序**)演示了如何使用 sendmsg() 和 recvmsg() 进行面向报文的进程间通信；
    - 编译：`gcc -Wall -g sendmsg-recvmsg.c -o sendmsg-recvmsg`
    - 运行：`./sendmsg-recvmsg`
    - 运行截图：

        ![screenshot of running sendmsg-recvmsg][img02]

---------
* 实际上，这些在 socket 上发送/接收数据的函数并不需要成对使用，这个意思是说，我们可以使用 send() 发送数据，然后使用 recvmsg() 去接收 send() 发送的数据，没有任何问题。

* 关闭 socket 和删除 socket 所关联的文件
    ```C
    #include <unistd.h>

    close(unix_sock)
    unlink(UNIX_SOCK_FILE_NAME);
    ```

---------
## 3 UNIX socket 的抽象命名空间

> 当我们使用 `socket(AF_UNIX, ......)` 建立一个 socket，并将其绑定到一个文件路径上(比如：`/tmp/unix_sock.sock`)时，系统会在文件系统上建立一个真实的文件，当所有打开这个 socket 的进程均关闭 socket 后，这个真实的文件并不会因此而消失，必须显式地使用 unlink() 或者 remove() 删除这个文件，才能将这个文件删除，如果一个打开 socket 的进程异常退出，没有显式删除 socket 相关联的文件，将导致在文件系统上留下一些没有用处的文件，我们把这种现象称为“文件系统污染”，显然，UNIX domain socket 会造成文件系统污染；

* Linux 为 Unix Domain Socket 提供一种特殊的寻址方案，即所谓的"抽象命名空间(Abstract Namespace)"，它可以避免使用 UNIX domain socket 时造成文件系统污染；
* 使用抽象命名空间的 socket，在建立连接时可以不用在文件系统上建立一个真实文件，当所有打开这个 socket 的进程终止后，这个在抽象命名空间的 socket 也会随之消失；
* 使用抽象命名空间建立 socket，不必再显式地删除其绑定的文件路径，即使打开该 socket 的进程是异常退出，这个 socket 也会随着所有进程的终止而消失；
* 显式地删除一个 socket 关联的文件路径还会带来一些其它不可预知的问题，比如有可能删除一个其它进程正在使用的文件等等；
* 抽象命名空间最大的问题是其代码的可移植性并不好，所以在实际应用中很少见到相应的代码；
* 抽象命名空间的另一个问题是其安全性，在文件系统上建立的 socket 文件可以使用文件权限来控制其读写权限，相对较为安全，抽象命名空间没有权限，任何知道其名称的进程均可以使用该 socket，其安全性不好；
* 实际上，在抽象命名空间中建立一个 socket 与在普通用户空间上建立一个 socket 的区别并不大，在设定地址时，都是使用 `struct sockaddr_un`，但使用抽象命名空间时 `sun_path` 字段的第一个字符必须是 `'\0'`，下面代码片段演示了如何将一个 socket 绑定到抽象命名空间上：
    ```C
    ......

    int sock_server = -1;
    int rc;
    socklen_t length;
    struct sockaddr_un server_addr;

    sock_server = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, "#unix_sock.sock");
    length = SUN_LEN(&server_addr);
    server_addr.sun_path[0] = '\0';

    bind(sock_server, (struct sockaddr *)&server_addr, length);

    ......
    ```
* 这段代码将字符串 `#unix_sock.sock` 拷贝到 sun_path 字段，其中的 `'#'` 是一个占位字符，在后面的代码中，`'#'` 被替换为 `'\0'`；
* 要注意的是，一定要在替换 `'#'` 前使用 `SUN_LEN()` 宏去计算 `server_addr` 的长度，否则因为 `'\0'` 的替换将导致长度计算错误；
* **源程序**：[abstract-socket.c][src03](**点击文件名下载源程序**)演示了如何使用抽象命名空间 socket 进行进程间通信；
    - 该程序建立了两个进程，一个是服务端进程，另一个是客户端进程；
    - 服务端进程在抽象命名空间上建立了一个 socket 并侦听在该 socket 上；
    - 客户端进程连接到该 socket 上并发送消息，服务端进程收到消息并给客户端一个回应消息，然后两个进程分别退出；
    - 为了简化程序，突出抽象命名空间的使用，该程序的 server 进程没有处理多个客户端进程连接的情况；
    - 在 server 进程中，socket 绑定完抽象命名空间的 socket 名称并开始侦听该 socket 后，执行了 `lsof` 命令，可以很清楚第看到一个名为 `@unix_socket.sock`，类型为 `STREAM` 的对象被打开；
* 编译：`gcc -Wall -g abstract-socket.c -o abstract-socket`
* 运行：`./abstract-socket`
* 运行截图：

    ![screenshot of running abstract-socket][img03]

---------
## 4 顺序数据包(SOCK_SEQPACKET)
* 通常情况下，基于连接的 socket(SOCK_STREAM) 被认为可以可靠地传输数据，而基于无连接的 socket(SOCK_DGRAM) 被认为是不可靠的，在传输数据中，有可能丢失数据；
* 但是，使用 SOCK_STREAM 创建的 socket 是基于数据流的，数据报之间的边界是不清晰的，接收方收到的数据包可能被拆分或者合并，导致收到的数据包可能并不是一个完整的数据报或者其中还包含着下一个数据报的一部分，这就是常说的“粘包(Sticky Packet)”；
* 使用 SOCK_DGRAM 创建的 socket 也是有明显优点的，它是基于数据报的，所以可以保证每次收到的数据是一个数据报，不会有类似“粘包”的问题；
* 当然，我们希望有一种 socket，它是基于连接的，数据传输可靠，同时，数据报之间的边界清晰，不会产生“粘包”；
* 顺序数据包(SOCK_SEQPACKET)正是这样一种 socket，它是基于连接的的可靠传输，同时保证有明确的报文边界，通常认为它是介于 SOCK_STREAM 和 SOCK_DGRAM 之间的一种连接形式；
* 目前这种 socket 仅能在 `UNIX domain socket(AF_UNIX)` 上使用，在 `Internet socket(AF_INET)` 上不能使用，所以我们在通常的应用中看不到使用 `SOCK_SEQPACKET` 的例子；
* 使用 `SOCK_SEQPACKET` 建立 socket 的编程方法与 `SOCK_STREAM` 非常类似，除了在建立 socket 时使用 `SOCK_SEQPACKET` 以外基本没有任何区别；
* 以下是 `ChatGPT 3.5` 给出的 `SOCK_SEQPACKET` 的特性：
    1. **有界数据包传输**：`SOCK_SEQPACKET` 提供有界数据包传输，这意味着它可以保留数据包的边界。发送方在发送数据时定义了数据包的边界，接收方可以按照这个边界来接收和处理数据包。这对于需要确保消息边界的应用程序非常有用。
    2. **可靠性**：与 `SOCK_DGRAM` 套接字类型不同，`SOCK_SEQPACKET` 提供可靠的数据传输。它使用面向连接的传输协议来确保数据的可靠性，即数据在发送和接收之间保持顺序，并且不会丢失或重复。
    3. **面向连接**：`SOCK_SEQPACKET` 是面向连接的套接字类型。在进行数据传输之前，需要先建立连接。连接的建立过程确保了通信双方之间的可靠通信，并提供了数据包传输的顺序保证。
    4. **双向通信**：`SOCK_SEQPACKET` 套接字类型支持双向通信，即可以在连接上进行双向的数据传输。发送方可以发送数据给接收方，接收方可以发送响应数据给发送方。
    5. **适用于可靠的流式服务**：由于 `SOCK_SEQPACKET` 提供可靠的、有界的数据包传输，它适用于需要确保消息边界和可靠性的应用程序。它通常用于基于TCP的应用程序，如文件传输、视频流传输和实时通信应用。

    > 需要注意的是，与其他套接字类型相比，`SOCK_SEQPACKET` 套接字可能会引入一些性能开销，因为它需要维护消息边界和数据包的顺序。因此，在选择套接字类型时，应权衡应用程序的需求和性能要求。

* **源程序**：[seqpacket.c][src04](**点击文件名下载源程序**)演示了如何使用顺序数据包进行进程间通信，同时演示了 SOCK_STREAM 的“粘包”现象以及 SOCK_SEQPACKET 有清晰报文边界的特性；
    - 建立了三个进程，第一个是 SOCK_STREAM 类型 socket 的服务端进程，第二个是 SOCK_SEQPACKET 类型 socket 的服务端进程，第三个是客户端进程；
    - 客户端进程使用同样的程序分别向两个服务端进程发送了三条连续的报文；
    - 两个服务端进程除了 socket 类型不同外，其它程序完全一样；
    - SOCK_STREAM 服务端进程会一次性地收到客户端发来的三条报文，三条报文“粘”在一起；
    - SOCK_SEQPACKET 服务端每次只会收到一条完整报文，三条报文需要接收三次，报文边界清晰；
    - 为简单起见，服务端程序连续收到 5 个 EAGAIN(EWOULDBLOCK) 错误时认为客户端进程已经完成发送；
* 编译：`gcc -Wall -g seqpacket.c -o seqpacket`
* 运行：`./seqpacket`
* 运行截图：

    ![screenshot of running seqpacket][img04]

----------
## 5 匿名 UNIX Socket 的使用
* 前面介绍的 UNIX domain socket 均是需要命名的，或者引用一个文件路径，或者在抽象命名空间中引用一个名称；
* Linux 同样支持匿名的 UNIX domain socket，就像在文章[《IPC之一：使用匿名管道进行父子进程间通信的例子》][article01]介绍的匿名管道一样，匿名 UNIX domain socket 也是仅能用于父子进程或拥有同一个父进程的子进程间进行通信；
* 在使用匿名管道进行全双工通信时，需要建立两个管道，一个用于读，另一个用于写，匿名 UNIX domain socket 与之类似，也是需要建立一对 socket，一个用于读，另一个用于写，使用 socketpair() 建立一对 socket；
    ```C
    #include <sys/types.h>          /* See NOTES */
    #include <sys/socket.h>

    int socketpair(int domain, int type, int protocol, int sv[2]);
    ```

    - 调用成功，该函数返回 0，sv 数组中存放这一对 socket，调用失败返回 -1，errno 中为错误代码；
    - 返回的一对 socket 是已经连接好的，这意味着无需再调用其它函数，可以直接在 sv 返回的 socket 上进行读/写操作；
    - socketpair() 目前只能用在 AF_UNIX 上，所以参数 domain 只能是 AF_UNIX；
    - type 可以是 SOCK_STREAM、SOCK_DGRAM 或者 SOCK_SEQPACKET；
    - protocol 通常填 0 即可；
    - sv 是一个整数数组的指针，用于在调用成功时返回一对 socket，调用失败时，sv 数组的内容不会被改动；
* 使用 socketpair() 建立的 socket 对，与使用 socket() 建立的 socket，在编程上没有区别；
* 因为是匿名的，所以只能通过继承的方式将 socket 对传递给子进程，所以只能用于父子进程间或拥有同一父进程的子进程之间进行通信；
* **源程序**：[socketpair.c][src05](**点击文件名下载源程序**)演示了如何 socketpair() 建立的 socket 对进行进程间通信；
    - 在父进程中建立一个 socket 对；
    - 建立了两个子进程，第一个是服务端进程，第二个是客户端进程，socket 对通过继承传到子进程；
    - 服务端进程接收客户端进程发送的消息
* 编译：`gcc -Wall -g socketpair.c -o socketpair`
* 运行：`./socketpair`
* 运行截图：

    ![screenshot of running socketpair][img05]


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


[src01]: https://whowin.gitee.io/sourcecodes/100019/sendto-recvfrom.c
[src02]: https://whowin.gitee.io/sourcecodes/100019/sendmsg-recvmsg.c
[src03]: https://whowin.gitee.io/sourcecodes/100019/abstract-socket.c
[src04]: https://whowin.gitee.io/sourcecodes/100019/seqpacket.c
[src05]: https://whowin.gitee.io/sourcecodes/100019/socketpair.c

[img01]: https://whowin.gitee.io/images/100019/screenshot-sendto-recvfrom.png
[img02]: https://whowin.gitee.io/images/100019/screenshot-sendmsg-recvmsg.png
[img03]: https://whowin.gitee.io/images/100019/screenshot-abstract-socket.png
[img04]: https://whowin.gitee.io/images/100019/screenshot-of-seqpacket.png
[img05]: https://whowin.gitee.io/images/100019/screenshot-of-socketpair.png
