---
title: "如何在Linux命令行下发送和接收UDP数据包"
date: 2022-12-10T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
  - "Network"
tags:
  - Linux
  - ubuntu
  - netcat
  - nc
  - "udp packet"

draft: false
#references: 
# - [Send and Receive UDP packets via Linux CLI](https://linuxhint.com/send_receive_udp_packets_linux_cli/)
postid: 180005
---

众所周知，在传输层有两个常用的协议 TCP 和 UDP，本文介绍在 Linux 命令行下，如何使用 nc 命令发送或接收 UDP 数据包，这些命令的用法对调试 UDP 通信程序将有所帮助，本文适合初学者阅读。
<!--more-->

## 1. 问题的提出
> 编写了一个使用 raw socket 在数据链路层接收原始 UDP 数据包的程序，调试的时候，需要使用局域网中的另一台机器发送 UDP 数据包，突然发现居然一下子想不出如何在命令行上发送数据包；首先想到的是用 curl，但又一想不对，curl 只能使用应用层协议透过 TCP 协议发送数据包，所以可以发 HTTP 请求包，FTP 请求包等，是不能发送 UDP 数据包的。

> 终于想起了不怎么使用的 nc 命令，可以很方便地发送 UDP 数据包

## 2. 文章要点
1. 理解 Linux 下的 nc 命令；
2. 使用 nc 命令在网络上发送或接收 UDP 报文；
3. 抓取 nc 命令发送的 UDP 报文；
4. 介绍其它可以发送 UDP 报文的命令。

## 3. netcat 命令
* nc 是 netcat 的简写，大多数的 Linux 发行版中默认是安装 netcat 的，打开一个终端窗口(ctrl+alt_t)，用下面命令检查一下是否已经安装了 netcat：
  ```bash
  nc
  ```
* 在 ubuntu 20.04 下的输出：
  ```bash
  usage: nc [-46CDdFhklNnrStUuvZz] [-I length] [-i interval] [-M ttl]
            [-m minttl] [-O length] [-P proxy_username] [-p source_port]
            [-q seconds] [-s source] [-T keyword] [-V rtable] [-W recvlimit] [-w timeout]
            [-X proxy_protocol] [-x proxy_address[:port]] 	  [destination] [port]
  ```
* 这个输出表示当前的 Linux 下已经有 nc 命令了。

## 4. 发送 UDP 数据包
* 举个例子，假定我们要从 A 机发送一个 UDP 报文到 B 机，按照 server-client 的概念，我们把 B 机作为 server 端，A 机作为 client 端；

  ![Server-client方式连接的两台机器][img01]

* A 机的 IP: 192.168.2.112；B 机的 IP：192.168.2.114
* **启动 server**
  - 在 B 机上用如下命令启动 Server
    ```bash
    nc –u –l 8888
    ```
  - 下面是屏幕截图

    ![在B机用nc启动服务器监听][img02]

  - 启动后，并没有更多的提示，当收到信息时，会显示收到的信息

* **检查 server 是否启动**
  - 在 B 机上启动一个新的终端窗口；
  - 在 B 机上使用如下命令检查是否已经启动 server；
    ```bash
    netstat -a|grep 8888
    ```
  - 屏幕截图

    ![检查server是否启动][img03]

* **发送 UDP 报文**
  - 在 A 机上启动 client
    ```bash
    nc -n 192.168.2.114 8888
    ```
  - client 启动后，和 server 端一样，并没有更多的提示，可以直接输入你要发出的信息
  - 在 A 机输入信息：I am from ubuntu system A.
  - 下面是 A 机上的屏幕截图

    ![在client端发送信息][img04]

  - 下面是 B 机上的屏幕截图

    ![在server端接收信息][img05]

  - 也可以在 B 机输入信息，信息会发送到 A 机
  - 在 B 机输入信息：I am from ubuntu system B.
  - 下面是在 B 机的截图

    ![在server端发送信息][img06]

  - 下面是在 A 机的截图

    ![在client端接收信息][img07]

## 5. 其它可以发送 UDP 数据包的命令
* 还有一个可以简单地发送 UDP 报文的方法，我们在 A 机上退出 nc，然后使用下面命令发出信息
  ```bash
  echo -n "I am sending an UDP packet using echo command" > /dev/udp/192.168.2.114/8888
  ```
* 下面是在 A 机上的截屏

  ![使用echo发送UDP报文][img08]

* 下面是在 B 机上的截屏

  ![接收使用echo发送的UDP报文][img09]

* 很显然使用 cat 命令也可以发送 UDP 报文：
  
  ![使用cat命令发送UDP报文][img10]

  ![接收使用cat命令发送的报文][img11]


## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[img01]:/images/180005/server_client_connection.png
[img02]:/images/180005/start_server_with_nc.png
[img03]:/images/180005/screenshot_for_started_server.png
[img04]:/images/180005/send_udp_packet_client.png
[img05]:/images/180005/receive_udp_packet_server.png
[img06]:/images/180005/send_udp_packet_server.png
[img07]:/images/180005/receive_udp_packet_client.png
[img08]:/images/180005/send_udp_packet_client_echo.png
[img09]:/images/180005/receive_udp_packet_server_echo.png
[img10]:/images/180005/send_udp_packet_client_cat.png
[img11]:/images/180005/receive_udp_packet_server_cat.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/369c015ee73c25e930c97f28feff97fe.png
[img02]:https://img-blog.csdnimg.cn/img_convert/a94450d7a118e06942e11cd3bee15b88.png
[img03]:https://img-blog.csdnimg.cn/img_convert/19d5de03be57aee2b04a33562ffda33b.png
[img04]:https://img-blog.csdnimg.cn/img_convert/bb6b617d6d89c52f07d9c1ae07bc9cee.png
[img05]:https://img-blog.csdnimg.cn/img_convert/204ea2b853a84b082a52841efe84e594.png
[img06]:https://img-blog.csdnimg.cn/img_convert/45abf0daf3d6af7486a55e81291f1dda.png
[img07]:https://img-blog.csdnimg.cn/img_convert/3ba1c8ddf249d7b76f15f25cefb78c13.png
[img08]:https://img-blog.csdnimg.cn/img_convert/f23656ae2dd4b30a1fa23d8e168eea6f.png
[img09]:https://img-blog.csdnimg.cn/img_convert/488b296f5e4b3a3f4ba699a2a374eb25.png
[img10]:https://img-blog.csdnimg.cn/img_convert/c6d7a298a76f29e727e89cefe7efdb12.png
[img11]:https://img-blog.csdnimg.cn/img_convert/70af03a0ea920551e5e5d626a85c262f.png
-->
