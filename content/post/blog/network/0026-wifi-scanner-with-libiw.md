---
title: "Linux下使用libiw进行无线信号扫描的实例"
date: 2024-07-05T16:43:29+08:00
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
    - "Linux"
    - "网络编程"
    - "802.11"
    - "wifi"
    - "无线网络"
    - ioctl
draft: false
#references: 

postid: 180026
---

打开电脑连接wifi是一件很平常的事情，但这些事情通常都是操作系统下的wifi管理程序替我们完成的，如何在程序中扫描wifi信号其实资料并不多，前面已经有两篇文章介绍了如何使用ioctl()扫描wifi信号，但其实在Linux下有一个简单的库对这些ioctl()的操作进行了封装，这个库就是libiw，使用libiw可以简化编程，本文介绍了如果使用libiw对wifi信号进行扫描的基本方法，本文将给出完整的源代码，本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；尽管本文内容主要涉及无线网络，但读者并不需要对 802.11 标准有所了解。

<!--more-->

## 1 前言
* 前面已经有两篇文章介绍了如何扫描 wifi 信号，[《使用ioctl扫描wifi信号获取信号属性的实例(一)》][article01]和[《使用ioctl扫描wifi信号获取信号属性的实例(二)》][article02]，这两篇文章均是使用 `ioctl()` 完成的 wifi 信号扫描；
* 本文介绍使用 libiw 库进行 wifi 信号扫描的方法，比较前两篇文章中介绍的方法，编程上更加简单；
* 实际上使用 libiw 扫描 wifi 信号，本质上还是使用 `ioctl()`；
* 在大多数以 Linux 内核为基础的操作系统中，都是包含 WE(Wireless Extensions) 的，WE 实际就是一组在用户空间操作无线网卡驱动程序的一组 API，库 libiw 是对 WE 的一个封装；
* 尽管库 libiw 可以给 wifi 编程带来一定的便利，但其实这是一个已经过时的库，这个库的最后更新日期是 2009 年，尽管如此，现在的绝大多数无线网卡驱动程序仍然支持 WE，所以我们仍然可以使用 libiw 进行 wifi 编程；
* 一些常用的 wifi 工具软件是使用 WE 实现的，比如：iwlist、iwconfig 等，由此也可以看出 WE 在 wifi 编程中仍然占有很重要的位置；

## 2 安装 libiw
* libiw 包含在开源项目 [Wireless Tools][proj01] 中，可以自行编译 libiw 库或者使用 apt 安装
* 在 ubuntu 上使用 apt 安装 libiw
    ```bash
    sudo apt update
    sudo apt install libiw-dev
    ```
* 自行编译 libiw
    - 克隆项目：
        ```
        git clone https://github.com/HewlettPackard/wireless-tools
        ```
    - 编译动态库 libiw.so.29
        ```bash
        gcc -Os -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wcast-qual -Winline -I. -MMD -fPIC -c -o iwlib.so iwlib.c
        gcc -shared -o libiw.so.29 -Wl,-soname,libiw.so.29  -lm -lc iwlib.so
        ```
    - 编译静态库 libiw.a
        ```
        rm -f libiw.a
        ar cru libiw.a iwlib.so
        ranlib libiw.a
        ```

* 要说明的是，使用 apt 安装的 libiw，其动态链接库为 `libiw.so.30`，但是使用这个项目的开源版本编译出来的动态链接库为 `libiw.so.29`，版本略有不同，`libiw.so.30` 更新于 2009 年，而开源项目的源代码更新于 2007 年，二者还是略有差别的；

## 3 wifi扫描涉及的 libiw 函数和数据结构
* 使用 libiw 编写 wifi 扫描程序，比起使用 `ioctl()` 要容易一些，但可以获得的信息远不如使用 `ioct()` 直接扫描 wifi 获得的信息多；
* 以下一些 libiw 中的函数和数据结构在本文的实例程序中会使用到，这些函数的更详细的说明，也可以查看另一篇文章[《libiw中的函数说明》][article06]；

* `int iw_sockets_open(void)`

    > 这个函数逐个尝试使用不同的协议族建立 socket，直至成功或者全部失败，尝试的顺序为：`AF_INET、AF_IPX、AF_AX25、AF_APPLETALK`，绝大多数情况下可以使用 `AF_INET` 建立 socket，成功返回 socket，失败则返回 -1；

* `void iw_enum_devices(int skfd, iw_enum_handler fn, char *args[], int count)`

    > 这个函数会列举出系统中的所有网络接口，每找到一个网络接口就会调用一次函数 `fn()`，`args[]` 是传给 `fn()` 的参数数组，count 是参数的数量，本文利用这个函数在所有网络接口中找到无线网络接口，然后对无线网络接口进行扫描；

    > 其查找无线网络接口的原理是：当 `iw_enum_devices()` 函数找到一个网络接口时会调用 `fn()`，在函数 `fn()` 辨别该网络接口是否为无线网络接口；

