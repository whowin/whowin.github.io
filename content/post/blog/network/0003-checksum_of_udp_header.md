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

UDP报头只有4个字段，分别是：源端口号、目的端口号、报文长度和报头checksum，其中的报头checksum这个字段在IPv4中并不是强制的，但在IPv6中是强制的，本文介绍UDP报头中checksum的计算方法，并给出相应的源程序。
<!--more-->

## 1. UDP报文结构
* UDP报文为两部分，报头+数据；
* 在Linux下，UDP报头定义在头文件linux/udp.h中；
    ```
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
    **<center>图1：UDP数据报结构</center>**

***

## 2. IP报头结构
* 之所以在这里介绍IP报头，是因为在计算UDP报头checksum时会用到IP头中的一些字段；
* 在Linux下，IP报头定义在头文件linux/ip.h中，可以自行查看，我们这里仅给出图示；

    ![IP报头][img02]
    **<center>图2：IP报头结构</center>**

***

## 3. UDP报头checksum的计算
* UDP报头checksum的定义及计算方法在[RFC 768](http://www.faqs.org/rfcs/rfc768.html)中有明确的说明；
* UDP报头checksum的具体算法在[RFC 1071](http://www.faqs.org/rfcs/rfc1071.html)有明确的说明；
* 在计算UDP报头checksum前要首先为UDP报头加上一个伪报头；
* 加上伪报头的UDP报文格式如下：

    ![伪报头][img03]
    **<center>图3：伪报头</center>**

***

* 伪报头中源IP地址(Source IP address)、目的IP地址(Destination IP address)和协议(Protocol)这三个字段都是从IP报头中取过来的；
* 源IP地址和目的IP地址是32-bit的IP地址，Protocol字段是网络协议号，UDP协议号为17(0X11)；
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
* 下面是计算udp报头checksum的完整源代码：
    ```
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <stdint.h>

    #include <arpa/inet.h>
    #include <linux/udp.h>

    // udp pseudo header structure
    struct pseudohdr {
        uint32_t source_ip;
        uint32_t destination_ip;
        uint8_t zero;
        uint8_t protocol;
        uint16_t udp_len;
    };
    // udp packet structure for calculating checksum
    struct udpcheckhdr {
        struct pseudohdr pseudo_hdr;
        struct udphdr udp_hdr;
        unsigned char data[16];
    };
    // initial pseudo header
    void init_pseudohdr(struct udpcheckhdr *p) {
        p->pseudo_hdr.source_ip = inet_addr("152.1.51.27");         // 0X0198 0X1B33
        p->pseudo_hdr.destination_ip = inet_addr("152.14.94.75");   // 0X0E98 0X4B5E
        p->pseudo_hdr.zero = 0;                     // 0X00 - 0X1100
        p->pseudo_hdr.protocol = 17;                // 0X11
        p->pseudo_hdr.udp_len = 13;                 // 0X000D
    }
    // initial udp header
    void init_udphdr(struct udpcheckhdr *p) {
        p->udp_hdr.source = 56020;                  // 0xDAD4
        p->udp_hdr.dest = 8000;                     // 0X1F40
        p->udp_hdr.len = 13;                        // 0X000D
        p->udp_hdr.check = 0;                       // 0X0000
    }
    // initial udp data
    void init_udpdata(struct udpcheckhdr *p) {
        p->data[0] = 'h';                           // 0X68 - 0X6568
        p->data[1] = 'e';                           // 0X65
        p->data[2] = 'l';                           // 0X6C - 0X6C6C
        p->data[3] = 'l';                           // 0X6C
        p->data[4] = 'o';                           // 0X6F - 0X006F
        p->data[5] = 0;
    }

    uint16_t checksum1(uint16_t *p, int count) {
        register long sum = 0;
        unsigned short checksum;

        uint16_t temp;
        uint16_t i = 0;
        while (count > 1) {
            /*  This is the inner loop */
            temp = (unsigned short)*(unsigned short *)p++;
            printf("Step %02d: 0X%08lX + 0X%04X\n", ++i, sum, temp);
            sum += temp;
            count -= 2;
        }

        /*  Add left-over byte, if any */
        if (count > 0) {
            temp = (unsigned short)*(unsigned short *)p;
            printf("Step %02d: 0X%08lX + 0X%04X\n", ++i, sum, temp);
            sum += *(unsigned char *)p;
        }

        printf("Result before folding: 0X%08lX\n", sum);
        /*  Fold 32-bit sum to 16 bits */
        while (sum >> 16)
            sum = (sum & 0xffff) + (sum >> 16);

        printf("Result after folding: 0X%08lX\n", sum);

        checksum = ~(unsigned short)sum;
        printf("\nChecksum = 0x%04X\n", checksum);

        return checksum;
    }

    uint16_t checksum2(uint16_t *p, int count) {
        register long sum = 0;
        unsigned short checksum;

        uint16_t temp;
        uint16_t i = 0;
        while (count > 1) {
            /*  This is the inner loop */
            temp = (unsigned short)*(unsigned short *)p++;
            printf("Step %02d: 0X%08lX + 0X%04X(0X%04X)\n", ++i, sum, (uint16_t)~temp, temp);
            sum += (uint16_t)~temp;
            count -= 2;
        }

        /*  Add left-over byte, if any */
        if (count > 0) {
            temp = (unsigned short)*(unsigned short *)p;
            printf("Step %02d: 0X%08lX + 0X%04X(0X%04X)\n", ++i, sum, (uint16_t)~temp, temp);
            sum += (uint16_t)~temp;
        }

        printf("Result before folding: 0X%08lX\n", sum);
        /*  Fold 32-bit sum to 16 bits */
        while (sum >> 16)
            sum = (sum & 0xffff) + (sum >> 16);

        printf("Result after folding: 0X%08lX\n", sum);

        checksum = (unsigned short)sum;
        printf("\nChecksum = 0x%04X\n", checksum);
        return checksum;
    }

    int main(int argc, char **argv) {
        struct udpcheckhdr udp_packet;

        init_pseudohdr(&udp_packet);
        init_udphdr(&udp_packet);
        init_udpdata(&udp_packet);

        unsigned short *p = (unsigned short *)&udp_packet;
        int count = sizeof(struct pseudohdr) + udp_packet.udp_hdr.len;

        printf("\nThe one's complement code of 16-bit true code sum\n");
        checksum1(p, count);
        printf("\nThe one's complement sum\n");
        checksum2(p, count);

        return 0;
    }
    ```
* 其中在计算checksum的程序中参考了RFC1071中给出的源代码；
* checksum1()使用的常规的算法，checksum2()使用的把每个16-bit字求反在相加的算法进行的运算，两种算法的结果是一样的。
* 读者可以根据需要适当第调整数据，以使其计算出不同的结果；
* 下面是我的机器上的运行结果截屏

    ![程序运行截屏][img04]
    **<center>图4：程序运行截屏</center>**

***

## 4. UDP报头checksum的验证
* UDP报文的接收端是需要对checksum字段进行验证的，方法如下：
    1. 加入伪报头；
    2. 将(伪报头+UDP头+DATA)按16-bit分成若干个16-bit字，如果最后一个字节无法组成一个16-bit字，要在DATA的最后补0；
    3. 将所有的16-bit字相加(包括checksum字段)，其结果仍然是16-bit字，如果出现溢出则结果+1；
    4. 如果最后结果为全1，即：0XFFFF，则表示UDP报文正确，否则应该认为UDP报文有错误，应该丢弃。


**欢迎访问我的博客：https://whowin.cn**



[img01]:/images/180003/udp_packet_structure.png
[img02]:/images/180003/ip_header.png
[img03]:/images/180003/udp_packet_pseudo_header.png.png
[img04]:/images/180003/udp_checksum_screenshot.png

