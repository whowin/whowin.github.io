---
title: "IPC之留：使用 POSIX 信号量集进行进程间同步的实例"
date: 2023-08-10T16:43:29+08:00
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
  - 信号量
  - semaphore
  - POSIX
draft: true
#references: 
# - [Synchronizing Threads with POSIX Semaphores](http://www.csc.villanova.edu/~mdamian/threads/posixsem.html)
# - [How to use POSIX semaphores in C language](https://www.geeksforgeeks.org/use-posix-semaphores-c/)
# - [overview of POSIX semaphores](https://linux.die.net/man/7/sem_overview)

postid: 100016
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍信号量集(Semaphore Sets)，尽管信号量集被认为是 IPC 的一种方式，但实际上通常把信号量集用于进程间同步或者资源互斥，和共享内存(Shared Memory)配合使用，可以实现完美的进程间通信；Linux 既支持 UNIX SYSTEM V 的信号量集，也支持 POSIX 的信号量集，本文针对 POSIX 信号量集，POSIX 标准引入了一个简单的基于文件的接口，使应用程序可以轻松地与消息队列进行交互；本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文适合 Linux 编程的初学者阅读。


<!--more-->
