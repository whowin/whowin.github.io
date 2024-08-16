---
title: "libiw中的函数说明"
date: 2024-04-13T16:43:29+08:00
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
# - [Wireless tools for Linux](https://en.wikipedia.org/wiki/Wireless_tools_for_Linux)

postid: 180027
---

打开电脑连接wifi是一件很平常的事情，但这些事情通常都是操作系统下的wifi管理程序替我们完成的，如何在程序中连接指定的wifi其实很少有资料介绍，在网络专栏的文章中，有两篇是关于wfi编程的文章，其中对无线网卡的操作都是通过ioctl()完成的，显得有些繁琐和晦涩，但其实WE(Wireless Extensions)有一个简单的库libiw，这个库的实现也是使用ioctl()，但是经过封装后，会使wifi编程变得容易一些，本文为一篇资料类的文章，主要描述libiw中API的调用方法。

<!--more-->

## 1 前言
* 在大多数以 Linux 内核为基础的操作系统中，都是包含 WE(Wireless Extensions) 的，WE 实际就是一组在用户空间操作无线网卡驱动程序的一组 API，库 libiw 是对 WE 的一个封装；
* 尽管库 libiw 可以给 wifi 编程带来一定的便利，但其实这是一个已经过时的库，这个库的最后更新日期是 2009 年，尽管如此，现在的绝大多数无线网卡驱动程序仍然支持 WE，所以我们仍然可以使用 libiw 进行 wifi 编程；
* 一些常用的 wifi 工具软件是使用 WE 实现的，比如：iwlist、iwconfig 等，由此也可以看出 WE 在 wifi 编程中仍然占有很重要的位置；
* WE 的基本实现是使用 ioctl()，前面的两篇文章 [《使用ioctl扫描wifi信号获取信号属性的实例(一)》][article01] 和 [《使用ioctl扫描wifi信号获取信号属性的实例(二)》][article02] 都是使用 ioctl() 实现的；
* libiw 是对 WE 的一个封装，使编程者不必直接面对 ioctl() 中复杂的调用方法和返回数据，可以给编程带来一些便利；
* 之所以要写这样一份资料类的文章，是因为完整的 libiw 中 API 调用方法的资料几乎没有，libiw API 的使用方法几乎只能通过阅读源码来学习，很不方便；
* 本文是通过阅读 libiw 的源码 `iwlib.c` 和 `iwlib.h` 以及一些 wifi 工具软件的源码总结而成；

* 项目 [Wireless Tools][article04] 中的 `iwlib.c` 和 `iwlib.h` 组成了 libiw
* 克隆项目：
    ```
    git clone https://github.com/HewlettPackard/wireless-tools
    ```
* 编译静态库 libiw.so.29
    ```bash
    gcc -Os -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wcast-qual -Winline -I. -MMD -fPIC -c -o iwlib.so iwlib.c
    gcc -shared -o libiw.so.29 -Wl,-soname,libiw.so.29  -lm -lc iwlib.so
    ```
* 编译静态库 libiw.a
    ```
    rm -f libiw.a
    ar cru libiw.a iwlib.so
    ranlib libiw.a
    ```

* 在 ubuntu 上，要使用 libiw 需要单独安装，安装方法非常容易：
    ```bash
    sudo apt update
    sudo apt install libiw-dev
    ```

* 要说明的是，使用 apt 安装的 libiw，看到的动态链接库为 `libiw.so.30`，但是使用这个项目的开源版本编译出来的动态链接库为 `libiw.so.29`，版本略有不同，从 `iwlib.h` 看，`libiw.so.30` 更新于 2009 年，而开源项目的源代码更新于 2007 年，二者还是有一些差距的；
* 从 `libiw.h` 看，`libiw.so.30` 中包含了一些 `libiw.so.29` 中没有的函数，但总体看，区别不是很大。

## 2. 库函数列表(共计 52 个)
* **SOCKET SUBROUTINES(2 个)**
    1. `int iw_sockets_open(void);`
    2. `void iw_enum_devices(int skfd, iw_enum_handler fn, char *args[], int count);`
* **WIRELESS SUBROUTINES(6个)**
    1. `int iw_get_kernel_we_version(void);`
    2. `int iw_print_version_info(const char *toolname);`
    3. `int iw_get_range_info(int skfd, const char *ifname, iwrange *range);`
    4. `int iw_get_priv_info(int skfd, const char *ifname, iwprivargs **    ppriv);`
    5. `int iw_get_basic_config(int skfd, const char *ifname, wireless_config *info);`
    6. `int iw_set_basic_config(int skfd, const char *ifname, wireless_config *info);`
* **PROTOCOL SUBROUTINES(1个)**
    1. `int iw_protocol_compare(const char *protocol1, const char *protocol2);`
* **ESSID SUBROUTINES(2个)**
    1. `void iw_essid_escape(char *dest, const char *src, const int slen);`
    2. `int iw_essid_unescape(char *dest, const char *src);`
* **FREQUENCY SUBROUTINES(7个)**
    1. `void iw_float2freq(double in, iwfreq *out);`
    2. `double iw_freq2float(const iwfreq *in);`
    3. `void iw_print_freq_value(char *buffer, int buflen, double freq);`
    4. `void    iw_print_freq(char *buffer, int buflen, double freq, int channel, int freq_flags);`
    5. `int iw_freq_to_channel(double freq, const struct iw_range *range);`
    6. `int iw_channel_to_freq(int channel, double *pfreq, const struct iw_range *range);`
    7. `void iw_print_bitrate(char *buffer, int buflen, int bitrate);`
