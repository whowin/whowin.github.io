---
title: "C语言如何获取ipv6地址"
date: 2022-10-16T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "C Language"
  - "Linux"
  - "Network"
tags:
  - "C语言"
  - ipv6
  - "Network Programming"
  - "网络编程"
draft: false
#references: 
# - [Get IPv6 addresses in linux using ioctl](https://stackoverflow.com/questions/20743709/get-ipv6-addresses-in-linux-using-ioctl)
# - [IPv6-related entries in /proc/net/](https://tldp.org/HOWTO/Linux+IPv6-HOWTO/ch11s04.html)
# - [scanf, fscanf, sscanf, scanf_s, fscanf_s, sscanf_s](https://en.cppreference.com/w/c/io/fscanf)
# - [C代码，用于在Linux中获取IP地址的接口名称](http://www.955yes.com/ask/127327909.html)
postid: 180001
---

使用通常获取ipv4的IP地址的方法是无法获取ipv6地址的，本文介绍了使用C语言获取ipv6地址的三种方法：从proc文件从系统获取ipv6地址、使用getifaddrs()函数获取ipv6地址和使用netlink获取ipv6地址，每种方法均给出了完整的源程序，本文所有实例在 ubuntu 20.04 下测试通过，gcc 版本 9.4.0。
<!--more-->

## 1. ipv4的IP地址的获取方法
* 不论是获取 ipv4 的 IP 地址还是 ipv6 的地址，应用程序都需要与内核通讯才可以完成；
* ioctl 是和内核通讯的一种常用方法，也是用来获取 ipv4 的 IP 地址的常用方法，下面代码演示了如何使用 ioctl 来获取本机所有接口的 IP 地址：
    ```C
    #include <stdio.h>
    #include <stdlib.h>

    #include <sys/ioctl.h>
    #include <linux/if.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    int main() {
        int i = 0;
        int sockfd;
        struct ifconf ifc;
        char buf[512] = {0};
        struct ifreq *ifr;

        ifc.ifc_len = 512;
        ifc.ifc_buf = buf;

        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("socket");
            return -1;
        }
        ioctl(sockfd, SIOCGIFCONF, &ifc);
        ifr = (struct ifreq*)buf;

        for (i = (ifc.ifc_len /sizeof(struct ifreq)); i > 0; i--) {
            printf("%s: %s\n",ifr->ifr_name, 
                    inet_ntoa(((struct sockaddr_in *)&(ifr->ifr_addr))->sin_addr));
            ifr++;
        }
    }
    ```
* 但是使用 ioctl 无法获取 ipv6 地址，即便我们建立一个 AF_INET6 的 socket，ioctl 仍然只返回 ipv4 的信息，我们可以试试下面代码；
    ```C
    #include <stdio.h>
    #include <stdlib.h>

    #include <sys/ioctl.h>
    #include <linux/if.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    int main() {
        int i = 0;
        int sockfd;
        struct ifconf ifc;
        char buf[1024] = {0};
        struct ifreq *ifr;

        ifc.ifc_len = 1024;
        ifc.ifc_buf = buf;

        if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
            perror("socket");
            return -1;
        }
        ioctl(sockfd, SIOCGIFCONF, &ifc);
        ifr = (struct ifreq*)buf;

        struct sockaddr_in *sa;
        for (i = (ifc.ifc_len /sizeof(struct ifreq)); i > 0; i--) {
            sa = (struct sockaddr_in *)&(ifr->ifr_addr);
            if (sa->sin_family == AF_INET6) {
                printf("%s: AF_INET6\n", ifr->ifr_name);
            } else if (sa->sin_family == AF_INET){
                printf("%s: AF_INET\n", ifr->ifr_name);
            } else {
                printf("%s: %d.  It is an unknown address family.\n", 
                        ifr->ifr_name, sa->sin_family);
            }
            ifr++;
        }
    }
    ```
* 这段程序在我的机器上的运行结果是这样的：

    ![ioctl无法获取ipv6地址][img01]

    <center><b>图1：ioctl无法获取ipv6地址</b></center>

-----------------------

