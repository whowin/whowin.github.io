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

Magic Packet是进行网络唤醒的数据包，将这个数据包以广播的形式发到局域网上，与数据包中所关联的MAC相同的电脑就会被唤醒开机，通常我们都是使用UDP报文的形式来发送这个数据包，但实际上在进行网络唤醒的时候，只要报文中包含Magic Packet应该就可以唤醒相关的计算机，与IP协议、UDP协议没有任何关系，本文将试图抛开网络层(IP层)和传输层(TCP/UDP层)，直接在数据链路层发出Magic Packet，并成功实现网络唤醒，本文将提供完整的源代码。
<!--more-->

## 1. Magic Packet
* 我比较喜欢把这个数据包称作"网络唤醒包"；有很多地方把它翻译成"魔术包"或者"魔法数据包"，我个人觉着太过表面，无法表达其实际的含义；本文将这个数据包称为"网络唤醒包"或者"Magic Packet"，二者具有完全相同的含义；
* 以前写过一篇与嵌入式相关的文章[《远程开机：一个简单的嵌入式项目开发》][article01]，在嵌入式环境下使用Magic Packet进行远程开机的小项目，有兴趣的读者可以参考；
* ```Magic Packet``` 就是一个指定格式的数据包，其格式为：6 个 **0xff**，然后16组需要被网络唤醒的电脑的 **MAC** 地址，比如需要被唤醒的电脑的 MAC 为：```00:e0:2b:69:00:03```，则 ```Magic Packet``` 为(16进制表述)：
    ```
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
  - 以太网报头定义在头文件linux/if_ether.h中：
    ```
    struct ethhdr {
        unsigned char  h_dest[ETH_ALEN];    /* destination eth addr  */
        unsigned char  h_source[ETH_ALEN];  /* source ether addr  */
        __be16         h_proto;             /* packet type ID field  */
    } __attribute__((packed));
    ```
  - h_dest字段为目的MAC地址，h_source字段为源MAC地址；
  - h_proto表示当前数据包在网络层使用的协议，Linux支持的协议在头文件linux/if_ether.h中定义；通常在网络层使用的IP协议，这个字段的值是0x0800(ETH_P_IP)；
  - 但是本文中的 h_proto 字段要填写 **0x842**，很遗憾这个协议在头文件中没有定义，也基本找不到相关资料；

* **raw socket**
  - 可以参考我的另两篇文章[《Linux下如何在数据链路层接收原始数据包》][article02]和[《如何使用raw socket发送UDP报文》][article03]，这里仅做一个简单回顾；
  - 打开一个raw socket
    ```
    int sock_raw;
    sock_raw = socket(AF_PACKET, SOCK_RAW, 0);
    ```
  - 第三个参数还可以有其它选择，这个参数往往会对socket的接收产生影响，本文并不接收任何信息，所以对本文来说无关紧要。

* **struct sockaddr_ll**
  - 这个结构在linux/if_packet.h中定义，有关该结构的详细说明请参考其它文章，本文仅就相关字段做出说明；
  - 这个结构与IPv4 socket编程中的结构(struct sockaddr_in)的作用类似，是用在raw socket上的一个地址结构，烦请自行理解，其中'll'表示Low Level
    ```
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
  - sll_protocol是标准的以太网协议类型，定义在头文件linux/if_ether.h中，通常情况下应该 ETH_P_IP(0x800) 表示IP协议，本文要填 0x842；
  - sll_ifindex是网络接口的索引号，我们可以根据接口名称使用ioctl获得；
  - sll_halen是硬件地址(MAC)的长度，ha是Hardware Address的意思，填常数 ETH_ALEN(定义在头文件linux/if_ether.h中)；
  - sll_addr是目的MAC地址
  - 实际上，在发送数据时，由于sll_family和sll_protocol都是和socket中一样的，所以大多数情况下都可以不填，只要填sll_ifindex、sll_halen和sll_addr即可，但是本文的例子中，如果使用bind()绑定地址，应该尽量完整第填写，否则执行bind()是会出错。

