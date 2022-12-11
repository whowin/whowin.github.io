---
title: "如何使用 raw socket 编程"
date: 2022-10-16T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "C Language"
tags:
  - C语言
  - "raw socket"
draft: true
#references: 
# - [A Guide to Using Raw Sockets](https://www.opensourceforu.com/2015/03/a-guide-to-using-raw-sockets/)
postid: 130004
---

本文将讲述如何使用 raw socket 直接接收来自物理层的数据包，以及如何使用 raw socket 绕过正常的 TCP/IP 协议，将数据包发送出去，本文给出的源代码在 ubuntu 20.04 上验证成功。
<!--more-->

## 前言
> raw socket 常用于在应用程序中接收原始数据包(见图1)，所谓原始数据包指的是从物理层直接发出来的数据包；使用 raw socket 还可以绕过 TCP/IP 处理流程，在应用程序中直接把数据发送到物理层。使用 raw socket 编程，并不需要对 Linux 内核有深入的了解。

  ![raw socket直接从网络层接收数据送给应用程序][img01]
  **图1: raw socket直接从网络层接收数据送给应用程序**

* **raw socket 和其它 socket 的比较**
> 应用程序使用普通 socket，比如 stream socket(TCP) 和 datagram socket(UDP)，都是从传输层接收数据，收到的数据中只有数据内容，没有报头，所以收到的数据中并没有数据发送方的源 IP 和 MAC，两个正在通讯的应用程序，不管它们是运行在同一台机器上或者在不同的机器上，实际上它们之间只是在交换数据。

> raw socket 是不同的，raw socket 让应用程序可以直接读写较低一级的协议，这意味着 raw socket 可以接收还没有拆包的数据包(看图2)；与 stream 和 datagram sockets 不同，不需要为 raw socket 提供 IP 和端口号。

  ![raw socket 和其它 socket 工作方式的比较][img02]
  **图 2: raw socket 和其它 socket 工作方式的比较**

* **网络数据包和数据包嗅探器**
> 应用程序将数据发送到网络上时，各个网络协议层会对发送的数据进行处理，每个网络协议层都会给数据添加上一个报头，这些报头中包含着许多有用的信息，比如源地址和目的地址，这种被网络协议层包裹着的数据称为网络数据包(见图3)。不同的以太网协议，会产生多种类型的网络数据包，比如：Internet 协议数据包、Xerox PUP 数据包、Ethernet Loopback 数据包等。Linux 中，可以从文件 if_ether.h 头文件中看到所有支持的协议(见图4)。

  ![网络数据包的一般表示形式][img03]
  **图 3: 网络数据包的一般表示形式**

  ![Internet 协议的网络数据包][img04]
  **图 4: Internet 协议的网络数据包**

> 当我们连接到互联网时，我们会收到来自互联网的网络数据包，我们的电脑会解开包裹在数据外面的所有网络协议层报头，并根据从报头中获得的信息将数据发送到特定的应用程序。例如，当我们在浏览器中输入 www.baidu.com 时，我们会收到来自百度的数据包，我们的电脑会提取所有网络协议层的报头并将最终数据发送给我们的浏览器。

> 默认情况下，电脑只接收与本机目的地址相同的数据包，这种模式称为非混杂模式(non-promiscuous mode)；当我们想要接收所有的数据包(不仅仅目的地址是本机)，我们必须使用混杂模式(promiscuous mode)。ioctls 可以帮助我们在这两种模式间切换。

> 借助数据包嗅探器的帮助，我们可以对不同网络协议层的报头进行直观的查看和研究；Linux 的数据包嗅探器有很多种，例如 Wireshark；tcpdump 是一个命令行嗅探器。如果你有足够的网络知识和 C 的能力，也可以轻松编写一个自己的数据包嗅探器。

## 使用 raw socket 开发一个数据包嗅探器
> 要开发一个数据包嗅探器，首先要打开一个 raw socket，只有有效 User ID 为 0 的进程才能打开 raw socket，所以，只有 root 用户才能运行数据包嗅探器这种程序。

### 打开 raw socket
> 要打开一个 socket，必须要知道三件事：socket family、socket type 和 protocol，对 raw socket 而言，socket family 为 AF_PACKET；socket type 为 SOCK_RAW；protocol 请参考头文件 if_ether.h，接收所有数据包，protocol 使用宏 ETH_P_ALL；接收 IP 数据包，protocol 使用宏 ETH_P_IP。

  ```
  int sock_r;
  sock_r = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
  if (sock_r < 0) {
      printf("error in socket\n");
      return -1;
  }
  ```

