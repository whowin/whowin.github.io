---
title: "linux下使用netlink获取gateway的IP地址"
date: 2023-02-04T16:43:29+08:00
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
  - Socket
  - 网络编程
  - netlink
  - rtnetlink
draft: false
#references: 
# - [Default Gateway in C on Linux](https://stackoverflow.com/questions/548105/default-gateway-in-c-on-linux)
#     - 里面的源代码可以从文件/proc/net/route中获得gataway
# - [Linux : how to set default route from C?](https://stackoverflow.com/questions/22733967/linux-how-to-set-default-route-from-c/22751236)
#     - 使用ioctl设置默认网关的源代码
# - [How to Change default gateway vie C](https://stackoverflow.com/questions/57178084/how-to-change-default-gateway-vie-c)
#     - 另一个使用ioctl设置默认网关的源代码

# - [Linux用户空间与内核空间通信(Netlink通信机制)](https://cloud.tencent.com/developer/article/2139704)
# - [Netlink机制详解](https://blog.csdn.net/xinyuan510214/article/details/52635085)
# * [netlink消息类型和格式](https://insidelinuxdev.net/article/a09n96.html)
# * [Netlink分层模型及消息格式](https://onestraw.github.io/linux/netlink-message/)
#     - 这个人github博客里有几篇关于netlink的文章

# * [Introduction to Netlink](https://www.kernel.org/doc/html/latest/userspace-api/netlink/intro.html)
# - [rtnetlink(7) — Linux manual page](https://man7.org/linux/man-pages/man7/rtnetlink.7.html)
# - [netlink(7) - Linux man page](https://linux.die.net/man/7/netlink)
# * [netlink(3) - Linux man page](https://linux.die.net/man/3/netlink)
#     - NLMSG_宏的说明
# - [Getting Linux routing table using netlink](https://olegkutkov.me/2019/03/24/getting-linux-routing-table-using-netlink/)
#     - 里面有完整的源代码，使用了iovec结构体
# - [Getting gateway to use for a given ip in ANSI C](https://stackoverflow.com/questions/3288065/getting-gateway-to-use-for-a-given-ip-in-ansi-c)
#     - 里面的源代码是本文源代码的出处

# * [Netlink Sockets](https://nscpolteksby.ac.id/ebook/files/Ebook/Computer%20Engineering/Linux%20Kernel%20Networking%20-%20Implementation%20(2014)/chapter%202%20Netlink%20Sockets.pdf)
#     - pdf文档：chapter 2 Netlink Sockets.pdf
# - [Linux Kernel Networking - Implementation (2014)](https://nscpolteksby.ac.id/ebook/files/Ebook/Computer%20Engineering/Linux%20Kernel%20Networking%20-%20Implementation%20(2014)/)
postid: 180009
---

要在linux下的程序中获取gateway的IP地址，使用netlink是一种直接、可靠的方法，不需要依赖其它命令，直接从linux内核获取信息，netlink编程的中文资料很少，本文试图用尽可能简单的方式讨论使用netlink获取gataway的IP地址的编程方法，并有大量篇幅介绍netlink的相关数据结构和编程方法，阅读本文需要对linux下编程有一定了解，熟悉IPv4的socket编程；本文对网络编程的初学者有较大难度。
<!--more-->

------
> 在linux编程的资料中，netlink编程的资料并不多，但netlink编程显然是本文无法越过的一道坎，所以下面需要用一定篇幅对netlink编程做个介绍；本文的最终目标是使用netlink这种与linux内核通信的机制，从内核获得路由表并从中找到gateway的IP地址。在具体实践中，获取路由表或者获取gateway的IP地址通常并不需要使用netlink编程实现，这种方法对应用层编程来说显得有些繁琐，本文主要还是作为netlink编程的一个范例，并以此为题介绍一些netlink的编程方法；有关其它获取gateway的IP地址的方法，请参见另一篇文章[《从proc文件系统中获取gateway的IP地址》][article1]。

