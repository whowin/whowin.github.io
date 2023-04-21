---
title: "如何使用raw socket发送UDP报文"
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
  - 数据链路层编程
  - "raw socket"
draft: false
#references: 
# - [How to Calculate IP Header Checksum (With an Example)](https://www.thegeekstuff.com/2012/05/ip-header-checksum/)
# - [TCP/IP四层模型](https://www.cnblogs.com/BlueTzar/articles/811160.html)
# - [Linux网络编程（数据链路层）](https://blog.csdn.net/qq_36131611/article/details/118460589)
# - [linux基础编程 链路层socket](https://blog.csdn.net/ghostyu/article/details/7737966)
# - [Packet socket 和 sockaddr_ll](https://www.cnblogs.com/zhangshenghui/p/6097492.html)
# - [OSI七层与TCP/IP五层网络架构详解](http://www.2cto.com/net/201310/252965.html)
# - [data link layer programming in c](https://stackoverflow.com/questions/33073506/data-link-layer-programming-in-c)
# - [A Guide to Using Raw Sockets](https://www.opensourceforu.com/2015/03/a-guide-to-using-raw-sockets/)
# - [packet(7) — Linux manual page](https://man7.org/linux/man-pages/man7/packet.7.html)
# -      struct sockaddr_ll
# - [Send an arbitrary Ethernet frame using an AF_PACKET socket in C](http://www.microhowto.info/howto/send_an_arbitrary_ethernet_frame_using_an_af_packet_socket_in_c.html)
# - [Send an arbitrary IPv4 datagram using a raw socket in C](http://www.microhowto.info/howto/send_an_arbitrary_ipv4_datagram_using_a_raw_socket_in_c.html)
postid: 180006
---

使用raw socket发送报文比接收报文要复杂一些，因为需要在程序中构建数据链路层、网络层和传输层的报头，本文以发送UDP报文为例说明在使用raw socket时如何构建数据链路层、网络层和传输层的报头并发送报文，文中给出了完整的源程序；阅读本文需要掌握基本的IPv4下的socket编程方法，本文对初学者有一定难度。
<!--more-->

## 1. 前言
* 阅读本文前可以考虑先阅读一下我的另外一篇文章[《Linux下如何在数据链路层接收原始数据包》][article01]，那篇文章中已经介绍过的一些概念，本文中将不再赘述；下面仅罗列一些曾经在[《Linux下如何在数据链路层接收原始数据包》][article01]介绍过的技术要点；
* 发送数据时打开raw_socket
    ```C
    sock_raw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sock_raw == -1)
        printf("error in socket");
    ```
* 以太网报头结构(定义在头文件linux/if_ether.h中)
    ```C
    struct ethhdr {
        unsigned char  h_dest[ETH_ALEN];    /* destination eth addr  */
        unsigned char  h_source[ETH_ALEN];  /* source ether addr  */
        __be16         h_proto;             /* packet type ID field  */
    } __attribute__((packed));
    ```
* IP报头结构(定义在头文件linux/ip.h中)
    ```C
    struct iphdr {
        __u8    ihl:4,
                version:4;
        __u8    tos;
        __be16  tot_len;
        __be16  id;
        __be16  frag_off;
        __u8    ttl;
        __u8    protocol;
        __sum16 check;
        __be32  saddr;
        __be32  daddr;
        /*The options start here. */
    };
    ```
* UDP报头结构(定义在头文件linux/udp.h中)
    ```C
    struct udphdr {
        __be16    source;
        __be16    dest;
        __be16    len;
        __sum16    check;
    };
    ```