### 接收网络数据包
> 成功打开 raw socket 以后，就可以接收网络数据包了，这里需要用到 recvfrom() 函数，也可以使用 recv()，但 recvfrom() 更方便一些。

  ```
  unsigned char *buffer = (unsigned char *) malloc(65536); // to receive data
  memset(buffer, 0, 65536);
  struct sockaddr saddr;
  int saddr_len = sizeof (saddr);
  
  // Receive a network packet and copy in to buffer
  buflen = recvfrom(sock_r, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);
  if (buflen < 0) {
      printf("error in reading recvfrom function\n");
      return -1;
  }
  ```

> 下级协议会将数据包的源地址填充到 saddr 中。

### 提取以太网报头
> 现在我们网络数据包已经存在我们的 buffer 中，我们会从以太网报头中获得信息，里面包括源和目的的物理地址，或者 MAC 地址，以及收到的数据包的协议。以太网报头的结构在头文件 if_ether.h 中定义(见图5)。

  ![以太网报头的结构][img05]
  **图5: 以太网报头的结构**

> 我们可以读取以太网报头的各个字段，并打印出来

  ```
  struct ethhdr *eth = (struct ethhdr *)(buffer);
  printf("\nEthernet Header\n");
  printf("\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);
  printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
  printf("\t|-Protocol : %d\n", eth->h_proto);
  ```

> h_proto 告诉我们下一层的信息，如果这个字段为 0x800(ETH_P_IP)，表示下一个报头是 IP 头，后面我们会讨论 IP 头的结构。

* **注1：**物理地址为 6 个字节。

* **注2：**为了更好的理解，我们可以把打印的信息直接写到一个文件中。

  ```
  fprintf(log_txt, "\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);
  ```

> 使用 fflush 以避免写入文件时的输入输出缓存问题。

### 提取 IP 报头
> IP层提供各种信息，如源和目的 IP 地址、传输层协议等；IP 头的结构定义在 ip.h 头文件中(参见图6)。

  ![IP 头的结构][img06]
  **图6: IP 头的结构**

> IP 头紧跟在以太网头的后面，所以要获取 IP 头，要把指针增加一个以太网头的长度，使其指向 IP 头。

  ```
  unsigned short iphdrlen;
  struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
  memset(&source, 0, sizeof(source));
  source.sin_addr.s_addr = ip->saddr;
  memset(&dest, 0, sizeof(dest));
  dest.sin_addr.s_addr = ip->daddr;
  
  fprintf(log_txt, "\t|-Version : %d\n", (unsigned int)ip->version);
  fprintf(log_txt, "\t|-Internet Header Length : %d DWORDS or %d Bytes\n", (unsigned int)ip->ihl, ((unsigned int)(ip->ihl)) * 4);
  fprintf(log_txt, "\t|-Type Of Service : %d\n", (unsigned int)ip->tos);
  fprintf(log_txt, "\t|-Total Length : %d Bytes\n", ntohs(ip->tot_len));
  fprintf(log_txt, "\t|-Identification : %d\n", ntohs(ip->id));
  fprintf(log_txt, "\t|-Time To Live : %d\n", (unsigned int)ip->ttl);
  fprintf(log_txt, "\t|-Protocol : %d\n", (unsigned int)ip->protocol);
  fprintf(log_txt, "\t|-Header Checksum : %d\n", ntohs(ip->check));
  fprintf(log_txt, "\t|-Source IP : %s\n", inet_ntoa(source.sin_addr));
  fprintf(log_txt, "\t|-Destination IP : %s\n", inet_ntoa(dest.sin_addr));
  ```

  ![TCP 头的结构][img07]
  **图7: TCP 头的结构**

  ![UDP 头的结构][img08]
  **图8: UDP 头的结构**

### 提取传输层报头
> 有很多种传输层协议，传输层的下层报头是 IP 头，在 IP 层也有多种 IP 协议或者 Internet 协议，可以在 /etc/protocols 文件中看到这些协议，TCP 和 UDP 协议头的结构分别定义在 tcp.h 和 udp.h 中，这些结构中有源和目标的端口号，通过端口号，系统将数据发送给特定的应用程序(参见图 7 和图 8)。