* 我们看到，不管怎么折腾，返回的仍然只有 ipv4 的地址，所以我们需要一些其他的方法获得 ipv6 地址，下面介绍三种使用 C 语言获得 ipv6 地址的方法。

## 2. 从文件/proc/net/if_inet6中获取ipv6地址
* 我们先来看看文件/proc/net/if_inet6中有什么内容

    ![文件/proc/net/if_inet6内容][img02]

    <center><b>图2：文件/proc/net/if_inet6内容</b></center>

----------------------

* 这个文件中，每行为一个网络接口的数据，每行数据分成 6 个字段

    |字段序号|字段名称|字段说明|
    |:-----:|:------|:-----|
    |1|ipv6address|IPv6地址，32位16进制一组，中间没有:分隔符|
    |2|ifindex|接口设备号，每个设备都不同，按 16 进制显示|
    |3|prefixlen|16进制显示的前缀长度，类似 ipv4 的子网掩码的东西|
    |4|scopeid|scope id|
    |5|flags|接口标志，这些标志标识着这个接口的特性|
    |6|devname|接口设备名称|

* 所以从这个文件中可以很容易地获得所有接口的 ipv6 地址
    ```C
    #include <stdio.h>
    #include <linux/if.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    int main(void) {
        FILE *f;
        int scope, prefix;
        unsigned char _ipv6[16];
        char dname[IFNAMSIZ];
        char address[INET6_ADDRSTRLEN];

        f = fopen("/proc/net/if_inet6", "r");
        if (f == NULL) {
            return -1;
        }
        while (19 == fscanf(f,
                            " %2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx\
                            %2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx \
                            %*x %x %x %*x %s",
                            &_ipv6[0], &_ipv6[1], &_ipv6[2], &_ipv6[3], 
                            &_ipv6[4], &_ipv6[5], &_ipv6[6], &_ipv6[7],
                            &_ipv6[8], &_ipv6[9], &_ipv6[10], &_ipv6[11], 
                            &_ipv6[12], &_ipv6[13], &_ipv6[14], &_ipv6[15],
                            &prefix, &scope, dname)) {
            if (inet_ntop(AF_INET6, _ipv6, address, sizeof(address)) == NULL) {
                continue;
            }
            printf("%s: %s\n", dname, address);
        }
        fclose(f);

        return 0;
    }
    ```
* fscanf 中的 %2hhx 是一种不多见的用法，hhx 表示后面的指针 &_ipv6[x] 指向一个 unsigned char *，2 表示从文件中读取的长度，这个是常用的；
* 关于 fscanf 中的 hh 和 h 的用法，可以查看在线手册 man fscanf 了解更多的内容；
* ipv6 地址一共 128 位，16 位一组，一共 8 组，但是这里为什么不一次从文件中读入 4 个字符(16 位)，读 8 次，而要一次读入 2 个字符读 16 次呢？
    - 这个要去看 inet_ntop 的参数，我们先使用命令 man inet_ntop 看一下 inet_ntop 的在线手册
        ```C
        const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
        ```
    - 当第 1 个参数 af = AF_INET6 时，对于第 2 个参数，还有说明：
        ```plaintext
        AF_INET6
            src  points  to  a struct in6_addr (in network byte order) which is converted to a representation of
            this address in the most appropriate IPv6 network address format for this address.  The  buffer  dst
            must be at least INET6_ADDRSTRLEN bytes long.
        ```
    - 很显然，需要第 2 个参数指向一个 struct in6_addr，这个结构在 netinet/in.h 中定义：
        ```C
        /* IPv6 address */
        struct in6_addr
        {
            union
            {
                uint8_t  __u6_addr8[16];
                uint16_t __u6_addr16[8];
                uint32_t __u6_addr32[4];
            } __in6_u;
        #define s6_addr         __in6_u.__u6_addr8
        #ifdef __USE_MISC
        # define s6_addr16      __in6_u.__u6_addr16
        # define s6_addr32      __in6_u.__u6_addr32
        #endif
        };
        ```
    - 这个结构在一般情况下使用的是 uint8_t  __u6_addr8[16]，也就是 16 个 unsigned char 的数组；
    - 所以，实际上 struct in6_addr 的结构如下
        ```C
        struct in6_addr {
            unsigned char __u6_addr8[16];
        }
        ```
    - 这就是我们在读文件时为什么要一次读入 2 个字符，读 16 次，要保证读出的内容符合 struct in6_addr 的定义；
