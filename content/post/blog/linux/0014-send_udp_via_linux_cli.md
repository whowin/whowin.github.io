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
tags:
  - Linux
  - ubuntu
  - netcat
  - nc
  - "udp packet"

draft: false
#references: 
# - [Send and Receive UDP packets via Linux CLI](https://linuxhint.com/send_receive_udp_packets_linux_cli/)
postid: 100014
---

众所周知，在传输层有两个常用的协议 TCP 和 UDP，本文介绍在 Linux 命令行下，如何使用 nc 命令发送或接收 UDP 数据包，这些命令的用法对调试 UDP 通信程序将有所帮助。
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
  ```
  nc
  ```
* 在 ubuntu 20.04 下的输出：
  ```
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
    ```
    nc –u –l 8888
    ```
  - 下面是屏幕截图

    ![在B机用nc启动服务器监听][img02]

  - 启动后，并没有更多的提示，当收到信息时，会显示收到的信息

* **检查 server 是否启动**
  - 在 B 机上启动一个新的终端窗口；
  - 在 B 机上使用如下命令检查是否已经启动 server；
    ```
    netstat -a|grep 8888
    ```
  - 屏幕截图

    ![检查server是否启动][img03]

* **发送 UDP 报文**
  - 在 A 机上启动 client
    ```
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
  ```
  echo -n "I am sending an UDP packet using echo command" > /dev/udp/192.168.2.114/8888
  ```
* 下面是在 A 机上的截屏

  ![使用echo发送UDP报文][img08]

* 下面是在 B 机上的截屏

  ![接收使用echo发送的UDP报文][img09]

* 很显然使用 cat 命令也可以发送 UDP 报文：
  
  ![使用cat命令发送UDP报文][img10]

  ![接收使用cat命令发送的报文][img11]






[img01]:/images/100014/server_client_connection.png
[img02]:/images/100014/start_server_with_nc.png
[img03]:/images/100014/screenshot_for_started_server.png
[img04]:/images/100014/send_udp_packet_client.png
[img05]:/images/100014/receive_udp_packet_server.png
[img06]:/images/100014/send_udp_packet_server.png
[img07]:/images/100014/receive_udp_packet_client.png
[img08]:/images/100014/send_udp_packet_client_echo.png
[img09]:/images/100014/receive_udp_packet_server_echo.png
[img10]:/images/100014/send_udp_packet_client_cat.png
[img11]:/images/100014/receive_udp_packet_server_cat.png
