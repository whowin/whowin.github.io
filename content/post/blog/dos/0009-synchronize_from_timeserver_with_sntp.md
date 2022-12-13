---
title: "如何与时间服务器进行时间同步"
date: 2008-04-16T13:36:00+08:00
author: whowin
sidebar: false
authorbox: false
pager: true
toc: true
categories:
  - "DOS"
  - "network"
tags:
  - "DOS"
draft: true
postid: 160009
---

在互联网上发布精确时间，要依靠NTP（Network Time Protocol）协议，互联网上有许多时间服务器，负责提供精确时间，本文简要描述了 NTP 协议的原理，并实际实现了 SNTP(Simple Network Time Protocol) 下的客户端。
<!--more-->

## 1、NTP协议的基本原理

* 假定时间服务器是 A，客户端是B，则同步过程如下进行：
  1. B 向 A 发送一个消息包，记录发出消息包时的时间戳 t1（以 B 机时间为基准）
  2. A 收到消息包立即记录时间戳 t2（以 A 机时间为基准）
  3. A 向 B 返回一个消息包，返回消息包时记录时间戳 t3（以 A 机时间为基准）
  4. B 收到 A 返回的消息包，此时记录时间戳 t4（以 B 机时间为基准）

* t4 和 t1 是以 B 的时间标准记录的时间戳，其差 t4 - t1 表示整个消息传递过程所需要的时间间隔；
* t3 和 t2 是以 A 机的时间标准记录的时间戳，其差 t3 - t2 表明消息传递过程在 A 机逗留的时间；
* 那么 (t4 - t1) - (t3 - t2) 应该就是信息包从 B 到 A，再从 A 传回 B 的时间（中间刨去了在 A 机的逗留时间）；
* **如果假定信息包从 A 到 B 和从 B 到 A 所用的时间一样**，那么，从 A 到 B 或者从 B 到 A 信息包的传送时间 d 为：
  ```
  d = ((t4 - t1) - (t3 - t2)) / 2
  ···
* 假定 B 机相对于 A 机的时间误差是 c，则有下列等式：
  ```
  t2 = t1 + c + d
  t4 = t3 - c + d
  ```
* 从以上三个等式可以解出B机的时间误差 c 为：
  ```
  c = ((t2 - t1) + (t3 - t4)) / 2
  ```
* 如果一时没有转过来，可以自己在纸上画个图，在细细地琢磨一下，应该没有问题。

## 2、简单网络时间协议SNTPv4（Simple Network Time Protocol version 4）
* SNTPv4 由 NTP 改编而来，主要支持同步网络计算机时钟机制。SNTPv4 没有改变 NTP 规范和原有实现过程，它是对 NTP 的进一步改进，支持以一种简单、无状态远程过程调用模式执行精确而可靠的操作，这类似于UDP/TIME 协议。

## 3、SNTP(NTP)的协议结构
  ```
                    1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          Root Delay                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       Root Dispersion                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Reference Identifier                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                   Reference Timestamp (64)                    |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                   Originate Timestamp (64)                    |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                    Receive Timestamp (64)                     |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                    Transmit Timestamp (64)                    |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                 Key Identifier (optional) (32)                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                                                               |
  |                 Message Digest (optional) (128)               |
  |                                                               |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  ```
* LI(Leap Indicator)：闰秒指示，2 bits，bit 0和bit 1
  > 表示是否警告在当天的最后一分钟插入/删除一个闰秒。通常填0表示不需要警告。
* VN(Version Number)：NTP/SNTP版本号，3 bits
  > 这里我们填3，表示版本 3（IPv4）
* Mode：工作模式，3 bits，这里我们要填 3，表示是客户端方式，时间服务器返回时将把这个字段改为 4，表示是服务器工作模式
* Stratum：表示所处的层，8 bits。
  - 这是因为 NTP 的网络体系是分层结构，按照离外部 UTC(国际标准时间)源的远近将所有服务器归入不同的 Stratum(层)中，Stratum-0层为官方时钟所保留，Stratum-1在顶层，有外部 UTC 接入，而 Stratum-2 则从 Stratum-1 获取时间，Stratum-3 从 Stratum-2 获取时间，以此类推，但 Stratum 层的总数限制在 15 以内。所有这些服务器在逻辑上形成阶梯式的架构相互连接，而 Stratum-1 的时间服务器是整个系统的基础。
  - 这个字段一般填 0 即可。
* Poll Interval：表示连续信息间的最大间隔，8 bits，以 2 的 x 次幂秒的形式表示。实际填 0 即可。
* Precision：时间精度，以 2 的负 x 次幂秒表示，8 bits。实际填 0，服务器返回时会填写实际精度。
* Root Delay：到主参考时间源的往返总延迟。32 bits 固定小数点数，小数点在 bit 15 和 bit 16 之间。
* Root Dispersion：相对于主参考时间源的正常离差。32 bits 固定小数点数，小数点在 bit 15 和 bit 16 之间。
* Reference Identifier：用于识别特殊的参考源。
* Reference Timestamp：本地时中最后一次设置或修正时的时间，64bits，timestamp 格式。
* Originate Timestamp：前面原理部分说到的 t1
* Receive Timestamp：前面原理部分说到的 t2
* Transmit Timestamp：前面原理部分说到的 t3
* Authenticator（可选）：可选项。一般不填。
* 有对协议感兴趣的读者，可以在下面地址下载SNTP协议进行深入的研究: https://www.rfc-editor.org/rfc/pdfrfc/rfc1769.txt.pdf

## 4、SNTP的实际实现
* 说起来一大堆，但实现起来其实并不像说的那么复杂。
* 根据 SNTP 协议第 5 节《SNTP Client Operations》的说明，如果采用 unicast 模式，其目的就是为了获得准确时间的话，向服务器发送的请求包中，除了第一个字节以外，其他的 SNTP 字段都可以设为 0，这正好与我们的目的相符，下面我们给出实现本机与时间服务器同步的源程序。
* 还有两个细节要说明，一是 NTP 实用的端口号是 123，二是 NTP 的 header 中的字符顺序是 big-endian，也就是 Network Byte Order，而我们本机的字符顺序是 little-endian，所以在收到服务器的回应后要注意转换。
* 为方便说明，源程序前面加了行号。
  ```
  001  #include <stdio.h>
  002  #include <dos.h>
  003  #include <sys/socket.h>
  004  #include <arpa/inet.h>

  005  #define  SNTP_PORT       123
  006  #define  SNTP_SERVER     "210.72.145.44"     // China
  007  //#define  SNTP_SERVER     "192.43.244.18"     // NIST : sometimes ok
  008  //#define  SNTP_SERVER     "192.5.41.40"       // U.S Naval Observatory : ok
  009  //#define  SNTP_SERVER     "128.102.16.2"
  010  #define  SNTP_EPOCH      86400U * (365U * 70U + 17U)
  011  #define  SNTP_8HOUR      3600U * 8U

  012  struct Sntp_Header {
  013    unsigned char LiVnMode;
  014    unsigned char Stratum;
  015    unsigned char Poll;
  016    unsigned char Precision;
  017    long int RootDelay;
  018    long int RootDispersion;
  019    char RefID[4];
  020    long int RefTimeInt;
  021    long int RefTimeFraction;
  022    long int OriTimeInt;
  023    long int OriTimeFraction;
  024    long int RecvTimeInt;
  025    long int RecvTimeFraction;
  026    long int TranTimeInt;
  027    long int TranTimeFraction;
  028  };

  029  int main() {
  030    int retValue;
  031    fd_set readfds;
  032    // Structure of SNTP
  033    struct Sntp_Header sntpHeader;
  034    struct Sntp_Header sntpHeader1;
  035    struct Sntp_Header *p;
  036    char *p1;
  037    // vars of network
  038    int sendSock;
  039    struct sockaddr_in toAddr;
  040    int addrLen;
  041    char *pBuf;
  042    long int OriTimeInt;
  043    long int DestTimeInt;
  044    long int difference;
  045    unsigned char tempChar;

  046    struct timeval tv;

  047    struct date dateNow;
  048    struct time timeNow;

  049    sendSock = socket(AF_INET, SOCK_DGRAM, 0);

  050    if (sendSock < 0) {
  051      printf("\nsendSocket Creation Fail!");
  052      return -1;
  053    }

  054    toAddr.sin_family      = AF_INET;
  055    toAddr.sin_port        = htons(SNTP_PORT);
  056    toAddr.sin_addr.s_addr = inet_addr(SNTP_SERVER);
  057    bzero(&(toAddr.sin_zero), 8);
  058    addrLen = sizeof(struct sockaddr);

  059    bzero(&sntpHeader, sizeof(struct Sntp_Header));
  060    sntpHeader.LiVnMode = 0x1b;

  061    OriTimeInt = time(0) + SNTP_EPOCH - SNTP_8HOUR;
  062    retValue = sendto(sendSock, &sntpHeader, sizeof(struct Sntp_Header), 0,
                           (struct sockaddr *)&toAddr, addrLen);
  063    printf("\nSend %d chars.", retValue);
  064    p1 = (char *)&sntpHeader;
  065    pBuf = (char *)&sntpHeader1;

  066    printf("\n\tSent...\t\t\t\t\tReceived...");
  067    for (int j = 0; j < 12; j++) {
  068      FD_ZERO(&readfds);
  069      FD_SET(sendSock, &readfds);
  070      tv.tv_sec = 10;
  071      tv.tv_usec = 0;
  072      select(sendSock + 1, &readfds, NULL, NULL, &tv);
  073      if (FD_ISSET(sendSock, &readfds)) {
  074        retValue = recvfrom(sendSock,
                                 &pBuf[j * 4],
                                 4,
                                 0,
                                 (struct sockaddr *)&toAddr,
                                 &addrLen);
  075      } else {
  076        printf("\nDid not Get information from time server");
  077        return -1;
  078      }

  079      if (retValue <= 0 ) {
  080        printf("\nReceiving Fail");
  081        return -1;
  082      }
  083      printf("\n");
  084      for (int i = 0; i < retValue; i++) {
  085        printf("\t%02x", (unsigned char)p1[i + j * 4]);
  086      }
  087      printf("\t");
  088      for (int i = 0; i < retValue; i++) {
  089        printf("\t%02x", (unsigned char)pBuf[i + j * 4]);
  090      }
  091    }

  092    for (int j =  4; j < 12; j++) {
  093      tempChar = *(pBuf + j * 4);
  094      *(pBuf + j * 4) = *(pBuf + j * 4 + 3);
  095      *(pBuf + j * 4 + 3) = tempChar;
  096      tempChar = *(pBuf + j * 4 + 1);
  097      *(pBuf + j * 4 + 1) = *(pBuf + j * 4 + 2);
  098      *(pBuf + j * 4 + 2) = tempChar;
  099    }

  100    p = (struct Sntp_Header *)pBuf;

  101    DestTimeInt = time((time_t *)NULL) + SNTP_EPOCH - SNTP_8HOUR;    // t4
  102    printf("\nLocal TimeStamp = %lu", DestTimeInt);
  103    printf("\tRefTimeInt = %lu", p->RefTimeInt);
  104    printf("\nOriTimeInt = %lu", OriTimeInt);                        // t1
  105    printf("\tRecvTimeInt = %lu", p->RecvTimeInt);                   // t2
  106    printf("\nTranTimeInt = %lu", p->TranTimeInt);                   // t3
  107    difference = (p->RecvTimeInt - OriTimeInt) + (p->TranTimeInt - DestTimeInt);
  108    difference = difference / 2;
  109    printf("\tdifference = %ld", difference);

  110    tv.tv_usec = 0;
  111    tv.tv_sec = time(0) + difference;
  112    getdate(&dateNow);
  113    gettime(&timeNow);
  114    printf("\nDate and Time Before adjusting : %04d-%02d-%02d %02d:%02d:%02d",
                dateNow.da_year, dateNow.da_mon, dateNow.da_day,
                timeNow.ti_hour, timeNow.ti_min, timeNow.ti_sec);
  115    retValue = settimeofday(&tv);
  116    if (retValue == 0) {
  117      printf("\nSetting Time ok!");
  118    } else {
  119      printf("\nSetting Time Fail!");
  120    }
  121    getdate(&dateNow);
  122    gettime(&timeNow);
  123    printf("\nDate and Time after adjusting : %04d-%02d-%02d %02d:%02d:%02d\n",
                dateNow.da_year, dateNow.da_mon, dateNow.da_day,
                timeNow.ti_hour, timeNow.ti_min, timeNow.ti_sec);
  124    return 0;
  125  }
  ```

* 下面就程序的某些部分作一些说明。
  - 第 5 行是 NTP 的端口号
  - 第 6 行是我们要连接的时间服务器的 IP 地址，这是中科院国家授时中心的服务器，这几个时间服务器还是我在一年前某项目中使用 NTP 时找的时间服务器，在完成本篇文章之前，我又对这几台服务器做了测试，除第 9 行的服务器没有连接成功外，其余都没有问题。
  - 第 10 行的这个常数 SNTP_EPOCH 是计算了1970年1月1日0点0分0秒到1900年1月1日0点0分0秒之间的秒数，其中 86400 是每天的秒数，365 是一年的天数，70 是 1970 年到 1900 年一共 70 年，17 是在这 70 年中的闰年次数(每 4 年一次)。为什么要计算这个数呢？这是因为本机中的时间戳 timestamp 是以1970年1月1日为基准的，而 NTP 中的时间戳 timestamp 是以1900年1月1日为基准的，所以我们要使用这个常数进行转换。
  - 第 11 行的常数 SNTP_8HOUR 是因为北京的时区是 GMT + 8，有 8 个小时的时差，而时间服务器发布的时间都是国际标准时间，所以在转换时间戳时要用这个常数。
  - 第 12--28 行定义的结构是SNTP的请求头(Request Header)，发送信息包和接收信息包将以这个结构为基础。
  - 进入主程序 main 后，第 30--48 行定义了一些变量和结构，其中结构包括：sntpHeader 和 sntpHeader1 在前面已经有说明；sockaddr_in 在上一篇博文《在DOS下进行网络编程（下）》中有说明；timeval、date 和 time 请在 rhide 中查阅 libc 的在线文档，包括使用这几个结构的函数可以一起查；fd_set 通常叫做文件描述符集，在本文中是为了解决网络编程的阻塞问题而使用的，关于 fd_set、相关的宏以及 select() 的使用方法可以参考《Beej's Guide to Network Programming Using Internet Sockets》中的介绍，在以前的博文中介绍过如何获得这篇文章，或者以后有机会我写文章介绍。
  - 49--58 行的程序我们在上一篇博文《在DOS下进行网络编程（下）》中有说明。
  - 59、60 行，我们填了一个SNTP的请求头，只把第一个字节填上了 0x1b，其余字节均为 0，0x1b 展开为二进制为：00011011b，根据字段定义，LI=00b(无警告)，VN=011b(版本3)，mode=011b(client模式)。
  - 61行我们计算了发送时的时间戳，因为最后要用于计算本地时间的偏差，要与服务器返回的时间戳配合计算，所以我们给它转换成国际标准时间的时间戳。
  - 62行，我们采用UDP方式将SNTP的请求头发出。
  - 68--78 行是一种无阻塞接收信息的方式，70、71 行设定了一个 10 秒的超时，72 行的 select() 最多等待 10 秒钟，73 行检查是否有数据可读，如果有，才执行 recvfrom() 读取信息，这样就避开了阻塞得问题，有于 UPD 不会像 TCP 那样在通讯之前要进行 connect()，所以，无法收到信息的可能性更大，如果使用阻塞得方式，程序锁死的可能性相对要大一些。
  - 第 67 行的 for 语句告诉我们，我们并不是一下收取所有的信息，而是每次只收 4 个字节，共收 12 次，完成信息包的接收，这样做并没有什么特别的目的，只是为了打印数据方便。
  - 第 84--86 行我们在屏幕上打印了我们发出的信息包，第 88--90 行，我们打印了服务器返回的信息包，在屏幕上非常直观，可以很方便地进行比较。
  - 前面说过，服务器返回的数据字符顺序是 big-endian，本机的字符顺序是 little-endian，第 92--99 把收到的 Receive Timestamp 和 Transmit Timestamp 两个字段的字符顺序转换成 little-endian。
  - 101 行计算了收到服务器返回包的时间戳。
  - 107、108 行根据前面描述的原理计算了本机时间的偏差。
  - 102--109 行之间有一些 printf 语句，打印了一些我们计算所需要的数据。
  - 115 行重新设置了本机的日期、时间，在设置前后分别打印了本机的时间。
* 至此，程序解释完毕，该程序在 DOS6.22，DJGPP v2，WATT-32 下编译通过并运行良好。

> 下一篇文章计划写如何在DOS对AC'97的声卡进行编程，会举一个实例，敬请期待。