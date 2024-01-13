---
title: "LINUX下的IO多路复用，select()、poll()和epoll()"
date: 2024-01-11T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Network"
  - "Linux"
  - "C Language"
tags:
  - Linux
  - 网络编程
  - select
  - poll
  - epoll
draft: true
#references: 
# - [LINUX – IO MULTIPLEXING – SELECT VS POLL VS EPOLL](https://devarea.com/linux-io-multiplexing-select-vs-poll-vs-epoll/)

postid: 180021
---



<!--more-->


## 1 基本概念
* Linux 有一个基本概念即所谓："一切皆文件"，socket、磁盘文件、管道、设备以及其它操作系统对象，均使用文件描述符来表示，每个进程在内核中都有一个文件描述符表，表中的每一个文件描述符指向该进程所需要的各种对象；
* 在 Linux 启动时，有大量设备、文件以及各种操作系统对象需要初始化，