* **POWER SUBROUTINES(3个)**
    1. `int iw_dbm2mwatt(int in);`
    2. `int iw_mwatt2dbm(int in);`
    3. `void iw_print_txpower(char *buffer, int buflen, struct iw_param *txpower);`
* **STATISTICS SUBROUTINES(2个)**
    1. `int iw_get_stats(int skfd, const char *ifname, iwstats *stats, const iwrange *range, int has_range);`
    2. `void iw_print_stats(char *buffer, int buflen, const iwqual *qual, const iwrange *range, int has_range);`
* **ENCODING SUBROUTINES(3个)**
    1. `void iw_print_key(char *buffer, int buflen, const unsigned char *key, int key_size, int key_flags);`
    2. `int iw_in_key(const char *input, unsigned char *key);`
    3. `int iw_in_key_full(int skfd, const char *ifname, const char *input, unsigned char *key, __u16 *flags);`
* **POWER MANAGEMENT SUBROUTINES(2个)**
    1. `void iw_print_pm_value(char *buffer, int buflen, int value, int flags, int we_version);`
    2. `void iw_print_pm_mode(char *buffer, int buflen, int flags);`
* **RETRY LIMIT/LIFETIME SUBROUTINES(1个)**
    1. `void iw_print_retry_value(char *buffer, int buflen, int value, int flags, int we_version);`
* **TIME SUBROUTINES(1个)**
    1. `void iw_print_timeval(char *buffer, int buflen, const struct timeval *time, const struct timezone *tz);`
* **ADDRESS SUBROUTINES(9个)**
    1. `int iw_check_mac_addr_type(int skfd, const char *ifname);`
    2. `int iw_check_if_addr_type(int skfd, const char *ifname);`
    3. `char *iw_mac_ntop(const unsigned char *mac, int maclen, char *buf, int buflen);`
    4. `void iw_ether_ntop(const struct ether_addr *eth, char *buf);`
    5. `char *iw_sawap_ntop(const struct sockaddr *sap, char *buf);`
    6. `int iw_mac_aton(const char *orig, unsigned char *mac, int macmax);`
    7. `int iw_ether_aton(const char *bufp, struct ether_addr *eth);`
    8. `int iw_in_inet(char *bufp, struct sockaddr *sap);`
    9. `int iw_in_addr(int skfd, const char *ifname, char *bufp, struct sockaddr *sap);`
* **MISC SUBROUTINES(1个)**
    1. `int iw_get_priv_size(int args);`

* **EVENT SUBROUTINES(2个)**
    1. `void iw_init_event_stream(struct stream_descr *stream, char *data, int len);`
    2. `int iw_extract_event_stream(struct stream_descr *stream, struct iw_event *iwe, int we_version);`
* **SCANNING SUBROUTINES(2个)**
    1. `int iw_process_scan(int skfd, char *ifname, int we_version, wireless_scan_head *context);`
    2. `int iw_scan(int skfd, char *ifname, int we_version, wireless_scan_head *context);`
* **内联函数(8个)**
    1. `static inline int iw_set_ext(int skfd, const char *ifname, int request, struct iwreq *pwrq);`
    2. `static inline int iw_get_ext(int skfd, const char *ifname, int request, struct iwreq *pwrq);`
    3. `static inline void iw_sockets_close(int skfd);`
    4. `static inline char *iw_saether_ntop(const struct sockaddr *sap, char* bufp);`
    5. `static inline int iw_saether_aton(const char *bufp, struct sockaddr *sap);`
    6. `static inline void iw_broad_ether(struct sockaddr *sap);`
    7. `static inline void iw_null_ether(struct sockaddr *sap);`
    8. `static inline int iw_ether_cmp(const struct ether_addr* eth1, const struct ether_addr* eth2);`



## 3 SOCKET SUBROUTINES(2个函数)
* `int iw_sockets_open(void);`
    - 功能：打开一个 socket
    - 返回：成功时，返回一个打开的 socket；失败时，返回 `-1`
    - 说明：按顺序尝试使用不同的协议族打开 socket，直至成功或全部失败，协议族顺序为：`AF_INET、AF_IPX、AF_AX25、AF_APPLETALK`；99% 的情况下可以使用 AF_INET 协议族打开 socket。
* `void iw_enum_devices(int skfd, iw_enum_handler fn, char *args[], int count);`
    - 功能：枚举所有无线接口设备
    - 参数：
        + skfd 为一个使用 `iw_sockets_open()` 打开的 socket
        + fn 为一个函数的指针，每找到一个无线接口，都会调用 fn，iw_enum_handler 的定义如下：
            ```
            typedef int (*iw_enum_handler)(int skfd, char *ifname, char *args[], int count);
            ```
            > 由此可见 fn() 返回的是一个 int，其调用参数有 4 个，skfd 为一个打开的 socket，ifname 为接口名称，args[] 为传递给 fn() 的自定义参数，count 为 args[] 中的参数数量；
        + argc[] 和 count 为传递给 fn() 的自定义参数，count 表示 args[] 参数的数量；
    - 可以使用 fn() 打印接口信息等；
    - 说明：这个函数的实现是首先读取文件 `/proc/net/wireless` 获取无线网络接口名称，然后调用 *fn()，并将获得的设备接口名称自动填到 *fn() 第二个参数 ifname 上。

