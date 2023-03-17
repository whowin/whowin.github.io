---
title: "Linux下如何在数据链路层接收原始数据包"
date: 2022-12-07T16:43:29+08:00
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
# - [OSI七层与TCP/IP五层网络架构详解](http://www.2cto.com/net/201310/252965.html)
# - [data link layer programming in c](https://stackoverflow.com/questions/33073506/data-link-layer-programming-in-c)
postid: 180002
---

大多数的网络编程都是在应用层接收数据和发送数据的，程序员无需关注报文的各种报头，网络协议栈会解决这些问题，本文介绍在数据链路层的网络编程方法，介绍如何在数据链路层直接接收从物理层发过来的原始数据包，要得到数据，必须自己解开数据链路层、网络层和传输层的报头，文章给出了一个完整的范例程序，希望本文能帮助读者对网络通信有更深刻的理解。
<!--more-->

## 1. 概述
* linux下进行网络编程通常都是使用socket在应用层接收和发送数据；
* 本文介绍如何绕过数据链路层、网络层和传输层对数据包的处理，直接从数据链路层接收从物理层发过来的原始数据；
* 本文所介绍的内容在实际编程中很少会用到，但希望对读者理解网络结构和协议能有帮助；
* 本文会提供了直接从数据链路层接收数据的范例程序，源代码在ubuntu 20.04下编译运行成功；
* 本文可能并不适合初学者，但并不妨碍初学者收藏此文，以便在今后学习。

## 2. socket编程
* 在看下面的内容之前还是要简单地回顾一下TCP/IP的五层网络模型(OSI 七层架构的简化版)
  1. 应用层
  2. 传输层
  3. 网络层
  4. 数据链路层
  5. 物理层

* 使用socket进行网络编程时，我们通常只需要关心需要发送的数据，数据发送后，要发送的数据将从应用层向下传递
  - 在TCP/UDP(传输)层加入一个TCP头
  - 在IP(网络)层加上一个IP头
  - 在数据链路层加上一个以太网头
  - 交给物理层传输

* 当我们在应用层进行socket编程时，我们通常会这样发送数据(以UDP为例)：
  ```C
  ......
  struct sockaddr_in addr;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  ......
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(DST_IP);       // 目的IP
  addr.sin_port = htons(PORT);                    // 端口号
  .....
  sendto(sock, &DATA, DATA_LEN, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  close(fd);
  ```
* 当我们把DATA给sendto(......)以后，会发生什么呢？
  1. 数据从应用层被送到传输层，传输层给这个数据加上一个UDP 头；
  2. (UDP头+DATA)从传输层被送到网络层，IP协议会给数据包再加上一个IP头；
  3. (IP头+UDP头+DATA)从网络层被送到了数据链路层，数据链路层的以太网协议会给这个数据包加上一个以太网头；
  4. (以太网头+IP头+UDP头+DATA)从数据链路层被送到了物理层，数据就被发送走了。

  ![使用socket从应用程序发送数据的过程][img01]

  <center><b>图1：使用socket从应用程序发送数据的过程</b></center>

********************************  
* 当我们在应用层进行socket编程时，我们通常会这样接收数据(以UDP为例)：
  ```C
  ......
  struct sockaddr_in addr;
  int addr_len = sizeof(struct sockaddr_in);
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  ......
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  ......
  recvfrom(sock, buffer, sizeof(buffer), 0, &addr, &addr_len);
  ......

  ```
* 当我们调用recvfrom()函数并成功返回时，都发生了什么事情呢？
  1. 原始数据包(以太网头+IP头+UDP头+DATA)通过网卡驱动程序发送到数据链路层；
  2. 数据链路层从原始数据包中提取出以太网头，数据包的其余部分发送给网络层(IP头+UDP头+DATA)；
  3. 网络层从数据中提取出IP头，其余部分交给传输层(UDP头+DATA)；
  4. 传输层从数据中提取出UDP头，其余部分交给应用程序(DATA)；
  5. 所以我们在应用层收到的就只有数据了，报头已经被各协议层提取出来
  
  ![在应用程序中用socket接收数据的过程][img02]

  <center><b>图2：在应用程序中用socket接收数据的过程</b></center>

