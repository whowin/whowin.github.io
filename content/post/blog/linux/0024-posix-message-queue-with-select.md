---
title: "IPC之十：使用select处理多个POSIX消息队列的实例"
date: 2023-08-20T16:43:29+08:00
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
  - select
draft: true
#references: 
#

postid: 100021
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍消息队列(Message Queues)，消息队列可以完成同一台计算机上的进程之间的通信，相比较管道，消息队列要复杂一些，但使用起来更加灵活和方便，Linux 既支持 UNIX SYSTEM V 的消息队列，也支持 POSIX 的消息队列，本文针对 POSIX 消息队列，POSIX 标准引入了一个简单的基于文件的接口，这使用户程序有机会使用更多的方法处理消息队列，本文将讨论使用 select 处理多个 POSIX 消息队列并发的情况，本文给出了具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文适合 Linux 编程的初学者阅读。

<!--more-->