## 2. 其它技术要点
* struct ifreq
    > Linux支持标准的ioctl，使用ioctl，应用程序可以和Linux内核进行通信，从而可以获取网络设备的信息或者对网络设备进行设置，ioctl既可以用于普通的socket，也可以用于raw socket；应用程序使用ioctl时，需要把一个ifreq结构传递给ioctl，ioctl通过ifreq结构与应用程序交换数据；

    > struct ifreq定义在头文件<linux/if.h>中

    ```C
    struct ifreq {
    #define IFHWADDRLEN    6
        union
        {
            char   ifrn_name[IFNAMSIZ];     /* if name, e.g. "en0" */
        } ifr_ifrn;
        
        union {
            struct sockaddr ifru_addr;
            struct sockaddr ifru_dstaddr;
            struct sockaddr ifru_broadaddr;
            struct sockaddr ifru_netmask;
            struct sockaddr ifru_hwaddr;
            short  ifru_flags;
            int    ifru_ivalue;
            int    ifru_mtu;
            struct ifmap ifru_map;
            char   ifru_slave[IFNAMSIZ];    /* Just fits the size */
            char   ifru_newname[IFNAMSIZ];
            void * ifru_data;
            struct if_settings ifru_settings;
        } ifr_ifru;
    };
    ```
    > 简而言之，使用ifreq将设备名称(ifrn_name)传递给ioctl后，可以通过ioctl获取设备的索引号(index)，硬件地址(MAC)等一些信息；

    > man netdevice可以在线了解有关ifreq的更详细的信息

* ioctl的调用
    - ioctl的定义在sys/ioctl.h中定义
    - ioctl调用中可以允许的request在bits/ioctl.h中定义
    ```C
    int ioctl(int fd, unsigned long request, ...);
    ```

* 获取网络接口的索引号(ifr_ifindex)
    > 在发送数据之前，必须要确定从哪个网络接口发送数据，因为你的机器上可能有多个网络接口：有线网口、无线网口以及loopback，可以使用ifconfig命令查看所有接口的名称；

    ```C
    struct ifreq ifreq_index;

    memset(&ifreq_index, 0, sizeof(ifreq_index));
    strncpy(ifreq_index.ifr_name, "eth0", IFNAMSIZ - 1);
    if (ioctl(sock_raw, SIOCGIFINDEX, &ifreq_index) < 0)
        perror("ioctl() with SIOCGIFINDEX");
    else printf("index=%d\n", ifreq_index.ifr_ifindex);
    ```

    > 调用ioctl之前将设备名称(例中为eth0)，填写到ifreq结构中，使用SIOCGIFINDEX作为request，网络接口的设备索引号将返回在ifreq结构中

* 获取网络接口的MAC地址(ifr_hwaddr)
    ```C
    struct ifreq if_req;

    memset(&if_req, 0, sizeof(struct ifreq));
    strncpy(if_req.ifr_name, "eth0", IFNAMSIZ - 1);

    if ((ioctl(sock_raw, SIOCGIFHWADDR, &if_req)) < 0) {
        perror("ioctl() with SIOCGIFHWADDR");
        exit;
    }

    int i;
    unsigned char mac[6];
    for (i = 0; i < 6; ++i) 
        mac[i] = (unsigned char)(if_req.ifr_hwaddr.sa_data[i]);

    printf("Mac = %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
    ```

    > 与上面类似，使用SIOCGIFHWADDR作为request，网络接口的MAC地址将返回在ifreq结构中

* 获取网络接口的IP地址(ifr_addr)
    ```C
    struct ifreq if_req;
    char ip[16] = {0};

    memset(&if_req, 0, sizeof(struct ifreq));
    strncpy(if_req.ifr_name, "eth0", IFNAMSIZ - 1);
    if (ioctl(sock_raw, SIOCGIFADDR, &if_req) < 0) {
        perror("ioctl() with SIOCGIFADDR");
    } else {
        strcpy(ip, 
               inet_ntoa((((struct sockaddr_in *)&(if_req.ifr_addr))->sin_addr)));
        printf("IP address: %s\n", ip);
    }
    ```

    > 与上面类似，使用SIOCGIFADDR作为request，网络接口的MAC地址将返回在ifreq结构中

