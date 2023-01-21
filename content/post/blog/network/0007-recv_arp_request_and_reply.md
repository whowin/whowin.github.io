---
title: "接收arp请求并发送回应的实例"
date: 2023-01-12T16:43:29+08:00
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
draft: false
#references: 
# - [linux系统中使用socket直接发送ARP数据](http://t.zoukankan.com/ceblog-p-10626828.html)
# - [Packet socket 和 sockaddr_ll](https://www.cnblogs.com/zhangshenghui/p/6097492.html)
# - (参考源程序)[arp request and reply using c socket programming](https://stackoverflow.com/questions/16710040/arp-request-and-reply-using-c-socket-programming)
# - [Address Resolution Protocol](https://en.wikipedia.org/wiki/Address_Resolution_Protocol)
# - [Linux socket发送ARP请求包 C语言](https://blog.csdn.net/u014302425/article/details/113822870)
postid: 180007
---

本文简单介绍了arp协议，用一个实例查看收到的ARP请求，并对该请求发出ARP回应，实例有完整的源代码，使用C语言在Linux下实现，代码中有详细的注释。
<!--more-->

## 1. ARP协议
* ARP(Address Resolution Protocol)，地址解析协议；在局域网上通过IP地址获取物理地址MAC的协议，该协议工作在数据链路层；
* 为什么需要ARP协议
  - 以TCP/IP协议为例，应用程序在应用层发出信息后，在传输层(TCP层)加上一个TCP报头，TCP报头中需要填写源端口和目的端口，端口号标识着一台机器上的某个确定的应用程序，在网络层(IP层)加上一个IP报头，IP报头需要填写源IP地址和目的IP地址，IP地址标识着互联网上一台唯一的机器，所以，TCP报头和IP报头可以确定下来互联网上的某台机器上的某个应用程序；
  - 在数据链路层需要给数据包加上以太网报头，在以太网报头中，需要填的是机器的物理地址(MAC地址)，源地址和目的地址均要填MAC地址；
  - 我们可以使用ioctl获取本机的MAC地址，但是却无法获取目的地址的MAC地址，这时就需要使用ARP协议了；

* ARP的工作原理
  - 在以太网络中，每台机器均有一个ARP cache，里面存着一些IP地址和MAC地址的对应关系，在ubuntu下，内核把ARP cache映射为文件：/proc/net/arp
  - 在数据链路层需要填MAC地址时，首先查询ARP cache中是否有相应的记录，如果有则直接使用；
  - 如果在ARP cache中没有记录，则向局域网广播一个ARP请求，里面有需要获取MAC地址的机器所对应的IP地址，当这个IP地址收到这个ARP请求时，则会回应一个ARP回应，将自己的MAC地址告知发送请求的机器，发出请求的机器收到该回应后，应该在ARP cache中添加上这条记录；
  - 有关ARP cache的操作，请参考文章[《如何用C语言操作arp cache》][article3]。

* ARP请求/回应的格式
  - 对一个ARP请求的回应被称为ARP回应，ARP请求数据包和回应数据包的结构是一样的；

  ![arp request][img01]

*****************
* **以太网报头(Ethernet Header)**
  - 数据链路层的以太网报头定义在头文件linux/if_ether.h中：
    ```
    struct ethhdr {
        unsigned char  h_dest[ETH_ALEN];    /* destination eth addr  */
        unsigned char  h_source[ETH_ALEN];  /* source ether addr  */
        __be16         h_proto;             /* packet type ID field  */
    } __attribute__((packed));
    ```
  - 上面的ETH_ALAN的值为6，定义在头文件linux/if_ether.h中；
  - h_dest字段为目的MAC地址，h_source字段为源MAC地址；
  - h_proto表示当前数据包在网络层使用的协议，Linux支持的协议在头文件linux/if_ether.h中定义；通常在网络层使用的IP协议，这个字段的值是0x0800(宏ETH_P_IP)，**对于ARP报文，这个字段的值为0x0806(ETH_P_ARP)**，表示网络层使用的是ARP协议；
  - 对于一个ARP请求数据包来说，h_dest通常为6个0xff，这是因为发送者并不知道目的机器的MAC地址，发送者的意图就是通过一个ARP请求包获得目的机器的MAC地址，所以发送者需要把这个数据包广播出去，使每台机器都可以收到，6个0xff表示一个广播地址；
  - 对于ARP请求的数据包来说，h_source是发送者自己的MAC地址，发送者在发送ARP请求时，自己的MAC地址是显然已知的；
  - 对于ARP回应的数据包来说，目的地址就是发送ARP请求包的机器，h_dest应该就是收到的ARP请求包中的h_source；
  - 对于ARP回应的数据包来说，h_source就是自己的MAC地址，这个当然是已知的；
  - 不管是ARP请求还是回应数据包，h_proto都是0x0806(宏ETH_P_ARP)，表示网络层使用的协议是ARP协议

