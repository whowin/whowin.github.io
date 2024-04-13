---
title: "使用ioctl扫描wifi信号获取信号属性的实例(一)"
date: 2023-06-26T16:43:29+08:00
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
    - 网络编程
    - "802.11"
    - wifi
    - "无线网络"
    - ioctl
draft: false
#references: 
# - https://github.com/shadowkane/wifiScanner
#     - 本文参考程序
# - [is_wireless.c](https://gist.github.com/edufelipe/6108057)
# - [802.11帧结构分析](https://wenku.baidu.com/view/07d927bacfc789eb162dc86f)
# - [802.11 Frame Types](https://en.wikipedia.org/wiki/802.11_Frame_Types)
# - [Linux wireless](https://wireless.wiki.kernel.org/en/developers/documentation/nl80211)
# - ```man ioctl_list``` 
# - [Wireless Extensions](https://wireless.wiki.kernel.org/en/developers/documentation/wireless-extensions)
# - [How can I get a list of available wireless networks on Linux?](https://stackoverflow.com/questions/400240/how-can-i-get-a-list-of-available-wireless-networks-on-linux/)
# - [Wireless Tools for Linux](https://github.com/HewlettPackard/wireless-tools)
# - [Wireless Tools for Linux](https://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html)
# - [wifi-scan.c](https://gist.github.com/rosenhouse/685e6c4c7f3b1332aa0f)
# - [How do the nl80211 library & cfg80211 work?](https://stackoverflow.com/questions/21456235/how-do-the-nl80211-library-cfg80211-work)
# - [How can I get a list of available wireless networks on Linux?](https://stackoverflow.com/questions/400240/how-can-i-get-a-list-of-available-wireless-networks-on-linux/)
# - [wifi.pdf](https://pages.cpsc.ucalgary.ca/~carey/CPSC441/archive/W2018/tutorials/WiFi.pdf)
# - [libnm Reference Manual](https://developer-old.gnome.org/libnm/stable/)
# - [Connect to WIFI from C program](https://stackoverflow.com/questions/57586732/connect-to-wifi-from-c-program)
#     - 使用libnm
postid: 180022
---

使用 wifi 是一件再平常不过的是事情，有很多 wifi 工具可以帮助你扫描附近的 wifi 信号，测试信号强度等，但如何通过编程来操作 wifi 却鲜有文章涉及；本文立足实践，不使用任何第三方库，仅使用 ioctl 扫描附近的 wifi 信号，并获取这些 AP 的 ESSID、MAC 地址、占用信道和工作频率，本文将给出完整的源程序，今后还会写一些文章讨论如果编程获取 wifi 信号的其它属性(比如：信号强度、加密方式等)的方法，敬请关注；本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；阅读本文并不需要对 IEEE802.11 协议有所了解。
<!--more-->

## 1 前言
* 目前的无线网络都是采用 `IEEE802.11` 协议，`802.11` 是一个协议簇，目前无线网络最常用的是 `802.11n`，理论最高速度高达 `600Mbit/s`
* WIFI 是 `802.11` 规范的一种具体实现；
* 本文的目标是使用 C 语言在 ubuntu 下编写出一个扫描 WIFI 信号的程序，电脑上至少要有一片无线网卡才能扫描附近的 WIFI 信号；
* 扫描 WIFI 信号显然是要操作无线网卡才能实现，通常情况下无线网卡的驱动程序是在内核空间的，用户空间的应用程序是无法直接控制驱动程序的；
* 为了能够从用户空间控制无线网卡的驱动程序，我们在用户空间编写的程序需要使用 IPC 通信与内核进程进行通信；
* 实现 IPC 进程间通信的方式有很多，本文采用的是 `ioctl`，但还有其它方式，比如 `netlink` 等；
* 本文采用的 `ioctl` 方法是基于 **Wireless Extensions**(简称 **WE** 或 WEXT)的，WE 是一组通用 API，可以控制无线网卡驱动程序向用户空间进程传送 wifi 的配置和统计信息；
* 2006年，出现了 `cfg80211` 和 `nl80211`，其目标是取代 WE，`cfg80211` 和 `nl80211` 不再使用 `ioctl` 与无线网卡驱动程序进行通信，而是采用 `netlink`；
* 有些无线网络工具是使用 `cfg80211` 和 `nl80211的`，像 `iw、hostapd` 或 `wpa_supplicant` 程序，它们需要使用 `netlink` 库(如 `libnl` 或 `libnl-tiny`)和 `netlink` 头文件 `nl80211.h`；
* 使用 WE 的另一个好处就是不需要依赖其它库(比如 `libnl`)，只要有标准 C 语言库即可实现，像无线网络工具 `iwlist` 等使用的就是 WE
* 实际上，不管是 WE 还是 `cfg80211` 和 `nl80211`，都鲜有资料和范例，本文介绍了 WE 的使用，后续文章可能会介绍 `cfg80211` 和 `nl80211` 的使用；
* 尽管前面多次提到 802.11 协议，但阅读本文并不需要对该协议有所了解，但需要有一定的 C 语言基础，范例中大量使用了单向链表和系统调用 `ioctl()`，读者需要对这些知识有足够的了解；
* 本文旨在向读者介绍如何使用 ioctl() 对 wifi 信号进行扫描并获取扫描结果，在处理扫描结果上仅处理了三类数据，以便搭建起一个大致的框架，后续文章会着重介绍对扫描结果的处理。