## 1. netlink socket及netlink消息结构
* **netlink socket**
  - netlink是一个socket，所以它的编程与普通的socket编程类似，其socket的创建方法如下：
    ```C
    sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    ```
  - 第一个参数可以是AF_NETLINK或者PF_NETLINK，表示要创建一个NETLINK socket，其实AF_NETLINK和PF_NETLINK是一样的，可以到 ```bits/socket.h``` 中查看相应的定义；
  - 第二个参数可以是SOCK_RAW或者SOCK_DGRAM，linux内核的netlink子系统并不会区分SOCK_RAW和SOCK_DGRAM(内核5.15，也许更高的版本会区分)，所以使用SOCK_RAW和SOCK_DGRAM是一样的；
  - 第三个参数是NETLINK协议，这些协议定义在文件 ```linux/netlink.h``` 中，在linux 5.15中定义了23种协议，协议数量最多为32个
    ```C
    #define NETLINK_ROUTE       0    /* Routing/device hook                     */
    #define NETLINK_UNUSED      1    /* Unused number                           */
    #define NETLINK_USERSOCK    2    /* Reserved for user mode socket protocols */
    #define NETLINK_FIREWALL    3    /* Unused number, formerly ip_queue        */
    #define NETLINK_SOCK_DIAG   4    /* socket monitoring                       */
    #define NETLINK_NFLOG       5    /* netfilter/iptables ULOG */
    #define NETLINK_XFRM        6    /* ipsec */
    #define NETLINK_SELINUX     7    /* SELinux event notifications */
    #define NETLINK_ISCSI       8    /* Open-iSCSI */
    #define NETLINK_AUDIT       9    /* auditing */
    #define NETLINK_FIB_LOOKUP  10    
    #define NETLINK_CONNECTOR   11
    #define NETLINK_NETFILTER   12    /* netfilter subsystem */
    #define NETLINK_IP6_FW      13
    #define NETLINK_DNRTMSG     14    /* DECnet routing messages */
    #define NETLINK_KOBJECT_UEVENT  15    /* Kernel messages to userspace */
    #define NETLINK_GENERIC     16
    /* leave room for NETLINK_DM (DM Events) */
    #define NETLINK_SCSITRANSPORT   18    /* SCSI Transports */
    #define NETLINK_ECRYPTFS    19
    #define NETLINK_RDMA        20
    #define NETLINK_CRYPTO      21    /* Crypto layer */
    #define NETLINK_SMC         22    /* SMC monitoring */
    ```
  - 尽管定义的协议很多，但常用的协议并不多，本例中仅使用**NETLINK_ROUTE**；
  - 创建netlink socket，netlink协议使用**NETLINK_ROUTE**时，创建的socket又被称为**rtnetlink**，可以使用在线手册 ```man 7 rtnetlink``` 了解更详细的信息，本文仅讨论rtnetlink，以下将尽可能使用rtnetlink代替netlink socket。

  > netlink socket本质上是进程间的通信，一个用户进程使用netlink socket不仅可以和内核进程进行通信，两个(多个)用户进程间也可以使用netlink socket进行通信；


  > netlink除了使用内核定义的协议外，也可以使用自定义协议，在上面定义的netlink协议中的NETLINK_GENERIC就是用于用户自定义协议的，我们可以编写一个内核进程，然后在用户进程中使用NETLINK_GENERIC进行通信；


  > netlink socket有一个多播的特性，所谓多播，就是一个进程发出的消息可以有多个其它进程接收，netlink允许最多32个多播组，当向一个多播组发出消息时，所有已经加入这个多播组的进程都可以收到这个消息。


  > 当使用rtnetlink与内核进行通信时，**用户进程需要先向内核进程发出一个请求，然后接收内核进程返回的消息**，从而实现与内核的通信；


  > netlink socket可以使用send、sendto、sendmsg发送消息，使用recv、recvfrom、recvmsg接收消息，这些和IPv4 socket是一样的；

* **netlink消息结构**
  - netlink消息是由一个或多个netlink消息头和与其相关的payload组成的一个字节流

    ![netlink信息][img01]

  ------------
  - netlink报头的结构为(struct nlmsghdr)，在头文件 ```<linux/netlink.h>``` 中定义，在下一节中会介绍了这个结构；
  - 这个字节流只能使用一组标准的以NLMSG_开头的宏进行存取(netlink的手册中是这样说的)，这组宏定义在头文件linux/netlink.h中，可以使用在线手册```man 3 netlink```了解更多的信息，本文在后面章节也会简要介绍这些宏；
  - payload对不同的netlink协议和信息类型而言，其信息结构和长度都是不一样的，本文仅讨论NETLINK_ROUTE协议下的信息类型为RTM_GETROUTE下的消息结构，这个类型的payload结构为(struct rtmsg)，定义在头文件 ```<linux/rtnetlink.h>``` 中，这个结构也会在下一节介绍；详细信息可以查阅 ```man 7 rtnetlink``` ；
  - 使用rtnetlink与内核通信时，需要首先建立一个rtnetlink，然后向这个rtnetlink发送一个请求消息，请求中说明想要的操作，然后从这个rtnetlink上接收内核的回应，解析回应信息获得结果；
  - 在多部分消息中(一个字节流中包含多个netlink报头和相关payload)，第一个报头和后面所有的报头都会设置NLM_F_MULTI标志，最后一个报头的类型为NLMSG_DONE，表示多部分消息结束；有关NLM_F_MULTI和NLMSG_DONE的含义，下一节会做介绍。

## 2. rtnetlink常用数据结构
> netlink编程涉及的各种结构非常多，这里仅介绍几个与本文讨论的话题相关的结构，但仍然会占很大的篇幅，这几个结构在第5、6节会大量用到，所以必须先介绍一下，**可以先大致看一下**，在阅读相应章节遇到具体结构时再回来仔细看。

