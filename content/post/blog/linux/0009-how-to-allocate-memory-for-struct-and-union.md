---
title: "在64位linux下如何为struct分配内存和内存对齐的"
date: 2023-06-25T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags:
  - Linux
  - struct
  - union
draft: true
#references: 
# - [struct的用法和struct的对齐原则](https://blog.csdn.net/qq_21989927/article/details/110139636)
# - [How Struct Memory Alignment Works in C](https://levelup.gitconnected.com/how-struct-memory-alignment-works-in-c-3ee897697236)
# - [Structure Member Alignment, Padding and Data Packing](https://www.geeksforgeeks.org/structure-member-alignment-padding-and-data-packing/)
postid: 100009
---

性能是优秀软件最重要的方面之一。出于这个原因，使用了许多优化技术来实现良好的性能。数据对齐当然是其中之一。


<!--more-->