## 4 WIRELESS SUBROUTINES(6个函数)
* `int iw_get_kernel_we_version(void);`
    - 功能：获取内核中 WE(Wireless Extension) 的版本号
    - 返回：版本号
    - 说明：这个函数的实现是从文件 `/proc/net/wireless` 中提取出 WE 的版本号。
* `int iw_print_version_info(const char *toolname);`
    - 功能：打印 Wireless Tools 所使用的 WE 的版本号，Wireless Tools 的版本号，Wireless Tools 的版本号其实就是常数 WT_VERSION，以及接口支持的 WE 版本号
    - 参数：toolsname - Wireless Tools 名称，从源码看，这个参数可以是任意字符串，或者为 NULL
    - 返回：成功返回 0，如果打开 socket 失败则返回 -1
* `int iw_get_range_info(int skfd, const char *ifname, iwrange *range);`
    - 功能：从驱动程序中获取 range 信息
    - 参数：
        + skfd 为一个使用 iw_sockets_open() 打开的 socket
        + ifname 为网络接口名称
        + iwrange 就是 `struct iw_range`，获取的信息将放在 range 指向的 `struct iw_range` 中，`struct iw_range` 定义在 `wireless.h` 中；
    - 返回：成功返回 0，range 中存放获取的信息，失败则返回 -1
    - 说明：该函数使用 `ioctl()` 的 SIOCGIWRANGE 命令获取 range 信息，`struct iw_range` 结构比较庞大，不同的驱动程序可以返回的信息也不同。
* `int iw_get_priv_info(int skfd, const char *ifname, iwprivargs **ppriv);`
    - 功能：获取有关驱动程序支持哪些私有 ioctl 的信息
    - 参数：
        + skfd 为一个使用 `iw_sockets_open()` 打开的 socket
        + ifname 为网络接口名称
        + iwprivargs 就是 `struct iw_priv_args`，该结构定义在 wireless.h 中，如下：
            ```C
            struct iw_priv_args {
                __u32       cmd;            /* Number of the ioctl to issue */
                __u16       set_args;       /* Type and number of args */
                __u16       get_args;       /* Type and number of args */
                char        name[IFNAMSIZ]; /* Name of the extension */
            };
            ```
        + ppriv 为一个数组指针，成功返回后将存放该接口支持的 ioctl
    - 返回：调用成功返回支持的 ioctl 的数量，失败返回 -1；
    - 说明：该函数会动态为 ppriv 参数申请内存，所以使用完毕一定要记得 `free()`，源码中提示，即便返回的数据长度为 0，仍然要调用 `free(*ppriv)`。
* `int iw_get_basic_config(int skfd, const char *ifname, wireless_config *info);`
    - 功能：从设备驱动程序获取必要的(基本的)无线配置
    - 参数：
        + skfd 为一个使用 iw_sockets_open() 打开的 socket
        + ifname 为网络接口名称
        + wireless_config 就是 `struct wireless_config`，定义如下：
            ```C
            typedef struct wireless_config {
                char        name[IFNAMSIZ + 1];     /* Wireless/protocol name */
                int         has_nwid;
                iwparam     nwid;                   /* Network ID */
                int         has_freq;
                double      freq;                   /* Frequency/channel */
                int         freq_flags;
                int         has_key;
                unsigned char    key[IW_ENCODING_TOKEN_MAX];    /* Encoding key used */
                int         key_size;               /* Number of bytes */
                int         key_flags;              /* Various flags */
                int         has_essid;
                int         essid_on;
                char        essid[IW_ESSID_MAX_SIZE + 1];       /* ESSID (extended network) */
                int         has_mode;
                int         mode;                   /* Operation mode */
            } wireless_config;
            ```
        + info 是一个指向 `struct wireless_config` 的指针，调用成功将存放获得的无线配置
    - 返回：成功返回 0，info 中存放着获得的无线配置，失败返回 -1
    - 说明：该函数会调用 ioctl() 的如下命令已获得无线配置信息：SIOCGIWNAME、SIOCGIWNWID、SIOCGIWFREQ、SIOCGIWENCODE、SIOCGIWESSID、SIOCGIWMODE
* `int iw_set_basic_config(int skfd, const char *ifname, wireless_config *info);`
    - 功能：在设备驱动程序中设置必要的(基本的)无线配置
    - 参数：
        + skfd 为一个使用 `iw_sockets_open() `打开的 socket
        + ifname 为网络接口名称
        + info 说明同上，为一个 `struct wireless_config` 结构指针；
    - 返回：全部成功返回 0，有一项或多项失败返回 -1；
    - 说明：该函数会调用 `ioctl()` 的如下命令已设置无线配置信息：SIOCSGIWNAME、SIOCSIWMODE、SIOCSIWFREQ、SIOCSIWENCODE、SIOCSIWNWID、SIOCSIWESSID

## 5 PROTOCOL SUBROUTINES(1个函数)
* `int iw_protocol_compare(const char *protocol1, const char *protocol2);`
    - 功能：比较协议标识符
    - 参数：
        + protocol1 为第一个协议(字符串)
        + protecol2 为第二个协议(字符串)
    - 返回：如果协议兼容则返回 1，否则返回 0
    - 说明：如果 protocol1 和 protocol2 完全一样，返回 1；如果 protocol1 和 protocol2 的起始字符串都是 "IEEE 802.11"，则 protocol1 和 protocol2 后面字符串中只要含有 "Dbg" 之中任一个字符(protocol1 和 protocol2 中可以不一样)，则返回 1，或者 protocol1 和 protocol2 中均含有 "a"(5g)，也将返回 1；其它情况返回 0。

