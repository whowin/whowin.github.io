---
title: "如何用C语言操作arp cache"
date: 2023-01-12T16:43:29+08:00
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
  - arp
  - ioctl
draft: false
#references: 
# - [linux系统中使用socket直接发送ARP数据](http://t.zoukankan.com/ceblog-p-10626828.html)
# - [Packet socket 和 sockaddr_ll](https://www.cnblogs.com/zhangshenghui/p/6097492.html)
# - [arp request and reply using c socket programming](https://stackoverflow.com/questions/16710040/arp-request-and-reply-using-c-socket-programming)
# - [Address Resolution Protocol](https://en.wikipedia.org/wiki/Address_Resolution_Protocol)
# - [Linux socket发送ARP请求包 C语言](https://blog.csdn.net/u014302425/article/details/113822870)
# - [Linux网络接口操作之ioctl-2_ARP条目](https://segmentfault.com/a/1190000005365739)
# - man 7 arp
# - [Linux arp cache timeout values](https://serverfault.com/questions/551688/linux-arp-cache-timeout-values)
# - [When do STALE arp entries become FAILED when never used?](https://serverfault.com/questions/765380/when-do-stale-arp-entries-become-failed-when-never-used)
postid: 180014
---

arp cache中存放着局域网内IP地址和MAC地址的对应关系，对socket通信是至关重要的，arp cache由Linux内核进行维护，本文介绍如何用ioctl获取arp cache记录，添加新记录到arp cache中，删除arp cache中记录，每一种操作均给出了完整的源程序，本文程序在ubuntu 20.4中编译运行成功，gcc版本9.4.0，阅读本文需要有基本的socket编程和ioctl的知识，并对ARP协议有所了解，本文对网络编程的初学者难度不大。
<!--more-->

## 1. ARP cache
* ARP cache中存放着已知的IP地址与MAC地址的对应关系，局域网内计算机之间通信时，数据链路层需要将目的计算机的MAC地址填写在以太网报头中，这时就需要查询ARP cache以获得目的MAC地址；
* ARP cache是由内核进行维护的，用户程序在用户空间无法对其进行直接操作；
* 如果用户程序希望操作ARP cache就需要与内核进行通信，Linux下的ioctl提供了三种有关操作ARP cache的方法：
  1. SIOCDARP：从ARP cache中删除一条记录
  2. SIOCGARP：从ARP cache中获取一条记录
  3. SIOCSARP：向ARP cache中添加一条记录
* 如果仅仅是要查询ARP cache中的记录，也可以读取文件 ```/proc/net/arp```，该文件是内核中ARP cache的一个映射；
* ARP cache中的记录分为动态记录和静态记录，动态记录是有时效的，时效到了记录会失效；静态记录则永久有效；
* 通常，通过在局域网上广播arp请求获得的arp回应，在ARP cache中一定是动态记录，而使用ioctl设置的记录，通过设置arp_flags可以成为静态记录，永久有效，当然也可以是动态记录，在后面实例中会说明这一点；
* 当我们 ```ping [IP address]``` 的时候，这个[IP address]的arp记录就会被加到ARP cache中；
* 既然ARP cache中的记录多为动态记录，而动态记录有一个超时时间，过了这个时间，记录就会失效，那么这个超时时间是多少呢？
* 内核中有专门的arp垃圾清除程序，这个垃圾清除程序可以清除已经失效的arp记录，该程序的启动遵循以下原则：
  - 内核中一些与arp垃圾清除有关的变量有：gc_thresh1、gc_thresh2、gc_thresh3和gc_interval
  - 可以用下面命令查看这些变量的值
    ```
    cat /proc/sys/net/ipv4/neigh/default/gc_thresh1
    cat /proc/sys/net/ipv4/neigh/default/gc_thresh2
    cat /proc/sys/net/ipv4/neigh/default/gc_thresh3
    cat /proc/sys/net/ipv4/neigh/default/gc_interval
    ```
  - 当arp cache中的记录数量小于gc_thresh1时，不启动arp垃圾清除程序，也就是说不会删除arp cache中的任何记录，包括过期记录；
  - 当arp cache中记录数在gc_thresh1和gc_thresh2之间时，按照一定频率启动arp垃圾清除程序，这个频率由gc_interval设置；
  - 当arp cache中记录数在gc_thresh2和gc_thresh3之间时，每5秒启动一次arp垃圾清除程序；
  - 当arp cache中记录数在gc_thresh3时，当加入新记录时，强制启动arp垃圾清除程序。
* 所以，当你用程序插入一条动态记录时，你会发现等了很长时间这条记录都不会被删除，这可能是你的arp cache中记录数太少了。

