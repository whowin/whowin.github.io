---
title: "使用SOCK_DGRAM类型的socket实现的ping程序"
date: 2023-03-16T16:43:29+08:00
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
  - ICMP
  - 网络编程
  - "SOCK_DGRAM"
  - ping
  - "internet checksum"
draft: false
#references: 
# - [Ping in C](https://www.geeksforgeeks.org/ping-in-c/)
# - [ping sourcecode in git](https://github.com/amitsaha/ping/blob/master/ping.c)
#     - 这个程序使用sock_dgram发送了icmp报文，但是接收是使用的recvmsg，显得很臃肿，我想是为了获得IP头里的TTL
# - [Internet Control Message Protocol](https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol)
#     - 有中文版
# - [rfc 792](https://www.rfc-editor.org/rfc/rfc792)
# - [什么是互联网控制消息协议（ICMP）？](https://www.cloudflare.com/zh-cn/learning/ddos/glossary/internet-control-message-protocol-icmp/)
# - [What does mdev mean in ping(8)?](https://serverfault.com/questions/333116/what-does-mdev-mean-in-ping8)
#     - 关于ping统计结果的计算，特别是mdev的计算
# - [Socket Timeouts](https://www.masterraghu.com/subjects/np/introduction/unix_network_programming_v1.3/ch14lev1sec2.html)
#     - 按这篇文章应该可以开一个题
postid: 180020
---

SOCK_DGRAM类型的socket常用于UDP通信，本文将尝试把这种socket用于ICMP协议，并完成一个简单的ping程序。使用ping去测试某个主机是否可用可能是一件很平常的事，尽管ping非常普通，但是编写一个实现ping功能的程序却并不是那么简单，因为ping使用的ICMP协议并不是一个应用层协议，在网上看到的实现ping的例子大多使用raw socket去实现，不仅增加了解析IP报头的麻烦，而且还需要有root权限才能运行，本文简要介绍ICMP协议，并给出一个使用普通的常用于UDP通信的socket实现ping的实例，本文将提供完整的源程序，本文的程序在 Ubuntu 20.04 下测试通过，gcc 版本号 9.4.0；阅读本文需要熟悉socket编程，对初学者而言，本文有一定的难度。
<!--more-->

## 1. 前言
* ICMP协议和UDP一样，都是面向无连接的；
* 发送一个ICMP数据包和发送一个UDP数据包非常类似，对UDP而言是构建一个UDP报头然后和数据一起发出去，对ICMP而言就是构建一个ICMP报头然后和数据一起发出去；
* 创建一个socket时，常用的socket类型有三种：SOCK_STREAM、SOCK_DGRAM和SOCK_RAW，SOCK_STREAM常用于TCP通信，SOCK_DGRAM常用于UDP通信，SOCK_RAW用于接收和发送原始数据包；
* 其实socket的种类也不止这三种，这些socket类型定义在头文件<bits/socket_type.h>中，但除了常用的这三个外，其它的基本都还没有实现，大多是因为缺少标准的协议支持，还有的是已经淘汰的socket类型，比如SOCK_PACKET；
* 可以用下面的代码测试在你的操作系统下，是否支持某个的socket类型，以SOCK_RDM为例：
  ```C
  #include <stdio.h>
  #include <sys/socket.h>

  int main() {
    int sock_fd = socket(AF_INET, SOCK_RDM, 0 );
    perror("socket: ");
    printf("sock_fd: %d\n", sock_fd);
    return 0;
  }
  ```
* 运行结果截图

  ![Screenshot for testing SOCK_RDM][img01]

* SOCK_STREAM这种socket类型显然不适合用在ping程序上，因为这种socket是面向连接的，使用之前要先建立连接，但是如果可以建立了连接就完全没有必要使用ping去测试目的主机是否可用了，SOCK_RAW过于复杂而且必须要有root权限才能运行，我们放弃不用，所以最终我们使用SOCK_DGRAM来尝试发送ICMP数据包以实现一个ping程序；