1. **struct sockaddr_nl** - 定义在头文件 ```<linux/netlink.h>``` 中
  ```C
  struct sockaddr_nl {
      __kernel_sa_family_t  nl_family;  /* AF_NETLINK  */
      unsigned short        nl_pad;     /* zero        */
      __u32                 nl_pid;     /* port ID     */
      __u32                 nl_groups;  /* multicast groups mask */
  };
  ```
  - 这个结构与IPv4 socket编程中的(struct sockaddr_in)的作用类似，(struct sockaddr_in)表示一个IPv4地址，这个结构表示一个netlink地址
  - nl_family字段与(struct sockaddr_in)中的sa_family一样，只是这里要填AF_NETLINK
  - nl_pad目前没有使用，填0即可；
  - nl_pid是当前进程的ID，可以使用getpid()获得；
  - nl_groups表示加入到那个多播组中，前面说过netlink最多允许32个多播组，这个字段每个bit代表一个多播组，为1表示加入这个多播组。

2. **struct nlmsghdr** - 定义在头文件 ```<linux/netlink.h>``` 中
  - 这个结构是netlink报头的定义
    ```C
    struct nlmsghdr {
        __u32   nlmsg_len;    /* Length of message including header */
        __u16   nlmsg_type;   /* Message content */
        __u16   nlmsg_flags;  /* Additional flags */
        __u32   nlmsg_seq;    /* Sequence number */
        __u32   nlmsg_pid;    /* Sending process port ID */
    };
    ```
  - nlmsg_len: netlink报文的长度，按4字节对齐；包括(struct nlmsghdr)的长度和后面payload的长度；
  - nlmsg_type: netlink报文的类型，不同的类型对应的netlink报文的结构也会不同，这些类型定义在头文件 ```<linux/rtnetlink.h>``` 中开头为"RTM_"的常数，本例中在发送netlink请求时类型为：RTM_GETROUTE，含义为从内核获取路由表，常用的常数有：
    ```C
    NLMSG_NOOP      1       无用，可忽略
    NLMSG_ERROR     2       出现错误
    NLMSG_DONE      3       数据输出结束
    NLMSG_OVERRUN   4       数据丢失

    RTM_NEWROUTE    24      新路由
    RTM_GETROUTE    26      从内核中获取路由表，发送netlink包请求时填写
    ```
  - nl_flags: 附加标志，在 ```<linux/netlink.h>``` 中定义，每一位代表一个标志，与本文相关的定义如下：
    ```
    NLM_F_REQUEST   0x01    这是一个请求信息
    NLM_F_MULTI     0x02    分片信息包的一部分，直到收到NLMSG_DONE结束

    NLM_F_ROOT      0x100   整体返回数据，而不是一条一条返回
    NLM_F_MATCH     0x200   返回所有相符的数据

    NLM_F_DUMP      (NLM_F_ROOT|NLM_F_MATCH)

    ```
  - nlmsg_seq: 序列号，信息的唯一性编号，可以自行编号，只要保证唯一性即可，通常可以使用时间戳；
  - nlmsg_pid: 发送方的pid，用于识别信息的归属，可以使用getpid()获得；
  - 所有netlink报文的报头都是这个结构。