* **ARP报头(ARP header)**

  ![ARP header][img02]

  ---------------
  - arp报头定义在头文件linux/if_arp.h中：
    ```
    struct arphdr {
        __be16          ar_hrd;       /* format of hardware address   */
        __be16          ar_pro;       /* format of protocol address   */
        unsigned char   ar_hln;       /* length of hardware address   */
        unsigned char   ar_pln;       /* length of protocol address   */
        __be16          ar_op;        /* ARP opcode (command)         */
    }
    ```
  - ar_hrd字段为ARP协议硬件类型，相关定义在linux/if_arp.h中，对以太网而言，硬件类型为1(宏ARPHRD_ETHER)；
  - ar_pro字段为协议类型，协议类型定义在linux/if_ether.h中，IP协议的协议类型为0x800(宏ETH_P_IP)；
  - ar_hln字段为硬件地址的长度，对以太网而言，硬件地址就是MAC地址，长度是6 bytes(宏ETH_ALEN)；
  - ar_pln字段为协议地址长度，对IP协议的IPv4而言，协议地址就是IP地址，长度是4 bytes；
  - ar_op字段表示要执行的操作，1-ARP请求，2-ARP回应；
  - 在发送ARP回应时，ARP报头比较好处理，只需要把ar_op从1(宏ARROP_REQUEST)改成2(ARROP_REPLY)即可，其它字段均不需要改动。

* **ARP Payload**

  ![ARP payload][img03]

  -----------------
  - 对于以太网IPv4而言，ARP payload的定义如下
    ```
    struct eth_arpmsg {
        unsigned char   arp_sha[ETH_ALEN];   /* Sender Hardware Address(MAC) */
        unsigned char   arp_spa[4];          /* Sender Protocol Address(IP) */
        unsigned char   arp_dha[ETH_ALEN];   /* Destination Hardware Address(MAC) */
        unsigned char   arp_dpa[4];          /* Destination Protocol Address(IP) */
    }
    ```
  - ARP payload的实际长度要根据ARP报头来确定，对于以太网IPv4而言，其结构是可以确定的，但对于其它网络而言，硬件地址的表达形式可能不是MAC地址，那么sender hardware address的长度也有可能不是6；协议地址的长度也是一样，比如IPv6的协议地址长度就是128位，16个字节；所以，如果不是以太网，则arp payload是要重新定义的，好在我们日常见到的网络都是以太网IPv4；
  - 对于ARP请求数据包而言，arp_sha和arp_spa为发送者的MAC地址和IP地址，发送者是已知的，接收者的IP也是知道的，所以arp_dpa字段也是填好的，但接收者的MAC地址，也就是arp_dha字段发送者并不知道(发送者发送ARP请求就是为了获得这个MAC)，所以收到的ARP请求包里arp_dha字段为6个0；
  - 在回应ARP请求包时，ARP请求包的发送者成为了接收者，arp_dha和arp_dpa(目的机器的MAC和IP)应该就是请求包中的arp_sha和arp_spa；
  - 在ARP回应数据包中，arp_sha和arp_spa就是本机的MAC地址和IP地址，这些信息都是已知的。

