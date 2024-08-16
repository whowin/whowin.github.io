---
title: "使用ioctl扫描wifi信号获取信号属性的实例(二)"
date: 2024-04-11T16:43:29+08:00
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
# - https://github.com/HewlettPackard/wireless-tools
# - [Wireless LAN resources for Linux](https://hewlettpackard.github.io/wireless-tools/Wireless.html)
# - [Wireless connection: Link Quality: What does "31/70" indicate?](https://superuser.com/questions/866005/wireless-connection-link-quality-what-does-31-70-indicate)
# - [Introduction to Linux Wext and nl80211 interfaces](https://topic.alibabacloud.com/a/introduction-to-linux-wext-and-nl80211-interfaces_1_16_30227238.html)
# - [wireless ad hoc network (WANET)](https://www.techtarget.com/searchmobilecomputing/definition/ad-hoc-network)
# - [WiFi modes of operation (802.11 or Wi-Fi)](https://www.puntoflotante.net/Wifi-modes-of-operation.pdf)
# - [Wi-Fi研习者](https://www.zhihu.com/people/edward_xu)
#       - 基本都是讨论 wifi 协议的，值得学习
# - [802.11协议精读10：节能模式（PSM）](https://zhuanlan.zhihu.com/p/21623985?utm_id=0)
#       - 该文既介绍了 beacon 也介绍了 TSF
# - [A closer look at WiFi Security IE (Information Elements)](https://blogs.arubanetworks.com/industries/a-closer-look-at-wifi-security-ie-information-elements/)
# - [WPA Information Element](https://www.hitchhikersguidetolearning.com/2017/09/17/wpa-information-element/)
# - [802.11I-2004.PDF](https://paginas.fe.up.pt/~jaime/0506/SSR/802.11i-2004.pdf)



postid: 180025
---


使用工具软件扫描 wifi 信号是一件很平常的事情，在知晓 wifi 密码的前提下，通常我们会尽可能地连接信号质量比较好的 wifi 信号，但是如何通过编程来扫描 wifi 信号并获得这些信号的属性(比如信号强度等)，却鲜有文章提及，本文在前面博文的基础上通过实例向读者介绍如何通过编程扫描 wifi 信号，并获得信号的一系列的属性，本文给出了完整的源代码，本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；阅读本文并不需要对 IEEE802.11 协议有所了解，但本文实例中大量涉及链表和指针，所以本文可能不适合初学者阅读。

<!--more-->

## 1 前言
* 在[『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)专栏里写过一篇wifi信号扫描的文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]，与该文相比本文所附带的实例将可以获取更多的 wifi 属性；
* 在阅读本文前，请阅读[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]，并请理解范例中的程序，该文中所涉及的概念以及数据结构，本文将不再做介绍；
* 在[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]中，我们使用 `ioctl()` 启动了 wifi 信号的扫描，并获取了 wifi 信号的 SSID、MAC地址、工作频率和工作信道，但有一些重要的信号属性并没有获得，比如：信号强度、信号质量、信号噪音以及加密方式等，本文将讨论如何获取这些属性；
* 本文提供的实例的基本框架与文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]中的基本一致；
* 本文所提供的实例中并不需要第三方库的支持，所以不需要安装任何其它支持软件和库。



## 2 遍历网络设备列表
* 在对无线网卡操作之前，首先要找到无线网卡的设备名，在文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]中，使用 `getifaddrs()` 找到所有的网络接口，然后再用 `ioctl()` 的 SIOCGIWNAME 命令从中找到无线网卡；
* 其实我们可以从 `/proc/net/dev` 中找到所有的网络接口，而不必使用 `getifaddrs()`；
* 就本文而言，需要知道的就是无线网卡的设备名，我们也可以从文件 `/proc/net/wireless` 文件中直接获得，这种方法更加简单一些，先来看一下这个文件中有什么内容：
    ```
    $ cat /proc/net/wireless 
    Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
    face  | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
    wlp1s0: 0000   70. -256.  -256     0      0      0      0      1        0
    $ 
    ```
