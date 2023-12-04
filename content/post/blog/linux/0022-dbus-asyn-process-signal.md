title: "IPC之十二：使用D-Bus异步机制处理信号的实例"
date: 2023-11-06T16:43:29+08:00
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
  - 进程间通信
  - files
  - IPC
draft: false
#references: 

# - [D-Bus : Transmit a Data Array in Simple and Useful Form](http://gaiger-programming.blogspot.com/2015/08/d-bus-simple-and-useful-example-to-send.html)
#   - 里面有一个很不错异步处理信号的例子

postid: 100021
---


IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本 IPC 系列文章的前十篇介绍了几乎所有的常用的 IPC 方法，每种方法都给出了具体实例，本文介绍在 Linux 下 IPC 的另外一种实现，D-Bus，D-Bus 是一种用于进程间通信的消息总线系统，它提供了一个可靠且灵活的机制，使得不同进程之间能够相互通信，本文给出了使用 D-Bus 进行基本 IPC 的实例，并附有完整的源代码；本文实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文不适合 Linux 编程的初学者阅读。

<!--more-->