3. **struct rtmsg** - 定义在头文件linux/rtnetlink.h中
  - 这个结构通常是紧跟在(struct nlmsghdr)后面的，当(struct nlmsghdr)中的nlmsg_type为 **RTM_NEWROUTE**、**RTM_DELROUTE**和**RTM_GETROUTE**时，(struct nlmsghdr)后面跟的数据才符合下面的结构，当nlmsg_type为其他值时，(struct nlmsghdr)后面跟的数据结构是不同的；
    ```C
    struct rtmsg {
        unsigned char rtm_family;   /* Address family of route */
        unsigned char rtm_dst_len;  /* Length of destination */
        unsigned char rtm_src_len;  /* Length of source */
        unsigned char rtm_tos;      /* TOS filter */
        unsigned char rtm_table;    /* Routing table ID */
        unsigned char rtm_protocol; /* Routing protocol */
        unsigned char rtm_scope;    /* See below */
        unsigned char rtm_type;     /* See below */

        unsigned int  rtm_flags;
    };
    ```
  - 以下仅讨论当(struct nlmsghdr)中的nlmsg_type为**RTM_GETROUTE**时(与本文例子相符)，该结构的情况
  - 将rtm_dst_len和rtm_src_len设置为0表示想要获取指定路由表(routing table)中的所有项；
  - 在发送netlink请求时，只需把rtm_family设为AF_INET，其它全部为0即可；
  - 内核发回的回应中，rtm_dst_len、rtm_src_len、rtm_tos均没有意义可以不用管；
  - rtm_table表示当前路由表的类型，在linux/rtnetlink.h中有定义：
    ```C
    enum rt_class_t {
        RT_TABLE_UNSPEC=0,
        /* User defined values */
        RT_TABLE_COMPAT=252,
        RT_TABLE_DEFAULT=253,
        RT_TABLE_MAIN=254,
        RT_TABLE_LOCAL=255,
        RT_TABLE_MAX=0xFFFFFFFF
    };
    ```
  - RT_TABLE_UNSPEC表示是一个不明路由表；RT_TABLE_DEFAULT表示是一个默认路由表；RT_TABLE_MAIN表示是一个主路由表；RT_TABLE_LOCAL表示是一个本地路由表；在本例中，我们要得到的就是一个主路由表(RT_TABLE_MAIN)
  - rtm_protocol是路由协议，在 ```<linux/rtnetlink.h>``` 中有定义：
    ```C
    #define RTPROT_UNSPEC     0     不明
    #define RTPROT_REDIRECT   1     当前的IPv4下没有使用
    #define RTPROT_KERNEL     2     路由由内核设置
    #define RTPROT_BOOT       3     路由在启动时设置
    #define RTPROT_STATIC     4     路由由管理员设置
    ```
  - 在头文件 ```<linux/rtnetlink.h>``` 中专门有说明，当 rtm_protocol>RTPROT_STATIC 时，Linux内核将不予理会，只能作为用户信息；在本例中，我们得到的路由表应该是由内核或者管理员设置的，当然也可以是在启动中由某个启动例程设置，所以 **RTPROT_KERNEL**、**RTPROT_BOOT**或者**RTPROT_STATIC** 都是可能的；
  - rtm_scope的值也是定义在 ```<linux/rtnetlink.h>``` 中，在这个头文件中说rtm_scope更像是一个到达目的地址的距离，它有下面几个可能的值：
    ```C
    enum rt_scope_t {
        RT_SCOPE_UNIVERSE=0,      /* global route */
    /* User defined values */
        RT_SCOPE_SITE=200,        /* interior route in the local autonomous system */
        RT_SCOPE_LINK=253,        /* route on this link */
        RT_SCOPE_HOST=254,        /* route on the local host */
        RT_SCOPE_NOWHERE=255      /* destination doesn't exist */
    };
    ```
  - 其实这个值没有什么意义，本例中会返回RT_SCOPE_UNIVERSE，表示是一个全球路由；
  - rtm_type表示当前路由的类型(rtm_table表示路由表类型，和这个字段是不同的)，在 ```<linux/rtnetlink.h>``` 中定义：
    ```C
    enum {
        RTN_UNSPEC,
        RTN_UNICAST,      /* Gateway or direct route */
        RTN_LOCAL,        /* Accept locally */
        RTN_BROADCAST,    /* Accept locally as broadcast,
                              send as broadcast */
        RTN_ANYCAST,      /* Accept locally as broadcast,
                              but send as unicast */
        RTN_MULTICAST,    /* Multicast route */
        RTN_BLACKHOLE,    /* Drop */
        RTN_UNREACHABLE,  /* Destination is unreachable */
        RTN_PROHIBIT,     /* Administratively prohibited */
        RTN_THROW,        /* Not in this table */
        RTN_NAT,          /* Translate this address */
        RTN_XRESOLVE,     /* Use external resolver */
        __RTN_MAX
    };
    ```
  - 本例中，我们要获取的是gateway IP，所以内核返回的rtm_type为1(RTN_UNICAST)
  - rtm_flags在本例中没有作用，其可能的值在 ```<linux/rtnetlink.h>``` 中定义：
    ```C
    #define RTM_F_NOTIFY        0x100     /* Notify user of route change */
    #define RTM_F_CLONED        0x200     /* This route is cloned */
    #define RTM_F_EQUALIZE      0x400     /* Multipath equalizer: NI */
    #define RTM_F_PREFIX        0x800     /* Prefix addresses */
    #define RTM_F_LOOKUP_TABLE  0x1000    /* set rtm_table to FIB lookup result */
    #define RTM_F_FIB_MATCH     0x2000    /* return full fib lookup match */
    ```

4. **struct rtattr** - 定义在头文件 ```<linux/rtnetlink.h>``` 中
  - 一个或多个(struct rtattr) + data将跟在(struct rtmsg)后面，data里表达着一个路由中的一项属性；
    ```C
    struct rtattr {
        unsigned short  rta_len;
        unsigned short  rta_type;
    };
    ```
  - rta_len字段表示(struct rtattr) + data的总长度，rta_type表示data中的数据类型，在 ```<linux/rtnetlink.h>``` 中定义：
    ```C
    enum rtattr_type_t {
        RTA_UNSPEC,
        RTA_DST,
        RTA_SRC,
        RTA_IIF,
        RTA_OIF,
        RTA_GATEWAY,
        RTA_PRIORITY,
        RTA_PREFSRC,
        ......
        RTA_TABLE,
        ......
        __RTA_MAX
    };
    ```
  - 本例中只会用到前面几个定义，如果需要查看全部的rta_type的值，可以去 ```<linux/rtnetlink.h>``` 中去查找，以下仅就本文中用到的值做一下解释；
  - **RTA_UNSPEC** 表示data数据可以忽略；
  - **RTA_DST** 表示data中的数据为一个目的地址IP，data中数据结构为(struct in_addr)，一个32位16进制数字表示的IP地址；
  - **RTA_SRC** 表示data中的数据为一个源地址IP，data中数据结构为(struct in_addr)，一个32位16进制数字表示的IP地址；
  - **RTA_IIF** 表示data中的数据为输入接口索引(Input Interface Index)，data中数据为一个int；
  - **RTA_OIF** 表示data中的数据为输出接口索引(Onput Interface Index)，data中数据为一个int；
  - **RTA_GATEWAY** 表示data中的数据为gateway的IP地址，data中数据结构为(struct in_addr)，一个32位16进制数字表示的IP地址；
  - **RTA_PRIORITY** 表示data中的数据为路由优先级，data中数据为一个int；
  - **RTA_PREFSRC** 表示data中的数据为一个优先的源地址IP，data中数据结构为(struct in_addr)，一个32位16进制数字表示的IP地址；
  - **RTA_TABLE** 表示data中的数据为路由表的ID，data中数据为一个int；
  - 其实**RTA_SRC**和**RTA_PREFSRC**到底区别在哪里我也不知道，我以为会返回RTA_SRC，但在我的机器上实际返回的是RTA_PREFSRC；
  - 本例中我们会遇到的rta_type有：RTA_DST、RTA_OIF、RTA_GATEWAY、RTA_PRIORITY、RTA_PREFSRC和RTA_TABLE。