> IP 头的大小是变化的，从 20 字节到 60 字节，通过 IP 头的字段 ihl(Internet Header Length) 可以计算出 IP 头的实际大小，这个字段表示 IP 头的总长度占用的 32 位字的数量，也就是说，ihl x 4 就是 IP 头占用的字节数。

  ```
  struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));

  /* getting actual size of IP header */
  iphdrlen = ip->ihl * 4;
  /* getting pointer to udp header*/
  struct tcphdr *udp = (struct udphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
  ```

> 现在，指针已经指向了 UDP 头，我们来查看一下 UDP 头的字段。

* **注：**如果你的电脑使用的小端模式，需要用 ntohs 进行一下转换，因为网络上使用的大端模式。

  ```
  fprintf(log_txt, "\t|-Source Port : %d\n" , ntohs(udp->source));
  fprintf(log_txt, "\t|-Destination Port : %d\n" , ntohs(udp->dest));
  fprintf(log_txt, "\t|-UDP Length : %d\n", ntohs(udp->len));
  fprintf(log_txt, "\t|-UDP Checksum : %d\n", ntohs(udp->check));
  ```

> 用同样的方法也可以存取 TCP 头的字段。

### 从数据包中提取数据
> 传输层报头的后面就是发送的数据了，我们把指针指向数据，然后把数据打印出来。

  ```
  unsigned char *data = (buffer + iphdrlen + sizeof(struct ethhdr) + sizeof(struct udphdr));
  ```

> 下面打印数据，每行打印 16 个字节。

  ```
  int remaining_data = buflen - (iphdrlen + sizeof(struct ethhdr) + sizeof(struct udphdr));
  
  for (i = 0; i < remaining_data; i++) {
      if (i != 0 && i % 16 == 0)
      fprintf(log_txt, "\n");
      fprintf(log_txt, " %.2X ", data[i]);
  }
  ```

> 收到的数据包中的数据，应该和下面图 9 和图 10 中相似。

  ![UDP 包][img09]
  **图9: UDP 包**

  ![TCP 包][img10]
  **图10: TCP 包**

## 使用 raw socket 发送数据包
> 要发送一个数据包，首先要知道源和目的的 IP 地址和 MAC 地址，你需要另外准备一台电脑作为目的地址。有两种办法获得你本机的 IP 地址和 MAC 地址：

1. 输入命令 ifconfig 获得指定网络接口的 IP 地址和 MAC 地址
2. 在程序中使用 ioctl 获得 IP 地址和 MAC 地址

> 推荐使用第二种方法，在程序中获得 IP 和 MAC，否则，你需要在每台使用你得程序的机器上使用 ifconfig 命令。

## 打开 raw socket
> 和前面一样，打开一个 socket 要明确三件事：Socket Family - AF_PACKET；socket type - SOCK_RAW；protocol 这里使用 IPPROTO_RAW，IPPROTO_RAW 在头文件 in.h 中定义。

  ```
  sock_raw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
  if (sock_raw == -1)
      printf("error in socket");
  ```

### 关于 struct ifreq?
> Linux 支持一些标准的 ioctls，用于配置网络设备，ioctls 可以用于任何 socket 的文件描述符上，不管这个 socket 的 family 或 type 是什么，在调用 ioctl 时，会传递一个 ifreq 结构给 ioctl，当调用 ioctl 成功后，ioctl 会填充好 ifreq 结构中的字段并返回给调用者；简单地说，ifreq 结构是获取或者设置网络配置的一种方法，struct ifreq 在头文件 if.h 中定义，也可以使用在线手册 'man netdevice' 查看它的定义(参见图11)。

  ![ifreq 结构][img11]
  **图11: ifreq 结构**

  ![Graphical representation of packets with their structure and payload][img12]
  **Figure 12: Graphical representation of packets with their structure and payload**

### 获取发送数据包的接口的索引
> 你的机器上或许会有多个网络接口，比如：lookback、有线接口和无线接口，你必须确认你要从那个接口发送数据包，在确定接口后，可以按下面步骤获取这个接口的索引：

  1. 在 ifreq 结构的 ifr_name 中设置好接口的名称；
  2. 使用参数 SIOCGIFINDEX 调用 ioctl；
  3. 从 ifreq 结构中的 ifr_ifindex 字段中获得接口的索引。

  ```
  struct ifreq ifreq_i;
  memset(&ifreq_i, 0, sizeof(ifreq_i));
  strncpy(ifreq_i.ifr_name, "wlan0", IFNAMSIZ - 1); // giving name of Interface
  
  if ((ioctl(sock_raw, SIOCGIFINDEX, &ifreq_i)) < 0)
      printf("error in index ioctl reading");       // getting Index Name
  
  printf("index=%d\n", ifreq_i.ifr_ifindex);
  ```