* 一次从文件中读入 4 个字符(16 位)，读 8 次，和一次读入 2 个字符读 16 次有什么不同呢？我们以 16 进制的 f8e9 为例
    - 当我们每次读入 2 个字符，读 2 次时，在内存中的排列是这样的
        ```C
        unsigned char _ipv6[16];
        fscanf(f, "%2hhx2hhx", &_ipv6[0], &_ipv6[1]);
        unsigned char *p = _ipv6
        f8  e9
        -+  -+
         |   |
         |   +------- p + 1
         +----------- p
        ```
    - 当我们每次读入 4 个字符，读 1 次时，在内存中的排列是这样的
        ```C
        unsigned int _ipv6[8]
        fscanf(f, "%4x", &_ipv6[0])
        unsigned char *p = (unsigned char *)_ipv6
        e9  f8
        -+  -+
         |   |
         |   +------- p + 1
         +----------- p
        ```
    - 这是因为 X86 系列 CPU 的存储模式是小端模式，也就是高位字节要存放在高地址上，f8e9 这个数，f8 是高位字节，e9 是低位字节，所以当我们把 f8e9 作为一个整数读出的时候，e9 将存储在低地址，f8 存储在高地址，这和 struct in6_addr 的定义是不相符的；
    - 所以如果我们一次读 4 个字符， 读 8 次，我们就不能使用 inet_ntop() 去把 ipv6 地址转换成我们所需要的字符串，当然我们可以自己转换，但有些麻烦，参考下面代码
        ```C
        unsigned short int _ipv6[8];
        int zero_flag = 0;
        while (11 == fscanf(f,
                            " %4hx%4hx%4hx%4hx%4hx%4hx%4hx%4hx %*x %x %x %*x %s",
                            &_ipv6[0], &_ipv6[1], &_ipv6[2], &_ipv6[3], 
                            &_ipv6[4], &_ipv6[5], &_ipv6[6], &_ipv6[7],
                            &prefix, &scope, dname)) {
            printf("%s: ", dname);
            for (int i = 0; i < 8; ++i) {
                if (_ipv6[i] != 0) {
                    if (i) putc(':', stdout); 
                    printf("%x", _ipv6[i]);
                    zero_flag = 0;
                } else {
                    if (!zero_flag) putc(':', stdout);
                    zero_flag = 1;
                }
            }
            putc('\n', stdout);
        }
        ```
    - 和上面的代码比较，多了不少麻烦，自己去体会吧。    

## 3. 使用getifaddrs()获取 ipv6 地址
* 可以通过在线手册 man getifaddrs 了解详细的关于 getifaddrs 函数的信息；
* getifaddrs 函数会创建一个本地网络接口的结构链表，该结构链表定义在 struct ifaddrs 中；
* 关于 ifaddrs 结构有很多文章介绍，本文仅简单介绍一下与本文密切相关的内容，下面是 struct ifaddrs 的定义
    ```C
    struct ifaddrs {
        struct ifaddrs  *ifa_next;    /* Next item in list */
        char            *ifa_name;    /* Name of interface */
        unsigned int     ifa_flags;   /* Flags from SIOCGIFFLAGS */
        struct sockaddr *ifa_addr;    /* Address of interface */
        struct sockaddr *ifa_netmask; /* Netmask of interface */
        union {
            struct sockaddr *ifu_broadaddr;
                            /* Broadcast address of interface */
            struct sockaddr *ifu_dstaddr;
                            /* Point-to-point destination address */
        } ifa_ifu;
    #define              ifa_broadaddr ifa_ifu.ifu_broadaddr
    #define              ifa_dstaddr   ifa_ifu.ifu_dstaddr
        void            *ifa_data;    /* Address-specific data */
    };
    ```
* ifa_next 是结构链表的后向指针，指向链表的下一项，当前项为最后一项时，该指针为 NULL；
* ifa_addr 是本文主要用到的项，这是一个 struct sockaddr， 看一下 struct sockaddr 的定义：
    ```C
    struct sockaddr {
        sa_family_t sa_family;
        char        sa_data[14];
    }
    ```