## 2. ioctl的调用方法
* 对于arp cache的操作，其调用方法为：
  ```C
  #include <sys/sockio.h>
  #include <sys/socket.h>

  #include <net/if.h>
  #include <net/if_arp.h>

  struct arpreq arpreq;

  ioctl(s, SIOCSARP, (struct arpreq *)&arpreq);
  ioctl(s, SIOCGARP, (struct arpreq *)&arpreq);
  ioctl(s, SIOCDARP, (struct arpreq *)&arpreq);
  ```
* ioctl的第一个参数s是socket，第二个参数是要进行的操作，第三个参数是一个arp请求的结构；
* 第二个参数使用的三个宏定义，定义在头文件 bits/ioctls.h 中；
* ARP ioctl请求结构，定义在net/if_arp.h或者linux/if_arp.h中；
  ```C
  /* ARP ioctl request.  */
  struct arpreq {
      struct sockaddr arp_pa;       /* Protocol address.  */
      struct sockaddr arp_ha;       /* Hardware address.  */
      int arp_flags;                /* Flags.  */
      struct sockaddr arp_netmask;  /* Netmask (only for proxy arps).  */
      char arp_dev[16];
  };
  ```
  - arp_pa即Prototol Address，也就是IP地址；
  - arp_ha即Hardware Address，也就是MAC地址；
  - arp_netmask仅用于代理ARP，本文中用不上；
  - arp_dev用于指定本地网络接口的名称，需要填写；