* 可以看到，这个文件的前两行是标题，从第三行起开始是无线网卡的信息，其中接口名称后面紧跟着 ": "；
* 这台电脑上只有一个无线网卡，其接口名称为：wlp1s0，下面程序片段从 `/proc/net/wireless` 中提取出无线接口的名称：

    ```C
    FILE *fh;
    char buff[1024];

    fh = fopen("/proc/net/wireless", "r");
    if (fh != NULL) {
        // Skip 2 lines of header
        fgets(buff, sizeof(buff), fh);
        fgets(buff, sizeof(buff), fh);
        // Read each device line
        while (fgets(buff, sizeof(buff), fh)) {
            char name[IFNAMSIZ + 1];

            // Skip empty lines.
            if ((buff[0] == '\0') || (buff[1] == '\0')) continue;
            // Extract interface name
            char *p = buff;
            // Skip leading spaces
            while (isspace(*p)) p++;
            char *end;
            end = strstr(buf, ": ");
            // Not found
            if (end == NULL) continue;
            // Copy
            memcpy(name, p, (end - p));
            name[end - p] = '\0';
            printf("The wireless interface name is %s\n", name);
        }
        fclose(fh);
    } else {
        printf("Can't open file /proc/net/wireless\n");
    }
    ```

## 3 信号质量、信号强度、信号噪音的获取
* 通过阅读文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]，应该可以了解如何使用 `ioctl()` 启动 wifi 信号的扫描并获得扫描结果，在此简单回顾一下：
    - `struct iwreq` 定义，其中 `struct iwreq_data` 见下面定义；
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
    - `ioctl()` 的调用方式：`int ioctl(int socket, unsigned long request, struct iwreq *wrq)`
    - 启动 wifi 扫描时，`request=SIOCSIWSCAN`，`wrq->ifr_name` 设置为无线接口名称，`wrq->u.data.pointer=NULL`，`wrq->u.data.flags=0`，`wrq->u.data.length=0`，然后调用 `ioctl()`;
    - 获取扫描结果时，`request=SIOCGIWSCAN`，`wrq->u.data.pointer=接收数据缓冲区指针`，`wrq->u.data.length=接收缓冲区的长度`，`wrq->u.data.flags=0`，然后调用 `ioctl()`，调用时，`wrq->ifr_name` 也是要设置为无线接口名称的，只是因为在启动扫描时已经设置过，所以这里通常不需要再设置；
    - 如果返回的扫描结果数据比较大，设置的接收缓冲区不够用，`ioctl()` 将返回 -1，errno 为 E2BIG，此时应该重新为缓冲区分配内存并再次调用 `ioctl()` 获取扫描结果；
    - 如果在使用 `ioctl()` 获取扫描结果时，扫描还没有完成，`ioctl()` 将返回 -1，errno 为 EAGAIN，此时应该等待一会再次调用 `ioctl()` 获取扫描结果；
    - 正常获取扫描结果时，`wreq->u.data.falgs` 将被设为 1(调用时为 0)，`wreq->u.data.length` 中为返回数据的实际长度，返回的数据被存放在 `wreq->u.data.pointer` 指向的数据缓冲区中；