* 实际上，当 ifa_addr->sa_family 为 AF_INET 时，ifa_addr 指向 struct sockaddr_in；当 ifa_addr->sa_family 为 AF_INET6 时，ifa_addr 指向一个 struct sockaddr_in6；
* sockaddr_in 和 sockaddr_in6 这两个结构同样可以找到很多介绍文章，这里就不多说了，反正这里面是结构套着结构，要把思路捋顺了才不至于搞乱；
* 下面是使用 getifaddrs() 获取 ipv6 地址的源程序，可以看到，打印 ipv6 地址的那几行，与上面的那个例子是一样的；
    ```C
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <stdio.h>
    #include <stdlib.h>

    int main () {
        struct ifaddrs *ifap, *ifa;
        struct sockaddr_in6 *sa;
        char addr[INET6_ADDRSTRLEN];

        if (getifaddrs(&ifap) == -1) {
            perror("getifaddrs");
            exit(1);
        }

        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
                // 打印ipv6地址
                sa = (struct sockaddr_in6 *)ifa->ifa_addr;
                if (inet_ntop(AF_INET6, (void *)&sa->sin6_addr, addr, INET6_ADDRSTRLEN) == NULL)
                    continue;
                printf("%s: %s\n", ifa->ifa_name, addr);
            }
        }

        freeifaddrs(ifap);
        return 0;
    }
    ```
* 最后要注意的是，使用 getifaddrs() 后，一定要记得使用 freeifaddrs() 释放掉链表所占用的内存。
* 这个例子中，我们使用 inet_ntop() 将 sin6_addr 结构转换成了字符串形式的 ipv6 地址，还可以使用 getnameinfo() 来获取 ipv6 的字符串形式的地址；
* 可以通过在线手册 man getnameinfo 了解 getnameinfo() 的详细信息
* 下面是使用 getifaddrs() 获取 ipv6 地址并使用 getnameinfo() 将将 ipv6 地址转变为字符串的源程序
    ```C
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <stdio.h>
    #include <stdlib.h>

    #include <netdb.h>

    int main () {
        struct ifaddrs *ifap, *ifa;
        char addr[INET6_ADDRSTRLEN];

        if (getifaddrs(&ifap) == -1) {
            perror("getifaddrs");
            exit(1);
        }

        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
                // 打印ipv6地址
                if (getnameinfo(ifa->ifa_addr, 
                                sizeof(struct sockaddr_in6), 
                                addr, sizeof(addr), 
                                NULL, 0, 
                                NI_NUMERICHOST))
                    continue;
                printf("%s: %s\n", ifa->ifa_name, addr);
            }
        }

        freeifaddrs(ifap);
        return 0;
    }
    ```
* 和前面那个程序相比，这个程序增加了一个包含文件 netdb.h，这里面有 getnameinfo() 的一些相关定义；
* 在这里使用函数 getnameinfo 时，要明确 ifa->ifa_addr 指向的是一个 struct sockaddr_in6，后面的常数 NI_NUMERICHOST 表示返回的主机地址为数字字符串；
* 和上面的例子略有不同的是，使用 getnameinfo 获取的 ipv6 地址的最后会使用 ‘%’ 连接一个网络接口的名称，如下图所示：

    ![使用getnameinfo获取ipv6地址][img03]

    <center><b>图3：使用getnameinfo获取ipv6地址</b></center>

------------------