### 获取接口的 MAC 地址
> 和上面类似，使用宏 SIOCGIFHWADDR 作为参数调用 ioctl 获取接口的 MAC 地址。

  ```
  struct ifreq ifreq_c;
  memset(&ifreq_c, 0, sizeof(ifreq_c));
  strncpy(ifreq_c.ifr_name, "wlan0", IFNAMSIZ - 1);     // giving name of Interface
  
  if ((ioctl(sock_raw, SIOCGIFHWADDR, &ifreq_c)) < 0)   // getting MAC Address
      printf("error in SIOCGIFHWADDR ioctl reading");
  ```

### 获取接口的 IP 地址
> 使用宏 SIOCGIFADDR 调用 ioctl 获取接口的 IP 地址。

  ```
  struct ifreq ifreq_ip;
  memset(&ifreq_ip, 0, sizeof(ifreq_ip));
  strncpy(ifreq_ip.ifr_name, "wlan0", IFNAMSIZ - 1);  // giving name of Interface
  if (ioctl(sock_raw, SIOCGIFADDR, &ifreq_ip) < 0)    // getting IP Address
      printf("error in SIOCGIFADDR \n");
  ```

### 构造以太网报头
> 使用 raw socket 发送数据包是需要自己构建以太网报头、IP 头和 UDP 头的，在获取了接口的索引、MAC 地址和 IP 地址后，就要开始构建以太网报头了，首先要在内存中申请一个 buffer，这个 buffer 中将放置所有的信息，比如以太网报头、IP 头、UDP 头和数据，简而言之，这个 buffer 将成为你要发送的数据包。

  ```
  sendbuff = (unsigned char *)malloc(64);   // increase in case of more data
  memset(sendbuff, 0, 64);
  ```

> 填充 ethhdr 结构中的所有字段以构建以太网报头：

  ```
  struct ethhdr *eth = (struct ethhdr *)(sendbuff);
  
  eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]);
  eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]);
  eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]);
  eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]);
  eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]);
  eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]);
  
  /* filling destination mac. DESTMAC0 to DESTMAC5 are macro having octets of mac address. */
  eth->h_dest[0] = DESTMAC0;
  eth->h_dest[1] = DESTMAC1;
  eth->h_dest[2] = DESTMAC2;
  eth->h_dest[3] = DESTMAC3;
  eth->h_dest[4] = DESTMAC4;
  eth->h_dest[5] = DESTMAC5;
  
  eth->h_proto = htons(ETH_P_IP); // means next header will be IP header
  
  /* end of ethernet header */
  total_len += sizeof(struct ethhdr);
  ```

### 构造 IP 头
> 构建 IP 头，参照以下步骤和要点：
  1. senbuff 的指针增加以太网报头的大小即可指向 IP 头的起始点；
  2. 填充 iphdr 结构中的每一个字段；
  3. IP 头后面的数据称作 IP 头的 payload，同理，以太网报头后面的数据称作以太网头的 payload；
  4. 在 IP 头里，有一个字段叫 tot_len，这个字段的含义是 IP 头长度 + payload 的长度，很显然，要知道 IP payload 的长度，需要知道 UDP 头的长度和 UDP payload 的长度，所以这个字段的值要等到填充好 UDP 头以后才能计算出来。

  ```
  struct iphdr *iph = (struct iphdr *)(sendbuff + sizeof(struct ethhdr));
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 16;
  iph->id = htons(10201);
  iph->ttl = 64;
  iph->protocol = 17;
  iph->saddr = inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
  iph->daddr = inet_addr("destination_ip"); // put destination IP address
  
  total_len += sizeof(struct iphdr);
  ```

### 构造 UDP 头
> 构建 UDP 头和构建 IP 头很类似：

  1. sendbuff 的指针增加以太网头和 IP 头的长度；
  2. 填充 udphdr 结构中个字段的值；

  ```
  struct udphdr *uh = (struct udphdr *)(sendbuff + sizeof(struct iphdr) + sizeof(struct ethhdr));
  
  uh->source = htons(23451);
  uh->dest = htons(23452);
  uh->check = 0;
  
  total_len+= sizeof(struct udphdr);
  ```

> 和 IP 头相似，UDP 头里有一个字段 len，其含义是 UDP 头的长度 + UDP payload 的长度，要填写该字段，首先要知道要发送的实际数据，也就是 UDP payload。