## 2 使用ioctl进行wifi信号扫描的基本原理
### 2.1 WE API
* **WE**(Wireless Extensions) 定义了一系列关于无线网络接口的系统调用，使用 `ioctl()` 实现，这些系统调用实现了用户空间的应用程序与内核中的无线网络接口驱动程序之间的通信；
* 这些系统调用定义在头文件 `/usr/include/linux/wireless.h`，调用 `ioctl()` 的基本方法如下：
    ```C
    int ioctl(int socket, unsigned long request, struct iwreq *wrq);
    ```
* 其中的 request 在 `wireless.h` 中定义，以 SIOC 开头的宏定义；`struct iwreq` 同样在 `wireless.h` 中定义，所有 WE 中的调用均使用这个结构的指针作为 `ioctl()` 的第三个参数；
* 下面一段代码可以获得无线网络接口 `wlp3s0` 当前连接的 WIFI 信号的 ESSID，将其中的 `wlp3s0` 改成你的电脑上的无线网卡的设备名就可以编译运行了
    ```C
    #include <string.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <linux/wireless.h>

    #define IF_NAME         "wlp3s0"
    int main() {
        struct iwreq wrq;
        char essid[IW_ESSID_MAX_SIZE + 1] = {0};

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&wrq, 0, sizeof(struct iwreq));
        strncpy(wrq.ifr_name, IF_NAME, IFNAMSIZ);
        ioctl(sock, SIOCGIWNAME, &wrq);
        printf("Protocol: %s\n", wrq.u.name);
        wrq.u.data.pointer = essid;
        ioctl(sock, SIOCGIWESSID, &wrq);
        printf("ESSID is %s\n", (char *)wrq.u.essid.pointer);
        close(sock);
        return 0;
    }
    ```
* 这段代码调用了两次 `ioctl()`，第一次的指令是 `SIOCGIWNAME`，获取了无线网卡的协议，第二次的指令是 `SIOCGIWESSID`，获取了无线网卡连接的 wifi 信号的 `ESSID`；
* 对 WE 的很多指令而言，在执行 `ioctl()` 之前，需要先调用一下 **SIOCGIWNAME**，这个调用比较简单，只需要设置一下接口名称，调用成功会返回协议名称，可以用来检验是否为无线网卡，有线接口的设备名在调用这个 `ioctl()` 时会出错；
* 这段程序没有任何错误处理，如果要实际应用一定要补充一些代码；
* 编译：```gcc -Wall wifi-essid.c -o wifi-essid```
* 运行：```./wifi-essid```

### 2.2 启动 wifi 信号扫描
* 在头文件 `wireless.h` 中定义的众多指令中，有一个 **SIOCSIWSCAN** 可以使用指定的无线网卡扫描附近的 AP(Access Point)，然后使用 **SIOCGIWSCAN** 获取扫描结果；
* 在使用 `SIOCSIWSCAN` 启动扫描之前，不需要先调用 `SIOCGIWNAME`
* 下面这段代码会在无线网卡 `wlp3s` 上启动 AP 扫描
    ```C
    struct iwreq wrq;
    memset(&wrq, 0, sizeof(struct iwreq));

    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
    wrq.u.data.pointer = NULL;
    wrq.u.data.flags = 0;
    wrq.u.data.length = 0;
    ioctl(sockfd, SIOCSIWSCAN, &wrq);
    ```
* 在启动 `SIOCSIWSCAN` 之前，要初始化 `struct iwreq` 中的四个字段，参考上面程序。

### 2.3 获取 wifi 信号的扫描结果
* 使用头文件 `wireless.h` 中的 **SIOCGIWSCAN** 可以获取 wifi 信号的扫描结果
* 在启动 wifi 信号扫描后，并不能立即返回结果，要等待几秒后再发出 `SIOCGIWSCAN` 获取扫描结果，等待的时间主要取决于当前的系统和驱动程序，所以在调用 `ioctl()` 获取扫描结果时，要监视 **errno**，如果 `error == EAGAIN`，则需要 sleep 一下后再次调用 `ioctl()`
* 下面这段程序演示了获取扫描结果的过程
    ```C
    struct iwreq wrq;

    GET_AGAIN:
    wrq.u.data.pointer = buffer;
    wrq.u.data.length = buflen;
    wrq.u.data.flags = 0;
    if (ioctl(sockfd, SIOCGIWSCAN, &wrq) == -1) {
        if (errno == EAGAIN) {
            sleep(2);
            goto GET_AGAIN;
        }
    }

    ```