* 返回的扫描结果是一个数据流(stream)，其中包含着许多的事件(event)，每个 event 包含着一个属性，返回的扫描结果数据符合 `struct iw_event`，每个 event 数据也符合 `struct iw_event`，这个结构定义在 `wireless.h` 中：
    ```C
    union iwreq_data {
        /* Config - generic */
        char                name[IFNAMSIZ];
        /* Name : used to verify the presence of  wireless extensions.
         * Name of the protocol/provider... */

        struct iw_point     essid;      /* Extended network name */
        struct iw_param     nwid;       /* network id (or domain - the cell) */
        struct iw_freq      freq;       /* frequency or channel :
                                         * 0-1000 = channel
                                         * > 1000 = frequency in Hz */

        struct iw_param     sens;       /* signal level threshold */
        struct iw_param     bitrate;    /* default bit rate */
        struct iw_param     txpower;    /* default transmit power */
        struct iw_param     rts;        /* RTS threshold threshold */
        struct iw_param     frag;       /* Fragmentation threshold */
        __u32               mode;       /* Operation mode */
        struct iw_param     retry;      /* Retry limits & lifetime */

        struct iw_point     encoding;   /* Encoding stuff : tokens */
        struct iw_param     power;      /* PM duration/timeout */
        struct iw_quality   qual;       /* Quality part of statistics */

        struct sockaddr     ap_addr;    /* Access point address */
        struct sockaddr     addr;       /* Destination address (hw/mac) */

        struct iw_param     param;      /* Other small parameters */
        struct iw_point     data;       /* Other large parameters */
    };
    struct iw_event {
        __u16               len;        /* Real length of this stuff */
        __u16               cmd;        /* Wireless IOCTL */
        union iwreq_data    u;          /* IOCTL fixed payload */
    }
    ```
* `struct iw_event` 的前两个字段，len 表明了这个 event 的数据长度，cmd 表明了这个 event 的类别，不同的 event，字段 u 中对应的数据结构也不相同；
* 在文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]中，我们只解析了三个 cmd，SIOCGIWAP(MAC地址)、SIOCGIWESSID(SSID) 和 SIOCGIWFREQ(Frequence 和 Channel)：
    - 当 cmd 为 SIOCGIWAP 时，字段 u 对应的数据结构为 `struct sockaddr`，MAC 地址存放在 `u.addr.sa_data` 的前 6 个字节中；
    - 当 cmd 为 SIOCGIWESSID 时，字段 u 对应的数据结构为 `struct iw_essid`，这个是自己定义的，在上面 union 中并没有列出，这个定义可以使问题更加简单一些；
    - 当 cmd 为 SIOCGIWFREQ 时，字段 u 对应的数据结构为 `struct iw_freq freq`，当计算出的频率大于 1000 时，则结果为 wifi 信号的工作频率，否则为该信号的工作信道；
* 当 cmd 为 IWEVQUAL，获得的信息为统计数据的信号质量部分(Quality part of statistics)，这部分数据中包括信号质量、信号强度、信号噪音等信息；
    - 此时字段 u 对应的数据结构为 `struct iw_quality`，在 `wireless.h` 中定义，如下：
        ```C
        struct iw_quality {
            __u8        qual;       /* link quality */
            __u8        level;      /* signal level (dBm) */
            __u8        noise;      /* noise level (dBm) */
            __u8        updated;    /* Flags to know if updated */
        };
        ```
    - qual 字段为信号连接质量，先看一下使用无线工具 `sudo iwlist [wifname] scan` 扫描信号看到的信号连接质量是什么样子的，[wifname] 是无线网络接口的名称，在我的电脑上是 `wlp1s0`，不同电脑可能会不一样；

        ![Screenshot of wifi scanning][img01]

    - 图中红线所示部分就是信号连接质量，其表达方式为 `47/70`，这是什么含义呢？
    - WE(Wireless Extension) 假设信号范围为 `-110dBm ~~ -40dBm`，信号质量的值为信号强度 +110 得出的，这样信号质量值的范围为 `0~~70`，`47/70` 的 47 表示信号连接质量值为 47，70 标志信号质量值最大为 70；
    - level 字段为信号的强度，其单位为 dBm(分贝毫瓦)，通常用于表示无线电信号的功率，如上所述，正常情况下，`level + 110 = qual`
    - noise 字段为信号背景噪音的强度，这个字段在我的电脑上并不支持，如何判断是否支持，请看下面对 updated 字段的介绍；
    - updated 字段是一个位掩码(bit mask)，在 `wireless.h` 中定义了该字段每个 bit 表达的含义，如下：
        ```C
        /* Statistics flags (bitmask in updated) */
        #define IW_QUAL_QUAL_UPDATED    0x01    /* Value was updated since last read */
        #define IW_QUAL_LEVEL_UPDATED   0x02
        #define IW_QUAL_NOISE_UPDATED   0x04
        #define IW_QUAL_ALL_UPDATED     0x07
        #define IW_QUAL_DBM             0x08    /* Level + Noise are dBm */
        #define IW_QUAL_QUAL_INVALID    0x10    /* Driver doesn't provide value */
        #define IW_QUAL_LEVEL_INVALID   0x20
        #define IW_QUAL_NOISE_INVALID   0x40
        #define IW_QUAL_RCPI            0x80    /* Level + Noise are 802.11k RCPI */
        #define IW_QUAL_ALL_INVALID     0x70

        ```
    - 如果网卡驱动程序不支持 quality、level 或者 noise，则 IW_QUAL_QUAL_INVALID、IW_QUAL_LEVEL_INVALID 或者 IW_QUAL_NOISE_INVALID 对应的 bit 就会被置 1；
    - 如果自上次读取 quality、level 或者 noise 后，数据已经被网卡驱动程序再次更新，则 IW_QUAL_QUAL_UPDATED、IW_QUAL_LEVEL_UPDATED 或者 IW_QUAL_NOISE_UPDATED 对应的 bit 会被置 1；

