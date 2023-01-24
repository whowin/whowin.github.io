---
title: "如何计算IP报头的checksum"
date: 2022-12-14T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - network
tags:
  - checksum
  - "network programming"
  - "网络编程"
draft: false
#references: 
# - [How to Calculate IP Header Checksum (With an Example)](https://www.thegeekstuff.com/2012/05/ip-header-checksum/)
postid: 180004
---

如果你研究过TCP/IP协议，那么你肯定知道IP报头中的checksum字段，或许你还曾经为如何计算这个字段的值所困扰，本文我们将讨论checksum的概念，并详细介绍IP报头中的checksum是如何计算的。
<!--more-->

## 1. checksum是什么？
* 简单地说，checksum就是从数据包中计算出来的一个值，用于检查数据包完整性；通过检查checksum来判断收到的数据是否有错误；
* 数据在网络上传输时，由于各种原因数据包有可能损坏，所以在接收端必须要有一种方法来判断数据是否已经损坏，为此，在报头中加入checksum字段；
* 在发送端要按照规定的算法计算checksum并将其设置为报头中的一个字段中；在接收端，要使用同样的算法重新计算checksum，并与收到的报头中的checksum进行交叉校验，以确定数据包是否正常。

## 2. IP报头中的checksum
* IP报头的checksum仅用于验证IP报头是否正确，所以仅需在IP报头上计算即可，与IP报头后面数据无关，因为IP报头后面的数据(比如UDP、TCP、ICMP等)通常都有自己的checksum；
* 计算IP报头的checksum当然要了解IP协议的基本报头结构，下面是IP报头的基本格式：

  ![IP报头的基本格式][img01]
  **<center>图1：IP报头的基本格式</center>**