* 在发出指令 `SIOCGIWSCAN` 之前，需要初始化 `struct iwreq` 中的三个字段，参考上面程序，`buffer` 是存放返回结果的内存缓冲区，`buflen` 是 `buffer` 的长度；
* 扫描结果的数据需要多大的内存空间，在调用 `SIOCGIWSCAN` 之前并不知道，所以在调用 `ioctl()` 时可能会因为 `buffer` 不够大而失败，这时我们不得不重新为 `buffer` 申请一块更大的内存并再次调用 `ioctl()`；
* 下面这段代码演示了获取扫描结果的全过程
    ```C
    struct iwreq wrq;
    char *buffer = NULL;
    uint32_t buflen = IW_SCAN_MAX_DATA;
    int counter = 0;

    REALLOC_MEM:
    if (buffer) {
        free(buffer);
        exit(-1);
    }
    buflen = IW_SCAN_MAX_DATA * (counter + 1);
    buffer = (char *)malloc(buflen);
    if (buffer == NULL) {
        printf("Can't allocate enough memory for scanning result.\n");
        exit(-1);
    }

    GET_AGAIN:
    wrq.u.data.pointer = buffer;
    wrq.u.data.length = buflen;
    wrq.u.data.flags = 0;
    if (ioctl(sockfd, SIOCGIWSCAN, &wrq) == -1) {
        if (errno == EAGAIN) {
            sleep(2);
            goto GET_AGAIN;
        }
        if (errno == E2BIG) {
            counter++;
            goto REALLOC_MEM;
        }
        if (buffer) {
            free(buffer);
        }
        exit(-1);
    }
    /* TODO */
    ```
* 当 `errno == E2BIG` 表示 buffer 不够大；`IW_SCAN_MAX_DATA` 是头文件 `wireless.h` 中定义的一个常数，在我的版本下是 4096；

### 2.4 扫描结果的数据格式
* 首先，扫描结果是一个数据流(stream)，所谓数据流，就是收到的数据是各种不同结构的数据连接在一起的连续字节序列，中间并不会有分隔符，这些数据需要自行进行解析、分割；
* 在收到的数据中，包含有扫描到的所有 wifi 信号的各种属性，比如：ESSID、MAC、工作频率、占用信道等等，如果不能正确解析，将导致混乱；
* 下面所展示的 `struct、union` 等如无特别说明，均在 `wireless.h` 中定义；
* 我们先来看一下前面经常提到的 `struct iwreq`，WE 中每次发起 `ioctl()` 都会用到这个结构，调用前设置参数，调用后返回数据，均使用这个结构；
    ```C
    struct iwreq {
        union
        {
            char  ifrn_name[IFNAMSIZ];  /* if name, e.g. "eth0" */
        } ifr_ifrn;

        /* Data part (defined just above) */
        union iwreq_data  u;
    };
    ```
* 在头文件 `/usr/include/linux/if.h`，有一个宏定义，使得我们可以较为方便地访问 `struct iwreq` 中的 `ifrn_name` 字段；
    ```C
    #define ifr_name    ifr_ifrn.ifrn_name;
    ```
* 下面这段代码使用这个宏定义去访问 `struct iwreq` 中的 `ifrn_name` 字段；
    ```C
    struct iwreq wrq;
    strncpy(wrq.ifr_name, IF_NAME, IFNAMSIZ);
    ```
* 因为 `if.h` 中的这个宏定义，上面代码中的 `wrq.ifr_name` 实际访问的是 `wrq.ifr_ifrn.ifrn_name`，在前面的代码中，也曾有过这种用法，如果你当时有疑问的话，现在应该清楚了；
* `struct iwreq` 的第二个字段是 `union iwreq_data`，这个 union 的定义如下(中间省略了一些本例用不上的定义)：
    ```C
    union iwreq_data {
      /* Config - generic */
      char    name[IFNAMSIZ];
              /* Name : used to verify the presence of  wireless extensions.
               * Name of the protocol/provider... */

      struct iw_point  essid;   /* Extended network name */
      struct iw_param  nwid;    /* network id (or domain - the cell) */
      struct iw_freq   freq;    /* frequency or channel :
                                 * 0-1000 = channel
                                 * > 1000 = frequency in Hz */
        ......
      struct sockaddr  ap_addr; /* Access point address */
      struct sockaddr  addr;    /* Destination address (hw/mac) */

      struct iw_param  param;   /* Other small parameters */
      struct iw_point  data;    /* Other large parameters */
    };
    ```
* 我们在前面的代码中多次用到的 `wrq.u.data`，按照上面的定义，是一个 `struct iw_point`，这个结构的定义如下：
    ```C
    struct iw_point {
        void  *pointer;   /* Pointer to the data  (in user space) */
        __u16 length;     /* number of fields or size in bytes */
        __u16 flags;      /* Optional params */
    };
    ```