## 4 无线信号的工作方式
* wireless.h 中定义了 8 中无线信号的工作方式：
    ```C
    /* Modes of operation */
    #define IW_MODE_AUTO        0   /* Let the driver decides */
    #define IW_MODE_ADHOC       1   /* Single cell network */
    #define IW_MODE_INFRA       2   /* Multi cell network, roaming, ... */
    #define IW_MODE_MASTER      3   /* Synchronisation master or Access Point */
    #define IW_MODE_REPEAT      4   /* Wireless Repeater (forwarder) */
    #define IW_MODE_SECOND      5   /* Secondary master/repeater (backup) */
    #define IW_MODE_MONITOR     6   /* Passive monitor (listen only) */
    #define IW_MODE_MESH        7   /* Mesh (IEEE 802.11s) network */
    ```
* 我们扫描到的信号，大多数应该是 Master；
* 当使用 `ioctl()` 扫描无线信号时，返回的 `struct iw_event` 中当 cmd 字段为 SIOCGIWMODE，该事件为工作方式；
* 当 cmd 为 SIOCGIWMODE时，`struct iw_event` 中的 `u.mode` 为该无线信号的工作方式；

## 5 无线信号支持的传输速率
* 当使用 `ioctl()` 扫描无线信号时，返回的 `struct iw_event` 中当 cmd 字段为 SIOCGIWRATE 时，该事件中的数据为信号支持的传输速率；
* 一个信号支持的传输速率通常有很多种，所以这个数据通常也是有很多组的，下面是一组实际的数据(16进制数)：
    ```
    0000:   28 00 21 8B 00 00 00 00 80 8D 5B 00 00 00 00 00 
    0010:   00 1B B7 00 00 00 00 00 00 36 6E 01 00 00 00 00 
    0020:   00 6C DC 02 00 00 00 00
    ```
* 按照 `struct iw_event` 的定义，前两个字节是这个 event 的长度，为 0x0028，也就是 40 个字节，后面两个字节 0x8B21 是 cmd 字段，0x8b21 也就是 SIOCGIWRATE(见 `wireless.h` 中的定义)，所以这个 event 的数据是传输速率；
* 当收到的是传输速率时，`struct iw_event` 中的 `u.bitrate` 为对应的传输率的数据结构(见第 3 节关于 `union iwreq_data` 的介绍)，`u.bitrate` 是一个 `struct iw_param`，其定义如下(见 wireless.h)：
    ```C
    struct iw_param {
        __s32   value;      /* The value of the parameter itself */
        __u8    fixed;      /* Hardware should not use auto select */
        __u8    disabled;   /* Disable the feature */
        __u16   flags;      /* Various specifc flags (if any) */
    };
    ```
