---
title: "从proc文件系统中获取gateway的IP地址"
date: 2023-02-05T16:43:29+08:00
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
  - proc文件系统
draft: false
#references: 
# - [Default Gateway in C on Linux](https://stackoverflow.com/questions/548105/default-gateway-in-c-on-linux)
#     - 里面的源代码可以从文件/proc/net/route中获得gataway
# - [获取路由信息 /proc/net/route](https://blog.csdn.net/xclshwd/article/details/90176336)
postid: 180008
---

在linux的命令行下获取当前网络环境的gateway的IP并不是一件难事，常用的命令有```ip route```或者```route -n```，其实```route -n```也是通过读取proc文件系统下的文件来从内核获取路由表的，但```ip route```是通过netlink来获取的路由表；本文将讨论如何编写程序从proc文件系统中获取路由表，并从路由表中获取gateway的IP地址，文章最后给出了完整的源程序。
<!--more-->

## 1. 为什么要获取网关的IP地址
> 以前写过一些与raw socket有关的文章，在使用raw socket发送报文的时候，是需要自己构建以太网报头的，以太网报头中是要填写源地址和目的地址的MAC地址的，源地址的MAC地址就是本机的MAC地址，可以使用ioctl解决，但是目的地址的MAC地址就不好办了，如果是局域网内，我们可以通过arp获取目的地址的MAC地址，因为我们是知道目的地址的IP的，但是如果目的地址在局域网外，我们就要在以太网报头中填写gateway的MAC地址，这时候我们就一定要知道gateway的IP地址，然后通过arp cache获取gateway的MAC地址；

> 获得了gateway的IP地址以后，可以很容易地从本地arp cache中获得MAC地址的，因为gateway的MAC地址一定会在本地arp cache中，有关如何在操作arp cache，可以参考我的另一篇文章[《如何用C语言操作arp cache》][article01]

## 2. 从proc文件系统中获取gateway的IP
* Linux下的proc文件系统是一个虚拟文件系统，所谓虚拟文件系统指的是它并不是一个存放在硬盘上的目录，而是内核建立的用于让用户空间了解内核运行状态或者调试的运行时的文件系统；
* 内核的路由表存放在/proc/net/route文件下，在终端上用命令```cat /proc/net/route```可以很容易地打印出路由表；下面是在我的机器上看到的结果：

  ![routing table][img01]

----------
* /proc/net/route文件的结构
  - 第一行为表头，显示每一列的名称，从第二行起为路由数据，每行代表一条路由；
  - 每列之间的分隔符为TAB(\t)；
  - 第1列：Iface，为接口名称(Interface Name);
  - 第2列：Destination，为目标网段或者目标主机的IP，以字符串表达的一个16进制的32位(4字节)数字(比如0X0103A8C0将表示为字符串"0103A8C0")，把这个字符串按照16进制转换成一个32位数字，则表达着一个IP地址；
  - 第3列：Gateway，为gateway的IP，与第三列的表达形式一样；
  - 第4列：Flags，为一个标志，每一位代表一个标志，定义在linux/route.h中，如下：
    ```C
    #define RTF_UP          0x0001        /* route usable                 */
    #define RTF_GATEWAY     0x0002        /* destination is a gateway     */
    #define RTF_HOST        0x0004        /* host entry (net otherwise)   */
    #define RTF_REINSTATE   0x0008        /* reinstate route after tmout  */
    #define RTF_DYNAMIC     0x0010        /* created dyn. (by redirect)   */
    #define RTF_MODIFIED    0x0020        /* modified dyn. (by redirect)  */
    #define RTF_MTU         0x0040        /* specific MTU for this route  */
    #define RTF_MSS         RTF_MTU       /* Compatibility :-(            */
    #define RTF_WINDOW      0x0080        /* per route window clamping    */
    #define RTF_IRTT        0x0100        /* Initial round trip time      */
    #define RTF_REJECT      0x0200        /* Reject route                 */
    ```
  - 看着挺多，但其实常用的很少，就是前三个，RTF_UP表示该路由可用，RTF_GATEWAY表示该路由为一个网关，组合在一起就是3，表示一个可用的网关；
  - 第5列：RefCnt，为该路由被引用的次数，Linux内核中没有使用这个；
  - 第6列：Use，该路由被查找的次数；
  - 第7列：Metric，到目的地的"距离"，通常以"跳数"表示，所谓"跳数"可以理解为要经过的网关的数量，实际数字并不准确；
  - 第8列：Mask，网络掩码；
  - 第9列：MTU，路由上可以传送的最大数据包；
  - 第10列：Window，TCP窗口的大小，只在AX.25网络上使用
  - 第11列：IRTT，通过这条路由的初始往返时间，只在AX.25网络上使用；
  - 可以使用在线手册```man route```了解更详细的信息；
  - 当Destination为"00000000"时，表示任意目的地址；
  - 当Gateway为"00000000"时，表示不需要经过网关，比如本地局域网中的目的地址，Gateway字段应该是"00000000"。