* 在调用 `ioctl()` 获取扫描结果前，我们把存储返回数据的指针放在了 `struct iw_point` 的 `pointer` 中，把 `length` 设置为缓冲区的长度，把 `flags` 设置为0；
* 当这个 `ioctl()` 调用成功后，`struct iw_point` 中的 `flags` 被设置为 1，`length` 返回数据的实际长度，当然数据的指针还在 `pointer` 中；
* 下面这段代码简单回顾一下到现在为止我们在这一节的成果：
    ```C
    struct iwreq wrq;

    wrq.u.data.pointer = buffer;
    wrq.u.data.length = buflen;
    wrq.u.data.flags = 0;
    ioctl(socket, SIOCGIWSCAN, &wrq);
    /* 
       wrq.u.data.flags   由初始值0变为1
       wrq.u.data.pointer 扫描结果数据指针
       wrq.u.data.length  扫描结果数据的实际长度
    */
    ```
* 获得了返回数据的实际长度，我们就可以遍历数据，而不至于产生越界等不可预知的错误；
* 前面说过，wifi 信号扫描结果返回的是一个数据流(stream)，这些数据的首指针就是 `wrq.u.data.pointer`，通常称这个数据流为 event stream，数据流中包含着很多 wifi 信号的属性，每个属性被称为一个 event；
* 这个 `event stream` 中每个 `event` 符合 `struct iw_event`，定义如下：
    ```C
    struct iw_event {
        __u16		len;			/* Real length of this stuff */
        __u16		cmd;			/* Wireless IOCTL */
        union iwreq_data	u;		/* IOCTL fixed payload */
    };
    ```
* 先来看一下实际收到的数据(`wrq.u.data.pointer`指向的数据)是什么样子：
    ```plain
    18 00 15 8B 00 00 00 00 01 00 DC FE 18 68 73 80 
    00 00 00 00 00 00 00 00 10 00 05 8B 00 00 00 00 
    9D 00 00 00 00 00 00 00 10 00 05 8B 00 00 00 00 
    99 16 00 00 06 00 00 00 17 00 1B 8B 00 00 00 00 
    07 00 01 00 00 00 00 00 31 35 2D 31 31 30 31
    ```
* 用 `struct iw_event` 去对应这个数据，那么，`len` 是 0x0018(十进制24)，表示这个 event 数据的总长度，所以可以确定这个 event 的数据如下：
    ```plain
    18 00 15 8B 00 00 00 00 01 00 DC FE 18 68 73 80 
    00 00 00 00 00 00 00 00 
    ```
* 这是第 1 个 event(简称为 event_1)，后面的数据是另一个 event，仍然可以用 `struct iw_event` 去对应，以此类推，还可以再分割出三个 event；
* 第 2 个 event(event_2)，长度是0X0010(十进制16)：
    ```plain
    10 00 05 8B 00 00 00 00 9D 00 00 00 00 00 00 00
    ```
* 第 3 个 event(event_3)，长度是0X0010(十进制16)：
    ```plain
    10 00 05 8B 00 00 00 00 99 16 00 00 06 00 00 00
    ```
* 第 4 个 event(event_4)，长度是0X0017(十进制23)：
    ```plain
    17 00 1B 8B 00 00 00 00 07 00 01 00 00 00 00 00 
    31 35 2D 31 31 30 31
    ```
* 在 event_1 中，`struct iw_enent` 中的 `cmd` 在这个 event 中是 0X8B15，这个值决定着 `struct iw_event` 中的 `union iwreq_data` 如何取值；
* 前面说到过 `WE API` 定义了一组与无线网卡驱动程序交互的指令，定义在头文件 `wireless.h` 中，以 SIOC 开头的宏定义，这些指令代码适用于 `struct iw_event` 中的 `cmd`；
* 从 `wireless.h` 中可以查到 0X8B15 的指令宏定义是 `SIOCGIWAP`，含义是 **获取AP的MAC地址**，可以把指令为 `SIOCGIWAP` 的 event 称为 `SIOCGIWAP event`；
* 按照这个方法，可以把 event_2、event_3 和 event_4 的指令宏定义查出来：
    - event_2：指令代码是 0X8B05，宏定义为：`SIOCGIWFREQ`，含义为：获取 AP 的工作信道/工作频率；
    - event_3：指令代码是 0X8B05，宏定义为：`SIOCGIWFREQ`，含义为：获取 AP 的工作信道/工作频率；
    - event_4：指令代码是 0X8B1B，宏定义为：`SIOCGIWESSID`，含义为：获取 AP 的 ESSID；
* 这里面有两个 `SIOCGIWFREQ event`，一个返回的是占用的信道，另一个返回的工作频率；