* **获取arp cache中的记录(SIOCGARP)**
  - 需要填写(struct arpreq)中的arp_pa和arp_dev两个字段；
  - arp_pa实际对应(struct sockaddr_in)，其中sin_family字段必须为AF_INET，sin_addr字段填请求MAC地址对应的IP地址，sin_port不用填；
    ```C
    struct sockaddr {  
        sa_family_t sin_family;       /* Protocol family.  */
    　　 char sa_data[14];
    };

    struct sockaddr_in {
        sa_family_t sin_family;       /* Protocol family.  */
        in_port_t sin_port;           /* Port number.  */
        struct in_addr sin_addr;      /* Internet address.  */

        /* Pad to size of `struct sockaddr'.  */
        unsigned char sin_zero[8];
    };
    ```
  - 调用ioctl成功后，arp_pa中包含的IP地址对应的MAC地址保存在arp_ha.sa_data中；
  - 若指定的网络接口不存在，或存在但与指定的目标主机IP地址不对应，则ioctl以"(ENXIO)No such device or address"错误调用失败；
  - 无法通过ioctl操作获取**整个ARP cache**，Linux下的命令‘arp -a’是通过读取文件/proc/net/arp来实现的；
* **向arp cache中添加一条记录(SIOCSARP)**
  - 需要填写arp_pa、arp_dev、arp_ha.sa_data和arp_flags四个字段；
  - arp_pa实际对应(struct sockaddr_in)，其中sin_family字段必须为AF_INET，sin_addr字段填请求MAC地址对应的IP地址，sin_port不用填；
  - arp_ha中，sa_family填AF_UNSPEC，sa_data中填入在arp_pa中的IP地址对应的MAC地址；
  - arp_flags填'ATF_PERM | ATF_COM'，表示这条记录为完整的、永久性记录，因为arp cache中的动态记录都是有时效的，过一段时间就会失效，只有设置为永久记录才使其成为一条静态记录，不会失效;
  - arp_flags中每一位代表一个标志，每一位的定义在linux/if_arp或者net/if_arp中：
    ```C
    /* ARP Flag values.  */
    #define ATF_COM         0x02      /* Completed entry (ha valid).  */
    #define ATF_PERM        0x04      /* Permanent entry.  */
    #define ATF_PUBL        0x08      /* Publish entry.  */
    #define ATF_USETRAILERS 0x10      /* Has requested trailers.  */
    #define ATF_NETMASK     0x20      /* Want to use a netmask (only
                                         for proxy entries).  */
    #define ATF_DONTPUB     0x40      /* Don't answer this addresses.  */
    #define ATF_MAGIC       0x80      /* Automatically added entry.  */
    ```
* **删除arp cache中的一条记录(SIOCDARP)**
  - 需要填写arp_pa和arp_dev两个字段；
  - arp_pa实际对应(struct sockaddr_in)，其中sin_family字段必须为AF_INET，sin_addr字段填请求MAC地址对应的IP地址，sin_port不用填；
  - 调用ioctl成功后，在arp_pa中指定IP地址对应的ARP记录被删除；
  - 若指定的网络接口不存在，或存在但与指定的目标主机IP地址不对应，则ioctl以"(ENXIO)No such device or address"错误调用失败；

## 3. 获取arp cache中记录的实例
* ioctl获取arp cache的记录，只能一条一条的获取，不能一下获取一个完整的arp cache，要获取一个完整的arp cache，似乎唯一的办法就是读取文件：/proc/net/arp，至少我还没有找到其它方法；
* 在这个实例中，我们仅仅获得arp cache中的一条记录，文件名为：[arp-get.c][src01](**点击文件名下载源程序**)
* 编译
  ```bash
  gcc -Wall arp-get.c -o arp-get
  ```
* 运行
  ```bash
  cat /proc/net/arp
  ./arp-get enp0s3 192.168.2.112
  ```
* 运行截屏

  ![screenshot of arp_get][img01]

**************
## 4. 从/proc/net/arp文件中获取完整arp cache
* 如果想要看arp cache的完整记录，只能去读文件/proc/net/arp了，本例用读取文件的方式显示arp cache的全部记录；
* 如果你对宏str(s)和xstr(s)的用法不熟悉，可以参考[《Stringizing Operator (#) in C》][article1]
* 文件名：[arp-get-all.c][src02](**点击文件名下载源程序**)
* 文件/proc/net/arp中有6个字段，本程序只读出了其中的3个：IP地址、设备名称和MAC地址，如果需要，可以修改程序读出全部字段；
* 编译
  ```bash
  gcc -Wall arp-get-all.c -o arp-get-all
  ```
* 运行
  ```bash
  ./arp-get-all
  ```
* 运行截图

  ![screenshot of arp_get_all][img02]

************
## 5. 在arp cache中添加一条静态记录的实例
* 在这个程序中要重点解释的是关于arp_flags的设置
  ```C
  arp_req.arp_flags = ATF_PERM | ATF_COM;
  ```
* 关于arp_flags相关宏的定义在前面已经有介绍，其中ATF_COM表示记录是完整的，ATF_PERM表示记录是永久性的；
* arp_flags是必须要设置ATF_COM的，否则这条记录将被认为是不完整的，用arp -ae显示arp记录时，该记录将被标记为(incomplete)；
* 当需要添加一条动态记录时，arp_flags = ATF_COM即可，当需要添加一条静态记录时，要设置arp_flags = ATF_COM | ATF_PERM
* 文件名：[arp-set.c][src03](**点击文件名下载源程序**)
* 编译
  ```bash
  gcc -Wall arp-set.c -o arp-set
  ```
* 因为要修改内核中的arp cache，所以运行这个程序，是需要root权限的
  ```bash
  arp -ae
  sudo ./arp-set enp0s3 192.168.2.118 85:3b:4c:36:41:f5
  arp -ae
  ```
* 运行截图

  ![screenshot of arp_set][img03]

*****************
* 可以看出运行完arp_set后，arp cache中多了一条我们设置的记录，其标志为CM，前面介绍过有关arp_flags的宏定义，CM其实就是(ATF_COM | ATF_PERM)，含义为：完整的永久记录；我们加入的这条记录是静态记录，永久有效，其它的记录是动态记录，是有时效的，所以其它记录的标志为C，也就是常数ATF_COM，表示记录完整。

## 6. 在arp cache中删除一条记录的实例
* arp cache实际上是以[IP address]为键值的，所以，删除时只需要知道设备名称和IP地址即可；
* 文件名：[arp-del.c][src04](**点击文件名下载源程序**)
* 编译
  ```bash
  gcc -Wall arp-del.c -o arp-del
  ```
* 这个程序也是要root权限才能运行的，我们将上一节添加的记录删除掉
  ```bash
  arp -ae
  sudo ./arp-del enp0s3 192.168.2.118
  arp -ae
  ```
* 运行截屏

  ![screenshot of arp_del][img04]

## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180014/arp-get.c
[src02]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180014/arp-get-all.c
[src03]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180014/arp-set.c
[src04]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180014/arp-del.c

[img01]:/images/180014/screenshot_arp_get.png
[img02]:/images/180014/screenshot_of_arp_get_all.png
[img03]:/images/180014/screenshot_of_arp_set.png
[img04]:/images/180014/screenshot_of_arp_del.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/4d71776c4b22204b03c5e2f1143f2973.png
[img02]:https://img-blog.csdnimg.cn/img_convert/2f32607afb6680c3c746371510c02d5e.png
[img03]:https://img-blog.csdnimg.cn/img_convert/c8db77ee67af22366376e284a456aaac.png
[img04]:https://img-blog.csdnimg.cn/img_convert/72e9180977a2713dc7da39f3c17949a0.png
-->

[article1]:https://www.includehelp.com/c/stringize-operator-in-c.aspx