* 根据其定义，其中的 `u.bitrate.value` 字段即为传输速率；
* 如上数据，一个 wifi 信号通常都是支持多种传输速率的，这时可以将数据部分定义成一个 `struct iw_param` 的结构数组，并通过 event 的长度和 `struct iw_param` 的长度计算得出这个 event 中有多少组传输速率的数据，如下：
    ```C
    ......
    struct iw_event *evp = data;
    int rate_count = (evp.len - IW_EV_LCP_LEN) / sizeof(struct iw_param);
    struct iw_param *rates = &evp->u.bitrate;
    int i = 0;
    for (i = 0; i < rate_count; ++i) {
        ......
        printf("Bit rate: %d Mb/s\n", rates[i].value / 1000000);
    }
    ```
* 其中 IW_EV_LCP_LEN 为 `struct iw_event` 中结构头(len 和 cmd 字段)长度(包含为对齐而填充的空字符)，请见文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]中的相关解释；
* 当一个 WiFi 信号支持的传输速率比较多时，可能会收到两个 SIOCGIWRATE 事件(也许会有多个，但我没有遇到过)，每个事件中的速率是不一样的，所以都需要处理，不能忽略任何一个事件；

## 6 beacon 相关信息
* 当使用 `ioctl()` 扫描无线信号时，返回的 `struct iw_event` 中当 cmd 字段为 IWEVCUSTOM 时，该事件在 `wireless.h` 中定义为 "Driver specific ascii string"，意为：驱动程序特定的 ASCII 字符串；
* AP 要周期性地在 wifi 上广播 beacon 帧，用于在网络上宣告一个 wifi 信号的存在，之所以可以扫描到 wifi 信号就是因为收到了 beacon 帧；
* beacon 帧并不是本文要讨论的问题，本文不会展开讨论；
* 回到 WiFi 信号的扫描主题上，`wireless.h` 中并没有定义一个事件可以收到有关 beacon 帧的信息，但是我们在事件 IWEVCUSTOM 中看到了 beacon 信息；
* IWEVCUSTOM 事件中的这个字符串的结构与 essid 是一样的，所以可以用相同的方法提取，可以参考文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]；
* 有意思的是起初并不知道这个字符串中会有什么内容，但把收到的内容显示出来后，发现是类似下面的内容：

    ![Screenshot of driver specific ascii string][img02]

* 图中红线所示就是收到的字符串，原来这个字符串中藏着与 beacon 帧有关的信息，Last beacon 是最后收到的 beacon 帧时间，TSF 是 Timing Synchronization Function 的缩写，是 beacon 帧中的一个字段，用于 AP 和 Station 之间同步时间；
* 这个事件与 essid 还是有所不同，一个 WiFi 信号只有一个 essid，所以 SIOCGIWESSID 事件只会收到一次，但是 IWEVCUSTOM 有可能收到多次，而每次收到的字符串是不相同的，所以接收 IWEVCUSTOM 事件比接收 essid 还是要麻烦一些，在本文实例源程序中这部分有详细的中文注释；

## 7 Information Elements
* 当使用 `ioctl()` 扫描无线信号时，返回的 `struct iw_event` 中当 cmd 字段为 IWEVGENIE 时，该事件在 `wireless.h` 中定义为 "Generic IE (WPA, RSN, WMM, ..)"，意为通用 IE(Information Elements)；
* IE 可以提供非常多的信息，本文中仅就 SSID 和速率信息进行示范性的解析；
* 首先，这个事件在大多数情况下都是提供多个 IE，所以事件通常比较长，而且长度并不固定；
* 我们先来看一个实际收到的 IE 数据

    ![Real IE data][img03]

