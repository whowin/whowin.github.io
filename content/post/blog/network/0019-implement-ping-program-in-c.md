---
title: "使用raw socket实现一个简单的ping程序"
date: 2023-03-12T16:43:29+08:00
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
  - ICMP
  - 网络编程
  - "raw socket"
  - ping
  - "internet checksum"
draft: true
#references: 
# - [Ping in C](https://www.geeksforgeeks.org/ping-in-c/)
# - [ping sourcecode in git](https://github.com/amitsaha/ping/blob/master/ping.c)
#     - 这个程序使用sock_dgram发送了icmp报文，但是接收是使用的recvmsg，显得很臃肿，我想是为了获得IP头里的TTL
# - [Internet Control Message Protocol](https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol)
#     - 有中文版
# - [rfc 792](https://www.rfc-editor.org/rfc/rfc792)
# - [什么是互联网控制消息协议（ICMP）？](https://www.cloudflare.com/zh-cn/learning/ddos/glossary/internet-control-message-protocol-icmp/)
# - [What does mdev mean in ping(8)?](https://serverfault.com/questions/333116/what-does-mdev-mean-in-ping8)
#     - 关于ping统计结果的计算，特别是mdev的计算
# - [Socket Timeouts](https://www.masterraghu.com/subjects/np/introduction/unix_network_programming_v1.3/ch14lev1sec2.html)
#     - 按这篇文章应该可以开一个题
postid: 180019
---

相信每一位程序员都知道并且使用过ping，使用ping去测试某个IP或者某个主机是否可用可能是一件很平常的事，尽管ping是如此普通，但是编写一个实现ping功能的程序却并不是那么简单，因为ping使用的ICMP协议并不是一个应用层协议，所以使用普通应用层的网络编程的方法是无法实现的，本文简要介绍ICMP协议，并给出一个使用raw socket实现ping的主要功能的一个实例，本文将提供完整的源程序，本文的程序在 Ubuntu 20.04 下测试通过，gcc 版本号 9.4.0

## 1. ICMP协议
* ping使用ICMP(Internet Control Message Protocol)向目标主机发送数据包，ICMP是互联网协议族中的一个支持协议，用于在IP协议中发送控制信息，报告网络通信中可能发生的问题；
* 因为IP协议没有任何内在的机制来发送错误和控制消息，也就是说如果网络通信出现问题，IP协议本身是无法得知原因的，这时就要靠ICMP了；
* ICMP的作用：
  1. 报告错误
    > 当两个设备通过互联网连接时，ICMP会生成错误信息并与发送设备共享，以防发送数据未到达其预期目的地；例如：一个数据包的长度大于某个路由器所能接收的最大长度，路由器将丢弃该数据包并将 ICMP 消息发送回数据的发送设备。

  2. 执行网络诊断
    > 常用的终端实用程序 ```traceroute``` 和 ```ping``` 都是使用**ICMP**协议运行；```traceroute``` 实用程序用于显示两个互联网设备之间的路由路径；路由路径是请求数据到达目的地之前必须经过的路由器的实际物理路径；一个路由器与另一个路由器之间的路径称为"跃点"，路由跟踪还会报告经过每个跃点所需的时间；这对于确定网络延迟来源是非常有用的。

* ICMP协议常被归为网络层协议，但是ICMP报文是被包装在一个IP报文中的，把ICMP、IP同归为网络层似乎也不是那么合适；
* 但是ICMP与常用的传输层协议TCP和UDP也有明显的不同，因为它通常不会用于在两个或多个计算机系统之间传输数据，所以把ICMP称为传输层协议似乎也不大合适，所以，也有人说ICMP协议是IP协议的附属协议或者说ICMP是介于网络层和传输层之间的中间层协议；
* ICMP是那一层的协议其实并不重要，重要的是我们可以理解和使用它；
* ICMP报文的结构：

  ![Structure of ICMP packet][img01]

* 关于IP报头和以太网报头，可以参考文章：[《Linux下如何在数据链路层接收原始数据包》](article01)
* ICMP报头，定义在头文件<netinet/ip_icmp.h>中
  ```C
  struct icmphdr
  {
    uint8_t type;        /* message type */
    uint8_t code;        /* type sub-code */
    uint16_t checksum;
    union
    {
      struct
      {
        uint16_t id;
        uint16_t sequence;
      } echo;               /* echo datagram */
      uint32_t gateway;     /* gateway address */
      struct
      {
        uint16_t __glibc_reserved;
        uint16_t mtu;
      } frag;               /* path mtu discovery */
    } un;
  };
  ```
* ICMP报文有很多中不同的类型，对所有类型的ICMP报文，ICMP报头的前4个字节(前三个字段)都是相同的，但后四个字节的含义是不同的，关于ICMP报头的解释将放在下一节中介绍；
* ICMP报文的类型，头文件<netinet/ip_icmp.h>中定义了所有ICMP报文的类型：
  ```C
  #define ICMP_ECHOREPLY      0     /* Echo Reply             */
  #define ICMP_DEST_UNREACH   3     /* Destination Unreachable*/
  #define ICMP_SOURCE_QUENCH  4     /* Source Quench          */
  #define ICMP_REDIRECT       5     /* Redirect (change route)*/
  #define ICMP_ECHO           8     /* Echo Request           */
  #define ICMP_TIME_EXCEEDED  11    /* Time Exceeded          */
  #define ICMP_PARAMETERPROB  12    /* Parameter Problem      */
  #define ICMP_TIMESTAMP      13    /* Timestamp Request      */
  #define ICMP_TIMESTAMPREPLY 14    /* Timestamp Reply        */
  #define ICMP_INFO_REQUEST   15    /* Information Request    */
  #define ICMP_INFO_REPLY     16    /* Information Reply      */
  #define ICMP_ADDRESS        17    /* Address Mask Request   */
  #define ICMP_ADDRESSREPLY   18    /* Address Mask Reply     */
  #define NR_ICMP_TYPES       18
  ```