## 2. ICMP协议
* 经常编写网络程序的程序员应该都很熟悉IP协议，IP协议没有任何内在机制来发送错误和控制消息，也就是说如果网络通信出现问题，IP协议本身是无法得知原因的，所以需要ICMP协议来帮助IP协议来完成这件事；
* ICMP(Internet Control Message Protocol)协议是互联网协议族中的一个支持协议，用于在IP协议中发送控制信息，报告网络通信中可能发生的问题；
* 通常认为ICMP有两个主要作用：
  1. 报告错误
      > 当两个设备通过互联网连接时，ICMP会生成错误信息并将该信息发回到发送设备上，以防发送数据未到达其预期目的地；例如：一个数据包的长度大于某个路由器所能接收的最大长度，路由器将丢弃该数据包并将 ICMP 消息发送回数据的发送设备。

  2. 执行网络诊断
      > 常用的终端实用程序 ```traceroute``` 和 ```ping``` 都是使用**ICMP**协议运行；```traceroute``` 实用程序用于显示两个互联网设备之间的路由路径；路由路径是请求数据到达目的地之前必须经过的路由器的实际物理路径；一个路由器与另一个路由器之间的路径称为"跃点"，路由跟踪还会报告经过每个跃点所需的时间；这对于确定网络延迟来源是非常有用的。

* ICMP协议常被归为网络层协议，但是ICMP报文是被包装在一个IP报文中的，把ICMP、IP同归为网络层似乎也不是那么合适；
* 但是ICMP与常用的传输层协议TCP和UDP也有明显的不同，因为它通常不会用于在两个或多个计算机系统之间传输数据，所以把ICMP称为传输层协议似乎也不大合适，所以，也有人说ICMP协议是IP协议的附属协议或者说ICMP是介于网络层和传输层之间的中间层协议；
* ICMP是那一层的协议其实并不重要，重要的是我们可以理解和使用它；实际上在设计ICMP协议时并没有考虑OSI的网络模型，所以才造成了这种含混的现象，不过这并不影响它的使用；
* 和发送UDP报文相比，发送ICMP报文仅仅是两个协议的报头不一样而已，所以我们先了解ICMP报头结构，如果需要了解以太网报头、IP报头和UDP报头，可以参考文章：[《Linux下如何在数据链路层接收原始数据包》][article03]；
* ICMP报文的结构：

  ![Structure of ICMP packet][img02]

* ICMP报头，定义在头文件<netinet/ip_icmp.h>中
  ```C
  struct icmphdr
  {
    uint8_t type;        /* message type */
    uint8_t code;        /* type sub-code */
    uint16_t checksum;
    union
    {
      struct
      {
        uint16_t id;
        uint16_t sequence;
      } echo;               /* echo datagram */
      uint32_t gateway;     /* gateway address */
      struct
      {
        uint16_t __glibc_reserved;
        uint16_t mtu;
      } frag;               /* path mtu discovery */
    } un;
  };
  ```
  - type：ICMP报文类型，定义在头文件<netinet/ip_icmp.h>中：
    ```C
    #define ICMP_ECHOREPLY      0     /* Echo Reply             */
    #define ICMP_DEST_UNREACH   3     /* Destination Unreachable*/
    #define ICMP_SOURCE_QUENCH  4     /* Source Quench          */
    #define ICMP_REDIRECT       5     /* Redirect (change route)*/
    #define ICMP_ECHO           8     /* Echo Request           */
    #define ICMP_TIME_EXCEEDED  11    /* Time Exceeded          */
    #define ICMP_PARAMETERPROB  12    /* Parameter Problem      */
    #define ICMP_TIMESTAMP      13    /* Timestamp Request      */
    #define ICMP_TIMESTAMPREPLY 14    /* Timestamp Reply        */
    #define ICMP_INFO_REQUEST   15    /* Information Request    */
    #define ICMP_INFO_REPLY     16    /* Information Reply      */
    #define ICMP_ADDRESS        17    /* Address Mask Request   */
    #define ICMP_ADDRESSREPLY   18    /* Address Mask Reply     */
    #define NR_ICMP_TYPES       18
    ```
  - 本文只关心ICMP_ECHO和ICMP_ECHOREPLY两种报文类型，ping的实现实际上就是发送ICMP_ECHO报文，然后等待目的主机响应ICMP_ECHOREPLY报文，当然出现错误时可能会收到其他类型的报文，但本文的实例中将不予处理；
  - code：子类型编码，在我们要用到的这两种类型的ICMP报文中，code都是为0的，也就是用不上这个字段，但有些报文类型是要用到code的，比如ICMP_DEST_UNREACH类型的报文，表示目标不可达，这个报文中的code字段的值将说明目标不可达的原因；
  - checksum：ICMP报文的检查和，计算这个检查和时要包括ICMP报头和ICMP数据，其计算方法与internet checksum的计算方法一致，可以参考文章[《如何计算UDP头的checksum》][article01]和[《如何计算IP报头的checksum》][article02]
  - 由于我们仅关心ICMP_ECHO和ICMP_ECHOREPLY这两类报文，所以在ICMP报头的union中，我们使用echo这个结构：
    ```C
    struct
      {
        uint16_t id;
        uint16_t sequence;
      } echo;               /* echo datagram */
    ```
    + id：一个唯一的ID，用于标识收到的报文是由当前进程发出，一般可以使用进程ID作为这个标识，也可以是其他的任意唯一标识；
    + sequence：icmp数据包的序列号；也就是一个编号，通常使用一个递增的序号，发的第一个报文为1，第二个报文为2，......