## 3. 在数据链路层发送Magic Packet
* 先说一下目标，通常使用UDP发送Magic Packet的报文结构为：以太网报头 + IP报头 + UDP报头 + Magic Packet，我们的目标是：**以太网报头 + Magic Packet**
* 先看源程序，文件名为：magic_packet.c
  ```
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <string.h>
  #include <malloc.h>
  #include <errno.h>

  #include <sys/socket.h>
  #include <sys/ioctl.h>
  #include <arpa/inet.h>
  #include <linux/if.h>
  #include <linux/if_ether.h>
  #include <linux/if_packet.h>

  #define BUF_SIZE            256
  #define MAGIC_PACKET_SIZE   102
  #define MAGIC_PACKET_PROTO  0x842

  #define USING_SENDTO        1
  /********************************************************************
  * Function: int get_eth_index(int sock_raw, char *ifname)
  * Description: Get iterface index number from ifname using ioctl
  * 
  * retuen
  *          >= 0        iterface index
  *          < 0         error  
  ********************************************************************/
  int get_eth_index(int sock_raw, char *ifname) {
      struct ifreq if_req;
      memset(&if_req, 0, sizeof(struct ifreq));
      strncpy(if_req.ifr_name, ifname, IFNAMSIZ - 1);

      if ((ioctl(sock_raw, SIOCGIFINDEX, &if_req)) < 0) {
          printf("error in SIOCGIFINDEX ioctl reading.\n");
          return -1;
      }
      printf("Interface Name: %s\tInterface Index=%d\n", ifname, if_req.ifr_ifindex);
      return if_req.ifr_ifindex;
  }

  /**************************************************************************
  * Function: int get_mac(int sock_raw, unsigned char *mac, char *ifname)
  * Description: Get local MAC from ifname using ioctl
  * 
  * retuen
  *          = 0         success. MAC has been stored into mac
  *          < 0         error 
  **************************************************************************/
  int get_mac(int sock_raw, unsigned char *mac, char *ifname) {
      struct ifreq if_req;
      memset(&if_req, 0, sizeof(struct ifreq));
      strncpy(if_req.ifr_name, ifname, IFNAMSIZ - 1);

      if ((ioctl(sock_raw, SIOCGIFHWADDR, &if_req)) < 0) { 
          printf("error in SIOCGIFHWADDR ioctl reading.\n");
          return -1;
      }
      int i;
      for (i = 0; i < ETH_ALEN; ++i) mac[i] = (unsigned char)(if_req.ifr_hwaddr.sa_data[i]);

      printf("Source MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
      return 0;
  }
  /****************************************************************
  * Function: int is_valid_mac(char *mac)
  * Description: Check mac if valid or not
  * 
  * Return
  *          1       mac is valid
  *          0       mac is invalid
  ****************************************************************/
  int is_valid_mac(char *mac){
      if (strlen(mac) != 17) return 0;
      int i;
      for (i = 0; i < 17; ++i) {
          if ((i + 1) % 3 == 0) {
              if (mac[i] != ':') return 0;
          } else {
              if ((mac[i] >= '0' && mac[i] <= '9') || 
                  (mac[i] >= 'a' && mac[i] <= 'f') ||
                  (mac[i] >= 'A' && mac[i] <= 'F')) {
                  continue;
              } else return 0;
          }
      }
      return 1;
  }

  /******************************************************************
  * Main
  ******************************************************************/
  int main(int argc, char **argv) {
      int sock_raw;                           // raw socket
      unsigned char src_mac[ETH_ALEN] = {0};  // source MAC address
      unsigned char dest_mac[ETH_ALEN] = {0}; // destination MAC address
      int if_index;                           // interface index number
      char ifname[IFNAMSIZ] = {0};            // interface name
      unsigned char *send_buf;                // buffer for packet
      
      int ret_value = EXIT_SUCCESS;           // return value

      // check parameters' number
      if (argc != 3) {
          printf("Usage: %s [ifname] [destination MAC]\n", argv[0]);
          exit(EXIT_FAILURE);
      }

      // Check if MAC is valid
      if (!is_valid_mac(argv[2])) {
          printf("Invalid destination MAC address. %s\n", argv[2]);
          exit(EXIT_FAILURE);
      }
      // Check if ifname length is too long
      if (strlen(argv[1]) > IFNAMSIZ) {
          printf("Invalid Interface Name. %s\n", argv[1]);
          exit(EXIT_FAILURE);
      }
      // copy ifname
      strcpy(ifname, argv[1]);
      // convert MAC string to digits
      unsigned char *p = (unsigned char *)argv[2];
      sscanf(argv[2], "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
              &dest_mac[0], &dest_mac[1],&dest_mac[2], &dest_mac[3], &dest_mac[4],&dest_mac[5]);

      // Create a raw socket
      sock_raw = socket(AF_PACKET, SOCK_RAW, 0);
      if (sock_raw == -1) {
          printf("error in socket.\n");
          exit(EXIT_FAILURE);
      }

      // allocate memory for sending
      send_buf = (unsigned char*)malloc(BUF_SIZE);
      memset(send_buf, 0, BUF_SIZE);
      // get interface index
      if_index = get_eth_index(sock_raw, ifname);         // get interface index number
      if (if_index < 0) {
          ret_value = EXIT_FAILURE;
          goto quit;
      }
      // get local MAC as source mac
      if (get_mac(sock_raw, src_mac, ifname) < 0) {           // get MAC address
          ret_value = EXIT_FAILURE;
          goto quit;
      }
      printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
              dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5]);

      // Fill ethernet header
      struct ethhdr *eth_hdr = (struct ethhdr *)(send_buf);
      int i;
      for (i = 0; i < ETH_ALEN; ++i) {
          eth_hdr->h_source[i] = src_mac[i];
          eth_hdr->h_dest[i] = 0xff;
      }
      eth_hdr->h_proto = htons(MAGIC_PACKET_PROTO);   // 0x842

      // Construct magic packet
      p = send_buf;
      p += sizeof(struct ethhdr);
      for (i = 0; i < ETH_ALEN; i++) *p++ = 0xFF;
      for (i = 0; i < 16; i++) memcpy((p + i * 6), dest_mac, ETH_ALEN);

      // print the packet
      size_t total_len = MAGIC_PACKET_SIZE + sizeof(struct ethhdr);
      p = send_buf;
      for (i = 1; i <= total_len; ++i) {
          printf(" %02x", *p++);
          if ((i % 16) == 0) printf("\n");
      }
      printf("\n");

      // Construct struct sockaddr_ll
      struct sockaddr_ll saddr_ll;
      memset(&saddr_ll, 0, sizeof(struct sockaddr_ll));
      saddr_ll.sll_family   = AF_PACKET;
      saddr_ll.sll_ifindex  = if_index;
      saddr_ll.sll_pkttype  = PACKET_BROADCAST;
      saddr_ll.sll_halen    = ETH_ALEN;
      for (i = 0; i < ETH_ALEN; ++i) saddr_ll.sll_addr[i] = 0xff;
      saddr_ll.sll_protocol = htons(MAGIC_PACKET_PROTO);  // 0x842

  #if !USING_SENDTO
      if (bind(sock_raw, (const struct sockaddr *)&saddr_ll, sizeof(struct sockaddr_ll)) < 0) {
          perror("Bind()");
          ret_value = EXIT_FAILURE;
          goto quit;
      }
  #endif

    // Send the packet
      int send_len = 0;       // how many bytes sent
  #if USING_SENDTO
      if ((send_len = sendto(sock_raw, send_buf, total_len, 0, (const struct sockaddr *)&saddr_ll, sizeof(struct sockaddr_ll))) < 0) {
          perror("Sendto()");
      }
  #else
      if ((send_len = send(sock_raw, send_buf, total_len, 0)) < 0) {
          perror("Send()");
      }
  #endif

      printf("... done. send_len=%d\n", send_len);

  quit:
      free(send_buf);
      close(sock_raw);
      return ret_value;
  }

  ```