## 2. ARP请求/回应的发送和接收
* 对于习惯于使用socket基于TCP/IP协议的编程来说，arp的请求既无法使用TCP socket发送，也无法使用UDP socket发送；接收也是一样，TCP/UDP socket是无法收到ARP的回应信息的；
* 前面说过，ARP协议是工作在数据链路层的，而TCP/UDP socket是用在应用层的，应用层发送的数据在传输层要被加上一个TCP/UDP头，在网络层要被加上一个IP头，在数据链路层要被加上一个以太网头，然后才交给物理层发送；
* 相关概念请参考[《Linux下如何在数据链路层接收原始数据包》][article1]和[《如何使用raw socket发送UDP报文》][article2]
* 发送ARP请求、接收ARP回应只能在数据链路层完成，要在数据链路层上编程，需要用到raw socket，相关的知识也请参考上面两篇文章；
* 在发送ARP请求时，封装过程与文章[《如何使用raw socket发送UDP报文》][article2]中类似；
* 在接收ARP回应时，拆包过程与文章[《Linux下如何在数据链路层接收原始数据包》][article1]中类似；
* 使用程序获取、添加和删除ARP cache记录的方法，请参考文章[《如何用C语言操作arp cache》][article3]
* 关于如何构建以太网头、ARP报头和ARP payload，在上一节介绍以太网报头和ARP报文时已经做了介绍。

## 3. 接收ARP请求并发送回应的实例
* 在这个例子中，程序接收局域网上所有机器发出的arp请求包，如果这个请求包是发给本机的，即arp请求包中的目的IP为本机IP，则显示收到的以太网报头和arp报文，并向发送者发送一个arp回应；
* 程序的运行步骤如下：
  1. 创建一个raw socket
  2. 查询网络接口的索引号(interface index)
  3. 查询本机的MAC地址
  4. 查询本机的IP地址
  5. 为发送arp报文设置sockaddr_ll
  6. 等待接收来自网络的数据包
  7. 如果数据包不是arp数据包，则忽略，回到步骤6
  8. 如果数据包不是arp请求包，则忽略，回到步骤6
  9. 如果arp请求包不是发给本机的，则忽略，回到步骤6
  10. 显示以太网报头
  11. 显示arp数据包(arp header + arp payload)
  12. 构建用于发送arp reply的以太网报头
  13. 构建用于发送arp reply的arp数据包
  14. 将sockaddr_ll中的sll_addr字段填为目的MAC地址
  15. 显示用于发送arp reply的以太网报头
  16. 显示用于发送arp reply的arp数据包内容
  17. 发送arp reply
  18. 回到步骤6
* 该程序的主体在一个死循环中，所以程序只能使用ctrl+c退出，为了保证退出时能很好地清理现场，程序中拦截了SIGINT信号，当按下ctrl+c时，程序将收到这个信号，进行现场清理并退出；
* 还是要简单讨论一下构建发送arp reply的arp数据包的过程：
  1. 当收到一个arp请求时，其中的arp头部分基本不需要变，唯一要变的ar_op字段，因为这个字段在收到的数据包里为1，意即这是arp request，这个字段要改为2，意即这是一个arp reply
  2. 收到的arp报文中arp_sha为发送人的MAC，发送arp reply时，这个MAC应该成为收件人MAC，即：arp_dha，所以要将arp_sha拷贝到arp_dha
  3. 收到的arp报文中arp_spa为发送人的IP，发送arp reply时，这个IP应该成为收件人IP，即：arp_dpa，所以要将arp_spa拷贝到arp_dpa
  4. 发送arp reply时，发件人MAC，也就是arp_sha应该为本机的MAC
  5. 发送arp reply时，发件人IP，也就是arp_spa应该为本机IP
* 构建以太网报头的过程：
  1. 收到的以太网报头中的收件人的MAC(h_dest)，应该为6个0xff，因为发送者发送arp request时并不知道接收者的MAC，所以发的是广播报文；
  2. 收到的以太网报头中的发件人的MAC(h_source)，在发送arp reply时应该为以太网报头中的收件人的MAC(h_dest)；
  3. 发送arp reply时的发件人的MAC应该填本机的MAC；