## 3. 构建各层报头
* 构建以太网报头
    > 在获得了网络接口的索引号、MAC和IP地址后，就可以构造报头并发送报文了，本例中我们仅发送一组简单的UDP报文：hello

    > 首先要在内存中分配一块内存，用以存放以太网报头、IP报头、UDP报头和报文，以太网报头14个字节，IP报头20个字节，UDP报头8个字节，在加上报文的5个字节，所以在内存中分配一块64字节的空间已经足够

    ```C
    send_buf = (unsigned char*)malloc(64);
    memset(send_buf, 0, 64);
    ```

    > 构建以太网报头，需要在报头中填写源MAC地址、目的MAC地址，以及上一层报头的协议，源MAC地址上面已经介绍如何使用ioctl获得，上一层协议也很清楚是IP协议，麻烦的是如何填写目的MAC地址，我们这个例子中，目的地址和源地址在一个局域网内，我们有各种办法可以获得目的地址的MAC地址，但是很多情况下我们只知道目的IP地址，并不知道目的MAC地址，很遗憾这个问题我们并不想在本文中进行讨论，或许今后会另写一篇文章讨论这个问题，其实，不管实际的目的地址的MAC地址是什么，我们只要在目的MAC地址处填上路由器的MAC地址，这个问题就可以完美解决，路由器会帮助我们填上正确的MAC地址，当然，找到路由器的MAC地址也是要花费一点功夫的，这个问题我们也不在本文中讨论；

    > 为了简单起见，我们假定我们已经知道了目的MAC地址，并将其定义在常数：DEST_MAC_0~~DEST_MAC_5中；

    ```C
    struct ethhdr *eth_hdr = (struct ethhdr *)(send_buf);
    
    int i = 0;
    for (i = 0; i < 6; ++i) eth_hdr->h_source[i] = mac[i];
    
    /* filling destination mac. 
       DEST_MAC_0 to DEST_MAC_5 are macro having octets of mac address. */
    eth_hdr->h_dest[0] = DEST_MAC_0;
    eth_hdr->h_dest[1] = DEST_MAC_1;
    eth_hdr->h_dest[2] = DEST_MAC_2;
    eth_hdr->h_dest[3] = DEST_MAC_3;
    eth_hdr->h_dest[4] = DEST_MAC_4;
    eth_hdr->h_dest[5] = DEST_MAC_5;
    
    eth_hdr->h_proto = htons(ETH_P_IP); // means next header is IP header
    ```

* 构建IP报头
    > 和构建以太网报头类似，将iphdr结构中的字段填上即可；

    > id这个字段可以是任意一个唯一的数字，在IP包传输过程中要保持唯一，当一个IP包过长需要分片传输时，这个id对分片重组有着重要的意义；对于ipv4而言，version字段必须填4；ttl字段最大可以填255，每经过一个路由器时，该字段会被减1，当ttl=0时，该数据包将被丢弃，用于防止一个数据包在网络上永远不消失；protocol字段和以太网头中的h_proto的含义不同，为上一层协议号，各种协议的协议号定义在文件/etc/protocols中；

    ```C
    struct iphdr *ip_hdr = (struct iphdr *)(send_buf + sizeof(struct ethhdr));
    ip_hdr->ihl = 5;            // Internet Header Length - 20 bytes
    ip_hdr->version = 4;        // ipv4
    ip_hdr->tos = 0;            // Type Of Service - fill 0
    ip_hdr->id = htons(32501);  // any number
    ip_hdr->ttl = 64;           // Time To Live
    ip_hdr->protocol = 17;      // protocol number - 17 represents UDP protocol
    ip_hdr->saddr = inet_addr(ip);          // source IP address
    ip_hdr->daddr = inet_addr(DEST_IP);     // put destination IP address
    ```

    > 至此，整个IP报头还有三个字段没有填：frag_off这个字段是用于分片传输的，本例并不需要分片传输，可以不用填这个字段；tot_len这个字段表示总长度，包括IP头和IP payload，因为还没有填IP payload，所以还没有办法填这个字段，留在后面再填；check这个字段目前也无法填，因为IP头还没有填完，暂时还无法计算checksum，留待后面计算。

