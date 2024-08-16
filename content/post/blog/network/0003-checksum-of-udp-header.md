---
title: "如何计算UDP头的checksum"
date: 2022-12-12T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "C Language"
  - "Network"
tags:
  - C语言
  - UDP
  - "internet checksum"
  - "UDP checksum"
draft: false
#references: 
# - [How is checksum computed in UDP?](https://www.ques10.com/p/10930/how-is-checksum-computed-in-udp-1/)
# - [Short UDP Checksum Calculation HowTo](https://people.engr.ncsu.edu/mlsichit/Teaching/407/Resources/udpChecksum.html)
# - [RFC 768](http://www.faqs.org/rfcs/rfc768.html)
# - [RFC 1071](http://www.faqs.org/rfcs/rfc1071.html)
postid: 180003
---

UDP报头只有4个字段，分别是：源端口号、目的端口号、报文长度和报头checksum，其中的报头checksum这个字段在IPv4中并不是强制的，但在IPv6中是强制的，本文介绍UDP报头中checksum的计算方法，并给出相应的源程序，实际上，网络通信中常用的IP报头、TCP报头和UDP报头中都有checksum，其计算方法基本一样，所以把这些检查和一般统称为Internet Checksum；本文对网络编程的初学者难度不大。
<!--more-->

## 1. UDP报文结构
* UDP报文为两部分，报头+数据；
* 在Linux下，UDP报头定义在头文件linux/udp.h中；
    ```C
    struct udphdr {
        __be16	source;
        __be16	dest;
        __be16	len;
        __sum16	check;
    };
    ```
    - source - 来源端口号，可选项，如果不使用，填充 0；
    - dest - 目的端口号；
    - len - 报文长度；
    - check - 报头的校验和，在IPv4中是可选的，IPv6中是强制的，如果不使用，应填充0；

    ![UDP数据报结构][img01]

    <center><b>图1：UDP数据报结构</b></center>

***

## 2. IP报头结构
* 之所以在这里介绍IP报头，是因为在计算UDP报头checksum时会用到IP头中的一些字段；
* 在Linux下，IP报头定义在头文件linux/ip.h中，可以自行查看，我们这里仅给出图示；

    ![IP报头][img02]

    <center><b>图2：IP报头结构</b></center>

***

## 3. UDP报头checksum的计算
* UDP报头checksum的定义及计算方法在[RFC 768](http://www.faqs.org/rfcs/rfc768.html)中有明确的说明；
* UDP报头checksum的具体算法在[RFC 1071](http://www.faqs.org/rfcs/rfc1071.html)有明确的说明；
* 在计算UDP报头checksum前要首先为UDP报头加上一个伪报头；
* 加上伪报头的UDP报文格式如下：

    ![伪报头][img03]

    <center><b>图3：伪报头</b></center>

***

* 伪报头中源IP地址(Source IP address)、目的IP地址(Destination IP address)和协议(Protocol)这三个字段都是从IP报头中取过来的；
* 源IP地址和目的IP地址是32-bit的IP地址，Protocol字段是网络协议号，UDP协议号为17(0X11)；
* 伪报头中 UDP length 这个字段指的是 **UDP报头+数据** 的长度，并不包括伪报头的长度，其值和UDP报头中的len字段应该是一样的；
* 如果checksum中没有加入伪报头，UDP报文也是可以安全送达的，但是，如果IP报头有损坏或者被认为修改，报文有可能被送到错误的地址；
* 伪报头中加入protocol字段是为了保证报文为UDP报文，正常情况下protocol为17(0x11)，如果传输中这个字段变化了，那么在接收端计算出的checksum就会不正确，接收端会丢弃这个出现错误的报文；
* checksum 计算规则：
    1. UDP报头中的check字段填充0；
    2. (伪报头+UDP报头+DATA)的长度应该为16-bit字的整数倍，如果不是，DATA的最后部分要填充1个字节0，以使其为16-bit字的整数倍；
    3. (伪报头+UDP报头+DATA)看作是很多个16-bit字，并逐一相加，结果仍为16-bit字，如果出现溢出，则结果+1，然后继续，直至完成；
    4. 结果求反即为所需的checksum；
* 在RFC768中定义的UDP的checksum为：(伪报头+UDP报头+DATA)按16-bit字进行反码求和的结果就是checksum；但实际上原码求和再取反和反码求和是一样的结果，因为求反码再求和的方法对每一组16-bit字都要做一次求反运算，因此其运算量更大一些，在实际中没有人使用；
* 以上两种运算方法在本文给出的范例中均有体现，可以用来验证其结果的一致性；
* 按照RFC768的说明，当checksum的运算结果为0时，checksum应该作为全1来传输，因为在UDP协议中，checksum为0表示没有使用checksum，UDP的checksum在ipv4中并不是强制使用的。
* 下面是计算udp报头checksum的完整源代码，源代码文件名：[udp-checksum.c][src01](**点击文件名下载源程序**)

* 其中在计算checksum的程序中参考了RFC1071中给出的源代码；
* checksum1()使用的常规的算法，checksum2()使用的把每个16-bit字求反再相加的算法进行的运算，两种算法的结果是一样的。
* 读者可以根据需要适当地调整数据，以使其计算出不同的结果；
* 编译：```gcc -Wall udp-checksum.c -o udp-checksum```
* 运行：```./udp-checksum```
* 下面是我的机器上的运行结果截屏

    ![程序运行截屏][img04]

    <center><b>图4：程序运行截屏</b></center>

***

## 4. UDP报头checksum的验证
* UDP报文的接收端是需要对checksum字段进行验证的，方法如下：
    1. 加入伪报头；
    2. 将(伪报头+UDP头+DATA)按16-bit分成若干个16-bit字，如果最后一个字节无法组成一个16-bit字，要在DATA的最后补0；
    3. 将所有的16-bit字相加(包括checksum字段)，其结果仍然是16-bit字，如果出现溢出则结果+1；
    4. 如果最后结果为全1，即：0XFFFF，则表示UDP报文正确，否则应该认为UDP报文有错误，应该丢弃。



## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180003/udp-checksum.c

[img01]:https://whowin.gitee.io/images/180003/udp_packet_structure.png
[img02]:https://whowin.gitee.io/images/180003/ip_header.png
[img03]:https://whowin.gitee.io/images/180003/udp_packet_pseudo_header.png.png
[img04]:https://whowin.gitee.io/images/180003/udp_checksum_screenshot.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/308678e96fc65e323a882889600f3dfa.png
[img02]:https://img-blog.csdnimg.cn/img_convert/06950019d11c9bfbcf4ff8e387fc0858.png
[img03]:https://img-blog.csdnimg.cn/img_convert/acc464c2ab8e9ce23caadb2023866315.png
[img04]:https://img-blog.csdnimg.cn/img_convert/955ad417b9c32f315dd340a8869d1a2a.png
-->