* 再回到 `struct iw_event` 上来，我们已经搞清楚了其中的 len 和 cmd 两个字段，还有一个字段是 `union wreq_data u`；
* 从 `union wreq_data` 的定义(前面介绍过)中可以看到，这个 union 可以有很多种选择，本文的范例中仅处理了 `SIOCGIWAP、SIOCGIWFREQ、SIOCGIWESSID` 三个指令，仅以这三个指令为例做出说明；
    - 当指令为 `SIOCGIWAP` 时，`union wreq_data `` 应选择``struct sockaddr ap_addr`；
    - 当指令为 `SIOCGIWFREQ` 时，`union wreq_data u` 应选择 `struct iw_freq freq`；
    - 当指令为 `SIOCGIWESSID` 时，相对复杂一些，并不能选择 `struct iw_point essid`(在用指令 `SIOCGIWESSID` 获取 ESSID 时要选择这个结构)，建议自定义一个结构使问题变得简单一点；
        ```C
        struct iw_essid {
            uint16_t len;
            uint16_t flags;
            char __attribute__((aligned(8)))essid;
        };

        struct iw_event *essid_evt = ...;   /* 指向 SIOCGIWESSID event 数据 */
        struct iw_essid *essid_p = (struct iw_essid *)&(essid_evt->u.data);

        /* 
           essid_p->len     为 essid 的长度
           &essid_p->essid  指向 essid 字符串
           essid_p->flags   为 1
        */
        ```
* 下面这段程序可以打印出这段 event_1 中的 AP 的 MAC 地址：
    ```C
    #include <stdio.h>
    #include <stdint.h>
    #include <linux/wireless.h>

    uint8_t data[] = {0x18,0,0x15,0x8B,0,0,0,0,0x01,0,0xDC,0xFE,0x18,0x68,0x73,0x80,0,0,0,0,0,0,0,0};

    int main() {
        struct iw_event *wevt = (struct iw_event *)data;

        if (wevt->cmd == SIOCGIWAP){
            uint8_t *mac = (uint8_t *)wevt->u.ap_addr.sa_data;
            printf("MAC: ");
            for (int i = 0; i < 6; ++i) {
                printf("%02X", mac[i]);
                if (i < 5) putchar(':');
            }
            puts("");
        }
        return 0;
    }
    ```
* event_2 和 event_3 都是 `SIOCGIWFREQ event`，在 `wireless.h` 中有说明，当计算出来的频率大于 1000(Hz) 时，其值为 AP 的工作频率，否则为 AP 占用的信道，所以，event_2 和 event_3 一个返回的是频率，另一个返回的是信道；
* 前面说过，`SIOCGIWFREQ event` 返回数据使用 `struct iw_freq`，这个结构的定义如下：
    ```C
    struct iw_freq {
        __s32       m;      /* Mantissa */
        __s16       e;      /* Exponent */
        __u8        i;      /* List index (when in range struct) */
        __u8        flags;  /* Flags (fixed/auto) */
    };
    ```
    - 字段 flags 为 0 时，表示工作频率是由驱动程序自动选择的；为 1 时表示工作频率为固定设置值；
    - 字段 i 在本例中没有意义；
    - 频率由 m 和 e 两个字段计算得到，其中：e 为底数为 10 的指数，m 为尾数，frequency = m x 10<sup>e</sup> 
* 下面这段程序可以处理 event_2 和 event_3，打印出 AP 占用的信道号和工作频率：
    ```C
    #include <stdio.h>
    #include <stdint.h>
    #include <math.h>
    #include <linux/wireless.h>

    uint8_t data[] = {0x10,0,0x05,0x8B,0,0,0,0,0x9D,0,0,0,0,0,0,0,
                      0x10,0,0x05,0x8B,0,0,0,0,0x99,0x16,0,0,0x06,0,0,0};

    void channel_or_frequency(struct iw_event *wevt) {
        if (wevt->cmd == SIOCGIWFREQ){
            struct iw_freq *ap_freq = (struct iw_freq *)&(wevt->u.freq);
            double freq = (double)ap_freq->m * pow(10, ap_freq->e);
            if (freq > 1000) {
                // ap的工作频率
                printf("Frequency: %.3f\n", (float)freq / (1e9));
            } else {
                // AP的channel
                printf("Channel: %d\n", (int)freq);
            }
        }
    }
    int main() {
        struct iw_event *wevt = (struct iw_event *)data;
        channel_or_frequency(wevt);
        wevt = (struct iw_event *)(data + wevt->len);
        channel_or_frequency(wevt);
        return 0;
    }
    ```
    > 这段程序因为使用了数学函数 pow()，所以用 gcc 编译时要带上参数 **-lm**