## 6 ESSID SUBROUTINES(2个函数)
* `void iw_essid_escape(char *dest, const char *src, const int slen);`
* `int iw_essid_unescape(char *dest, const char *src);`
* 上面两个函数在 libiw.so.29 中没有，仅存在在 libiw.so.30 中。

## 7 FREQUENCY SUBROUTINES(7个函数)
* `void iw_float2freq(double in, iwfreq *out);`
    - 功能：将浮点数转换为频率，内核是不好处理浮点数的，所以需要将一个浮点数转换成一种内部格式(`struct iw_freq`)传递给内核
    - 参数：
        + in 为浮点数频率
        + iwfreq 就是 `struct iw_freq`，是一个内核可以接受的频率格式，定义如下：
            ```C
            struct iw_freq {
                __s32 m;      /* Mantissa */
                __s16 e;      /* Exponent */
                __u8  i;      /* List index (when in range struct) */
                __u8  flags;  /* Flags (fixed/auto) */
            };
            ```
        + in = m x 10<sup>e</sup>
    - 返回：转换结果存放在 out 中；

* `double iw_freq2float(const iwfreq *in);`
    - 功能：将内部格式(`struct iw_freq`)表示的频率转换为浮点数
    - 参数：
        + in 为使用 `struct iw_freq` 表示的频率
    - 返回：返回以浮点数表示的频率

* `void iw_print_freq_value(char *buffer, int buflen, double freq);`
    - 功能：将频率值转换成适当单位(GHz、MHz、kHz)的字符串
    - 参数：
        + buffer 存放转换完成的字符串的缓冲区
        + buflen 为 buffer 的长度
        + freq 频率值
    - 返回：转换好的字符串存放在 buffer 中

* `void iw_print_freq(char *buffer, int buflen, double freq, int channel, int freq_flags);`
    - 功能：将频率值转换成适当单位(GHz、MHz、kHz)的字符串，并分离出工作信道(channel)
    - 说明：当使用 ioctl() 获取工作频率时，如果返回值 `< 1000` 则该值为工作信道(channel)，否则为工作频率；当 `freq < 1000` 时，该函数转换成类似 `Channel:5` 这样的字符串，否则，转换成类似 `Frequency:5.265 GHz` 这样的字符串；
    - 参数：
        + buffer 存放转换完成的字符串的缓冲区
        + buflen 为 buffer 的长度
        + freq 频率值
        + 当 `channel >= 0` 时，转换的字符串类似 `Frequency:5.265 GHz (Channel 3)` 这样的字符串；
        + 当 `freq_flags = 1` 时，转换出的字符串类似 `Frequency=5.265 GHz`，否则为 `Frequency:5.265 GHz`
    - 返回：转换好的字符串存放在 buffer 中

* `int iw_freq_to_channel(double freq, const struct iw_range *range);`
    - 功能：得到工作频率对应的工作信道(channel)
    - 参数：
        + freq 为工作频率值
        + range 从接口驱动程序中获取的 range
    - 返回：`>=0` 时表示工作频率所在的工作信道，`-2` 表示在 range 中没有找到指定的频率 freq，`-1` 表示 `freq < 1000`
    - 说明：`struct iw_range` 定义在 wireless.h 中，其中有一个 `struct iw_freq` 的结构数组，该函数将从这个结构数组(`struct iw_freq).freq`)中查找与 freq 一致的频率，找到后返回 `(struct iw_freq).i`，否则返回 -2，当参数 `freq < 1000` 时，返回 -1

* `int iw_channel_to_freq(int channel, double *pfreq, const struct iw_range *range);`
    - 功能：将工作信道转换成工作频率
    - 参数：
        + channel 为工作信道
        + pfreq 为存放频率的指针，转换好的频率将存放在这里
        + range 从接口驱动程序中获取的 range
    - 返回：转换成功返回 channel，否则返回 `<0` 的值
    - 说明：参考前面 `struct iw_freq` 的定义，`struct iw_range` 定义在 wireless.h 中，从 range 中的 `struct iw_freq` 结构数组中的 `(struct iw_freq).i` 中查找与 channel 一致的项，并将其对应的 `(struct iw_freq).freq` 存放到 pfreq 中；

* `void iw_print_bitrate(char *buffer, int buflen, int bitrate);`
    - 功能：将传输速率(bitrate)以适当的单位(`Gb/s、Mb/s、kb/s`)表示的字符串
    - 参数：
        + buffer 为存放转换结果字符串的缓冲区
        + buflen 为 buffer 的长度
        + bitrate 为要转换的传输速率
    - 返回：转换好的字符串存放在 buffer 中
    - 说明：当速率 >10<sup>9</sup> 时，单位为 `Gb/s`，当速率 >10<sup>6</sup> 时，单位为 `Mb/s`，当速率 >1000 时，单位为 `kb/s` 

## 8 POWER SUBROUTINES(3个函数)
* `int iw_dbm2mwatt(int in);`
    - 功能：将 dBm 值转换为毫瓦值
    - 参数：
        + in 为功率的 dBm 值
    - 返回：转换后的毫瓦值

* `int iw_mwatt2dbm(int in);`
    - 功能：将毫瓦值转换为 dBm 值
    - 参数：
        + in 为以毫瓦表示的功率值
    - 返回：转换后的 dBm 值