* 报文是以广播的形式发出去的，发送广播时，以太网报头的目的MAC要全部填写0xff，(struct sockaddr_ll)中的sll_addr也要全部填写0xff；
* 尽管我们知道目的MAC，但是这个报文必须以广播报文发出，因为此时被唤醒的机器并没有开机，所以无法通过arp获知填写的MAC地址是否在局域网内，所以如果在以太网报头的h_dest中填上了要被唤醒的机器的MAC地址，报文是无法送达的；
* 关于使用ioctl获取接口索引号(Interface Index)和接口MAC的问题，请自行查找资料，这些是IPv4 socket编程中基础的东西，本文认为读者已经掌握；
* 程序中写好了用send()或者用sendto()发送报文的代码，使用一个常量USING_SENDTO来控制，当USING_SENDTO为1时，将使用sendto()发送报文，否则使用send()发送报文；
* 如果使用send()发送报文，需要使用bind()绑定目的地址；
* 具体实践中，如果使用sendto()发送报文，(struct sockaddr_ll)中只需填写sll_ifindex即可，这个也许和运行环境有关，请自行测试；理论上说，只要编译通过，运行没有出错，就表示这个Magic Packet发送了出去，应该就可以起作用；
* 源程序中有详细的注释，其它也没有什么更多解释的。
* **编译**：```gcc -Wall magic_packet.c -o magic_packet```
* **运行**：```sudo ./magic_packet enp0s3 00:e0:2b:68:00:03```
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

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png



[article01]:../embedded/0001-wake-on-lan/
[article02]:../0002-link-layer-programming/
[article03]:../0006-send-udp-with-raw-socket/
[article04]:https://wiki.wireshark.org/WakeOnLAN
[article05]:https://zh.wikipedia.org/zh-cn/%E7%B6%B2%E8%B7%AF%E5%96%9A%E9%86%92
[article06]:https://en.wikipedia.org/wiki/Wake-on-LAN

[img01]:/images/180002/sending_data_from_app_with_socket.png
[img02]:/images/180015/screenshot-magic-packet.png