## 添加数据即 UDP payload
> 我们可以发送任何数据：

  ```
  sendbuff[total_len++] = 0xAA;
  sendbuff[total_len++] = 0xBB;
  sendbuff[total_len++] = 0xCC;
  sendbuff[total_len++] = 0xDD;
  sendbuff[total_len++] = 0xEE;
  ```

### 填充 IP 头和 UDP 头剩余的(还没有填充的)字段
> 前面有两个字段没有填，一个是 IP 头的 tot_len，另一个是 UDP 头的 len，按下面方法填写。

  ```
  uh->len = htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr)));
  // UDP length field
  iph->tot_len = htons(total_len - sizeof(struct ethhdr));
  // IP length field
  ```

### IP 头的 checksum
> 还有一个字段没有填，就是 IP 头中的 check 字段，这个字段是一个检查和(checksum)，该字段用于检查报头的错误。

> 数据包到达路由器时，会计算检查和，如果计算出的检查和与 IP 头中的检查和不一致，这个数据包会被路由器抛弃；如果检查和相符，路由器会把 IP 头中的 ttl(Time To Live，生命周期) 字段的值减 1，并转发该数据包。

> 计算检查和的方法是：将 IP 头中的所有 16-bit 字逐一相加，生成一个 16-bit 字，如果遇到进位，则将这个进位加到这个 16-bit 字中，最后对生成的 16-bit 字求 1 的补码，生成我们需要的检查和；对检查和进行检验时，也必须用上面的算法。

  ```
  unsigned short checksum(unsigned short* buff, int _16bitword)
  {
      unsigned long sum;
      for (sum = 0; _16bitword > 0; _16bitword--)
          sum += htons(*(buff)++);
      sum = ((sum >> 16) + (sum & 0xFFFF));
      sum += (sum>>16);
      return (unsigned short)(~sum);
  }
  
  iph->check = checksum((unsigned short*)(sendbuff + sizeof(struct ethhdr)), (sizeof(struct iphdr) / 2));
  ```

### 发送数据包
> 我们已经准备好了要发送的数据包，在发送前，我们需要在 sockaddr_ll 结构中填好目的计算机的 MAC地址。

  ```
  struct sockaddr_ll sadr_ll;
  sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex; // index of interface
  sadr_ll.sll_halen = ETH_ALEN; // length of destination mac address
  sadr_ll.sll_addr[0] = DESTMAC0;
  sadr_ll.sll_addr[1] = DESTMAC1;
  sadr_ll.sll_addr[2] = DESTMAC2;
  sadr_ll.sll_addr[3] = DESTMAC3;
  sadr_ll.sll_addr[4] = DESTMAC4;
  sadr_ll.sll_addr[5] = DESTMAC5;
  ```

  > 现在我们可以使用 sendto() 发送数据包了：

  ```
  send_len = sendto(sock_raw, sendbuff, 64, 0, (const struct sockaddr*)&sadr_ll, sizeof(struct sockaddr_ll));
  if (send_len < 0) {
      printf("error in sending....sendlen=%d....errno=%d\n", send_len, errno);
      return -1;
  }
  ```

## 运行程序
1. 进入 root 用户；
2. 在一台机器上编译和运行程序；
3. 在另一台机器上，也就是目的地址上的机器，在 root 用户下运行数据包嗅探器程序；
4. 在目的地址的机器上，分析发出的数据。

## What to do next
> We made a packet sniffer as well as a packet sender, but this is a user space task. Now lets try the same things in kernel space. For this, try to understand struct sk_buff and make a module that can perform the same things in kernel space.




> Note: you can download the complete code [here](https://www.opensourceforu.com/wp-content/uploads/2021/08/rawsocket.zip)








[img01]:/images/130004/graphical_demonstration_of_a_raw_socket.png
[img02]:/images/130004/how_a_raw_socket_works_compared_to_other_sockets.png
[img03]:/images/130004/a_generic_representation_of_a_network_packet.png
[img04]:/images/130004/Network_Packet_for_internet_Protocol.png
[img05]:/images/130004/Structure-of-Ethernet-header.png
[img06]:/images/130004/Structure-of-IP-Header.png
[img07]:/images/130004/Structure-of-TCP-Header.png
[img08]:/images/130004/Structure-of-UDP-Header.png
[img09]:/images/130004/UDP-Packet.png
[img10]:/images/130004/TCP-Packet.png
[img11]:/images/130004/Structure-of-ifreq.jpg
[img12]:/images/130004/packets_with_their_structure_and_payload.png