* ICMP报头结构就是这么几个字段，但是比起UDP的报头还是要复杂一些，比如UDP报头中虽然也有checksum字段，但这个字段在IPv4中不是强制的，可以不填，但是ICMP报头中的checksum字段是必须要计算的，计算的不对报文将无法送达。 

## 2. ping程序的工作机制
* ```ping``` 程序的工作原理很像声纳回声定位，执行```ping```程序的主机向目标主机发送一个 ICMP_ECHO 请求数据包，然后目标主机返回一个 ICMP_ECHOREPLY 数据包；
* ```ping``` 程序是一个基本的Internet工具，可以验证一个 IP 或者主机名称所指向的主机是否存在并可以响应请求；
* ```ping``` 程序通过打开一个 socket 来发送 ICMP_ECHO 请求数据包，然后等待目标主机的响应，如果数据包顺利到达目标主机，而且主机也为可用状态，主机的**内核**将返回一个 ICMP_ECHOREPLY 数据包，如果出现错误，主机或者其他相关的网络设备将返回一个 ICMP 的错误信息数据包；
* 要注意的是 ICMP_ECHOREPLY 是由内核发出的，与任何应用层的程序无关；
* IP报头中有一个TTL(Time To Live)值，该值决定了路由器的最大跳数(就是报文经过的路由器最大数量，一般定为64)，当经过的路由器超过这个数量时，路由器将丢弃该数据包；
* 如果数据包没有到达，那么发送方将收到一个错误信息，错误信息有以下类型：
  1. 传输过程中 TTL 过期
  2. 目标主机不可达
  3. 请求超时(即没有收到回复)
  4. 其它原因
* 由此可见，一个ping程序可能会遇到以下类型的ICMP数据包(参考ICMP报头结构)：
  1. ICMP_ECHO请求 - type=ICMP_ECHO(8)，code=0
  2. ICMP_ECHOREPLY应答 - type=ICMP_ECHOREPLY(0)，code=0
  3. TTL 过期 - type=ICMP_TIME_EXCEEDED(11)，code=0
  4. 目标主机不可达 - type=ICMP_DEST_UNREACH(3)，code=0-5
  5. 没有收到回复 - 接收超时
* 当我们发出一个 ICMP ECHO 请求后，我们收到的回应并不一定来自目标主机，比如我们收到了一个type=3的ICMP数据包，也就是目标不可达的错误，当code=2时表示主机不可达，此时主机是不可能发出消息的，这个ICMP数据包一般会由目标主机的 gateway 发出；
* ICMP协议是一种面向无连接的协议，发送数据包之前无需像TCP一样先建立连接；