## 3. netlink编程中常用的宏定义
> 按照netlink手册中的要求，对(struct nlmsghdr)的访问要使用一组标准的宏来完成，本节将简单介绍这组宏，也可以使用在线手册 ```man 3 netlink``` 来更多地了解这组宏；这些宏定义在头文件 ```<linux/netlink.h>``` 中，必要时可以查看代码理解其意义。


> 这些宏这样看上去会很枯燥，但在第5、6节和源代码中均会大量出现，可以先大致看一下，等看到相关章节遇到具体的宏时在回来仔细阅读。

* **NLMSG_ALIGN(len)**
  - 返回len值按照4字节对齐后的值，比如len=5，则返回8；len=17，返回20；
  - 将len按照适当的值进行字节对齐；所谓适当的值，指的是宏NLMSG_ALIGNTO定义的值，目前宏NLMSG_ALIGNTO为4。

* **NLMSG_LENGTH(len)**
  - 返回(struct nlmsghdr)按4字节对齐的长度 + len的值；
  - 如果len是payload的长度，该宏返回的值应该和存放在(struct nlmsghdr)中的nlmsg_len字段的值一样

* **NLMSG_SPACE(len)**
  - 计算(struct nlmsghdr)按4字节对齐的长度 + len的值，返回这个值按4字节对齐后的结果；
  - 返回payload长度为len的netlink消息占用的字节数。

* **NLMSG_DATA(nlh)**
  - 假定nlh为一个指向(struct nlmsghdr)的指针，该宏将返回(struct nlmsghdr)后面payload的指针。

* **NLMSG_NEXT(nlh, len)**
  - 在多部分消息中，收到的报文中会有多个(struct nlmsghdr) + payload，nlh是指向当前(struct nlmsghdr)的指针，len是从nlh开始到报文结束的总字节数，该宏返回指向下一个(struct nlmsghdr)的指针，同时修改len为返回的(struct nlmsghdr)指针到报文结束的总字符数。
  - 应用程序在调用前需要自行检查当前的(struct nlmsghdr)是否设置了NLMSG_DONE，该宏不会检查该标志并返回NULL。

* **NLMSG_OK(nlh, len)**
  - 当nlh指向的(struct nlmsghdr)完好时会返回true，否则返回false；
  - nlh指向当前的(struct nlmsghdr)，len是从nlh开始到报文结束的总字节数；
  - (struct nlmsghdr)中有一个字段为nlmsg_len，该字段表示(struct nlmsghdr)和后面数据的总长度，
  - 该宏要求符合下面三个逻辑：
    1. len > (struct nlmsghdr)的长度
    2. (nlmsghdr).nlmsg_len > (struct nlmsghdr)的长度
    3. (nlmsghdr).nlmsg_len < len

* **NLMSG_PAYLOAD(nlh, len)**
  - 返回data的长度；
  - 假定nlh指向一个(struct nlmsghdr)的指针，len为data在payload部分的偏移；
  - (struct nlmsghdr)中有一个字段为nlmsg_len，该字段表示(struct nlmsghdr)和后面数据的总长度；
  - 该宏返回nlmsg_len - (struct nlmsghdr)长度 - len的值；
  - 举例来说，当len=0时，该宏将返回payload的长度，这也是这个宏的一种常用的方式。

## 4. rtnetlink编程中常用的宏
> 在头文件 ```<linux/rtnetlink.h>``` 中定义了一组操作(struct rtmsg)的宏，以RTM_开头，只有两个；还定义了一组操作(struct rtattr)的宏，以RTA_开头，在这里简单介绍一下；使用在线手册 ```man rtnetlink``` 可以了解关于操作(struct rtattr)的宏的更详细的信息。