* 构建UDP报头
    > 和构建IP报头类似，填充udphdr结构中的字段即可构建UDP报头；

    ```C
    struct udphdr *udp_hdr = (struct udphdr *)(send_buf + 
                                               sizeof(struct iphdr) + 
                                               sizeof(struct ethhdr));
    
    udp_hdr->source = htons(34561);
    udp_hdr->dest = htons(34562);
    udp_hdr->check = 0;
    ```

    > UDP报头中的check字段不是强制的，可以不用，填0即可；和IP报头一样，UDP报头中有一个len字段，这个长度字段包含UDP报头的长度和UDP payload的长度，所以在填完payload之前还无法填写这个字段。

* 构建要发送的数据
    ```C
    total_len = sizeof(struct ethhdr) + 
                sizeof(struct iphdr) + 
                sizeof(struct udphdr);
    send_buf[total_len++] = 'h';
    send_buf[total_len++] = 'e';
    send_buf[total_len++] = 'l';
    send_buf[total_len++] = 'l';
    send_buf[total_len++] = 'o';
    ```

* 填充IP和UDP报头中的长度字段
    ```C
    // UDP length field
    udp_hdr->len = htons(total_len - 
                         sizeof(struct iphdr) - 
                         sizeof(struct ethhdr));
    // IP length field
    ip_hdr->tot_len = htons(total_len - sizeof(struct ethhdr));
    ```

* IP报头中的checksum计算
    > 在IP报头中还有一个check字段没有填，关于这个字段的计算请参考我的另外两篇文章[《如何计算IP报头的checksum》][article02]和[《如何计算UDP头的checksum》][article03]；checksum字段是用于错误检测的，当报文经过一个路由器时，路由器会重新计算IP报文的checksum，并与IP报头中的checksum进行比较，如果不一致，该报文将被丢弃，否则，路由器会把IP报头中ttl字段减1，然后转发这个报文；

    ```C
    unsigned short checksum(unsigned short *buff, int _16bitword) {
        unsigned long sum;
        for (sum = 0; _16bitword > 0; _16bitword--)
            sum += htons(*(buff)++);
        sum = ((sum >> 16) + (sum & 0xFFFF));
        sum += (sum>>16);
        return (unsigned short)(~sum);
    }
    
    ip_hdr->check = checksum((unsigned short *)(send_buf + sizeof(struct ethhdr)), 
                             (sizeof(struct iphdr) / 2));
    ```

## 4. 发送数据
* 我们已经将报文组织好了，我们在发送数据的外面包装上了UDP报头、IP报头和以太网报头，但在发送之前，我们还需要了解sockaddr_ll结构，并使用目的MAC地址填充其中的字段；
* sendto的定义
    ```C
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen);
    ```
    > 通常情况下在应用层发送数据时，dest_addr会指向一个sockaddr_in结构，在这个结构中填入目的端口和目的IP后，即可调用sendto；但是当我们在数据链路层使用sendto发送数据时，dest_addr要指向一个sodkaddr_ll结构，ll即为link-layer的意思，在其中要填上网络接口的索引号、目的MAC地址等数据链路层的信息，然后才能调用sendto发送数据；
* sockaddr_ll结构
    > 这个结构在linux/if_packet.h中定义，有关该结构的详细说明请参考其它文章，本文仅就相关字段做出说明；

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

    > 当要在数据链路层发送数据时，需要填sll_family、sll_protocol、sll_ifindex、sll_halen和sll_addr，其它字段填0即可；在接收到数据包时会填写sll_hatype和sll_pkttype；其中sll_family为协议族，和建立raw socket是使用的协议族要一致，所以肯定是AF_PACKET(PF_PACKET)，sll_protocol是标准的以太网协议类型，定义在头文件linux/if_ether.h中，默认为socket的协议，可以和建立socket时的协议一致，也可以不填；sll_ifindex是网络接口的索引号，我们可以根据接口名称使用ioctl获得；sll_halen是硬件地址(MAC)的长度，ha是Hardware Address的意思，填常数ETH_ALEN(定义在头文件linux/if_ether.h中)；sll_addr是目的MAC地址。

    > 实际上，在发送数据时，由于sll_family和sll_protocol都是和socket中一样的，所以都可以不填，只要填sll_ifindex、sll_halen和sll_addr即可。

