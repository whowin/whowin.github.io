---
title: "使用raw socket发送magic packet"
date: 2023-02-09T16:43:29+08:00
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
  - "raw Socket"
  - 网络编程
draft: false
#references: 
# - [wikipidia-网络唤醒](https://zh.wikipedia.org/zh-cn/%E7%B6%B2%E8%B7%AF%E5%96%9A%E9%86%92)
# - [wikipedia-Wake-On-Lan](https://en.wikipedia.org/wiki/Wake-on-LAN)

postid: 180015
---

Magic Packet是进行网络唤醒的数据包，将这个数据包以广播的形式发到局域网上，与数据包中所关联的MAC相同的电脑就会被唤醒开机，通常我们都是使用UDP报文的形式来发送这个数据包，但实际上在进行网络唤醒的时候，只要报文中包含Magic Packet应该就可以唤醒相关的计算机，与IP协议、UDP协议没有任何关系，本文将试图抛开网络层(IP层)和传输层(TCP/UDP层)，直接在数据链路层发出Magic Packet，并成功实现网络唤醒，本文将提供完整的源代码。阅读本文需要有较好的网络编程基础，本文对网络编程的初学者有一定难度。
<!--more-->

## 1. Magic Packet
* 我比较喜欢把这个数据包称作"网络唤醒包"；有很多地方把它翻译成"魔术包"或者"魔法数据包"，我个人觉着太过表面，无法表达其实际的含义；本文将这个数据包称为"网络唤醒包"或者"Magic Packet"，二者具有完全相同的含义；
* 以前写过一篇与嵌入式相关的文章[《远程开机：一个简单的嵌入式项目开发》][article01]，在嵌入式环境下使用Magic Packet进行远程开机的小项目，有兴趣的读者可以参考；
* ```Magic Packet``` 就是一个指定格式的数据包，其格式为：6 个 **0xff**，然后16组需要被网络唤醒的电脑的 **MAC** 地址，比如需要被唤醒的电脑的 MAC 为：```00:e0:2b:69:00:03```，则 ```Magic Packet``` 为(16进制表述)：
    ```plaintext
    ff ff ff ff ff ff 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    ```
* 关于 *Magic Packet* 的更多的信息请参考
  - [wikipidia-网络唤醒][article05]
  - [wikipedia-Wake-On-Lan][article06]
* 对Magic Packet也没有什么好解释的，理论上只要一个报文中存在这个Magic Packet，那么有网络唤醒功能的NIC都会相应，What is NIC? NIC就是网卡，Network Interface Controller；
* 但是大多数的 802.11 的无线网卡是收不到 Magic Packet 的，这一点在[wikipedia-Wake-On-Lan][article06]上有明确的说明；所以不要尝试在无线网卡上做网络唤醒，但是有一个叫做**WoWLAN**的标准是专门支持无线网卡的网络唤醒的，以后有时间的时候试一下；
* 通常发送Magic Packet是使用UDP广播的方式发，这方面的文章很多，有兴趣的读者可以百个度或者谷个歌去找一下，我的另一篇文章[《远程开机：一个简单的嵌入式项目开发》][article01]也是用这种方式发的，这里就不赘述了；

## 2. 相关技术要点
> 以下要点将会在本文的范例程序中用到，在此需要简单回顾以下。

* **TCP/IP的五层网络模型**(OSI 七层架构的简化版)
  1. 应用层
  2. 传输层(TCP/UDP)
  3. 网络层(IP)
  4. 数据链路层(Ethernet)
  5. 物理层

* **基于TCP/IP的数据报文的结构**
  - 一个基于TCP/IP的完整报文结构为：以太网报头 + IP报头 + TCP/UDP报头 + data，如下图：
  
  ![Packet structure based on TCP/IP][img01]

  -------------

* **以太网报头**
  - 以太网报头定义在头文件<linux/if_ether.h>中：
    ```C
    struct ethhdr {
        unsigned char  h_dest[ETH_ALEN];    /* destination eth addr  */
        unsigned char  h_source[ETH_ALEN];  /* source ether addr  */
        __be16         h_proto;             /* packet type ID field  */
    } __attribute__((packed));
    ```
  - h_dest字段为目的MAC地址，h_source字段为源MAC地址；
  - h_proto表示当前数据包在网络层使用的协议，Linux支持的协议在头文件<linux/if_ether.h>中定义；通常在网络层使用的IP协议，这个字段的值是0x0800(ETH_P_IP)；
  - 但是本文中的 **h_proto** 字段要填写 **0x842**，很遗憾这个协议在头文件中没有定义，也基本找不到相关资料；

* **raw socket**
  - 可以参考我的另两篇文章[《Linux下如何在数据链路层接收原始数据包》][article02]和[《如何使用raw socket发送UDP报文》][article03]，这里仅做一个简单回顾；
  - 打开一个raw socket
    ```C
    int sock_raw;
    sock_raw = socket(AF_PACKET, SOCK_RAW, 0);
    ```
  - 第三个参数还可以有其它选择，这个参数往往会对socket的接收产生影响，本文并不接收任何信息，所以对本文来说无关紧要。

