---
title: "linux下使用C语言获取gateway的IP地址"
date: 2023-01-01T16:43:29+08:00
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
# - [Getting gateway to use for a given ip in ANSI C](https://stackoverflow.com/questions/3288065/getting-gateway-to-use-for-a-given-ip-in-ansi-c)
# - [Getting Linux routing table using netlink](https://olegkutkov.me/2019/03/24/getting-linux-routing-table-using-netlink/)
# - [Default Gateway in C on Linux](https://stackoverflow.com/questions/548105/default-gateway-in-c-on-linux)
# - [How to get gateway ip and nameserver ip using ioctl in linux](https://stackoverflow.com/questions/29249665/how-to-get-gateway-ip-and-nameserver-ip-using-ioctl-in-linux)
# - [Linux用户空间与内核空间通信(Netlink通信机制)](https://cloud.tencent.com/developer/article/2139704)
# - [Netlink机制详解](https://blog.csdn.net/xinyuan510214/article/details/52635085)
postid: 180009
---

linux下用户程序和内核通讯不外乎四种方式：系统调用、虚拟文件系统(/proc、/sys等)、ioctl和netlink，本文试图用尽可能简单的方式讨论netlink的编程方法。
<!--more-->


## 1. 概述
* netlink是一个socket，所以它的编程与普通的socket编程类似，其socket的创建方法如下：
    ```
    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    ```
    - 第一个参数可以是AF_NETLINK或者PF_NETLINK，表示要创建一个NETLINK socket，其实AF_NETLINK和PF_NETLINK是一样的，可以到bits/socket.h中查看相应的定义；
    - 第二个参数可以是SOCK_RAW或者SOCK_DGRAM；
    - 第三个参数是NETLINK协议，这些协议定义在文件linux/netlink.h中，在linux 5.15中定义了23中协议，协议的最大值为32个
        ```
        #define NETLINK_ROUTE       0    /* Routing/device hook                     */
        #define NETLINK_UNUSED      1    /* Unused number                           */
        #define NETLINK_USERSOCK    2    /* Reserved for user mode socket protocols */
        #define NETLINK_FIREWALL    3    /* Unused number, formerly ip_queue        */
        #define NETLINK_SOCK_DIAG   4    /* socket monitoring                       */
        #define NETLINK_NFLOG       5    /* netfilter/iptables ULOG */
        #define NETLINK_XFRM        6    /* ipsec */
        #define NETLINK_SELINUX     7    /* SELinux event notifications */
        #define NETLINK_ISCSI       8    /* Open-iSCSI */
        #define NETLINK_AUDIT       9    /* auditing */
        #define NETLINK_FIB_LOOKUP  10    
        #define NETLINK_CONNECTOR   11
        #define NETLINK_NETFILTER   12    /* netfilter subsystem */
        #define NETLINK_IP6_FW      13
        #define NETLINK_DNRTMSG     14    /* DECnet routing messages */
        #define NETLINK_KOBJECT_UEVENT  15    /* Kernel messages to userspace */
        #define NETLINK_GENERIC     16
        /* leave room for NETLINK_DM (DM Events) */
        #define NETLINK_SCSITRANSPORT   18    /* SCSI Transports */
        #define NETLINK_ECRYPTFS    19
        #define NETLINK_RDMA        20
        #define NETLINK_CRYPTO      21    /* Crypto layer */
        #define NETLINK_SMC         22    /* SMC monitoring */
        ```
    - 尽管定义的协议很多，但常用的协议并不多

* netlink socket本质上是进程间的通信，一个用户进程使用netlink socket不仅可以和内核进程进行通信，两个(多个)用户进程间也可以使用netlink socket进行通信；
* netlink除了使用内核定义的协议外，也可以使用自定义协议，在上面定义的netlink协议中的NETLINK_GENERIC就是用于用户自定义协议的，我们可以编写一个内核进程，然后在用户进程中使用NETLINK_GENERIC进行通信；
* netlink socket有一个多播的特性，所谓多播，就是一个进程发出的消息可以有多个其它进程接收，netlink允许最多32个多播组，当向一个多播组发出消息时，所有已经加入这个多播组的进程都可以收到这个消息。
* 当使用netlink socket与内核进行通信时，用户进程需要先向内核进程发出一个请求，然后接收内核进程返回的消息，从而实现与内核的通信；
* netlink socket可以使用send、sendto、sendmsg发送消息，使用recv、recvfrom、recvmsg接收消息，这些和不同socket是一样的；












// in linux/netlink
struct nlmsghdr {
    __u32   nlmsg_len;    /* Length of message including header */
    __u16   nlmsg_type;   /* Message content */
    __u16   nlmsg_flags;  /* Additional flags */
    __u32   nlmsg_seq;    /* Sequence number */
    __u32   nlmsg_pid;    /* Sending process port ID */
};


// in linux/rtnetlink.h
struct rtmsg {
	unsigned char		rtm_family;
	unsigned char		rtm_dst_len;
	unsigned char		rtm_src_len;
	unsigned char		rtm_tos;

	unsigned char		rtm_table;	  /* Routing table id */
	unsigned char		rtm_protocol; /* Routing protocol; see below	*/
	unsigned char		rtm_scope;	  /* See below */	
	unsigned char		rtm_type;	    /* See below	*/

	unsigned		    rtm_flags;
};


// in linux/in.h
struct in_addr
  {
    in_addr_t s_addr;
  };