* `int iw_get_range_info(int skfd, const char *ifname, iwrange *range)`

    > 使用这个函数主要是为了得到当前系统 WE(Wireless Extensions) 的版本号，在调用 iw_scan() 对无线网络接口进行扫描时，需要 WE 的版本号作为参数，这是因为不同版本的 WE 在进行扫描时方法略有差异；

* `int iw_scan(int skfd, char *ifname, int we_version, wireless_scan_head *context)`

    > 对无线网络接口进行扫描，ifname 为网络接口名称，扫描结果放在 context 中，context 是 `struct wireless_scan_head` 的指针，里面存放了扫描结果链表的首指针 result，`struct wireless_scan_head` 定义在 iwlib.h 中，其具体结构如下：

    ```C
    typedef struct wireless_scan_head
    {
        wireless_scan   *result;    /* Result of the scan */
        int             retry;      /* Retry level */
    } wireless_scan_head;
    ```

    > 其中的 wireless_scan 就是 `struct wireless_scan`，同样定义在 iwlib.h 中，其具体定义如下：

    ```C
    typedef struct wireless_scan
    {
        /* Linked list */
        struct wireless_scan *next;

        /* Cell identifiaction */
        int         has_ap_addr;
        sockaddr    ap_addr;        /* Access point address */

        /* Other information */
        struct wireless_config  b;  /* Basic information */
        iwstats stats;              /* Signal strength */
        int     has_stats;
        iwparam maxbitrate;         /* Max bit rate in bps */
        int     has_maxbitrate;
    } wireless_scan;

    ```

    > 这是一个结构链表，next 指向链表的下一项，链表中的每一项表示一个扫描到的信号，在 struct wireless_scan 中，当 has_ap_addr 不为 0 时，ap_addr 中存放着该信号对应的 AP 的 MAC 地址；当 has_maxbitrate 不为 0 时，maxbitrate 中存放着该信号所支持的最大传输速率；当 has_stats 不为 0 时，stats 中存放着信号强度；信号的 SSID、工作频率等，存放在 b 中，b 是一个 struct wireless_scan 结构，定义在 iwlib.h 中，其具体定义如下：

    ```C
    struct wireless_config
    {
        char    name[IFNAMSIZ + 1]; /* Wireless/protocol name */
        int     has_nwid;
        iwparam nwid;               /* Network ID */
        int     has_freq;
        double  freq;               /* Frequency/channel */
        int     freq_flags;
        int     has_key;
        unsigned char   key[IW_ENCODING_TOKEN_MAX]; /* Encoding key used */
        int     key_size;           /* Number of bytes */
        int     key_flags;          /* Various flags */
        int     has_essid;
        int     essid_on;
        char    essid[IW_ESSID_MAX_SIZE + 2];   /* ESSID (extended network) */
        int     essid_len;
        int     has_mode;
        int     mode;               /* Operation mode */
    }
    ```

    > 实际上这里面的一些项很多 WE 都已经不再支持，本例中我们会用到以下字段：name、freq、essid，其它字段很多 WE 并不返回数据；

* `void iw_sockets_close(int skfd)`

    > 这个函数将关闭一个使用 iw_sockets_open() 打开的 socket，其实这个函数和 close() 无异，所以你可以直接用 close(skfd) 关闭 socket。


## 4 使用 libiw 扫描 wifi 信号的基本思路
* **扫描 wifi 信号的基本流程**
    1. 使用 `iw_sockets_open()` 建立一个socket
    2. 使用 `iw_enum_devices()` 枚举所有的网络设备，并从中找到无线网卡设备名称
    3. 使用 `iw_get_range_ino()` 获取 WE 的版本号
    4. 使用 `iw_scan()` 启动 wifi 信号扫描并获取扫描结果
    5. 使用 `iw_sockets_close()` 关闭使用 `iw_sockets_open()` 打开的 socket