* 由于ARP工作在数据链路层，所以不能使用普通的socket进行接收和发送，需要使用raw socket，使用raw socket时需要使用一个结构(struct sockaddr_ll)，这个结构的作用类似于IPv4的socket编程中的(struct sockaddr_in)的作用，有关这个结构的简单介绍，可以参考文章[《如何使用raw socket发送UDP报文》][article2]
* 由于这个程序是使用raw socket在数据链路层上接收数据，所以实际上程序可以收到在局域网上的所有数据包，程序过滤掉了不是ARP协议的数据包，然后又过滤掉了目的IP不是本机的数据包，在ctrl+c退出时，程序会显示收到了多少数据包，其中ARP协议数据包有多少，ARP请求包有多少，程序一共回应了多少个数据包，数据还是有点意思的；
* 下面是源程序，文件名：arp_server.c
  ```
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <signal.h>

  #include <sys/socket.h>
  #include <sys/ioctl.h>

  #include <linux/if_ether.h>
  #include <linux/if_arp.h>

  #include <arpa/inet.h>

  #define BUF_SIZE            42
  #define DEVICE              "enp0s3"    // Please modify according to the actual device

  int sock_raw = 0;               // Socket descriptor
  void *buffer = NULL;            // Buffer for receiving and sending packet
  long total_packets = 0;         // how many packets were received
  long total_arp_packets = 0;     // how many arp packets were received
  long total_arp_req_packets = 0; // how many arp request packets were received
  long answered_packets = 0;      // how many arp reply packets were sent

  void sigint(int signum);

  struct __attribute__((packed)) arp_header {
      struct arphdr arp_hdr;              // arp header
      unsigned char arp_sha[ETH_ALEN];    // Sender Hardware Address
      unsigned char arp_spa[4];           // Sender Protocol Address
      unsigned char arp_dha[ETH_ALEN];    // Destination Hardware Address
      unsigned char arp_dpa[4];           // Destination Protocol Address
  };

  int main(void) {
      buffer = (void *)malloc(BUF_SIZE);                  // Buffer for Ethernet Frame
      unsigned char *eth_header = buffer;                 // Pointer to Ethenet Header
      struct ethhdr *eh = (struct ethhdr *)eth_header;    // Another pointer to ethernet header
      unsigned char *arp_header = buffer + sizeof(struct ethhdr); // it is arp header after ethernet header
      struct arp_header *ah = (struct arp_header *)arp_header;    // pointer to ARP header

      struct ifreq ifr;               // use for getting MAC and IP via ioctl
      unsigned char src_mac[ETH_ALEN];// local MAC address
      uint32_t src_ip;                // local IP address

      struct sockaddr_ll saddr_ll;    // structure for sending packet via raw socket
      int ifindex = 0;                // Ethernet Interface index
      int length;                     // length of received packet
      int sent;                       // length of sent packet

      printf("Server started, entering initialiation phase...\n");

      // Step 1: create raw socket
      //===========================
      sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
      if (sock_raw == -1) {
          perror("socket():");
          exit(EXIT_FAILURE);
      }
      printf("Successfully created socket: %i\n", sock_raw);

      // Step 2: retrieve ethernet interface index
      //===========================================
      memset(&ifr, 0, sizeof(struct ifreq));
      strncpy(ifr.ifr_name, DEVICE, IFNAMSIZ - 1);
      if (ioctl(sock_raw, SIOCGIFINDEX, &ifr) == -1) {
          perror("SIOCGIFINDEX");
          close(sock_raw);
          exit(EXIT_FAILURE);
      }
      ifindex = ifr.ifr_ifindex;
      printf("Successfully got interface index: %i\n", ifindex);

      // Step 3: retrieve corresponding MAC
      //====================================
      if (ioctl(sock_raw, SIOCGIFHWADDR, &ifr) == -1) {
          perror("SIOCGIFINDEX");
          close(sock_raw);
          exit(EXIT_FAILURE);
      }
      memcpy(src_mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
      printf("Successfully got local MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
              src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);

      // Step 4: retrieve corresponding IP
      //===================================
      memset(&ifr, 0, sizeof(struct ifreq));
      strncpy(ifr.ifr_name, DEVICE, IFNAMSIZ - 1);
      if (ioctl(sock_raw, SIOCGIFADDR, &ifr) < 0) {
          printf("SIOCGIFADDR.\n");
          close(sock_raw);
          exit(EXIT_FAILURE);
      }
      memcpy((void *)&src_ip, &(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr), 4);
      printf("successfully got local IP address: %s\n", inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));

      // step 5: prepare sockaddr_ll for sending packet
      //================================================
      memset((void *)&saddr_ll, 0, sizeof(struct sockaddr_ll));
      saddr_ll.sll_family   = PF_PACKET;
      saddr_ll.sll_protocol = htons(ETH_P_IP);
      saddr_ll.sll_ifindex  = ifindex;
      saddr_ll.sll_hatype   = ARPHRD_ETHER;         // hardware address type: ethernet
      saddr_ll.sll_pkttype  = PACKET_OTHERHOST;     // packet type: To someone else
      saddr_ll.sll_halen    = ETH_ALEN;

      // Step 5: establish signal handler
      //==================================
      signal(SIGINT, sigint);
      printf("Successfully established signal handler for SIGINT\n\n");

      while (sock_raw > 0) {
          // Step 6: Wait for incoming packet...
          //======================================
          memset(buffer, 0, BUF_SIZE);
          length = recvfrom(sock_raw, buffer, BUF_SIZE, 0, NULL, NULL);
          if (length == -1) {
              perror("recvfrom():");
              close(sock_raw);
              exit(EXIT_FAILURE);
          }
          total_packets++;        // how many packets were received
          // Step 7: ignore the packet if not arp protocol
          //=============================================== 
          if (htons(eh->h_proto) != ETH_P_ARP)           // 0x806
              continue;
          total_arp_packets++;        // how many arp packets were received

          // Step 8: ignore the packet if not arp request(may reply)
          //=========================================================
          if (htons(ah->arp_hdr.ar_op) != ARPOP_REQUEST) {    // 0x0001
              /*
              printf("Received an arp reply. Source IP:%d.%d.%d.%d\tDest IP: %d.%d.%d.%d\n",
                      ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3],
                      ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);
              */
              continue;
          }
          total_arp_req_packets++;    // how many arp request packets were received

          // Step 9: ignore the packet if the destination is not this machine
          //==================================================================
          if (src_ip != *((uint32_t *)ah->arp_dpa)) {
              printf("Received an arp request. Source IP:%d.%d.%d.%d\tDest IP: %d.%d.%d.%d\n",
                      ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3],
                      ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);
              continue;
          } 
          // receive an arp request packet and the destination IP is this machine
          printf("\nReceived an arp request to this machine.\n");

          // Step 10: display ethernet header
          //==================================
          printf("================== ETHERNET HEADER ========================\n");
          printf("ETHER DST MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  eh->h_dest[0], eh->h_dest[1], eh->h_dest[2],
                  eh->h_dest[3], eh->h_dest[4], eh->h_dest[5]);
          printf("ETHER SRC MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  eh->h_source[0], eh->h_source[1], eh->h_source[2],
                  eh->h_source[3], eh->h_source[4], eh->h_source[5]);

          // Step 11: display arp packet 
          //=============================
          printf("================== ARP PACKET ========================\n");
          printf("------------------ ARP Header ------------------------\n");
          printf("  H/D TYPE: 0x%04x\t  PROTO TYPE: 0x%04x\n", 
                  ntohs(ah->arp_hdr.ar_hrd), ntohs(ah->arp_hdr.ar_pro));
          printf("H/D length: %d\t\tPROTO length: %d\n", 
                  (unsigned short)ah->arp_hdr.ar_hln, (unsigned short)ah->arp_hdr.ar_pln);
          printf(" OPERATION: %x\n", ntohs(ah->arp_hdr.ar_op));
          printf("------------------ ARP Payload -----------------------\n");
          printf("SENDER MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  ah->arp_sha[0], ah->arp_sha[1], ah->arp_sha[2],
                  ah->arp_sha[3], ah->arp_sha[4], ah->arp_sha[5]);
          printf(" SENDER IP address: %d.%d.%d.%d\n",
                  ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3]);
          printf("TARGET MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  ah->arp_dha[0], ah->arp_dha[1], ah->arp_dha[2],
                  ah->arp_dha[3], ah->arp_dha[4], ah->arp_dha[5]);
          printf(" TARGET IP address: %d.%d.%d.%d\n",
                  ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);

          // Step 12: construct ethernet header for sending arp reply
          //==========================================================
          // Destination MAC: should be received sender's MAC
          memcpy((void *)eth_header, (const void *)(eth_header + ETH_ALEN), ETH_ALEN);
          // Source MAC: should be local MAC
          memcpy((void *)(eth_header + ETH_ALEN), (const void *)src_mac, ETH_ALEN);
          eh->h_proto = ETH_P_ARP;        // protocol in network layer. defined in if_ether.h

          // Step 13: construct arp packet for sending arp reply
          //=====================================================
          ah->arp_hdr.ar_op = ARPOP_REPLY;            // 0x0002;

          memcpy(ah->arp_dha, ah->arp_sha, ETH_ALEN); // copy received source MAC to destination MAC
          memcpy(ah->arp_dpa, ah->arp_spa, 4);        // copy received source IP to destination IP
          memcpy(ah->arp_sha, src_mac, ETH_ALEN);     // change the sender MAC to local MAC
          memcpy(ah->arp_spa, (unsigned char *)&src_ip, 4);   // copy local IP to source IP

          // Step 14: change sll_addr to destination MAC
          //=============================================
          memcpy(saddr_ll.sll_addr, eh->h_dest, ETH_ALEN);    // copy h_dest field of ethernet header to sll_addr

          // Step 15: diaplay ethernet header for sending arp reply
          //========================================================
          printf("\n=========== ETHERNET HEARDER FOR SENDING PACKET =========\n");
          printf("ETHER DST MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  eh->h_dest[0], eh->h_dest[1], eh->h_dest[2],
                  eh->h_dest[3], eh->h_dest[4], eh->h_dest[5]);
          printf("ETHER SRC MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  eh->h_source[0], eh->h_source[1], eh->h_source[2],
                  eh->h_source[3], eh->h_source[4], eh->h_source[5]);

          // Step 16: display the content of arp reply
          //===========================================
          printf("================== ARP REPLY PACKET ========================\n");
          printf("------------------ ARP header ------------------------------\n");
          printf("  H/D TYPE: 0x%04x\t  PROTO TYPE: 0x%04x\n", 
                  ntohs(ah->arp_hdr.ar_hrd), ntohs(ah->arp_hdr.ar_pro));
          printf("H/D length: %d\t\tPROTO length: %d\n", 
                  (unsigned short)ah->arp_hdr.ar_hln, (unsigned short)ah->arp_hdr.ar_pln);
          printf(" OPERATION: %x\n", ah->arp_hdr.ar_op);
          printf("------------------ ARP Payload -----------------------------\n");
          printf("SENDER MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  ah->arp_sha[0], ah->arp_sha[1], ah->arp_sha[2],
                  ah->arp_sha[3], ah->arp_sha[4], ah->arp_sha[5]);
          printf(" SENDER IP address: %d.%d.%d.%d\n",
                  ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3]);
          printf("TARGET MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  ah->arp_dha[0], ah->arp_dha[1], ah->arp_dha[2],
                  ah->arp_dha[3], ah->arp_dha[4], ah->arp_dha[5]);
          printf(" TARGET IP address: %d.%d.%d.%d\n\n",
                  ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);

          // Step 17: Send the arp replay
          //==============================
          sent = sendto(sock_raw, buffer, BUF_SIZE, 0, (struct sockaddr *)&saddr_ll, sizeof(struct sockaddr_ll));
          if (sent == -1) {
              perror("sendto():");
              close(sock_raw);
              exit(EXIT_FAILURE);
          }
          answered_packets++;     // How many arp reply packets were sent
      }
  }

  void sigint(int signum) {
      // Clean up ......
      if (sock_raw > 0) close(sock_raw);
      if (buffer) free(buffer);

      printf("Server terminating....\n");

      printf("Totally received: %ld packets\n", total_packets);
      printf("Totally received: %ld ARP packets\n", total_arp_packets);
      printf("Totally received: %ld ARP request packets\n", total_arp_req_packets);
      printf("Answered %ld packets\n", answered_packets);
      exit(EXIT_SUCCESS);
  }
  ```
* 编译，程序在ubuntu 20.04下编译通过，gcc版本为9.4.0
  ```
  gcc -Wall arp_server.c -o arp_server
  ```
* 运行，由于使用了raw socket，必须要要有root权限才可以运行
  ```
  sudo ./arp_server
  ```
* 如果看不到有发给本机的arp请求，可以将你的手机连接到你得电脑所在的局域网上，然后，在你的电脑上开一个新的终端，在新的终端上ping一下你手机的IP，通常会激活手机向你的机器发送一个arp请求，当然你也可以ping另一台局域网中的电脑试试；
* 运行截图

  ![screenshot of running arp_server][img04]

-----------------
**欢迎访问我的博客：https://whowin.cn**



[img01]:/images/180007/arp_packet.png
[img02]:/images/180007/arp_header.png
[img03]:/images/180007/arp_payload.png
[img04]:/images/180007/screenshot_arp_server.png


[article1]:../0002-link_layer_programming/
[article2]:../0006-send_udp_with_raw_socket/
[article3]:../0014-handling_arp_cache/