*********************************
* 很显然，在应用层进行网络编程，我们不需要关心各协议层的报头，各层的协议栈会为我们处理好所有报头；
* 但这样的编程显然也是受限的，除了TCP和UDP以外，你还知道有什么其它的网络通信形式吗？这种在应用层的编程仅能收到发给这台机器的数据，而且在你收到的数据中，并没有源和目的地址的任何信息。
* 从图1和图2可以看出，当我们需要在传输层编程时，实际上就是比在应用层编程多了一个UDP(TCP)头；同理，当我们需要在网络层编程时，也就是比在传输层编程多加一个IP头；
* 本文介绍在数据链路层编程，与在应用层的网络编程相比，只是要多封装(提取)三个数据头：以太网头、IP头、UDP(TCP)头

## 3. raw socket
* raw socket也是一种socket，常用于接收原始数据包，所谓原始数据包指的是从物理层直接传送出来的数据包；使用raw socket可以绕过通常的TCP/IP处理流程，在应用程序中直接收到原始数据包(见图3)。使用raw socket编程，并不需要对Linux内核有深入的了解。

  ![在应用程序中使用raw socket接收数据][img03]

  <center><b>图3：在应用程序中使用raw socket接收数据</b></center>

****************
* 打开raw socket
  - 和普通socket一样，打开一个raw socket，必须要知道三件事：socket family、socket type 和 protocol；
  - 对raw socket而言，socket family为AF_PACKET，socket type为SOCK_RAW；
  - 接收数据时，protocol请参考头文件if_ether.h；接收所有数据包，protocol使用宏ETH_P_ALL；接收IP数据包，protocol使用宏ETH_P_IP。

    ```C
    int sock_raw;
    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        printf("error in socket\n");
        return -1;
    }
    ```
  - 发送数据时，protocol要参考头文件in.h，通常protocol使用IPPROTO_RAW；
    ```C
    sock_raw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sock_raw == -1)
        printf("error in socket");
    ```

## 4. 数据报的报头
* 前面提过，应用程序使用socket发送数据(以UDP为例)的时候，在经过传输层时，要增加一个UDP头，经过网络层时，要再加上一个IP头，在经过数据链路层时，还要加上一个以太网头，然后才能交给物理层发送，见图1；
* 同样，应用程序使用socket接收数据(以UDP为例)时，数据从物理层经过数据链路层时，将去除以太网头，在经过网络层时，要去掉IP头，在经过传输层时，还要去掉UDP头，所以到达应用程序时，就只有数据了，见图2；
* 当使用raw_socket在数据链路层编程时，收到的数据需要自行解开以太网头、IP头、UDP头；而发送数据时，需要自行在数据上封装UDP头、IP头和以太网头；
* **网络报文的报头的通用定义**
  - 网络报文的报头分为三个部分：传输层的传输层协议头、网络层的网络层协议头和数据链路层的以太网头，见图4；

  ![网络报头的通用定义][img04]

  <center><b>图4：网络报头的通用定义</b></center>

  *****************************
  - 以下仅就本文范例中用到的报头结构做一个简单说明。

* **数据链路层的以太网头**
  - 以太网报头定义在头文件linux/if_ether.h中：
    ```C
    struct ethhdr {
        unsigned char  h_dest[ETH_ALEN];    /* destination eth addr  */
        unsigned char  h_source[ETH_ALEN];  /* source ether addr  */
        __be16         h_proto;             /* packet type ID field  */
    } __attribute__((packed));
    ```
  - h_dest字段为目的MAC地址，h_source字段为源MAC地址；
  - h_proto表示当前数据包在网络层使用的协议，Linux支持的协议在头文件linux/if_ether.h中定义；通常在网络层使用的IP协议，这个字段的值是0x0800(ETH_P_IP)；