> 这些宏这样看上去会很枯燥，但在第5、6节和源代码中均会大量出现，可以先大致看一下，等看到相关章节遇到具体的宏时在回来仔细阅读。

* **RTM_RTA(r)**
  - 返回在r后面指向(struct rtattr)的指针；
  - r为指向(struct rtmsg)的指针；
  - 该宏认为(struct rtmsg)后面就是(struct rtattr)；

* **RTM_PAYLOAD(n)**
  - 返回(struct rtmsg)后面的payload的长度
  - n为一个指向(struct nlmsghdr)的指针
  - 该宏认为报文的结构为(struct nlmsghdr) + (struct rtmsg) + payload
  - 该宏认为(struct nlmsghdr)中的nlmsg_len为当前报文的总长度，减去(struct nlmsghdr)的长度，再减去(struct rtmsg)的长度就是payload的长度。

* **RTA_OK(rta, len)**
  - 当rta指向的(struct rtattr)完好时返回true，否则返回false；
  - rta指向当前(struct rtattr)的指针，len为(struct rtattr) + data的字节数，通常情况下，len不能为0；
  - 该宏要求符合下面三个逻辑：
    1. len >= (struct rtattr)的长度
    2. (rta).rta_len >= (struct nlmsghdr)的长度
    3. (rta).rta_len <= len

* **RTA_DATA(rta)**
  - 返回一个指向data的指针；rat为一个指向(struct rtattr)的指针；

* **RTA_PAYLOAD(rta)**
  - 返回data的长度；rat为一个指向(struct rtattr)的指针；

* **RTA_NEXT(rta, attrlen)**
  - 返回rta后面的下一个(struct rtattr)的指针；rta指向当前(struct rtattr)，attrlen为所有剩余(struct rtattr) + data的长度，初始值应该是所有(struct rtattr) + data的总长度；

* **RTA_LENGTH(len)**
  - 返回(struct rtattr)按4字节对齐的长度 + len的值；
  - 当len为data的长度，该宏返回的值应该和(struct rtattr)中的rta_len一样；

* **RTA_SPACE(len)**
  - 计算(struct rtattr)按4字节对齐的长度 + len的值，返回这个值按4字节对齐后的结果；
  - 返回data长度为len的(struct rtattr) + data占用的字节数。

## 5. 使用rtnetlink发送请求
> 前面讨论了一堆预备知识后，现在终于可以进行实际操作了；

1. **建立一个rtnetlink**
  ```C
  int nl_sock;
  nl_sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  ```
2. **构建netlink请求报文**
  - 请求消息的结构比较简单，就两个结构：(struct nlmsghdr) + (struct rtmsg)

    ![rtnetlink请求消息][img02]

    ----------
  - 下面代码构建了一个请求报文

    ```C
    struct nlmsghdr *nl_msg_hdr;
    struct rtmsg *rt_msg;
    char *msg_buf;
    int msg_buf_len;

    msg_buf_len = NLMSG_SPACE(sizeof(struct rtmsg));
    msg_buf = malloc(msg_buf_len);
    memset(msg_buf, 0, msg_buf_len);

    nl_msg_hdr = (struct nlmsghdr *)msg_buf;
    rt_msg = (struct rtmsg *)NLMSG_DATA(nl_msg_hdr);

    nl_msg_hdr->nlmsg_len   = msg_buf_len;
    nl_msg_hdr->nlmsg_type  = RTM_GETROUTE;

    nl_msg_hdr->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    nl_msg_hdr->nlmsg_seq   = time(NULL);
    nl_msg_hdr->nlmsg_pid   = getpid();
    rt_msg->rtm_family  = AF_INET;
    ```
  - NLM_SPACE()是一个操作(struct nlmsghdr)的宏，前面有介绍；
  - (struct nlmsghdr)中的nlmsg_type=RTM_GETROUTE，通知linux内核要获取路由表，这个字段在介绍(struct nlmsghdr)时有介绍；
  - (struct nlmsghdr)中的nlmsg_flags是一个组合标志，在介绍(struct nlmsghdr)时有介绍；
  - NLM_F_REQUEST表示这条报文是一条请求报文；
  - NLM_F_DUMP是NLM_F_ROOT和NLM_F_MATCH的组合，表示需要**所有**符合条件的记录，所有记录一次性发送过来，而不是一条一条发；
  - nlmsg_seq使用时间戳是一种常用的方法，也可以自行编号，但要保证唯一性；
  - AF_INET表示只要IPv4的路由表。

3. **发送netlink请求**
  ```C
  send(nl_sock, nl_msg_hdr, nl_msg_hdr->nlmsg_len, 0);
  ```

## 6. 接收和解析内核返回的路由表
> 通常的做法是先把所有的报文接收下来，然后再去解析，接收数据的方法和在IPv4下使用socket接收数据的方法是一样的。