*****************
* 更好地理解IP报头各字段的含义，可以参考我的另一篇文章[《Linux下如何在数据链路层接收原始数据包》](/post/blog/linux/0011-link_layer_programming/)或者参考 [IP Protocol Header Fundamentals Explained with Diagrams](https://www.thegeekstuff.com/2012/03/ip-protocol-header/);
* 仅就算法而言，IP报头的checksum定义为：IP报头中所有16-bit字的反码之和；也就是说把IP报头按照16-bit字分割，然后把它们逐一相加，要求相加的结果仍为16-bit字，如果出现溢出(结果超出16-bit字)，则丢弃溢出并把结果加1，全部16-bit字相加完成的结果再求反码，其结果就是checksum；
* 上面的计算方法是在报文的发送端完成的；在接收端首先要将IP报头中的checksum字段清0，然后用与发送端相同的方法计算，得到的值与收到的IP报头中的checksum字段比较，如果一样，则表示IP报头完好，否则认为IP报头已经损坏；
* 实际在发送端的做法是：将IP报头按照16-bit字分割，然后把它们逐一相加(包括收到的checksum字段)，其结果如果为全1(0XFFFF)，则表示IP报头完好，否则认为IP报头已经损坏。

## 3. IP报头checksum实例
* 对于IP报头的checksum，我们现在已经有了足够的理论知识，下面我们用一个实例实际做一下计算；
* 假定下面使我们收到的IP报头(按16进制)
  ```
  4500 003c 1c46 4000 4006 b1e6 ac10 0a63 ac10 0a0c
  ```
* 我们先来看看这些数字与IP报头中各个字段的对应关系(请参考图1)
  - '45'对应报头中的前两个字段，'4'对应Version字段，'5'对应Header Length字段，因为Header Length的单位是4字节，所以报头的实际长度是5x4=20字节；
  - '00'对应报头中的Service Type字段，这个字段通常不使用，'00'表示普通正常服务；
  - '003c'对应报头中的Total Length字段，这个字段的含义是IP报文的总长度，所以这个IP数据报的长度为60字节；
  - '1c46'对应报头中的Identification字段，这个字段是IP报文的一个唯一标识符；
  - '4000'需要分成两部分，bit0-2对应报头中的Flags，bit3-15对应Fragment Offset字段；
  - ‘4006’可分为‘40’和‘06’两个字节，第一个字节‘40’对应TTL字段，字节‘06’对应IP报头中的Protocol字段，‘06’表示协议是TCP；
  - ‘be16’对应报头中的checksum字段，这个值是在发送端设置的checksum；如前所述，在接收端计算checksum时，该字段将设置为零；
  - 'ac10 0a63'对应IP报头中的Source IP Address，也就是源IP地址，相当于IP地址：172.16.10.99
  - 'ac10 0a0c'对应IP报头中的Destination IP Address，也就是目的IP地址，相当于IP地址：172.16.10.12
* 现在我们已经搞清楚了这些数字与IP报头各个字段的对应关系，我们先把这些16进制的数字转换成二进制
  ```
  4500 -> 0100 0101 0000 0000
  003c -> 0000 0000 0011 1100
  1c46 -> 0001 1100 0100 0110
  4000 -> 0100 0000 0000 0000
  4006 -> 0100 0000 0000 0110
  0000 -> 0000 0000 0000 0000  // clear checksum field
  ac10 -> 1010 1100 0001 0000
  0a63 -> 0000 1010 0110 0011
  ac10 -> 1010 1100 0001 0000
  0a0c -> 0000 1010 0000 1100
  ```
* 我们把这些二进制数注意相加
  ```
  4500  ->   0100 0101 0000 0000
  003c  ->   0000 0000 0011 1100
  453C  ->   0100 0101 0011 1100  // First result

  453C  ->   0100 0101 0011 1100  // First result plus next 16-bit word.
  1c46  ->   0001 1100 0100 0110
  6182  ->   0110 0001 1000 0010  // Second result.

  6182  ->   0110 0001 1000 0010  // Second result plus next 16-bit word.
  4000  ->   0100 0000 0000 0000
  A182  ->   1010 0001 1000 0010  // Third result.

  A182  ->   1010 0001 1000 0010  // Third result plus next 16-bit word.
  4006  ->   0100 0000 0000 0110
  E188  ->   1110 0001 1000 1000  // Fourth result.

  E188  ->   1110 0001 1000 1000  // Fourth result plus next 16-bit word.
  AC10  ->   1010 1100 0001 0000
  18D98 -> 1 1000 1101 1001 1000 // Overflow, clear overflow bit and then the result plus 1.

  18D98 -> 1 1000 1101 1001 1000
  8D99  ->   1000 1101 1001 1001  // Fifth result

  8D99  ->   1000 1101 1001 1001  // Fifth result plus next 16-bit word.
  0A63  ->   0000 1010 0110 0011
  97FC  ->   1001 0111 1111 1100  // Sixth result

  97FC  ->   1001 0111 1111 1100  // Sixth result plus next 16-bit word.
  AC10  ->   1010 1100 0001 0000
  1440C -> 1 0100 0100 0000 1100 // Overflow again, the result plus 1(as done before)

  1440C -> 1 0100 0100 0000 1100
  440D  ->   0100 0100 0000 1101  // Seventh result

  440D  ->   0100 0100 0000 1101  // Seventh result plus next 16-bit word
  0A0C  ->   0000 1010 0000 1100
  4E19  ->   0100 1110 0001 1001  // Final result.
  ```
* 0100111000011001就是报头所有16-bit字求和的最终结果，最后一步，将这个数求反码即可得到checksum；
  ```
  4E19 -> 0100 1110 0001 1001
  B1E6 -> 1011 0001 1110 0110 // CHECKSUM
  ```
* 这个值与我们收到的IP报头中的checksum完全一致，说明这个IP报头完好；
* 作为接收方，我们也可以不做最后一步，也就是不对相加的结果求反码，而是再加上收到的checksum
  ```
  4E19 -> 0100 1110 0001 1001 // sum of all 16-bit words
  B1E6 -> 1011 0001 1110 0110 // checksum that received
  FFFF -> 1111 1111 1111 1111
  ```
* 计算结果为全1，表明这个IP报头完好无损。


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png



[img01]:/images/180004/ip_header.png


