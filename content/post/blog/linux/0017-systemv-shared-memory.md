---
title: "IPC之七：使用 POSIX 共享内存进行进程间通信的实例"
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
  - IPC
  - POSIX
  - 共享内存
  - "Shared Memory"
draft: true
#references: 
# - [Linux Interprocess Communications](https://tldp.org/LDP/lpg/node7.html)

postid: 100017
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍半双工管道，尽管很多人在编程中使用过管道，但一些特殊的用法还是鲜有文章涉及，本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文适合 Linux 编程的初学者阅读

<!--more-->







-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