1. **接收数据**
    > 接收数据之前并不知道有多少字节的数据需要接收，所以很难确定接收缓冲区的大小，所以最好是先检查一下socket上可以接收到多少字节的数据，然后给接收缓冲区分配内存，再去接收数据；

    > netlink返回的路由表的报文有点意思，路由表不会只有一条记录，所以这个返回的报文一定是个多部分消息，应该是由多个(struct nlmsghde) + payload组成，最后一个(struct nlmsghdr)中的nlmsg_type=NLMSG_DONE，表示整个路由表传送完毕，实际接收时发现要接收两次，第一次接收到的报文中，没有NLMSG_DONE消息，再接收一次，接收到的一个单独的NLMSG_DONE消息。

    > 强调接收两次是因为我们首先要使用recv()测试一下有多少字节需要接收，然后为接收缓冲区分配内存，但是这个测试只能测试第一次要接收的报文长度，这个长度并不包括第二次接收时的NLMSG_DONE消息的长度，所以获得的字节数实际还要加上(struct nlmsghdr) + (struct rtmsg)的长度，最后加上的这部分就是NLMSG_DONE的消息长度，请看下面的代码。


    ```C
    char *buf_ptr, *p;
    int buf_size, msg_len = 0, read_len;
    buf_size = recv(nl_sock, NULL, 0, MSG_PEEK|MSG_TRUNC));
    buf_size += NLMSG_SPACE(sizeof(struct rtmsg));

    buf_ptr = malloc(buf_size);
    memset(buf_ptr, 0, buf_size);

    p = buf_ptr;
    do {
        read_len = recv(nl_sock, p, buf_size - msg_len, MSG_DONTWAIT);
        if (read_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // TODO:
                break;
            }
            // TODO:
        } else if (read_len == 0) {
            // TODO:
        }
        p += read_len;
        msg_len += read_len;
    } while (msg_len < buf_size && read_len > 0);
    ```
  - ```NLMSG_SPACE(struct rtmsg)``` 这个宏计算了NLMSG_DONE这个报文所需要的空间；
  - recv()函数中的 **MSG_PEEK|MSG_TRUNC** 是一种固定用法，可以预知下一个报文有多少字节可以读取，但是并不会把这个报文标志为已读；
  - MSG_DONTWAIT使recv()执行时为无阻塞，按照 ```man recv``` 的说明，使用MSG_DONTWAIT时，如果recv()必须阻塞(意即无数据可读，需要等待在socket上)，则会返回错误，errno为EAGAIN或者EWOULDBLOCK，所以对这两个错误码一定要做判断；
  - 实际读取数据时发现NLMSG_DONE报文中的(struct nlmsghdr)是完整的，但后面并没有完整的(struct rtmsg)，而且其中并没有找到有效的信息，可以忽略。