* **使用 iw_scan() 获取扫描结果的问题**
    - 与使用 `ioctl()` 进行 wifi 信号扫描(请参考[《使用ioctl扫描wifi信号获取信号属性的实例(一)》][article01]和[《使用ioctl扫描wifi信号获取信号属性的实例(二)》][article02])相比，使用 libiw 扫描 wifi 信号得到的信息其实要少一些，这也是这种方法的最主要的不足；
    - 这主要是因为 `struct wireless_scan` 这个结构可以容纳的信息十分有限
    - `iw_scan()` 需要 4 个参数(见上节关于该函数的说明)，其中第 4 个参数是 `wireless_scan_head *context`；
    - wireless_scan_head 和其中的 wireless_scan 的数据结构在上节已经做了详细的介绍，扫描到的 wifi 信号的数据实际存放在一个 wireless_scan 结构链表中；
    - 受结构所限，很多信息都不得不减少，比如一般一个信号可以支持很多连接速率，但是该结构中只给出一个 maxbitrate 字段，所以无法所有信号支持的传输速率
    - 在比如一个信号通常可以支持多个不同的信道，每个信道为不同的工作频率，使用 libiw 扫描信号只能得到其列表中的最后一个频率值；
    - 这些缺失的数据在使用 ioctl() 进行信号扫描时都是可以得到的。


## 5 使用 libiw 扫描 wifi 信号的实例
* 完整的源代码，文件名：[iw-scanner.c][src01](**点击文件名下载源程序**)；
* 这个程序比起在文章[《使用ioctl扫描wifi信号获取信号属性的实例(一)》][article01]和[《使用ioctl扫描wifi信号获取信号属性的实例(二)》][article02]的程序要简单得多；
* 简述一下程序的基本流程：
    - 使用 `iw_sockets_open()` 为内核打开一个 socket；
    - 使用 `iw_enum_devices()` 枚举所有的网络接口，在调用 `iw_enum_devices()` 设置了一个回调函数 `iw_ifname()`，并将一个字符缓冲区的指针 ifname 传给该回调函数；
    - 当 `iw_enum_devices()` 发现一个网络接口时，便会调用 `iw_ifname()`，`iw_ifname()` 通过一个 ioctl() 的调用来判断该接口是否为无线网络接口，如果是，则将其接口名称复制到参数 ifname 中，并返回 1，否则返回 0；
    - 执行完 `iw_enum_devices()` 如果 ifname 中没有接口名称，则表示当前系统中么有无线网络接口，直接退出程序，如果 ifname 中已有网络接口名称，则使用该接口准备开始进行 wifi 信号扫描；
    - 使用 `iw_get_range_info()` 获取该无线网络接口的 range 信息，实际上只是需要获得 WE 的版本号，以便在下面使用；
    - 使用 libiw 的 `iw_scan()` 启动 wifi 信号扫描非常容易，`iw_scan()` 需要四个参数，第一个是用 `iw_sockets_open()` 打开的 socket，第二个是我们在上面获取的无线网络接口的名称，第三个是 WE 的版本号，第四个是用于存储扫描结果链表的头指针；
    - `iw_scan()` 的执行是需要 root 权限的，所以这个程序要以 sudo 的方式运行，另外 `iw_scan()` 的执行也需要一定时间；
    - 获取扫描结果，可以通过检查扫描结果链表的头指针来判断是否有扫描结果，如果这个指针不为空则表示有扫描结果；
    - 接下来就是遍历整个扫描结果链表，并将其中的信息显示出来，比较文章[《使用ioctl扫描wifi信号获取信号属性的实例(一)》][article01]和[《使用ioctl扫描wifi信号获取信号属性的实例(二)》][article02]中的扫描结果，这里获得的信息比较有限；
    - 最后使用 `iw_sockets_close()` 关闭打开的 socket。

* 程序中有些不明确的概念或做法，请参考文章[《使用ioctl扫描wifi信号获取信号属性的实例(一)》][article01]和[《使用ioctl扫描wifi信号获取信号属性的实例(二)》][article02]
* 编译：`gcc -Wall iw-scanner.c -o iw_scanner -liw`
* 运行：`sudo ./iw-scanner`
* 运行截图：

    ![Screenshot of iw-scanner][img01]






## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180026/iw-scanner.c

[proj01]:https://github.com/HewlettPackard/wireless-tools


[article01]:/post/blog/network/0022-how-to-scan-wifi-signal/
[article02]:/post/blog/network/0025-another-wifi-scanner-example/

<!--CSDN
[article01]:https://blog.csdn.net/whowin/article/details/131504380
[article02]:https://blog.csdn.net/whowin/article/details/137711398
-->

[article03]:https://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html
[article04]:https://github.com/HewlettPackard/wireless-tools
[article05]:https://people.iith.ac.in/tbr/teaching/docs/802.11-2007.pdf
<!-- libiw中的函数说明 -->
[article06]:/post/blog/network/0027-libiw-functions/
<!--CSDN
[article06]:https://blog.csdn.net/whowin/article/details/140196003
-->

[img01]:/images/180026/screenshot-of-iwscanner.png

<!-- CSDN
[img01]:https://i-blog.csdnimg.cn/direct/b4c58b69bdd346cc96b7310fd78235c2.png
-->