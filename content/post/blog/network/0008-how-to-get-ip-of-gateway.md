---
title: "如何使用C语言获取网关的IP地址"
date: 2022-12-27T16:43:29+08:00
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
  - Socket
  - 网络编程
  - arp
  - "raw socket"
draft: true
#references: 
# - [Find the next hop MAC address for ethernet header](https://stackoverflow.com/questions/51837388/find-the-next-hop-mac-address-for-ethernet-header)
# - [Getting gateway to use for a given ip in ANSI C](https://stackoverflow.com/questions/3288065/getting-gateway-to-use-for-a-given-ip-in-ansi-c)
# - [C example code to list all network interfaces and check whether they are wireless or not.](https://gist.github.com/edufelipe/6108057)
# - [List Network Interfaces using only 30 Lines of Code](https://www.cyberithub.com/list-network-interfaces/)
# - [Send an arbitrary Ethernet frame using an AF_PACKET socket in C](http://www.microhowto.info/howto/send_an_arbitrary_ethernet_frame_using_an_af_packet_socket_in_c.html)
# - [在 Linux 中如何从命令行查找默认网关的 IP 地址](https://www.51cto.com/article/720992.html)
# - [Linux用户空间与内核空间通信(Netlink通信机制)](https://cloud.tencent.com/developer/article/2139704)
# - [Default Gateway in C on Linux](https://stackoverflow.com/questions/548105/default-gateway-in-c-on-linux)
# - [How to get gateway ip and nameserver ip using ioctl in linux](https://stackoverflow.com/questions/29249665/how-to-get-gateway-ip-and-nameserver-ip-using-ioctl-in-linux)
postid: 180008
---

在linux的命令行下获取当前网络环境的默认网关IP并不是一件难事，常用的命令有ip route或者route -n，但是如何在程序中获取网关的ip却不是一件很容易的事，本文将讨论如何使用C语言获取默认网关的IP地址和MAC地址，并给出完整的源程序。
<!--more-->

## 1. 为什么要获取网关的IP地址
  > 想到写这篇文章是因为前面写了一篇[《如何使用raw socket发送UDP报文》][article01]的文章，当需要自己封装ethernet header的时候，需要填目的MAC地址，通常情况下，不管目的IP地址在那里，简单地把目的MAC地址填上网关的MAC地址即可