## 3. ping的简单实现
* 本文并不寻求实现 Linux 下目前存在的 ping 的所有功能，那样过于复杂，本文仅实现ping的基本功能；
* 功能列表
  1. 接收 IP 或者主机名作为输入，并可以自动识别
  2. 对主机名执行 DNS lookup，将其转换为 IP
  3. ctrl+c可以随时终止程序，并在终止时显示数据统计报告
  4. 按照一定的时间间隔向目标主机发送ICMP_ECHO报文，并等待目标主机回应ICMP_ECHOREPLY报文
  5. 接收超时则认为丢失报文
  6. 如果收到的报文不是ICMP_ECHOREPLY，则认为报文丢失

* 实现步骤
  1. **检查命令行输入的参数**
      > 判断命令行的输入是IP地址还是主机名，如果是主机名，要对主机名执行DNS lookup将其转换为IP地址
    
      ```C
      struct hostent *host;
      struct in_addr ipaddr;

      if ((host = gethostbyname(hostname)) == NULL) {
          // fail to DNS lookup
          herror("gethostbyname()");
          return -1;
      }
      ipaddr.s_addr = *(in_addr_t *)host->h_addr_list[0];
      ```
  2. **获取ICMP协议的编号**
      > 执行getprotobyname()获取ICMP的协议号是为了下面建立socket时有一个正确的协议号，其实不用这么麻烦，直接用宏IPPROTO_ICMP也是完全可以的；

      ```C
      struct protoent *protocol;
      protocol = getprotobyname("icmp")
      ```
  3. **建立socket**
      > 如果上面没有执行getprotobyname()获取协议号，这里的协议号直接用IPPROTO_ICMP没有问题；

      ```C
      int sockfd = socket(AF_INET, SOCK_DGRAM, protocol->p_proto)
      ```
  4. **设置接收超时，以避免接收时阻塞**
      > 设置接收超时是为了防止接收ICMP_ECHOREPLY时出现阻塞导致程序无法继续运行；

      ```C
      #define RECV_TIMEOUT    5

      struct timeval timeout;
      timeout.tv_sec  = RECV_TIMEOUT;
      timeout.tv_usec = 0;
      setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      ```
  5. **设置IP层的TTL(Time To Live)**
      > TTL其实就是报文最多经过多少路由器；TTL是个非常重要的参数，如果没有TTL，数据包有可能在互联网上无限循环，数据包每经过一个路由器时，TTL都会减1，当TTL为0时，路由器将丢弃该数据包；系统中会有一个默认的TTL值，一般这个默认值为64，查看默认的TTL值可以用命令 ```cat /proc/sys/net/ipv4/ip_default_ttl``` 或者 ```sudo sysctl -a|grep ip_default_ttl```，TTL最大可以为255，所以实际上不设置TTL也不会有什么问题；

      ```C
      int ttl_val = 64;

      setsockopt(sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val));
      ```
  6. **接管信号量SIGINT的处理程序**
      > 当按下ctrl+c时，将产生信号SIGINT，接管该信号意味着接管ctrl+c的处理程序；ping程序只能使用ctrl+c退出，所以接管该信号是必要的
    
      ```C
      signal(SIGINT, sigint);
      ```
  7. **记录开始时间**
      > 记录开始时间是为了退出时打印统计数据，统计数据中有一项是总的耗时时间，需要用到这个开始时间；所有时间均以时间戳的形式记录，计时的精度要达到0.00ms以上；

      ```C
      struct timeval start_time;
      gettimeofday(&start_time, NULL);
      ```
  8. **构建一个ICMP报文**
      - 一个ICMP报文就是一个ICMP报头+数据，通常一个ping数据包的数据部分为56个字节，ICMP报头为8个字节，一起为64个字节，加上IP报头20个字节，一共为84个字节，但是，IPv4报文的最大长度可以达到65535字节，所以理论上ICMP报文可以很长，不一定非要64个字节，按照ICMP协议规定，ICMP_ECHOREPLY报文会把ICMP_ECHO请求报文中的数据全部返回回来；

      - 一个ICMP报文最小长度是多少呢？IEEE 802.3标准中定义了一个以太网帧最小为64字节，这里面包含了以太网报头的14字节和帧结尾的4字节的CRC，这些占了18个字节，IP报头占用20字节，剩下留给ICMP报文的为：64 - 18 -20 = 26字节，ICMP报头占8字节，所以ICMP报文数据为18字节，其实ICMP报文长度还可以小，但没有任何意义，因为数据帧还是要填充到64字节发出去；

      - 关于ICMP报头也没什么好说的，type=ICMP_ECHO，返回来的ICMP数据包中type=ICMP_ECHOREPLY；code=0；checksum在计算前一定要先填0，这是计算checksum要求的；sequence一般是一个序号，从0或者1开始都没有关系，每发出一个数据包，sequence+1就好了；id可以填任意唯一标识，通常使用当前进程ID；

      - ICMP报文的数据部分，我们首先填了一个发送时的时间戳，ICMP_ECHO报文的数据部分会在ICMP_ECHOREPLY报文中完全返回来，所以这个时间戳会出现在收到的ICMP_ECHOREPLY报文中，我们会在收到ICMP_ECHOREPLY报文时利用这个时间戳计算icmp报文的往返时间；后面的数据我们填充上了字符'0'，完全可以什么都不填，让后面的数据为随机数据；

        ```C
        #define ICMP_DATA_SIZE      (64 - sizeof(struct icmphdr));

        unsigned char send_buf[512];
        int pack_size = sizeof(struct icmphdr) + ICMP_DATA_SIZE;
        struct icmphdr *icmp_hdr = (struct icmphdr *)send_buf;
        struct timeval *tval = (struct timevar *)(send_buf + sizeof(struct icmphdr));
        char *icmp_data = (char *)(send_buf + sizeof(struct icmphdr) + sizeof(struct timeval));

        icmp_hdr->type = ICMP_ECHO;              // ICMP_ECHO packet
        icmp_hdr->code = 0;                      // code=0
        icmp_hdr->checksum = 0;                  // checksum will be calculated later
        icmp_hdr->un.echo.sequence = pack_no;    // serial no
        icmp_hdr->un.echo.id = getpid();         // process id
    
        gettimeofday(tval, NULL);                // fill a sending timestamp into data
        int i = pack_size - sizeof(struct timeval);
        memset(icmp_data, '0', i);               // fill '0' into rest place of send_buf
        // checksum
        icmp_hdr->checksum = checksum((uint16_t *)ping_p, pack_size); 
        ```

  9. **计算ICMP报文的checksum**
      > 构建ICMP报文的最后一步就是计算checksum，这里仅给出程序，需要了解计算方法的可以参考文章[《如何计算UDP头的checksum》][article01]和[《如何计算IP报头的checksum》][article02]

      ```C
      uint16_t checksum(uint16_t *addr, int len) {
          register long sum = 0;
          uint16_t *w = addr;
          uint16_t check_sum = 0;
          int nleft = len;
      
          while (nleft > 1) {
              sum += *w++;
              nleft -= 2;
          }
          // Add left-over byte, if any
          if (nleft == 1) {
              check_sum = *(unsigned char *)w;
              sum += check_sum;
          }
          // Add carries
          while (sum >> 16)
              sum = (sum & 0xffff) + (sum >> 16);

          check_sum = ~(uint16_t)sum;     // one's complement
          return check_sum;
      }
      ```
  10. **发送ICMP_ECHO报文**
      > 像发送一个UDP报文那样，我们使用sendto()发送ICMP报文，ipaddr是在一开始执行DNS lookup时得到的目标IP地址，端口号port在这里设置为0，但实际上填上多少都没关系，比如1025，接收端会完全忽略掉这个值；

      ```C
      struct sockaddr_in dest_addr;
      d_addr.sin_family = AF_INET;
      d_addr.sin_port = htons(0);
      dest_addr.sin_addr.s_addr = ipaddr.s_addr;

      sendto(sockfd, send_buf, packet_size, 0, &dest_addr, sizeof(struct sockaddr_in));
      ```
  11. **接收返回的ICMP_ECHOREPLY报文**
      > 像接收一个UDP报文一样，我们使用recvfrom()接收报文，在第4步时已经设置了接收超时，所以这里的recvfrom()不会阻塞很久，如果产生超时，我们就认为数据包丢失；我们这里设置的接收缓冲区大小是固定的，如果你要用这个程序发送很大的ICMP_ECHO包时，小心返回的ICMP_ECHOREPLY可能无法完整接收；

      ```C
      #define RECV_BUF_SIZE       1024
      char recv_buf[RECV_BUF_SIZE];
      struct sockaddr_in from;
      int from_len = sizeof(struct sockaddr_in);

      recvfrom(sockfd, recv_buf, RECV_BUF_SIZE, 0, (struct sockaddr *)&from, &from_len);
      ```
  12. **检查收到的报文的checksum**
      > 在计算收到的报文的checksum之前，要先检查这个报文是不是一个ICMP_ECHOREPLY报文，本例不处理其它ICMP报文；其次要检查ICMP报头中的ID是否为当前进程的ID(在发送ICMP_ECHO报文时设置的)，第三要通过ICMP报头中的sequence判断该报文是否为重复报文(就是已经收到过相同sequence的报文)；

      > 用前面的checksum()运算一个带有checksum字段的ICMP报文，其结果应该为0，否则就是报文有问题。

  13. **计算报文的往返时间，填写统计数据**
      > 在发送ICMP_ECHO报文时，我们在数据包的数据部分填写了一个发送时的时间戳，为了统计数据需要，我们需要记录下面几个数据，每组icmp报文的往返时间之和**sum_time**、每组往返时间平方之和**qsum_time**、最小往返时间**min_time**和最大往返时间**max_time**，当然还要记录发出的icmp报文的数量**nsend**和收到的icmp报文的数量**nreceived**，至于这些数据的应用，我们在后面会提到；

      ```C
      struct timeval *send_time_p = (struct timeval *)(recv_buf + sizeof(struct icmphdr));
      struct timeval recv_time;
      gettimeofday(&recv_time, NULL);             // Receive time

      float interval = (recv_time.tv_sec - send_time_p->tv_sec) * 1000.00 + ((recv_time.tv_usec - send_time_p->tv_usec) * 1.00) / 1000;

      sum_time += interval;
      qsum_time += (interval * interval);
      if (interval < min_time) min_time = interval;
      if (interval > max_time) max_time = interval;
      ```
  14. **返回步骤8，发送下一个报文**
      > 当然要先打印出当前icmp报文的状况后再返回步骤8，开始发送下一个报文；

  15. **ctrl+c处理程序**
      > 前面我们拦截了ctrl+c的信号，这个信号的处理非常简单，只要使发送-接收icmp报文的循环结束即可，实际上就是在步骤14时不要再返回步骤8，而是直接打印统计结果然后退出程序；本例我们使用了一个公共变量ping_loop来控制循环，ping_loop=true时循环继续，否则循环停止；

      ```C
      void sigint(int signum) {
          ping_loop = false;
      }
      ```
  16. **打印统计数据**
    - 我们先看一下Linux(Ubuntu)提供的ping程序的统计结果输出的截屏

      ![screetshot of ping statistics][img03]

    - 其统计数据有
      1. 发送的ICMP_ECHO报文数量：nsend
      2. 接收到的ICMP_ECHOREPLY报文数量：nreceived
      3. 丢失的ICMP_ECHOREPLY报文数量：nsend - nreceived
      4. 最小往返时间(min)：min_time
      5. 最大往返时间(max)：max_time
      6. 平均往返时间(avg)：$\large {\sum_{1}^{n}rtt \over n} = {{sum\_time} \over nreceived} $ 
        > rtt是Round Trip Time的意思，意即数据包的往返时间

      7. 平均偏差(mdev)
        > mdev是Mean Deviation的意思，它表示这些ICMP的往返时间rtt偏离平均值的程度，一般认为这个值越大，网络的稳定性越差，这个值的计算公式为：

        $$
        {\sqrt{{\sum{x_i^2} \over n} - ({\sum{x_i} \over n})^2}} = {\sqrt{{qsum\_time \over nreceived} - {({sum\_time \over nreceived})^2}}} = {\sqrt{{qsum\_time \over nreceived} - avg^2}}
        $$
    - 下面是统计数据的主要代码
      ```C
      float avg = 0.0, mdev = 0.0;
      if (nreceived) {
          avg = (sum_time * 1.00) / nreceived;
          mdev = sqrt(((qsum_time * 1.00) / nreceived) - (avg * avg));
      }
      ```