* 构建sockaddr_ll结构
    ```C
    struct sockaddr_ll saddr_ll;
    saddr_ll.sll_ifindex = ifreq_index.ifr_ifindex;     // index of interface
    saddr_ll.sll_halen   = ETH_ALEN;      // length of destination mac address
    saddr_ll.sll_addr[0] = DEST_MAC_0;
    saddr_ll.sll_addr[1] = DEST_MAC_1;
    saddr_ll.sll_addr[2] = DEST_MAC_2;
    saddr_ll.sll_addr[3] = DEST_MAC_3;
    saddr_ll.sll_addr[4] = DEST_MAC_4;
    saddr_ll.sll_addr[5] = DEST_MAC_5;
    ```

* 发送数据
    ```C
    send_len = sendto(sock_raw, send_buf, 64, 0, 
                      (const struct sockaddr *)&saddr_ll, 
                      sizeof(struct sockaddr_ll));
    if (send_len < 0) {
        perror("sendto()");
        return -1;
    }
    ```
## 5. 完整的源程序
* 在这个实例中，目的IP地址为：192.168.2.112，目的MAC地址为：00:21:cc:d8:30:4b；源IP地址和MAC地址我们将从程序中得到；网络接口名称为：enp0s3；源端口号为：34561，目的端口号为：34562
* 这个程序需要使用root权限运行，因为使用了raw socket
* 下面是完整的源程序，文件名为：[send-raw-udp-packet.c][src01](**点击文件名下载源程序**)
* 运行程序
    1. 编译程序
        ```bash
        gcc -Wall send-raw-udp-packet.c -o send-raw-udp-packet
        ```
    2. 在目的电脑(192.168.2.112)上启动一个监听程序，监听UDP的34562端口，因为我们的程序会向这个端口发送一个UDP报文，内容是：hello；这里我们使用netcat命令，关于这个命令的介绍，请参考我的另一篇文章[《如何在Linux命令行下发送和接收UDP数据包》][article04]
        ```bash
        nc -u -l 34562
        ```
    3. 在源电脑上使用root权限启动程序
        ```bash
        sudo ./send-raw-udp-packet
        ```
    4. 在目的电脑(192.168.2.112)上应该可以看到发过来的数据：hello

## 6. 结束语
* 平常进行网络编程，大多是在应用层编程，基本上不会接触到数据链路层、网络层和传输层，这个程序实际上是在数据链路层上直接发送数据，可以让我们对网络模型及各层的工作原理有更深入的了解，掌握了这种编程方式，可以编写出更加复杂的网络程序；
* 在封装各层报头的过程中，实际上唯一比较让人头疼的就是目的MAC地址，本文受篇幅所限略去了这部分的讨论；
* 可以修改一下程序，尝试使用你的默认网关的MAC地址代替目的MAC地址，正常情况下报文也是可以送达的；
* 如果你可以在互联网上找到一台服务器，可以尝试向局域网外发送数据，同样，建议你将目的MAC地址填上默认网关的MAC地址，特别要注意的是要确认服务器上的防火墙放开了你在程序中设置的目的端口号，在服务器上启动netcat命令监听目的端口，在你自己的机器上运行程序，正常情况下，报文是可以送达的。


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

[src01]:https://whowin.gitee.io/sourcecodes/180006/send-raw-udp-packet.c

[article01]:https://whowin.gitee.io/post/blog/network/0002-link-layer-programming/
[article02]:https://whowin.gitee.io/post/blog/network/0004-checksum-of-ip-header/
[article03]:https://whowin.gitee.io/post/blog/network/0003-checksum-of-udp-header/
[article04]:https://whowin.gitee.io/post/blog/network/0005-send-udp-via-linux-cli/