* `void iw_print_txpower(char *buffer, int buflen, struct iw_param *txpower);`
    - 功能：txpower 表示发射功率，本函数对 txpower 进行适当的转换，转换结果以字符串输出
    - 参数：
        + buffer 为存放转换结果字符串的缓冲区
        + buflen 为 buffer 的长度
        + `struct iw_param` 时 WE 下的一个通用结构，定义在 wireless.h 中，具体如下：
            ```C
            struct iw_param {
                __s32       value;      /* The value of the parameter itself */
                __u8        fixed;      /* Hardware should not use auto select */
                __u8        disabled;   /* Disable the feature */
                __u16       flags;      /* Various specifc flags (if any) */
            };
            ```
        + `txpower->value` 为发射功率值，当 `txpower->flags` 的 bit 0 为 1 时，value 的单位为毫瓦(否则为 dBm)，当 bit 1 为 1 时表示 value 中的值是一个相对值，没有具体单位
    - 返回：转换结果存放在 buffer 中

## 9 STATISTICS SUBROUTINES(2个函数)
* `int iw_get_stats(int skfd, const char *ifname, iwstats *stats, const iwrange *range, int has_range);`
    - 功能：读取文件 `/proc/net/wireless` 获取最新的统计数据
    - 参数：
        + skfd 为一个使用 `iw_sockets_open()` 打开的 socket
        + ifname 为网络接口名称
        + iwstats 就是 `struct iw_statistics`，定义在 wireless.h 中，如下：
            ```C
            struct iw_statistics {
                __u16               status;     /* Status - device dependent for now */
                struct iw_quality   qual;       /* Quality of the link (instant/mean/max) */
                struct iw_discarded discard;    /* Packet discarded counts */
                struct iw_missed    miss;       /* Packet missed counts */
            };
            ```
        + 其中：`struct iw_quality`、`struct iw_discarded` 和 `struct iw_missed` 均定义在 wireless.h 中
        + 从文件 `/proc/net/wireless` 读出的统计信息存放在 stats 指向的 `struct iw_statistics` 中
        + has_range 为 1 表示 range 参数存在，range 参数仅用于比较 WE 的版本号，因为 WE version 11 以后才有 `/proc/net/wireless` 文件
    - 返回：调用成功返回 0，stats 中存放有统计数据，调用失败返回 -1

* `void iw_print_stats(char *buffer, int buflen, const iwqual *qual, const iwrange *range, int has_range);`
    - 功能：获取连接统计数据，包括信号质量、信号强度和信号噪音，以字符串形式输出
    - 参数：
        + buffer 为存放转换结果字符串的缓冲区
        + buflen 为 buffer 的长度
        + iwqual 就是 `struct iw_quality`，定义在 wireless.h 中，如下：
            ```C
            struct iw_quality {
                __u8    qual;       /* link quality (%retries, SNR, %missed beacons or better...) */
                __u8    level;      /* signal level (dBm) */
                __u8    noise;      /* noise level (dBm) */
                __u8    updated;    /* Flags to know if updated */
            };
            ```
        + 关于信号质量以及相关数据结构的说明，请参考文章[《使用ioctl扫描wifi信号获取信号属性的实例(二)》][article02]
        + has_range 为 1 表示 range 参数存在，该函数中将使用 `range->max_qual.qual`、`range->max_qual.level` 和 `range->max_qual.noise`
    - 返回：信号质量、信号强度和信号噪音，将转换成字符串存放在 buffer 中

## 10 ENCODING SUBROUTINES(3个函数)
* `void iw_print_key(char *buffer, int buflen, const unsigned char *key, int key_size, int key_flags);`
    - 功能：按照一个特定的格式显示经过编码后的秘钥
    - 参数：
        + buffer 为存放转换结果字符串的缓冲区
        + buflen 为 buffer 的长度
        + key 为经过编码的秘钥
        + key_size 为 key 的长度
        + key_flags 为秘钥的标志，当 bit 8 为 1 时，表示没有秘钥
    - 返回：秘钥转换成特定格式的字符串存放在 buffer中

* `int iw_in_key(const char *input, unsigned char *key);`
    - 功能：从命令行解析密钥
    - 参数：
        + input 为密码字符串
        + key 为转换为以 16 进制表示的秘钥
    - 返回：>0 时为秘钥的长度，=0 表示没有秘钥，
    - 说明：
        + input 为一个字符串，当没有任何标志时，应该是用字符串表达的 16 进制数，比如："01:2A:0C:E3:..."，转换完存放在 key 中是一个 8 位无符号整数数组，比如：`{0X01, 0X2A, 0X0C, 0XE3, ...}`
        + 当 input 起始字符为 "s:" 时，表示 input 中为一个由 ASCII 字符组成的字符串，比如："Abc123..."，转换完放在 key 中的 8 位无符号整数数组为：`{0X41, 0X62, 0X63, 0X31, 0X32, 0X33, ...}`

* `int iw_in_key_full(int skfd, const char *ifname, const char *input, unsigned char *key, __u16 *flags);`
    - 功能：从命令行解析密钥，当 input 起始字符为 "l:" 时，被认为是登录的 `username:password`，否则调用 `iw_in_key()`
    - 参数：
        + skfd 为一个使用 iw_sockets_open() 打开的 socket
        + ifname 为网络接口名称
        + input 为密码字符串
        + key 为转换为以 16 进制表示的秘钥
        + flags
    - 说明：skfd 和 ifname 用于获取 range，然后根据 WE 的版本号及 `range.encoding_login_index` 设置 flags