* 完整源代码，文件名：[ping-dgram.c][src01](**点击文件名下载源程序**)
* 编译，因为在统计部分使用了数学函数，所以编译时要加上 ```-lm``` 选项，意即连接数学函数库
  ```bash
  gcc -Wall ping-dgram.c -o ping-dgram -lm
  ```
* 运行：```./ping-dgram baidu.com```
* 运行截图

  ![screenshot of ping-dgram][img04]

## 4. 后记
* ping的输出中有一项是TTL，文中多次提到了这个值的意义，本例输出的这个值可能并不准确，这个值的准确值是放在IP报头中的，但本例使用的方法是读取不到IP报头的，所以无法取得准确的TTL值，这应该是使用SOCK_DGRAM类型的socket编写ping程序的一个小缺陷，在本例中我们使用了初始化socket时设置的TTL值进行了显示；
* 代码中对重复报文做了判断，其判断本身也并不是很准确，正常情况下是不应该收到重复的ICMP_ECHOREPLY报文的，但根据我的经验，除了在局域网中有重复IP的情况以外，多发生在网络中有多个并行路由器的情况下，有时是因为某台机器开启了某些有路由功能的进程，比如网络中有一台运行openWrt的机器，很可能上面就会运行有一些操作路由的进程，但是更详细的产生重复ICMP报文的情况并不十分清楚，因为这种情况并不多见，也不太容易捕捉到；
* 本例中仅仅处理了ICMP_ECHOREPLY报文，但实际上一个ICMP_ECHO报文发出后，只有在一切正常的情况下才返回ICMP_ECHOREPLY报文，如果出现错误，会返回其他类型的报文，比如：ICMP_DEST_UNREACH报文，表示报文没有到达目的地，code中将标识没有到达的原因，其实这类报文更有意义，如果读者有兴趣，可以在本例的基础上进行扩展，写出更完美的ping程序；
* 文中提到，一个ICMP报文可以很长，如果想用本例中的程序去测试比较长的ICMP报文，请注意本例的接收缓冲区的大小是一个固定值，请进行调整，避免缓冲区溢出或接收不到完整数据包的情况发生。


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://whowin.gitee.io/sourcecodes/180020/ping-dgram.c

<!--gitee
[article01]:https://whowin.gitee.io/post/blog/network/0003-checksum-of-udp-header/
[article02]:https://whowin.gitee.io/post/blog/network/0004-checksum-of-ip-header/
[article03]:https://whowin.gitee.io/post/blog/network/0002-link-layer-programming/
-->
[article01]:https://blog.csdn.net/whowin/article/details/128766194
[article02]:https://blog.csdn.net/whowin/article/details/128846658
[article03]:https://blog.csdn.net/whowin/article/details/128766145

[img01]:https://whowin.gitee.io/images/180020/screenshot-for-testing-sock-rdm.png
[img02]:https://whowin.gitee.io/images/180019/structure-of-icmp-packet.png
[img03]:https://whowin.gitee.io/images/180020/screenshot-of-ping-statistics.png
[img04]:https://whowin.gitee.io/images/180020/screenshot-of-ping-dgram.png