* 数据与普通事件一样，符合 `struct iw_event` 结构，前两个字节是事件数据的长度，0x00D2(十进制 210)字节，第 3、4 字节为事件类型，0x8C05(IWEVGENIE) 表示这个事件中是 IE 数据；
* 前四个字节组成了 `struct iw_event` 的头信息，紧跟着的 4 个字节为按 64 位对齐填充的字符，第 9、10 字节为实际 IE 数据所占的长度 ie_length，第 11-16 字节是对齐填充字节；
* 所以从第 17 个字节(第 2 行)开始才是真正的 IE 数据，正是因为这个原因，通常 ie_length(第 9、10 字节) 比事件的总长度(第 1、2 字节)小 16(0x10)，在这组实际数据中，事件长度为 0x00D2(十进制 210)，而 IE 数据的长度为 0x00C2(十进制 194)，要注意的是，事件长度是包含 `struct iw_event` 头信息的 8 个字节，而 IE 数据长度是不包括头信息的长度(仅为 IE 数据长度)；
* IEEE 标准 `802.11-2007` 文档中定义了 IE 的结构，该文档的下载地址如下：
    - [Wireless LAN Medium Access Control (MAC) and Physical Layer (PHY) Specifications][article04]
* 该标准的 `7.3.2 Information elements` 定义了 IE 的结构：

    ![IE Structure][img05]

* 每个 IE 符合 `type-length-value` 格式，即：第 1 个字节表示 IE 类型 Element ID，第 2 个字节表示数据的长度 Length，后面若干字节为实际数据 Information，数据长度为 Length；
    ```C
    struct ieee80211_ie {
        unsigned char eid;
        unsigned char len;
        unsigned char data[0];
    };
    ```
* 当收到一个 IE 数据的事件时，可以考虑如下定义：
    ```C
    struct iw_event_ie {
        unsigned short len;
        unsigned short cmd;
    #ifdef __x86_64__           // 64位系统按8字节对齐
        unsigned short __attribute((aligned(8)))ie_len;
        struct ieee80211_ie __attribute((aligned(8)))ie[0];
    #else                       // 32位系统按4字节对齐
        unsigned short __attribute((aligned(4)))ie_len;
        struct ieee80211_ie __attribute((aligned(4)))ie[0];
    #endif
    };
    ```
* 字段 len 和 cmd 与 `struct iw_event` 是一致的，ie_len 字段将得到 ie 这个字段所对应的数据的总长度，`struct ieee80211_ie` 则定义了每个 IE 的结构，具体有多少个 IE 则需要在遍历 IE 时根据 ie_len 字段的值做出判断；
* `struct ieee80211_ie` 中的 eid(Element ID) 字段定义了这个 IE 的类型，这些类型在 `802.11-2007` 的文档中第 100 页有定义，这里摘录其中的一部分：

    ![Parts of IE type definition][img04]

* 从上面定义可以看到，当 Element ID 为 0 时，其信息内容为 SSID，以上面的实际数据为例，数据的第二行，也就是第 1 个 IE 的数据为：
    ```plain
    00 07 31 35 2D 31 31 30 31 
    ```
    - 按照 IE 的格式定义，Element ID 为 0，表示其信息为 SSID，数据长度 Length 为 7，所以后面的 7 个字节 `31 35 2D 31 31 30 31` 为实际数据；
    - 查 ASCII 表，这个数据其实就是 "15-1101"，这就是这个信号的 SSID
* IE 提供了一种非常灵活的传递信息的方法，内容非常丰富，在本文所载实例中仅就其中的几个进行了解析； 
* 还要简单介绍一下所支持传输速率的 IE，因为在实例中解析了这个 IE;
* 根据 Element ID 的定义，当 Element ID 为 1 时，IE 的数据为该信号所支持的传输速率，其结构在 `802.11-2007` 文档的第 102 页有介绍：

    ![Supported Rates][img06]