* **网络层的 IP 头**
  - IP(Internet Protocol)协议是网络层最常用的协议；
  - IP报头定义在头文件linux/ip.h中；
    ```C
    struct iphdr {
    #if defined(__LITTLE_ENDIAN_BITFIELD)
        __u8  ihl:4,
              version:4;
    #elif defined (__BIG_ENDIAN_BITFIELD)
        __u8  version:4,
              ihl:4;
    #else
    #error  "Please fix <asm/byteorder.h>"
    #endif
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
    ![IP 报头][img05]

    <center><b>图5：IP 报头</b></center>

  ******************************
  - version - IPV4时，version=4
  - ihl(Internet Header Length) - 报头的长度，表示报头占用多少个32 bits字(4 字节)，IP报头最少要20 bytes，也就是ihl=5，最长可以是60 bytes，也就是ihl=15；ihl x 4就是IP报头占用的字节数；
  - tos - 这个字段通常并不使用，可以填0；
  - tot_len(Total Length) - 报文全长，包括IP头和IP payload，单位是字节；
  - id - IP报文的唯一标识，同一个IP报文分片传输时，其id是一样的，便于分片重组；
  - frag_off(Fragment Offest) - 其中bit 0、bit 1 和 bit 2用于控制和识别分片，bit 3 - 15这13个bit表示每个分片相对于原始报文开头的偏移量，以8字节作单位；
  - ttl(Time To Live) - 这个字段是为了防止报文在互联网上永远存在(比如进入路由环路)，在发送报文时设置这个值，最大255，通常设置为64，每经过一个路由器，该值将减1，当为0时，该报文将被丢弃；
  - protocol - 该字段定义了在传输层所用的协议，协议号列表文件在/etc/protocols文件中，UDP为17，TCP为6，其取值定义在头文件linux/in.h中；
  - check - IP头的检查和，不包括payload，关于IP头的检查和的计算方法有专门的文章介绍，开一参考[这里](https://www.thegeekstuff.com/2012/05/ip-header-checksum/)，也可以参考本文的范例源代码；
  - saddr - 源IP地址，此字段是一个4字节的IP地址转为二进制并拼在一起所得到的32位值；例如：10.9.8.7是00001010 00001001 00001000 00000111 
  - daddr - 目的IP地址，表示方法与saddr一样；
  - 当数据链路层的h_proto字段为ETH_P_IP时，表示网络层使用的是IP(Internet Protocol)协议；实际上，网络层支持一些其它的协议，比如：Ethernet Loopback、Xerox PUP等；
  - 网络层和传输层支持的协议可以在文件/etc/protocols中查看。

* **传输层的 UDP 头**
  - UDP(User Datagram Protocol)是传输层最常用的协议之一；
  - UDP头定义在头文件linux/udp.h中；
    ```C
    struct udphdr {
        __be16	source;
        __be16	dest;
        __be16	len;
        __sum16	check;
    };
    ```
  - source - 来源连接端口号，可选项，如果不使用，填充0；
  - dest - 目的连接端口号；
  - len - 报文长度；
  - check - 报头的校验和，在IPv4中是可选的，IPv6中是强制的，如果不使用，应填充0；校验和的计算还涉及到UDP的伪头部，请参考[相关文章](https://www.ques10.com/p/10930/how-is-checksum-computed-in-udp-1/)；

## 5. 使用 raw socket 接收数据
* 把上面介绍的内容综合起来就可以编写出一个在数据链路层使用raw socket接收原始数据包的程序了；
* 以接收一个UDP数据包为例说明接收数据的步骤：
  1. 打开一个raw socket；
  2. 在内存中分配一个buffer，并接收数据；
  3. 提取数据链路层的以太网协议头；
  4. 提取解开网络层的IP协议头；
  5. 提取解开传输层的UDP协议头；
  6. 提取收到的数据

* 下面是一个监听UDP数据包的范例程序，文件名receive_udp_packet.c
  ```C
  #include <stdio.h>
  #include <unistd.h>
  #include <string.h>
  #include <signal.h>
  #include <malloc.h>

  #include <sys/socket.h>
  #include <sys/types.h>

  #include <arpa/inet.h>           // to avoid warning at inet_ntoa

  #include<linux/if_packet.h>
  #include <linux/in.h>
  #include <linux/if_ether.h>
  #include <linux/ip.h>
  #include <linux/udp.h>
  #include <linux/tcp.h>

  #define LOG_FILE       "udp_packets.log"    // log file name

  struct ethhdr *eth_hdr;
  struct iphdr *ip_hdr;
  struct udphdr *udp_hdr;

  /*****************************************************************************
  * Function: unsigned int ethernet_header(unsigned char *buffer, int buflen)
  * Description: Extracting the Ethernet header
  *              struct ethhdr is defined in if_ether.h
  * 
  * Entry: buffer     data packet
  *        buf_len    length of data packet
  * Return: protocol of network layer or -1 when error
  *****************************************************************************/
  int ethernet_header(unsigned char *buffer, int buf_len) {
      if (buf_len < sizeof(struct ethhdr)) {
          printf("Wrong data packet.\n");
          return -1;
      }

      eth_hdr = (struct ethhdr *)(buffer);

      return ntohs(eth_hdr->h_proto);
  }
  /*********************************************************************************
  * Function: void log_ethernet_header(FILE *log_file, struct ethhdr *eth_hdr)
  * Description: write ether header into log file
  * 
  * Entry:   log_file    log file object
  *          eth_hdr     pointer of ethernet header structure
  *********************************************************************************/
  void log_ethernet_header(FILE *log_file, struct ethhdr *eth_hdr) {
      fprintf(log_file, "\nEthernet Header\n");
      fprintf(log_file, "\t|-Source MAC Address     : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", 
              eth_hdr->h_source[0], eth_hdr->h_source[1], eth_hdr->h_source[2], 
              eth_hdr->h_source[3], eth_hdr->h_source[4], eth_hdr->h_source[5]);
      fprintf(log_file, "\t|-Destination MAC Address: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", 
              eth_hdr->h_dest[0], eth_hdr->h_dest[1], eth_hdr->h_dest[2], 
              eth_hdr->h_dest[3], eth_hdr->h_dest[4], eth_hdr->h_dest[5]);
      fprintf(log_file, "\t|-Protocol               : 0X%04X\n", ntohs(eth_hdr->h_proto));
      // ETH_P_IP = 0x0800, ETH_P_LOOP = 0X0060
  }
  /********************************************************************************
  * Function: unsigned int ip_header(unsigned char *buffer, int buf_len)
  * Description: Extracting the IP header
  *              struct iphdr is defined in ip.h
  * 
  * Entry: buffer     data packet
  *        buf_len    length of data packet
  * return: protocol of transport layer or -1 when error
  ********************************************************************************/
  int ip_header(unsigned char *buffer, int buf_len) {
      if (buf_len < sizeof(struct ethhdr) + 20) {
          printf("Wrong data packet.\n");
          return -1;
      }

      ip_hdr = (struct iphdr *)(buffer + sizeof(struct ethhdr));
      int tot_len = ntohs(ip_hdr->tot_len);

      if (buf_len < sizeof(struct ethhdr) + tot_len) {
          printf("Wrong data packet.\n");
          return -1;
      }
      return (int)ip_hdr->protocol;
  }
  /********************************************************************************
  * Function: void log_ip_header(FILE *log_file, struct iphdr *ip_hdr)
  * Description: write ip header into log file
  * 
  * Entry:   log_file        log file's handler
  *          ip_hdr          the pointer of ip header structure
  ********************************************************************************/
  void log_ip_header(FILE *log_file, struct iphdr *ip_hdr) {
      struct sockaddr_in source, dest;

      memset(&source, 0, sizeof(source));
      source.sin_addr.s_addr = ip_hdr->saddr;     
      memset(&dest, 0, sizeof(dest));
      dest.sin_addr.s_addr = ip_hdr->daddr;     

      fprintf(log_file, "\nIP Header\n");

      fprintf(log_file, "\t|-Version               : %d\n", (unsigned int)ip_hdr->version);
      fprintf(log_file, "\t|-Internet Header Length: %d DWORDS or %d Bytes\n", (unsigned int)ip_hdr->ihl, ((unsigned int)(ip_hdr->ihl)) * 4);
      fprintf(log_file, "\t|-Type Of Service       : %d\n", (unsigned int)ip_hdr->tos);
      fprintf(log_file, "\t|-Total Length          : %d  Bytes\n", ntohs(ip_hdr->tot_len));
      fprintf(log_file, "\t|-Identification        : %d\n", ntohs(ip_hdr->id));
      fprintf(log_file, "\t|-Time To Live          : %d\n", (unsigned int)ip_hdr->ttl);
      fprintf(log_file, "\t|-Protocol              : %d\n", (unsigned char)ip_hdr->protocol);
      fprintf(log_file, "\t|-Header Checksum       : %d\n", ntohs(ip_hdr->check));
      fprintf(log_file, "\t|-Source IP             : %s\n", inet_ntoa(source.sin_addr));
      fprintf(log_file, "\t|-Destination IP        : %s\n", inet_ntoa(dest.sin_addr));
  }
  /************************************************************************
  * Function: udp_header(FILE *log_file, struct iphdr *ip_hdr)
  * Description: Extracting the UDP header
  * 
  * Entry:   log_file    log file
  *          ip_hdr      pointer of IP header
  ************************************************************************/
  void udp_header(FILE *log_file, struct iphdr *ip_hdr) {
      fprintf(log_file, "\nUDP Header\n");

      udp_hdr = (struct udphdr *)((unsigned char *)ip_hdr + (unsigned int)ip_hdr->ihl * 4);

      fprintf(log_file, "\t|-Source Port     : %d\n", ntohs(udp_hdr->source));
      fprintf(log_file, "\t|-Destination Port: %d\n", ntohs(udp_hdr->dest));
      fprintf(log_file, "\t|-UDP Length      : %d\n", ntohs(udp_hdr->len));
      fprintf(log_file, "\t|-UDP Checksum    : %d\n", ntohs(udp_hdr->check));
  }
  /**************************************************************************
  * Function: void udp_payload(FILE *log_file, struct udphdr *udp_hdr)
  * Description: Show data
  * 
  * Entry: buffer     data packet
  *        buf_len    length of data packet
  **************************************************************************/
  void udp_payload(FILE *log_file, struct udphdr *udp_hdr) {
      int i = 0;
      unsigned char *data = (unsigned char *)udp_hdr + sizeof(struct udphdr);
      fprintf(log_file, "\nData\n");
      int data_len = ntohs(udp_hdr->len) - sizeof(struct udphdr);

      for (i = 0; i < data_len; i++) {
          if (i != 0 && i % 16 == 0) 
              fprintf(log_file, "\n");
          fprintf(log_file, " %.2X ", data[i]);
      }

      fprintf(log_file, "\n");
  }
  /*****************************************************
  * Main 
  *****************************************************/
  int main() {
      FILE* log_file;                             // log file
      struct sockaddr saddr;

      int sock_raw, saddr_len, buf_len;
      int ret_value = 0;

      int done = 0;               // exit loop when done=1
      int udp = 0;                // udp packet count

      // open a raw socket
      sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
      if (sock_raw < 0) {
          printf("Error in socket\n");
          return -1;
      }
      // Allocate a block of memory for the receive buffer
      unsigned char *buffer = (unsigned char *)malloc(65536); 
      if (buffer == NULL) {
          printf("Unable to allocate memory.\n");
          close(sock_raw);
          return -1;
      }
      memset(buffer, 0, 65536);

      // Create a log file for storing output
      log_file = fopen(LOG_FILE, "w");
      if (!log_file) {
          printf("Unable to open %s\n", LOG_FILE);
          free(buffer);
          close(sock_raw);
          return -1;
      }

      printf("starting .... %d\n", sock_raw);
      while (!done) {
          // Receive data packet
          saddr_len = sizeof saddr;
          buf_len = recvfrom(sock_raw, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);

          if (buf_len < 0) {
              printf("Error in reading recvfrom function\n");
              ret_value = -1;
              goto QUIT;
          }

          fflush(log_file);
          // Extracting the Ethernet header
          if (ethernet_header(buffer, buf_len) != ETH_P_IP) {
              // drop the packet if network layer protocol is not IP
              continue;
          }
          // Extracting the IP header
          if (ip_header(buffer, buf_len) != 17) {
              // drop packet if transport layer protocol is not UDP
              continue;
          }
          fprintf(log_file, "\n**** UDP packet %02d*********************************\n", udp + 1);
          // Write ethernet header into log file
          log_ethernet_header(log_file, eth_hdr);
          // Write IP header into log file
          log_ip_header(log_file, ip_hdr);
          // Extracting the UDP header and write into log file
          udp_header(log_file, ip_hdr);
          // write UDP payload into log file
          udp_payload(log_file, udp_hdr);
          // exit when the count of received udp packets is more than 10
          if (++udp >= 10) done = 1;
      }

  QUIT:
      fclose(log_file);
      free(buffer);
      close(sock_raw);        // close raw socket 
      printf("DONE!!!!\n");
      return ret_value;
  }
  ```
* 该程序使用raw_socket在数据链路层直接接收从物理层发过来的数据，数据不会经过各个协议层的处理；
* 在应用层进行socket进行网络编程时，端口号可以用于区分接收数据的应用程序，使用raw socket接收数据时，端口号没有用；
* 该程序将收到的udp数据包的以太网头、IP头、UDP头提取出来，和数据一起写入到文件udp_packets.log文件中；
* 该程序丢弃了除UDP包以外的所有其它数据包；
* 为了避免冗长的log文件，这个程序接收10个UDP数据包后会自动退出；
* 该程序经过扩展后可以成为一个简单的数据包嗅探器；
* **编译程序**
  ```bash
  gcc -Wall receive_udp_packet.c -o receive_udp_packet
  ```
* **运行程序**
  ```bash
  sudo ./receive_udp_packet
  ```
  - 这个程序必须要使用root权限运行，因为使用了raw socket

* **测试程序**
  - 最好使用局域网中的两台机器(虚拟机)进行测试，因为在下面的测试方法中，从本机发送时，以太网头中的源和目的MAC地址可能会被填0；
  - 假定A机的IP地址为 192.168.2.114，在A机运行程序receive_udp_packet程序；
  - 我们从B机(与A机的IP不同)，发送数据：
    ```bash
    echo -n "udp packet 01" > /dev/udp/192.168.2.114/8000
    echo -n "udp packet 02" > /dev/udp/192.168.2.114/8001
    ......
    ```
  - 8000和8001是端口号，可以是任意的；
  - 连接在网络上的A机，有可能会从网络上收到其它的UDP包，所以A机启动receive_udp_packet程序后，要尽快在B机发出数据，否则可能你还没有发出数据，A机已经收到了10条UDP包并自动退出；
  - 查看log文件，看看有没有你发出来的数据
    ```bash
    cat udp_packets.log
    ```
  - 在我的电脑上看到的是这样的：

    ![收到的 udp 数据包][img06]

    <center><b>图6：收到的 UDP 数据包</b></center>


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[img01]:https://whowin.gitee.io/images/180002/sending_data_from_app_with_socket.png
[img02]:https://whowin.gitee.io/images/180002/receiving_data_in_app_with_socket.png
[img03]:https://whowin.gitee.io/images/180002/app_receive_data_with_raw_socket.png
[img04]:https://whowin.gitee.io/images/180002/a_generic_representation_of_a_network_packet.png
[img05]:https://whowin.gitee.io/images/180002/ip_header.png
[img06]:https://whowin.gitee.io/images/180002/receive_udp_packet.png