* **struct sockaddr_ll**
  - 这个结构在<linux/if_packet.h>中定义，有关该结构的详细说明请参考其它文章，本文仅就相关字段做出说明；
  - 这个结构与 IPv4 socket 编程中的结构(struct sockaddr_in)的作用类似，是用在raw socket上的一个地址结构，烦请自行理解，其中'll'表示Low Level
    ```C
    struct sockaddr_ll {
        unsigned short  sll_family;
        __be16          sll_protocol;
        int             sll_ifindex;
        unsigned short  sll_hatype;     // Hardware Address Type
        unsigned char   sll_pkttype;    // Packet Type
        unsigned char   sll_halen;      // Hardware Address Length
        unsigned char   sll_addr[8];    // Address(Hardware Address)
    };
    ```
  - sll_family为协议族，和建立raw socket是使用的协议族要一致，所以肯定是AF_PACKET；
  - sll_protocol是标准的以太网协议类型，定义在头文件<linux/if_ether.h>中，通常情况下应该 ETH_P_IP(0x800) 表示IP协议，本文要填 **0x842**；
  - sll_ifindex是网络接口的索引号，我们可以根据接口名称使用ioctl获得；
  - sll_halen是硬件地址(MAC)的长度，ha是Hardware Address的意思，填常数 ETH_ALEN(定义在头文件<linux/if_ether.h>中)；
  - sll_addr是目的MAC地址
  - 实际上，在发送数据时，由于sll_family和sll_protocol都是和socket中一样的，所以大多数情况下都可以不填，只要填sll_ifindex、sll_halen和sll_addr即可，但是本文的例子中，如果使用bind()绑定地址，应该尽量完整地填写，否则执行bind()是会出错。

## 3. 在数据链路层发送Magic Packet
* 先说一下目标，通常使用UDP发送Magic Packet的报文结构为：以太网报头 + IP报头 + UDP报头 + Magic Packet，我们的目标是：**以太网报头 + Magic Packet**
* 先看源程序，文件名为：[magic-packet.c][src01](**点击文件名下载源程序**)

* 报文是以广播的形式发出去的，发送广播时，以太网报头的目的MAC要全部填写0xff，(struct sockaddr_ll)中的sll_addr也要全部填写0xff；
* 尽管我们知道目的MAC，但是这个报文必须以广播报文发出，因为此时被唤醒的机器并没有开机，所以无法通过arp获知填写的MAC地址是否在局域网内，所以如果在以太网报头的h_dest中填上了要被唤醒的机器的MAC地址，报文是无法送达的；
* 关于使用ioctl获取接口索引号(Interface Index)和接口MAC的问题，请自行查找资料，或者参考文章[《如何使用raw socket发送UDP报文》][article03]，这篇文章中有部分内容涉及到这个问题；
* 程序中写好了用send()或者用sendto()发送报文的代码，使用一个常量USING_SENDTO来控制，当USING_SENDTO为1时，将使用sendto()发送报文，否则使用send()发送报文；
* 如果使用send()发送报文，需要使用bind()绑定目的地址；
* 具体实践中，如果使用sendto()发送报文，(struct sockaddr_ll)中只需填写sll_ifindex即可，这个也许和运行环境有关，请自行测试；理论上说，只要编译通过，运行没有出错，就表示这个Magic Packet发送了出去，应该就可以起作用；
* 源程序中有详细的注释，其它也没有什么更多解释的。
* **编译**：```gcc -Wall magic-packet.c -o magic-packet```
* **运行**：```sudo ./magic-packet enp0s3 00:e0:2b:68:00:03```
* 由于使用了raw socket，所以必须以**root**权限运行；
* 如果你填写的ifname和mac都正确的话，mac所对应的电脑应该被远程唤醒；
* 这个程序并不好调试，因为能够在数据链路层侦听的工具不多，加上使用的协议号为0x842，使得大多数工具都无法使用，**wireshark应该是可以的**，而且[wireshark的wiki][article04]上说，它有专门针对0x842号协议的侦听，不过我没有试过，有感兴趣的读者可以试一下；
* 远程唤醒是需要硬件支持的，主板和网卡都要支持，但是目前绝大多数有线网卡都应该是支持的，但是可能要在BIOS和其它地方做一些设置，请自行搜索相关资料，并参考我的另一篇文章[《远程开机：一个简单的嵌入式项目开发》][article01]
* 运行结果截图：

  ![Screenshot magic_packet][img02]

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://whowin.gitee.io/sourcecodes/180015/magic-packet.c

[article01]:https://whowin.gitee.io/post/blog/network/embedded/0001-wake-on-lan/
[article02]:https://whowin.gitee.io/post/blog/network/0002-link-layer-programming/
[article03]:https://whowin.gitee.io/post/blog/network/0006-send-udp-with-raw-socket/
[article04]:https://wiki.wireshark.org/WakeOnLAN
[article05]:https://zh.wikipedia.org/zh-cn/%E7%B6%B2%E8%B7%AF%E5%96%9A%E9%86%92
[article06]:https://en.wikipedia.org/wiki/Wake-on-LAN

[img01]:https://whowin.gitee.io/images/180002/sending_data_from_app_with_socket.png
[img02]:https://whowin.gitee.io/images/180015/screenshot-magic-packet.png
