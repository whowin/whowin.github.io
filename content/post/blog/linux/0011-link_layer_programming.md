---
title: "Linux下的链路层编程"
date: 2022-08-20T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags:
  - Linux
  - Socket
  - 网络编程
  - 链路层编程
draft: true
#references: 
# - [How to Calculate IP Header Checksum (With an Example)](https://www.thegeekstuff.com/2012/05/ip-header-checksum/)
# - [TCP/IP四层模型](https://www.cnblogs.com/BlueTzar/articles/811160.html)
# - [Linux网络编程（数据链路层）](https://blog.csdn.net/qq_36131611/article/details/118460589)
# - [linux基础编程 链路层socket](https://blog.csdn.net/ghostyu/article/details/7737966)
# - [OSI七层与TCP/IP五层网络架构详解](http://www.2cto.com/net/201310/252965.html)
postid: 100011
---

大多数的网络编程都是在应用层的，本文介绍基于链路层的网络编程
<!--more-->

## 1. 概述
* 本文并不会详细讲解应用层的网络编程；
* 本文主要介绍在数据链路层的网络编程；
* 本文所介绍的内容在实际编程中很少会用到，但希望对读者理解网络结构和协议能有帮助；
* 本文可能并不适合初学者，但并不妨碍初学者收藏此文，以便在今后学习。

## 2. socket 编程
* 使用 socket 进行网络编程时，我们通常只需要关心需要发送的数据，数据发送后，要发送的数据将从应用层向下传递
  - 在 TCP (传输)层加入一个 TCP 头
  - 在 IP (网络)层加上一个 IP 头
  - 在数据链路层加上一个以太网头
  - 交给物理层传输

* 在看下面的内容之前还是要简单地回顾一下 TCP/IP 的五层网络模型(OSI 七层架构的简化版)
  1. 应用层
  2. 传输层
  3. 网络层
  4. 数据链路层
  5. 物理层

* 当我们在应用层进行 socket 编程时，我们通常会这样发送数据(以 UDP 为例)：
  ```
  ......
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  ......
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(DST_IP);       // 目的IP
  sin.sin_port = htons(PORT);                    // 端口号
  .....
  sendto(fd, &DATA, DATA_LEN, 0, (struct sockaddr *)&sin, sizeof(sin));
  close(fd);
  ```
* 当我们把 DATA 给 sendto(......) 以后，会发生什么呢？
  1. 数据从应用层被送到传输层，传输层给这个数据加上一个 UDP 头；
  2. (UDP头 + DATA)从传输层被送到网络层，IP 协议会给数据包再加上一个 IP 头；
  3. (IP头 + UDP头 + DATA)从网络层被送到了数据链路层，数据链路层的以太网协议会给这个数据包加上一个以太网头；
  4. (以太网头 + IP头 + UDP头 + DATA)从数据链路层被送到了物理层，数据就被发送走了
* 接收数据基本上是发送数据的反过程，各层解开自己的协议头，最后我们在应用层收到的数据就只剩下 DATA 了；
* 很显然，在应用层进行网络编程，我们不需要关心各层的报头，各层的协议栈会为我们处理好所有报头；
* 但这样的编程显然也是受限的，除了 TCP 和 UDP 以外，你还知道有什么其它的网络通信形式吗？你能在应用层编写一个 ping 程序吗？这些都让我们看到在应用层做网络编程的一些限制，但这些限制并不影响大多数的应用。
* 当我们需要在传输层编程时，实际上就是比在应用层编程多了一个 UDP(TCP) 头；同理，当我们需要在网络层编程时，也就是比在传输层编程多加一个 IP 头；
* 本文介绍在数据链路层编程，与在应用层的网络编程相比，只是要多封装三个数据头：以太网头、IP头、UDP(TCP)头

## 3. 一个例子
* 我们从一个实际例子开始，先看源程序：