## 11 POWER MANAGEMENT SUBROUTINES(2个函数)
* `void iw_print_pm_value(char *buffer, int buflen, int value, int flags, int we_version);`
    - 功能：将电源管理值 value 根据 flags 的提示转换成适当的字符串存入 buffer 中
    - 参数：
        + buffer 为转换后存放字符串的缓冲区
        + buflen 为 buffer 的长度
        + value 为电源管理值(power management value)
        + flags 一些标志电源管理值属性的标志，bit0-
        + we_version 为 WE 的版本号 
    - 返回：buffer 中存放转换后的字符串
    - 说明：

* `void iw_print_pm_mode(char *buffer, int buflen, int flags);`
    - 功能：将 flags 中的电源管理方式转换成字符串存放在 buffer 中
    - 参数：
        + buffer 为转换后存放字符串的缓冲区
        + buflen 为 buffer 的长度
        + flags 为电源管理方式标志，bit8-11 为电源管理方式，bit8-Unicast only received，bit9-Multicast only received，bit10-All packets received，bit11-Force sending，bit12-Repeat multicasts

## 12 RETRY LIMIT/LIFETIME SUBROUTINES(1个函数)
* `void iw_print_retry_value(char *buffer, int buflen, int value, int flags, int we_version);`
    - 功能：将重试值 value 根据 flags 的提示转换成适当的字符串存入 buffer 中
    - 参数：
        + buffer 为转换后存放字符串的缓冲区
        + buflen 为 buffer 的长度
        + value 为重试值(retry value)
        + flags 一些属性的标志
        + we_version 为 WE 的版本号 
    - 返回：buffer 中存放转换后的字符串

## 13 TIME SUBROUTINES(1个函数)
* `void iw_print_timeval(char *buffer, int buflen, const struct timeval *time, const struct timezone *tz);`
    - 功能：将 time 中的时间戳转换成字符串存放在 buffer 中
    - 参数：
        + buffer 为转换后存放字符串的缓冲区
        + buflen 为 buffer 的长度
        + time 为时间戳
        + tz 为时间戳所在时区
    - 返回：buffer 中存放转换后的字符串

## 14 ADDRESS SUBROUTINES(9个函数)
* `int iw_check_mac_addr_type(int skfd, const char *ifname);`
    - 功能：检查网络接口是否支持正确 MAC 地址类型
    - 参数：
        + skfd 为一个使用 iw_sockets_open() 打开的 socket
        + ifname 为网络接口名称
    - 返回：0 表示支持 MAC 地址，-1 表示不支持 MAC 地址
    - 说明：通过 `ioctl()` 的 SIOCGIFHWADDR 命令获取 HW 的地址类型，应该为 ARPHRD_ETHER 或者 `ARPHRD_IEEE80211`

* `int iw_check_if_addr_type(int skfd, const char *ifname);`
    - 功能：检查网络接口是否支持正确的接口地址类型(INET)
    - 参数：
        + skfd 为一个使用 iw_sockets_open() 打开的 socket
        + ifname 为网络接口名称
    - 返回：0 表示支持接口地址类型，-1 表示不支持接口地址类型
    - 说明：通过 ioctl() 的 SIOCGIFADDR 命令获取接口地址类型，应该为 AF_INET

* `char *iw_mac_ntop(const unsigned char *mac, int maclen, char *buf, int buflen);`
    - 功能：以可读格式显示任意长度的 MAC 地址
    - 参数：
        + mac 为 16 进制表示的 MAC 地址
        + maclen 为 mac 的长度
        + buf 为 MAC 地址转换为字符串后的存储区
        + buflen 为 buf 的有效长度
    - 返回：成功返回 buf 指针，失败返回 NULL

* `void iw_ether_ntop(const struct ether_addr *eth, char *buf);`
    - 功能：以可读格式显示以太网地址(实际就是 MAC 地址)
    - 参数：
        + `struct ether_addr` 定义在 ethernet.h 中，如下：
            ```C
            struct ether_addr {
                uint8_t ether_addr_octet[ETH_ALEN];
            } __attribute__ ((__packed__));
            ```
        + 其实就是一个 8 位无符号整数的数组，数组有 6 个(因为 ETH_ALEN 为 6)元素
        + buf 转换后以字符串表示的以太网地址存放在这里
    - 返回：buf 中存放着已经转换好的字符串
    - 说明：`iw_mac_ntop()` 把一个任意长度的 MAC 地址转换成字符串，其输入参数为一个 `unsigned char` 数组，`iw_ether_ntop()` 将一个长度为 6 个字符的 `unsigned char` 数组转换为字符串，输入参数为 `struct ether_addr`，两个函数本质上非常相似。

* `char *iw_sawap_ntop(const struct sockaddr *sap, char *buf);`
    - 功能：以可读格式显示无线接入点 socket 地址
    - 参数：
        + `struct sockaddr` 定义在文件 socket.h 中，如下：
            ```C
            struct sockaddr {
                sa_family_t sa_family;	/* Common data: address family and length.  */
                char sa_data[14];		/* Address data.  */
            };
            ```
        + buf 转换后以字符串表示的以太网地址存放在这里
    - 返回：返回 buf 指针，buf 中存储着转换结果
    - 说明：实际上就是使用 `iw_ether_ntop()` 将 socket.sa_data 中的 MAC 地址转换成字符串，这个函数仍然是把 MAC 地址转换为字符串，不过输入参数是 `struct sockaddr` 而已。

> `iw_mac_ntop()`、`iw_ether_ntop()`、`iw_sawap_ntop()` 都是把 MAC 地址转换成易读的字符串形式，但各自的输入参数不同，`iw_mac_ntop()` 的输入参数是一个任意长度的二进制数组；`iw_ether_ntop()` 的输入参数是 `struct ether_addr`；iw_sawap_ntop() 的输入参数是 `struct sockaddr`