## 3. ip route和route -n是如何获取路由表的
* 使用命令```ip route```可以获得路由表的，下面是运行截图：

  ![screenshot of executing ip route][img02]

-------
* ```ip route```是使用netlink获取路由表的，我们可以使用strace命令跟踪```ip route```的执行，然后在输出中查找/proc/net/route和RTM_GETROUTE，从而确定这个命令是如何获得路由表的
* 如果不清楚宏RTM_GETROUTE的含义，请参考另一篇文章[《linux下使用netlink获取gateway的IP地址》][article02]
  ```C
  strace -o ip.txt ip route
  grep RTM_GET_ROUTE ip.txt
  grep /proc/net/route ip.txt
  ```

  - ```-o``` 选项表示跟踪输出结果写入文件ip.txt中，然后从文件ip.txt中分别查找 ```RTM_GETROUTE``` 和 ```/proc/net/route```
  - 下面是运行结果的截图

    ![trace ip route][img03]

  ----------
  - 可以很清晰第看到，在执行```ip route```的过程中，使用RTM_GETROUTE作为参数向socket上发送了一条获取路由表的信息，而找不到关于文件```/proc/net/route```的任何信息，由此可以肯定```ip route```是使用netlink获取的路由表；
  - 有关使用netlink获取gateway IP的方法，可以参考另一篇文章[《linux下使用netlink获取gateway的IP地址》][article02]

* 使用命令```route -n```也是可以获得路由表的，下面是运行截图：

  ![screenshot of route -n][img04]

-----------
* ```route -n``` 是通过读取proc文件系统中的文件 ```/proc/net/route``` 来获取路由表的，我们用同样的方法来跟踪一下 ```route -n``` 的运行情况：
  ```C
  strace -o route.txt route -n
  grep RTM_GET_ROUTE route.txt
  grep /proc/net/route route.txt
  ```
  - 下面是运行结果截图

    ![trace route -n][img05]

-----------
* 跟踪结果告诉我们在执行 ```route -n``` 时，文件/proc/net/route被读取，但是却没有找到使用 RTM_GETROUTE 从内核获取路由表的迹象，由此可以断定 ```route -n``` 是通过proc文件系统获取的路由表。

## 4. 从proc文件系统中获取gateway的IP地址
* 内核将路由表放在proc文件系统下，文件名为：/proc/net/route
* 读取文件```/proc/net/route```即可获得路由表，从中找到gateway这一行就可以了，按以下步骤：
  1. 打开文件/proc/net/route
  2. 读取一行不做任何处理，跳过路由表的表头行；
  3. 每次读取一行，检查其是否为gateway的记录，如果是，将gateway字段转换成32位的IP地址再转换成字符串；
  4. 关闭文件
* 下面是源代码，文件名：[get-gateway-proc.c][src01](**点击文件名下载源程序**)

* 这段程序也没什么好解释的，唯一要说明的地方是gateway路由的确定，这里是以目的地址为"00000000"作为判断，前面讨论过，目的地址为"00000000"的含义为任意目的地址；
* 编译：```gcc -Wall get-gateway-proc.c -o get-gateway-proc```
* 运行：```./get-gateway-proc```
* 下面是运行截图

  ![screenshot of executing get-gateway-proc][img06]

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:/sourcecodes/180008/get-gateway-proc.c

[article01]:https://whowin.gitee.io/post/blog/network/0014-handling-arp-cache/
[article02]:https://whowin.gitee.io/post/blog/network/0009-get-gateway-ip-via-netlink/

[img01]:https://whowin.gitee.io/images/180008/cat-proc-net-route.png
[img02]:https://whowin.gitee.io/images/180008/screenshot-of-ip-route.png
[img03]:https://whowin.gitee.io/images/180008/screenshot-of-tracing-ip-route.png
[img04]:https://whowin.gitee.io/images/180008/screenshot-of-route-n.png
[img05]:https://whowin.gitee.io/images/180008/screenshot-of-tracing-route-n.png
[img06]:https://whowin.gitee.io/images/180008/screenshot-get-gateway-proc.png