## 4. 使用 netlink 获取 ipv6 地址
* netlink socket 是用户空间与内核空间通信的又一种方法，本文并不讨论 netlink 的编程方法，但给出了使用 netlink 获取 ipv6 地址的源程序；
* 与上面两个方法比较，使用 netlink 获取 ipv6 地址的方法略显复杂，在实际应用中并不多见，所以本文也就不进行更多的讨论了；
* 下面是使用 netlink 获取 ipv6 地址的源程序
    ```C
    #include <asm/types.h>
    #include <arpa/inet.h>
    #include <linux/netlink.h>
    #include <linux/rtnetlink.h>
    #include <sys/socket.h>
    #include <string.h>
    #include <stdio.h>

    int main(int argc, char ** argv) {
        char buf1[16384], buf2[16384];

        struct {
            struct nlmsghdr nlhdr;
            struct ifaddrmsg addrmsg;
        } msg1;

        struct {
            struct nlmsghdr nlhdr;
            struct ifinfomsg infomsg;
        } msg2;

        struct nlmsghdr *retmsg1;
        struct nlmsghdr *retmsg2;

        int len1, len2;

        struct rtattr *retrta1, *retrta2;
        int attlen1, attlen2;

        char pradd[128], prname[128];

        int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

        memset(&msg1, 0, sizeof(msg1));
        msg1.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
        msg1.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
        msg1.nlhdr.nlmsg_type = RTM_GETADDR;
        msg1.addrmsg.ifa_family = AF_INET6;

        memset(&msg2, 0, sizeof(msg2));
        msg2.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
        msg2.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
        msg2.nlhdr.nlmsg_type = RTM_GETLINK;
        msg2.infomsg.ifi_family = AF_UNSPEC;

        send(sock, &msg1, msg1.nlhdr.nlmsg_len, 0);
        len1 = recv(sock, buf1, sizeof(buf1), 0);
        retmsg1 = (struct nlmsghdr *)buf1;

        while NLMSG_OK(retmsg1, len1) {
            struct ifaddrmsg *retaddr;
            retaddr = (struct ifaddrmsg *)NLMSG_DATA(retmsg1);
            int iface_idx = retaddr->ifa_index;

            retrta1 = (struct rtattr *)IFA_RTA(retaddr);
            attlen1 = IFA_PAYLOAD(retmsg1);

            while RTA_OK(retrta1, attlen1) {
                if (retrta1->rta_type == IFA_ADDRESS) {
                    inet_ntop(AF_INET6, RTA_DATA(retrta1), pradd, sizeof(pradd));

                    len2 = recv(sock, buf2, sizeof(buf2), 0);
                    send(sock, &msg2, msg2.nlhdr.nlmsg_len, 0);
                    len2 = recv(sock, buf2, sizeof(buf2), 0);
                    retmsg2 = (struct nlmsghdr *)buf2;
                    while NLMSG_OK(retmsg2, len2) {
                        struct ifinfomsg *retinfo;
                        retinfo = NLMSG_DATA(retmsg2);
                        memset(prname, 0, sizeof(prname));
                        if (retinfo->ifi_index == iface_idx) {
                            retrta2 = IFLA_RTA(retinfo);
                            attlen2 = IFLA_PAYLOAD(retmsg2);

                            while RTA_OK(retrta2, attlen2) {
                                if (retrta2->rta_type == IFLA_IFNAME) {
                                    strcpy(prname, RTA_DATA(retrta2));
                                    break;
                                }
                                retrta2 = RTA_NEXT(retrta2, attlen2);
                            }
                            break;
                        }
                        retmsg2 = NLMSG_NEXT(retmsg2, len2);       
                    }
                    printf("%s: %s\n", prname, pradd);
                }
                retrta1 = RTA_NEXT(retrta1, attlen1);
            }
            retmsg1 = NLMSG_NEXT(retmsg1, len1);       
        }
        return 0;
    }
    ```
## 5. 结语
* 本文给出了三种获取 ipv6 地址的方法，均给出了完整的源程序；
* 本文对三种方法并没有展开讨论，以免文章冗长；
* 仅就获取 ipv6 地址而言，前两种方法比较常用而且简单；
* 通常认为，用户程序与内核通讯有四种方法
    1. 系统调用
    2. 虚拟文件系统(/proc、/sys等)
    3. ioctl
    4. netlink
* 本文所述的三个方法，正是使用了上述 2、3、4 三种方法；而获取 ipv6 地址，简单地使用系统调用无法实现。

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[img01]:https://whowin.gitee.io/images/180001/ipv6_ioctl.png
[img02]:https://whowin.gitee.io/images/180001/file_if_inet6.png
[img03]:https://whowin.gitee.io/images/180001/ipv6_getnameinfo.png