* `int iw_mac_aton(const char *orig, unsigned char *mac, int macmax);`
    - 功能：将任意长度的字符串表达的 MAC 地址，转换成二进制
    - 参数：
        + orig 为以字符串格式表达的一个 MAC 地址
        + mac 为 MAC 地址转换为二进制后存放在这里
        + macmax 为 mac 的最大长度
    - 返回：成功则返回转换后 MAC 地址长度，失败则返回 0

* `int iw_ether_aton(const char *orig, struct ether_addr *eth);`
    - 功能：将一个以字符串形式表达的以太网地址转换成二进制
    - 参数：
        + orig 为以字符串格式表达的一个以太网地址
        + `struct ether_addr` 在前面有介绍，转换好的二进制以太网地址存放在 eth 中
    - 返回：成功则返回以太网地址的长度，失败返回 0
    - 说明：`iw_mac_aton()` 和 `iw_ether_aton()` 都是把一个字符串表达的 MAC 地址转换成二进制形式，`iw_mac_aton()` 的输出参数是一个 `unsigned char` 数组，`iw_ether_aton()` 的输出参数是 `struct ether_addr`，`iw_mac_aton()` 是任意长度，`iw_ether_aton()` 是 6 个字符长度。

> iw_mac_aton()、iw_ether_aton()、

* `int iw_in_inet(char *bufp, struct sockaddr *sap);`
    - 功能：将一个以字符串表达的互联网地址(域名)转换成二进制(DNS 名称、IP 地址等)
    - 参数：
        + bufp 为一个以字符串表达的互联网地址，可以是域名、IP 地址
        + sap 存放转换结果的结构，bufp 中如果不是官方域名，将改为官方域名
    - 返回：成功返回 0，失败返回 -1
    - 说明：该函数会使用 gethostbyname() 将域名(主机名)进行解析

* `int iw_in_addr(int skfd, const char *ifname, char *bufp, struct sockaddr *sap);`
    - 功能：将一个以字符串表达的地址转换成二进制
    - 参数：
        + skfd 为一个使用 iw_sockets_open() 打开的 socket
        + ifname 为网络接口名称
        + bufp 为一个用字符串格式表达的地址
        + sap 转换为二进制后存放在这个结构中
    - 返回：成功返回 0，失败返回 -1
    - 说明：bufp 中既可以是 MAC 地址，也可以是 IP 地址，该函数实际上就是用 MAC 地址生成一个 `struct sockaddr`

## 15 MISC SUBROUTINES(1个函数)
* `int iw_get_priv_size(int args);`
    - 功能：返回私有参数以字节为单位的最大长度
    - 参数：args 私有参数的标志，bit0-10 表示参数的数量，bit12-14 表示参数的类型
    - 返回：返回私有参数以字节为单位的最大长度

## 16 EVENT SUBROUTINES(2个函数)
* `void iw_init_event_stream(struct stream_descr *stream, char *data, int len);`
    - 功能：初始化结构体stream_descr，以便我们可以从事件流中提取单个事件
    - 参数：
        + `struct stream_descr` 定义在 iwlib.h 中，如下：
            ```C
            struct stream_descr {
                char *end;          /* End of the stream */
                char *current;      /* Current event in stream of events */
                char *value;        /* Current value in event */
            } stream_descr;
            ```
        + stream 为需要初始化的结构指针
        + data 为事件流数据的起始指针
        + len 为事件流数据的长度
    - 返回：stream->current=data，stream->end=(data+len)

* `int iw_extract_event_stream(struct stream_descr *stream, struct iw_event *iwe, int we_version);`
    - 功能：从事件流中提取下一个事件
    - 参数：
        + stream 为经过 `iw_init_event_stream()` 初始化的结构
        + `struct iw_event` 定义在 `wireless.h` 中，如下：
            ```C
            struct iw_event {
                __u16   len;            /* Real lenght of this stuff */
                __u16   cmd;            /* Wireless IOCTL */
                union iwreq_data u;     /* IOCTL fixed payload */
            };
            ```
        + iwe 将用于存储当前要提取的事件
        + we_version 为 WE 的版本号
    - 返回：成功返回 1，此时 iwe 中为当前事件，失败返回 -1，返回 2 表示跳过了当前事件

## 7 SCANNING SUBROUTINES(2个函数)
* `int iw_process_scan(int skfd, char *ifname, int we_version, wireless_scan_head *context);`
    - 功能：启动无线信号扫描程序并处理结果
    - 参数：
        + skfd 为一个使用 iw_sockets_open() 打开的 socket
        + ifname 为网络接口名称
        + we_version 为 WE 的版本号
        + wireless_scan_head 就是 `struct wireless_scan_head`，定义在 `iwlib.h` 中，如下：
            ```C
            struct wireless_scan_head {
                wireless_scan *result;      /* Result of the scan */
                int            retry;       /* Retry level */
            } wireless_scan_head;
            ```
        + context 中将存储获取的扫描结果的首指针，以及获取扫描结果重试的次数
        + `wireless_scan` 就是 `struct wireless_scan`，定义在 `iwlib.h` 中，如下：
            ```C
            struct wireless_scan
            {
                /* Linked list */
                struct wireless_scan *next;

                /* Cell identifiaction */
                int         has_ap_addr;
                sockaddr    ap_addr;        /* Access point address */

                /* Other information */
                struct  wireless_config	b;  /* Basic information */
                iwstats stats;              /* Signal strength */
                int     has_stats;
                iwparam maxbitrate;         /* Max bit rate in bps */
                int     has_maxbitrate;
            }
            ```
        + 扫描结果存放在 `struct wireless_scan` 中，并且以链表形式存储多个多个无线信号的扫描结果；
        + `struct wireless_scan` 中的 `struct wireless_config` 在前面有介绍；
    - 返回：成功则返回 0