2. **解析路由表报文**
  - 路由表报文结构(多部分报文)

    ![rtnetlink routing table][img03]
  
  ---------------
  - 多部分报文中，每一部分有一个(struct nlmsghdr)和一个(struct rtmsg)，在(struct rtmsg)后面有一个或多个(struct rtattr)，在(struct rtarrt)后面跟着数据，如下图，我们把这样一块数据叫做多部分报文中的一部分；

    ![a route of routing table][img04]

  ----------
  - 多部分报文的每一部分表示一条记录，具体到路由表就是路由表中的每条记录将构成多部分报文的一部分；我们可以用命令 ```cat /proc/net/route``` 查看实际的路由表，这个路由表中有几条记录，那么linux内核就会返回几个部分，最后在加上一个NLMSG_DONE消息；
  - 在每个部分中，(struct nlmsghdr)中的nlmsg_len定义了这一部分的总长度；(struct rtattr)中rta_len定义了(struct rtattr) + data的长度，rta_type定义了data中是什么数据，具体可以参阅前面有关数据结构的介绍；
  - 在每个部分中，(struct rtmsg)用来说明这条记录的特征，比如协议族，是IPv4还是IPv6；路由表是主路由表还是本地路由表；这条路由是内核设置的还是启动过程中设置的；等等，因为这些特征不能在(struct nlmsghdr)中表示，在(struct rtattr)中表示也不合适；
  - 我们需要的数据其实在data中，我们来举一个实际的例子来说明(struct rtattr) + data；假定要表达一个gateway的IP地址为192.168.0.1，则收到的数据如下(按16进制显示)：
    ```plaintext
    08 05 c0 a8 00 01
    ```
  - 其中08是rta_len，表示(struct rtattr) + data的长度为8个字节，rta_type是05，05是RTA_GATEWAY，表示data中的数据为gateway的IP地址，后面的4个字节组成了一个32位的IP地址，其实就是192.168.0.1，0xc0就是十进制的192，0xa8就是十进制的168
  - 所以我们在解析报文时应该分成两步，第一步按照(struct nlmsghdr)分开，这样分开的每一部分是路由表的一条记录；第二步是在一个部分中解析出每个结构下每个字段的数据
  - 下面这段程序完成了第一步，并将每部分的数据交给函数parse_message去完成第二步
    ```C
    struct nlmsghdr *nlmsg_hdr;
    int msg_buf_len;

    nlmsg_hdr = buf_ptr;
    msg_buf_len = msg_len;
    for (; NLMSG_OK(nlmsg_hdr, msg_buf_len); nlmsg_hdr = NLMSG_NEXT(nlmsg_hdr, msg_buf_len)) {
        if (nlmsg_hdr->nlmsg_type == NLMSG_DONE) {
            break;
        }
        parse_message(nlmsg_hdr);

        // TODO: 
    }
    ```
  - 这段程序中的几个宏：NLMSG_OK()、NLMSG_NEXT()和NLMSG_DONE在前面都有介绍；
  - 下面这段程序，完成了上面这段程序中函数parse_message()的功能，将nlmsg_hdr指向多部分报文的其中一部分的(struct nlmsghdr)，便可以解析出所有(struct rtattr)下的数据，以本文讨论的话题而言，我们只需要主路由表中gateway的IP，所以其中增加了IPv4和主路由表的判断；

    ```C
    struct rtmsg *rt_msg;
    struct rtattr *rt_attr;
    int rt_len;

    struct in_addr dst_addr;            // destination IP address
    struct in_addr src_addr;            // source IP address
    struct in_addr gateway;             // gateway IP address
    char ifname[IF_NAMESIZE];           // network interface name

    rt_msg = (struct rtmsg *)NLMSG_DATA(nlmsg_hdr);

    if ((rt_msg->rtm_family != AF_INET) || (rt_msg->rtm_table != RT_TABLE_MAIN)) {
        // return if it is not IPv4 or not main routing table.
        return;
    }

    rt_attr = (struct rtattr *)RTM_RTA(rt_msg);
    rt_len = RTM_PAYLOAD(nlmsg_hdr);

    unsigned char *p;
    for (; RTA_OK(rt_attr, rt_len); rt_attr = RTA_NEXT(rt_attr, rt_len)) {
        switch (rt_attr->rta_type) {
            case RTA_OIF:       // rta_data is index of network interface. converter it to ifterface name here.
                if_indextoname(*(int *)RTA_DATA(rt_attr), ifname);
                break;
            case RTA_GATEWAY:   // rta_date is gateway ip in 32bits(struct in_addr).
                memcpy(&gateway, RTA_DATA(rt_attr), sizeof(gateway));
                break;
            case RTA_PREFSRC:   // Preferred source IP address in 32bits(struct in_addr)
                memcpy(&src_addr, RTA_DATA(rt_attr), sizeof(src_addr));
                break;
            case RTA_DST:       // Destination IP address in 32 bits(struct in_addr)
                memcpy(&dst_addr, RTA_DATA(rt_attr), sizeof(dst_addr));
                break;
            case RTA_TABLE:     // Routing table ID. 
                break;
            case RTA_PRIORITY:  // Priority of route.
                break;
            default:            // Unknown routing attribute
                break;
        }
    }
    ```
  - 上面这段程序中获得的IP地址都是存放在一个(struct in_addr)中，熟悉IPv4下socket编程的程序员应该了解这是什么。

## 7. 完整源代码
* 源代码文件名：[get-gateway-netlink.c][src01](**点击文件名下载源程序**)，里面有详细的注释

* 编译：```gcc -Wall get-gataway-netlink.c -o get-gateway-netlink```
* 运行：```./get-gateway-netlink```
* 带调试信息运行：```./get-gateway-netlink 1```，可以打印出大量的中间过程信息，对程序的理解将有很大帮助。
* 这个程序的主要部分在前面都已经讨论过了；
* 这个程序其实是可以获取整个路由表的，但程序中做了过滤，一旦找到gateway的IP地址便不再解析下面的信息，所以这个程序稍加修改可以获取整个路由表；
* 获取gateway IP地址的方法不止本文介绍的这一种方法，其实我认为netlink的方法尽管看上去比较"高级"，但也十分复杂和繁琐，最好的方法我认为是从proc文件系统中获取，想要了解这种方法的读者可以参考我的另一篇文章[《从proc文件系统中获取gateway的IP地址》][article1]。

## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180009/get-gateway-netlink.c

[img01]:https://whowin.gitee.io/images/180009/netlink-message-format.png
[img02]:https://whowin.gitee.io/images/180009/rtnetlink-request_message.png
[img03]:https://whowin.gitee.io/images/180009/rtnetlink-routing-table.png
[img04]:https://whowin.gitee.io/images/180009/route-of-routing-table.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/3ae07ec8c1811bde08555be1530ddb05.png
[img02]:https://img-blog.csdnimg.cn/img_convert/10d396db0ad696c2ec0d0a8745cfaafd.png
[img03]:https://img-blog.csdnimg.cn/img_convert/d35277d37941f00fddee641a51557939.png
[img04]:https://img-blog.csdnimg.cn/img_convert/a0c34be7072ac1daccd73a0caa17c1f8.png
-->

<!--gitee
[article1]:https://whowin.gitee.io/post/blog/network/0008-get-gateway-ip-from-proc-filesys/
-->
[article1]:https://blog.csdn.net/whowin/article/details/129177942