* event_4 中的 essid 在前面已经基本说清楚了，下面这段代码会打印出 event_4 中的 essid
    ```C
    #include <stdio.h>
    #include <stdint.h>
    #include <linux/wireless.h>

    uint8_t data[] = {0x17,0,0x1B,0x8B,0,0,0,0,0x07,0,0x01,0,0,0,0,0,0x31,0x35,0x2D,0x31,0x31,0x30,0x31};

    struct iw_essid {
        uint16_t len;
        uint16_t flags;
        char __attribute__((aligned(8)))essid;
    };

    int main() {
        struct iw_event *wevt = (struct iw_event *)data;
        struct iw_essid *essid_p;

        if (wevt->cmd == SIOCGIWESSID){
            essid_p = (struct iw_essid *)&wevt->u.data;
            printf("Len: %d\tflags: %d\n", essid_p->len, essid_p->flags);
            char *p = &essid_p->essid;
            printf("ESSID: ");
            int i;
            for (i = 0; i < essid_p->len; ++i) {
                printf("%c", p[i]);
            }
            puts("");
        }
        return 0;
    }
    ```
* 这里简要介绍一下 SSID 的概念，经常说的 ESSID 和 SSID 其实是一个东西；
    - Basic Service Set 简称 BSS，指的是一个 WAP(Wireless Access Point) 所覆盖(服务)的区域，BSSID 指这个 BSS 的标识，为一个 6-bytes(48-bits)的 ID，实际就是这个 WAP 的 MAC 地址；
    - Extended Service Set 简称 ESS，指的是多个 WAP 共同覆盖(服务)的区域，ESSID 是这个 ESS 的标识，是一个 32个字符长度的字符串(ASCII码)，这些 WAP 各自拥有不同 BSSID 但使用相同的 ESSID；
    - ESSID 常常简称为 SSID。

* 本文范例中仅处理四个 wifi 信号的属性：MAC、Channel、Frequency、ESSID，涉及三个指令代码：SIOCGIWAP、SIOCGIWFREQ、SIOCGIWESSID；
* 本文涉及的相关数据结构及调用方法至此已经介绍完毕。

## 3 wifi信号扫描的步骤和方法
### 3.1 wifi信号扫描的基本步骤
1. 获取本机所有的网络接口
2. 从所有的网络接口中找到无线网络接口
3. 向无线网络接口发出wifi信号扫描指令
4. 等待扫描结果的返回
5. 分析返回结果，解析出所有的 event，并生成 event 链表 
6. 遍历 event 链表并从中提取出 wifi 信号的属性
7. 将 wifi 信号的属性显示在屏幕上

### 3.2 wifi信号扫描的基本方法
* 本例中，大量的信息的长度和数量都是未知的：
    1. 本机网络接口的数量
    2. 本机无线网络接口数量
    3. wifi信号扫描后返回的结果的长度
    4. 返回结果中有多少个 event
    5. 扫描到了多少个wifi信号
* 为此，本例中大量使用的单向链表结构，主要有下面四个单向链表：
    - 本机网络接口链表 - `struct ifaddrs`
        > 调用 getifaddrs() 生成该链表
    - 本机无线网络接口链表 - `struct wifs_chain`
        > 扫描本机网络接口链表，找出其中的无线网络接口，生成本机无线网络接口链表，当本机只有一片无线网卡时，通常这个链表中只有一项；如果没有找到无线网络接口，应该终止程序运行
    - 扫描返回结果的 event 链表 - `struct events_chain`
        > 向无线网卡发出扫描指令 `SIOCSIWSCAN` 后，使用 `SIOCGIWSCAN` 指令获取扫描结果，分析扫描结果生成 `event` 链表
    - 无线 AP(Access Point) 链表 - `struct aps_chain`
        > 遍历 event 链表，提取出各个 AP 的属性，生成无线 AP 链表

### 3.3 如何获取本机的所有网络接口
* 使用 `getifaddrs()` 可以非常容易地获取全部网络接口
* 可以通过在线手册 `man getifaddrs` 了解详细的关于 `getifaddrs` 函数的信息；
* `getifaddrs` 函数会创建一个本地网络接口的结构链表，该结构链表定义在 `struct ifaddrs` 中(头文件 `ifaddrs.h`)；
* 关于 `ifaddrs` 结构有很多文章介绍，本文仅简单介绍一下与本文密切相关的内容，下面是 `struct ifaddrs` 的定义
    ```C
    struct ifaddrs {
        struct ifaddrs  *ifa_next;          /* Next item in list */
        char            *ifa_name;          /* Name of interface */
        unsigned int     ifa_flags;         /* Flags from SIOCGIFFLAGS */
        struct sockaddr *ifa_addr;          /* Address of interface */
        struct sockaddr *ifa_netmask;       /* Netmask of interface */
        union {
            struct sockaddr *ifu_broadaddr; /* Broadcast address of interface */
            struct sockaddr *ifu_dstaddr;   /* Point-to-point destination address */
        } ifa_ifu;
    #define              ifa_broadaddr ifa_ifu.ifu_broadaddr
    #define              ifa_dstaddr   ifa_ifu.ifu_dstaddr
        void            *ifa_data;          /* Address-specific data */
    };
    ```
