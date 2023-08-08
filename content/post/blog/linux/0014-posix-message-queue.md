---
title: "IPC之四：使用 POSIX 消息队列进行进程间通信的实例"
date: 2023-07-08T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
  - "C Language"
tags:
  - Linux
  - 进程间通信
  - 消息队列
  - "Message queues"
draft: true
#references: 
# - [Linux Interprocess Communications](https://tldp.org/LDP/lpg/node7.html)
# - [进程间通信(二):IPC键值生成(ftok函数)与使用](http://blog.chinaunix.net/uid-70024505-id-5874635.html)
# - [mq_overview(7) — Linux manual page](https://man7.org/linux/man-pages/man7/mq_overview.7.html)
#     - POSIX 消息队列
# - [Implementation of Message Queues in the Linux Kernel](https://www.baeldung.com/linux/kernel-message-queues)

postid: 100014
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍消息队列(Message Queues)，消息队列可以完成同一台计算机上的进程之间的通信，相比较管道，消息队列要复杂一些，但使用起来更加灵活和方便，本文针对 POSIX 消息队列，并给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文适合 Linux 编程的初学者阅读。

<!--more-->


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png