* `int iw_scan(int skfd, char *ifname, int we_version, wireless_scan_head *context);`
    - 功能：对指定接口执行无线信号扫描
    - 参数：
        + skfd 为一个使用 `iw_sockets_open()` 打开的 socket
        + ifname 为网络接口名称
        + we_version 为 WE 的版本号
        + context 中将存储获取的扫描结果的首指针，以及获取扫描结果重试的次数
    - 返回：成功则返回 0，失败返回 -1

## 8 内联函数(8个函数)
* 这 8 个内联函数(inline)定义在 iwlib.h 中；
* `static inline int iw_set_ext(int skfd, const char *ifname, int request, struct iwreq *pwrq);`
    - 功能：将无线网卡的参数通过调用 ioctl() 设置到驱动程序中
* `static inline int iw_get_ext(int skfd, const char *ifname, int request, struct iwreq *pwrq);`
    - 功能：通过调用 ioctl() 从无线网卡驱动程序中获取无线参数
    - 参数：
        + skfd 为一个使用 `iw_sockets_open()` 打开的 socket
        + ifname 为网络接口名称
        + request 为调用 ioctl() 时的命令代码，定义在 wireless.h 中
        + pwrq 为一个 `struct iwreq` 的结构指针，不同的 request，其填写方法也不一样
    - 返回：返回 ioctl() 调用的返回值，0 表示调用成功，-1 表示出现错误，errno 中为错误代码

> 说明：上面这两个内联函数其实是一样的，都是调用指定的 `ioctl()`，WE 支持的 ioctl 调用方式为：`int ioctl(int fd, unsigned long request, struct iwreq *pwrq)`，其 request 通常有两种，一种是 SIOCS*，另一种是 SIOCG*，均定义在 wireless.h 中，按照惯例，当 request 为 SIOCS* 时，使用 `iw_set_ext()`，当 request 为 SIOCG* 时，使用 iw_get_ext()。

* `static inline void iw_sockets_close(int skfd);`
    - 功能：关闭一个打开的 socket
    - 参数：
        + skfd 为一个使用 `iw_sockets_open()` 打开的 socket
    - 返回：无
    - 说明：实际调用 `close(skfd)`

* `static inline char *iw_saether_ntop(const struct sockaddr *sap, char* bufp);`
    - 功能：将 struct sockaddr 中的 MAC 地址转换成可读格式的字符串
    - 参数：
        + sap 是一个 struct sockaddr 的结构指针，这个结构在 socket 编程中常会用到
        + bufp 一个字符缓冲区指针，转换好的字符串存放在这里
    - 返回：转换完成的字符串指针，也就是 bufp
    - 说明：bufp 的长度务必大于等于 18个字符，否则会内存溢出，该函数实际是调用了 `iw_ether_ntop()`

* `static inline int iw_saether_aton(const char *bufp, struct sockaddr *sap);`
    - 功能：将一个以字符串表示的 MAC 地址转换成二进制格式
    - 参数：
        + bufp 指向 MAC 地址字符串的指针
        + sap 指向结构 struct sockaddr 的指针，转换结果将放到该结构的 sa_data 字段中
    - 返回：成功则返回 MAC 地址的长度，失败则返回 0
    - 说明：该函数实际调用 `iw_ether_aton()` 函数

* `static inline void iw_broad_ether(struct sockaddr *sap);`
    - 功能：创建一个以太网广播地址
    - 参数：
        + sap 指向结构 struct sockaddr 的指针
    - 返回：无，建立的广播地址放在 sap 的 sa_data 字段中
    - 说明：广播地址就是：FF:FF:FF:FF:FF:FF，该函数实际就是在 sap 的 sa_data 中填上 6 个 0xff

* `static inline void iw_null_ether(struct sockaddr *sap);`
    - 功能：建立一个以太网空地址
    - 参数：
        + sap 指向结构 struct sockaddr 的指针
    - 返回：无，建立的广播地址放在 sap 的 sa_data 字段中
    - 说明：空地址就是：00:00:00:00:00:00，该函数实际就是在 sap 的 sa_data 中填上 6 个 0

* `static inline int iw_ether_cmp(const struct ether_addr* eth1, const struct ether_addr* eth2);`
    - 功能：比较两个以太网地址
    - 参数：
        + eth1 指向结构 struct ether_addr 的指针
        + eth2 指向结构 struct ether_addr 的指针
    - 返回：`eth1 < eth2` 则小于 0，`eth1 > eth2` 则大于 0，否则等于 0
    - 说明：该函数使用 memcmp() 对两个以太网地址的二进制数据进行比较，其意义不甚清楚，其结果的意义似乎也不明确








## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://whowin.gitee.io/sourcecodes/180025/wifi-new-scanner.c

[article01]:https://blog.csdn.net/whowin/article/details/131504380
[article02]:https://blog.csdn.net/whowin/article/details/137711398

[article03]:https://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html
[article04]:https://github.com/HewlettPackard/wireless-tools
[article05]:https://people.iith.ac.in/tbr/teaching/docs/802.11-2007.pdf