* `ifa_next` 是结构链表的后向指针，指向链表的下一项，当前项为最后一项时，该指针为 NULL；
* 本例中，我们的目标是找到这些网络接口中的无线网络接口，实际上我们仅需要 `ifa_name` 这个字段，也就是接口名称；
* 下面是获取全部网络接口的代码片段：
    ```C
    struct ifaddrs *ifs_start_pointer = NULL;

    if (getifaddrs(&ifs_start_pointer) == -1) {
        perror("can't get local address\n");
        exit(-1);
    }
    ```
### 3.4 如何判断网络接口是无线网络接口
* 头文件 `wireless.h` 中定义了一个 `SIOCGIWNAME` 指令，使用 `ioctl()` 调用该指令时只需设置接口名称，如果该接口是无线网络接口，`ioctl()` 执行成功并返回该接口使用的协议，否则，执行失败；
* 当我们生成了网络接口链表后，只需遍历该链表，并依此调用 `SIOCGIWNAME` 指令，便可找到所有的无线网络接口，并生成无线网络接口链表；
* 下面代码检查网络接口是否为无线接口，其中 `if_name` 为网络接口名称：
    ```C
    int sock;
    struct iwreq wreq;

    memset(&wreq, 0, sizeof(wreq));
    strncpy(wreq.ifr_name, if_name, IFNAMSIZ);      // 接口名称
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ioctl(sock, SIOCGIWNAME, &wreq) == 0) {
        printf("\nThe [%s] is a wireless interface. The protocol is %s\n", if_name, wreq.u.name);
    } else {
        printf("\nThe [%s] is a wireless interface.\n", if_name);
    }
    close(sock);
    ```
### 3.5 wifi信号扫描有关的其它技术要点
* 本文第 2 节已经对 wifi 信号的扫描原理做了详尽的描述，请参考 [**2 使用ioctl进行wifi信号扫描的基本原理**]；
* [**2.2 启动 wifi 信号扫描**] - 详细描述了启动 wifi 信号扫描的方法；
* [**2.3 获取 wifi 信号的扫描结果**] - 详细描述了获取 wifi 信号扫描结果的方法；
* [**2.4 扫描结果的数据格式**] - 详细描述了如何从扫描结果中提取出 event，以及如何从 event 提取出 wifi 信号属性的方法；
* 下面这张图对 wifi 信号扫描的过程做了简单的回顾：

    ![wifi signals scanning][img01]

### 3.6 关于内存对齐(memory alignment)
* 编写应用程序的程序员可能很少关心内存对齐问题，绝大多数情况下，内存对齐对应用程序的影响也不大，但内存对齐问题对本文有重要的影响；
* 我们用前面介绍过的 `struct iw_event` 来说明内存对齐对这个结构的影响：
    ```C
    struct iw_event {
        __u16		len;			/* Real length of this stuff */
        __u16		cmd;			/* Wireless IOCTL */
        union iwreq_data	u;		/* IOCTL fixed payload */
    };
    ```
* 不使用 `sizeof()` 函数，你能够猜到系统会为这个结构分配多少内存吗？
* 首先，对于 union 而言，系统会选择其中最大的一个结构为其分配内存，`union iwreq_data` 中最大字段的长度是16个字节，所以系统会为其分配 16 字节内存，加上 len 和 cmd 两个字段共 4 个字节，似乎系统应该为这个结构分配 20 个字节；
* 但是，如果用 `sizeof(struct iw_event)` 计算这个结构的大小，给出的结果是 24，那么多出来的 4 个字节在哪里呢？
* 这 4 个字节用于内存对齐了，我的 ubuntu 系统是 64 位(数据总线是 64 位)的，内存当然是按照 8 字节对齐的(32 位系统是按 4 字节对齐)，len 和 cmd 两个字段共用前 8 个字节中的前 4 个字节，后 4 个字节空着用于内存对齐，然后从第 9 个字节开始为 `union iwreq_data` 分配 16个字节的内存，这样算下来刚好是 24 个字节；
* 下面这段程序可以很直观地看到内存分配的实际情况
    ```C
    #include <stdio.h>
    #include <stdint.h>
    #include <string.h>
    #include <linux/wireless.h>

    int main() {
        struct iw_event wevt;

        wevt.cmd = 0x8b15;
        wevt.len = 20;
        strcpy(wevt.u.name, "struct iw_event");

        printf("sizeof(struct iw_event): %ld\n", sizeof(struct iw_event));
        printf("pointer of len: %p\n", &wevt.len);
        printf("pointer of cmd: %p\n", &wevt.cmd);
        printf("pointer of u.name: %p\n", &wevt.u);

        uint8_t *p = (uint8_t *)&wevt;
        for (int i = 0; i < sizeof(struct iw_event); ++i) {
            printf("%02x ", p[i]);
        }
        puts("");

        return 0;
    }
    ```