* 根据文档，一个 IE 中最多描述 8 个传输速率，单位为 500 kb/s，每个速率占用 1 个字节，其最高位(bit 7)有其它意义，(bit 0 - 6) 表示速率值，所以在计算时要将 bit 7 过滤掉；
* IE 信息会有空信息，也就是长度字段为 0，这种 IE 通常只有 2 个字节，没有意义，在实际解析中要过滤掉，比如：
    ```00 00```

## 8 实例
* 完整的源代码，文件名：[wifi-new-scanner.c][src01](**点击文件名下载源程序**)，请务必使用 UTF-8 字符集，否则源程序中的中文注释为乱码；
* 阅读这个源码最好先阅读文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]，并搞懂其中的源代码；
* 简述一下程序的基本流程：
    - 通过读取文件 `/proc/net/wireless` 获取无线网络接口的列表(在文章[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]中使用的是另一种方法)；
    - 使用 `ioctl()` 向无线网络接口发出 wifi 信号扫描指令；
    - 使用 `ioctl()` 获取扫描结果，根据返回的结果生成事件链表；
    - 从事件链表中分析每一个事件，从中解析出每个信号的属性，生成 ap 链表；
    - 如果有 IE 事件，还要在 AP 链表中生成 IE 链表；
    - 从 AP 链表中解析出信息并显示出来；
* 比较[《使用ioctl扫描wifi信号获取信号属性的一个范例(一)》][article03]中的实例，本文除了解析出 MAC 地址、ESSID、工作频率、工作信道外，还可以解析出信号质量、工作模式、信号支持的传输速率、驱动程序字符串以及 Information Elements；
* 源程序中有比较详细的注释请自行参考；
* 其中比较复杂的是 IE 的解析，IE 的内容极其丰富，作为示范，本例仅解析了 SSID 和速率，需要更多信息的读者可以查阅 [802.11-2007][article04] 文档；
* 编译：`gcc -Wall wifi-new-scanner.c -o wifi-new-scanner -lm`
* 运行：`sudo ./wifi-new-scanner`
* 运行截图：

    ![GIF of running wifi-new0scanner][img07]









## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180025/wifi-new-scanner.c

[article01]:https://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html
[article02]:https://github.com/HewlettPackard/wireless-tools

[article03]:https://blog.csdn.net/whowin/article/details/131504380
[article04]:https://people.iith.ac.in/tbr/teaching/docs/802.11-2007.pdf

[img01]: https://whowin.gitee.io/images/180025/screenshot-of-iwlist-scan.png
[img02]: https://whowin.gitee.io/images/180025/screenshot-of-driver-string.png
[img03]: https://whowin.gitee.io/images/180025/screenshot-of-IE-data.png
[img04]: https://whowin.gitee.io/images/180025/screenshot-of-80211-2007-1.png
[img05]: https://whowin.gitee.io/images/180025/screenshot-of-IE-structure.png
[img06]: https://whowin.gitee.io/images/180025/screenshot-of-rates.png
[img07]: https://whowin.gitee.io/images/180025/wifi-new-scanner.gif

<!-- CSDN
[img01]: https://img-blog.csdnimg.cn/img_convert/7d6fc4505697687bb568f4dea1e7f7e3.png
[img02]: https://img-blog.csdnimg.cn/img_convert/3052fde5a47530e97638609e2f6cfebf.png
[img03]: https://img-blog.csdnimg.cn/img_convert/20a274b8e264f90ccf902ce54283a7b3.png
[img04]: https://img-blog.csdnimg.cn/img_convert/36acac0e26b0c21a42747c47d1ab4fcb.png
[img05]: https://img-blog.csdnimg.cn/img_convert/dc8030541ffad49b50e9d338c4893f84.png
[img06]: https://img-blog.csdnimg.cn/img_convert/36a9a890228b3fc5597b57b2ee32b430.png
[img07]: https://img-blog.csdnimg.cn/img_convert/5a5bd5b1aa4d9ac0912e3b89efc43d89.gif
-->