* 本文我们要实现的 ping 程序，只需要关心 ICMP_ECHOREPLY 和 ICMP_ECHO 两种类型的报文；

## 2. ping程序的工作机制
* ```ping``` 程序是一个基本的Internet工具，可以验证一个 IP 或者主机名称所指向的主机是否存在并可以响应请求；
* ```ping``` 程序通过打开一个 raw socket 来发送 ICMP_ECHO 请求数据包，然后等待目标主机的响应，如果数据包顺利到达目标主机，而且主机也为可用状态，主机的内核将返回一个 ICMP_ECHOREPLY 数据包，如果出现错误，主机或者其他相关的网络设备将返回一个 ICMP 的错误信息数据包；
* ```ping``` 程序的工作原理很像声纳回声定位，发送一个包含ICMP ECHO REQUEST 的数据包到指定的计算机，然后该计算机返回一个 ICMP ECHO REPLY包
* 数据包有一个TTL(Time To Live)值，该值决定了路由器的最大跳数(就是经过的路由器数量，一般定为64)；
* 如果数据包没有到达，那么发送方将收到一个错误信息，错误信息有以下类型：
  1. 传输过程中 TTL 过期
  2. 目标主机不可达
  3. 请求超时(即没有收到回复)
  4. 不明主机
* 由此可见，一个ping程序可能会遇到以下类型的ICMP数据包(参考ICMP报头结构)：
  1. ICMP 回显请求 - type=ICMP_ECHO(8)，code=0
  2. ICMP 回显应答 - type=ICMP_ECHOREPLY(0)，code=0
  3. TTL 过期 - type=ICMP_TIME_EXCEEDED(11)，code=0
  4. 目标主机不可达 - type=ICMP_DEST_UNREACH(3)，code=0-5
  5. 没有收到回复 - 接收超时
* 当我们发出一个 ICMP ECHO 请求后，我们收到的回应并不一定来自目标主机，比如我们收到了一个type=3的ICMP数据包，也就是目标不可达的错误，当code=2时表示主机不可达，此时主机是不可能发出消息的，这个ICMP数据包一般会由目标主机的gateway发出；
* ICMP协议是一种面向无连接的协议，发送数据包之前无需像TCP一样先建立连接；
* ICMP ECHO 请求数据包的响应是由目标主机的内核发出的，一般与应用程序无直接关系。

## 3. ping的简单实现
* 本文并不寻求实现 Linux 下目前存在的 ping 的所有功能，那样过于复杂，本文仅实现一些主要的功能；
* 功能列表
  1. 接收 IP 或者主机名作为输入，并可以自动识别

  2. 对主机名执行 DNS lookup，将其转换为 IP
  
  3. ctrl+c可以随时终止程序，并在终止时显示数据报告

  4. 



> 一个简单的ping程序所遵循的步骤是
1. 以主机名作为输入
2. 进行DNS查找

> DNS查找可以使用gethostbyname()完成。gethostbyname()函数转换一个普通的人类可读的网站，并返回一个类型为hostent的结构，其中包含二进制点表示法形式的IP地址和地址类型。

> 一些ping程序，比如ubuntu提供的程序，支持反向DNS查找。反向DNS查找使用getnameinfo()执行，它将点表示法IP地址转换为主机名。例如，google.com的ping经常会给出一个奇怪的地址:bom07s18-in-f14.1e100.net这是反向DNS查找的结果。

3. 使用SOCK Raw打开一个Raw套接字，协议为IPPROTO ICMP。注意:原始套接字需要超级用户权限，所以你必须使用sudo运行这段代码
4. 当按下CTRL + C时，ping会给出一个报告。这个中断被一个中断处理程序捕获，它只是将我们的ping循环条件设置为false。
5. 这里是主要的ping发送循环。我们必须
  - 设置socket中的ttl选项，设置TTL值是为了限制数据包的跳数
  - 设置recv函数的超时时间如果没有设置timeout, recv将永远等待，循环将终止
  - 填充icmp报文
    + 报文头类型”选择“ICMP ECHO”
    + 将id设置为进程的pid
    + 随机填写信息部分
    + 计算校验和并在校验和字段中填写。
  - 发送数据包
  - 等待它被接收。这里的主要问题是，收到的数据包并不意味着目的地正在工作。Echo reply表示目的地正常。从目标OS内核发送回显应答。这是所有类型和代码的列表。这里的一个问题是，如果一切正常，程序将显示类型69和代码0，而不是代表echo reply的0。





-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[article01]:https://whowin.gitee.io/post/blog/network/0002-link-layer-programming/



[img01]:/images/180019/structure-of-icmp-packet.png