* 这段程序的运行截图

    ![Screenshot of iwevent test][img02]

    - 首先可以看到系统确实为 `struct iw_event` 分配了 24 字节的内存，而不是 20 字节；
    - 字段 len 的地址是 `~f160`，字段 cmd 的地址是 `~f162`，因为 len 的数据类型是 __u16，占用 2 个字节；
    - cmd 字段的类型也是 __u16，按理也应该占用 2 个字节，但字段 u 的地址却是 `~f168`，而不是 `~f164`，这其中多出的 4 个字节就是为了内存对齐；
    - 最后我们打印出了这个结构的所有数据，红线所示的 4 个字节就是为了内存对齐而填充的；

* 那么，为什么不在 cmd 字段后面为 `union iwreq_data` 分配内存呢？这是因为 64 位的系统每次从内存读 8 个字节，如果按照 8 字节对齐分配内存，读取 `union iwreq_data` 需要读两次，否则就需要读三次，速度降低 50%，当然也可以强制不按 8 字节对齐，节省了内存但会损失性能；
* 系统为这个结构按内存对齐规则分配内存后，是否还能和实际的数据流对应呢？我们看看 event_1 的数据：
    ```plain
    18 00 15 8B 00 00 00 00 01 00 DC FE 18 68 73 80 
    00 00 00 00 00 00 00 00 
    ```
* 0x0018 对应 len 字段，0x8B15 对应 cmd 字段，后面的 4 个字节刚好和 `struct iw_event` 中的 4 个用于对齐的字节一致，然后应该是 `struct sockaddr`，0x0001 是 `struct sockaddr` 中的 `sa_family`，再后面的 14 个字节是 `struct sockaddr` 中的 `sa_data` 字段，对应的非常好；
* 为了提取出 AP 的 ESSID，我们在前面自定义了一个结构 `struct iw_essid`
    ```C
    struct iw_essid {
        uint16_t len;
        uint16_t flags;
        char __attribute((aligned(8)))essid;
    }
    ```
    - 这里用了 `__attribute((aligned(8)))`，其作用也是为了 essid 字段能够按照 8 字节对齐的方式分配内存，因为 essid 字段是 char 型，仅占 1 个字节，与 len 和 flags 合起来也不超过 8 个字节，所以会紧跟着 flags 分配内存，这样会和实际的数据流不一致，所以这里必须要使用 `__attribute((aligned(8)))`；
* 如果希望更多地了解有关内存对齐的相关信息，请自行搜索相关文章。

## 4 完整的 wifi 信号扫描程序
* 完整的源代码，文件名：[wifi-scanner.c][src01](**点击文件名下载源程序**)，请务必使用 UTF-8 字符集，否则源程序中的中文注释为乱码；
* 编译，因为在计算工作频率时使用了函数 pow()，所以编译时要加上 `-lm` 选项，意即连接数学函数库；
    ```bash
    gcc -Wall wifi-scanner.c wifi-scanner -lm
    ```
* 运行，本程序的运行需要 root 权限，这主要是因为操作 wifi 需要 root 权限；
    ```bash
    sudo ./wifi-scanner
    ```
* 运行截图

    ![Screenshot of wifi-scanner][img03]

## 5 后记
* 本文仅处理了很少的几个 wifi 信号的属性，wifi 信号的属性还有很多，比如：信号强度、加密方式、安全协议等，读者可以试着扩展该程序，获取更多的 wifi 信号属性；
* 本文范例中的很多地方都是有修改空间的，比如在获取无线网络接口上，我们可以直接从 proc 文件系统上获得，读取 `/proc/net/wireless` 文件即可，proc 文件系统也是 IPC 的一种方式；
* 本文范例使用了 **Wireless Extensions** 的 API，非常遗憾的是，几乎找不到这方面的完整资料，学习其调用方法的唯一办法是认真阅读头文件 `wireless.h` 和学习一些使用 WE 的源代码，下面是有关的两个链接：
    - 相对完整的 **Wireless Extensions** 资料：[Wireless Tools for Linux][article01]
    - 官方发布的使用 WE 的无线工具源代码：[Wireless Tools for Linux][article02]
* 扫描 wifi 信号除了本文介绍的 ioctl() 方法外还有一些其它方法，希望今后有机会介绍其它方法；
* 对 wifi 信号的操作，扫描仅仅是一个基本的操作，还有其它操作，比如：连接 wifi、共享 wifi、配置wifi等；
* 实际上，对 wifi 进行编程的文章和资料并不多，希望这篇文章能给你带来一些启发和帮助。


## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://whowin.gitee.io/sourcecodes/180022/wifi-scanner.c

[article01]:https://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html
[article02]:https://github.com/HewlettPackard/wireless-tools

[img01]:https://whowin.gitee.io/images/180022/SIOCGIWSCAN.png
[img02]:https://whowin.gitee.io/images/180022/screenshot-of-iwevent-test.png
[img03]:https://whowin.gitee.io/images/180022/screenshot-of-wifi-scanner.png

