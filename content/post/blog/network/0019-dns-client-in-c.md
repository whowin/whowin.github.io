---
title: "用C语言实现的一个DNS客户端"
date: 2023-03-28T16:43:29+08:00
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
  - DNS
  - 网络编程
  - "dns resolver"
  - “dns client"
  - UDP
draft: false
#references: 
# - [rfc 1034: Domain Concepts and Facilities](https://www.ietf.org/rfc/rfc1034.txt)
#     - 第5章：5. RESOLVERS 描述的一个DNS解析器的工作方式
# - [rfc 1035: Domain Implementation and Specification](https://www.ietf.org/rfc/rfc1035.txt)
# - [Domain Name System](https://en.wikipedia.org/wiki/Domain_Name_System)
# - [4.6. UDP Socket Programming: DNS](https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/UDPSockets.html)
# - [gfw_dns_resolver](https://github.com/examplecode/gfw_dns_resolver)
#     - dns解析程序
postid: 180019
---

DNS可以帮助我们把域名映射到一个IP地址上，或者查询一个IP地址下有那些域名，使用域名访问一个网站或者服务器是一件很平常的事情，很少有人关心域名变成IP地址的实际过程，本文将使用C语言实现一个基本的DNS解析器，通过与DNS服务器的通信完成将一个域名转换成IP地址的过程，本文将提供完整的源程序；阅读本文需要有一定的网络编程基础，熟悉基本的socket编程并对DNS有一些了解，本文对网络编程的初学者难度较大。
<!--more-->

## 1. 目标
* 本文要实现一个DNS的客户端解析器(DNS resolver)，意即通过直接与DNS服务器通讯，将一个域名转换成其所对应的IP地址；
* 对DNS客户端解析器的要求：
  1. 命令行接受用户输入的域名
  2. 向DNS服务器发出查询请求，并将查询结果显示在屏幕上
  3. 仅查询域名的A记录(QTYPE=HOST，QCLASS=IN)，后面会讨论相关细节
  4. 如果查询结果有多条记录，要求显示所有查询结果
  5. 如果查询的域名为别名(Alias)，要求显示其实际域名(Canonical Name)
  6. 仅查询IPv4地址。
* 在C语言编程中，当需要将一个域名转换成IP地址时，通常是使用getaddrbyname()或者getaddrinfo()函数，这两个函数会使用系统设定的DNS服务器，本文实现的DNS客户端将使用自己定义的DNS服务器；
* 一个DNS的客户端无非就是按照一定的格式向DNS服务器发送一个报文，然后接收来自DNS服务器的响应，并解析收到的信息，从而获得结果。

## 2. DNS协议
* 要编写一个DNS客户端程序，了解DNS协议是必须的，本节将仅对我们有用的有关DNS协议中的内容加以说明；看协议是枯燥的，也可以先不看，到后面需要时再回来查阅；
* DNS协议的主要内容包含在下面两个文件中
  - [rfc 1034: Domain Concepts and Facilities][article01]
  - [rfc 1035: Domain Implementation and Specification][article02]

* rfc 1035中对一些参数的最大值做了限制
  - labels - 最多63个字符
  - names - 最多255字符
  - TTL - 32bit有符号数字，只能是正数
  - UDP messages - 最多512个字符
* 这些限制告诉我们：
  1. 一个域名最长为255个字符，以'.'分开的各个部分，每部分最多63个字符
  2. 使用UDP与DNS通信时，每个报文的长度不能超过512个字节
  3. TTL(Time To Live)，指查询到的一个DNS信息的生命周期，过了这个时间，这条信息即为作废，应该重新查询；

* 在DNS的各种文章中，会经常看到**RR**的表述，这个是Resource Record的缩写，从DNS服务器返回的各种信息，都会存储在RR中；
* RR其实就是一个符合某种格式的数据结构，RR是DNS协议中非常重要的一个结构，rfc 1035(3.2)中，对**RR**做了定义：
  ```plaintext
                                  1  1  1  1  1  1
    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                                               |
  /                                               /
  /                      NAME                     /
  |                                               |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                      TYPE                     |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                     CLASS                     |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                      TTL                      |
  |                                               |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                   RDLENGTH                    |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
  /                     RDATA                     /
  /                                               /
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  ```
* 其中
  - NAME - 拥有者名称，意即该资源记录RR所属的节点名称；实际上就是个域名，这个字段的长度是可变的，下面会详细说明其记录方式；
  - TYPE - RR的类别，占2个字节；常用的TYPE如下，更多信息请查阅[rfc 1035][article02]第3.2.2；本文中，会用到TYPE=A和TYPE=CNAME两种；
    ```
    TYPE  VALUE   MEANING
    -----------------------------
    A       1     主机地址
    CNAME   5     别名的正式名称
    MX      15    邮件交换
    TXT     16    文本信息
    ```

  - CLASS - RR的适用的网络类别；CLASS的值常用的只有一个，即CLASS=IN，其值为1，表示互联网(Internet)
  - TTL - 32bit的正整数表示该RR可以被缓存的时长，以秒为单位；该值为0时表示该RR只能用于当前事务，不能被缓存；
  - RDLENGTH - 16bit无符号整数；该值表示RDATA字段的长度(字节数)；
  - RDATA - 用于描述资源的可变长度字串；其格式取决于TYPE和CLASS字段的值，比如当TYPE=A时，RDATA中是一个32bit的IP地址。

* 在[rfc 1035][article02]的第4章定义了DNS协议的消息格式，向DNS服务器发送的查询报文以及DNS服务器的返回报文都符合这个格式；
  ```plaintext
  +---------------------+
  |        Header       |
  +---------------------+
  |       Question      | the question for the name server
  +---------------------+
  |        Answer       | RRs answering the question
  +---------------------+
  |      Authority      | RRs pointing toward an authority
  +---------------------+
  |      Additional     | RRs holding additional information
  +---------------------+
  ```
* 在这个报文格式中，不管是查询请求报文还是应答报文，都会有一个报头(Header)、在查询请求报文中，显然不需要有Answer、Authority和Additional三部分；
* Question部分有自己的格式，Answer、Authority和Additional这三部分的格式是一样的，下面将就这三种格式(Header、Question、Answer)做个简要说明；
* 如果觉着这部分枯燥也可以先跳过，等看不懂程序时才回来查阅；

* **Header的格式**
  - Header部分占了12个字节
    ```plaintext
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ```
  - 其中
    + ID - 随机标识，16bit长，随便填一个数即可；
    + QR - Query Response；0表示该报文为查询报文，1表示该报文为应答报文；
    + Opcode - Operation Code；表示报文的查询类型，该值由查询的发起方设置，并复制到应答报文中；0表示一个标准查询(QUERY)，1表示一个反向查询(IQUERY)，2表示查询服务器状态(STATUS)，3-15备用；
    + AA - Authoritative Answer；权威答案，该位仅在响应报文中有效，该位为1表示当前名称服务器对所查询的域名具有权威性；
    + TC - TrunCation；该位为1表示该报文不完整；由于该报文的长度过长，超过了传输通道允许的最大长度，该报文被截断；
    + RD - Recursion Desired；该位为1表示要求DNS服务器做递归查询；如果在查询报文中设置了RD，在应答报文中将复制该位；
    + RA - Recursion Available；递归可用，在应答报文中将该位置1表示名称服务器支持递归查询，将该位清0表示不支持递归查询(因为协议规定递归查询功能是可选的)；
    + Z  - Reserved；备用
    + RCODE - Response code；响应码，在相应报文中设置，按照 [rfc 1035][article02] 的定义，其值的含义如下：

      |值|含义|
      |:--:|:----|
      |0|没有错误|
      |1|格式错误 - 名称服务器无法解释查询报文|
      |2|服务器故障 - 由于服务器故障，无法处理此查询|
      |3|名称错误 - 仅对来自权威名称服务器的响应有意义，表示查询的域名不存在|
      |4|功能未实现 - 名称服务器不支持所请求的查询类型|
      |5|拒绝 - 名称服务器出于政策原因拒绝执行指定的操作|
      |6-15|备用|

    + QDCOUNT - 无符号16位整数，表示该报文中 **QUESTION部分** 有多少条查询请求；
    + ANCOUNT - 无符号16位整数，表示该报文中 **ANSWER部分** 有多少条RR(Resource Record)；
    + NSCOUNT - 无符号16位整数，表示该报文中 **Authority部分** 有多少条有权威性的RR(Resource Record)
    + ARCOUNT - 无符号16位整数，表示该报文中 **Additional部分** 有多少条附加的RR(Resource Record)

* **Question的格式**
  - Question部分是可变长度的
    ```plaintext
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                     QNAME                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QTYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QCLASS                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ```
  - 其中QNAME是可变长度的，QTYPE和QCLASS各占2个字节，16bits
  - QNAME - 域名；以标签(label)方式表示的域名；每个标签的第一个字节表示这个标签的长度，后面跟着与该长度相同的字符，多个标签组成一个域名，标签的最后填充1个字节的0，表示标签结束；举例来说，www.baidu.com将用如下方式表达(以16进制表示)：
    ```plaintext
    length------length------------length------end 
    03 77 77 77 05 62 61 69 64 75 03 63 6f 6d 00
       w  w  w     b  a  i  d  u     c  o  m
    ```
  - 要注意的是，这个字段的长度(字节数)可能是奇数，不需要填充以保证4字节或2字节对齐；
  - QTYPE - 2字节，表示查询类型；在前面介绍RR时曾介绍了RR中的TYPE字段，所有TYPE的值这里都使用，本文用到的有：QTYPE=1(A记录)，QTYPE=5(CNAME记录)
  - QCLASS - 2字节，表示网络类别，常用的值只有1个，即CALSS=1(Internet - 表示互联网)

* **Answer的格式**
  - Answer、Authority、Additional的格式是一样的，就是前面介绍的RR的格式，只是这里可能放着多个RR，其具体数量由Header中的ANCOUNT、AUCOUNT和ARCOUNT确定；
  - 我们把RR的的格式在这里再展示一下：
    ```plaintext
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                                               /
    /                      NAME                     /
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TTL                      |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   RDLENGTH                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
    /                     RDATA                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ```
  - 要说明的是，为了减小报文的长度，其NAME部分常常采用一种压缩方案(Compression Scheme)；因为在服务器发回的应答报文中，会包含查询请求报文中的QUESTION字段，这个字段中已经存放了域名，如果在ANSWER中再存放一次域名，显然是重复的，所以通常采用压缩方案；
  - 比如我们查询 baidu.com 的A记录，在发出DNS请求的报文中，会包含 baidu.com 这个域名，在应答报文中，会把请求报文中的 question 部分复制过来，那么在 answer 部分的 RR 中的 baidu.com 与 question 中的 baidu.com 就是重复的，这时候，在 anwser 部分的 baidu.com 就会采用压缩方案存储；
  - 采用标签方式存储域名时，第一个字符用于表示这个标签的长度，DNS协议规定，当这个长度字节的最高两位为0时，后面的6位表示这个标签的长度，当这个字节的最高两位全为1时，这个字节的后面6位连同下一个字节一起表示一个从报文首字节开始的偏移，指向一个域名；
  - 前面说过，一个域名的总长度最大为255个字符，以'.'分开的各个部分，每部分的最大长度为63个字符，**为什么会限制为63个字符呢**？因为在DNS协议里规定只有6 bit用于表示域名每部分的长度，所以每部分的长度最大只能是 2<sup>6</sup> - 1 = 63
  - 我们用一个实际的例子来说明，这个例子是查询域名 baidu.com 的 A 记录时，实际返回的数据：
    ```
    0000:   0xf1  0x01  0x81  0x80  0x00  0x01  0x00  0x02  
    0008:   0x00  0x00  0x00  0x00  0x05  0x62  0x61  0x69  
    0010:   0x64  0x75  0x03  0x63  0x6f  0x6d  0x00  0x00  
    0018:   0x01  0x00  0x01  0xc0  0x0c  0x00  0x01  0x00  
    0020:   0x01  0x00  0x00  0x01  0x72  0x00  0x04  0x6e  
    0028:   0xf2  0x44  0x42  0xc0  0x0c  0x00  0x01  0x00  
    0030:   0x01  0x00  0x00  0x01  0x72  0x00  0x04  0x27  
    0038:   0x9c  0x42  0x0a 
    ```
  - 最左边的一列是在内存中的偏移地址，在 question 部分的域名 baidu.com 出现在偏移地址为 0x000c 的位置，我们把它单独拿出来看：
    ```
    000c:   0x05  0x62  0x61  0x69  0x64  0x75  0x03  0x63  0x6f  0x6d  0x00
    ```
  - 这样可能还不直观，我们换一种更加直观的方式：
    ```
    000c:   0x05  'b'  'a'  'i'  'd'  'u'  0x03  'c'  'o'  'm'  0x00
    ```
  - 这是一种标准的标签方式记录的域名；
  - 这个回应报文中的 answer 有两个，第一个从偏移地址 0x001b 开始，answer 的开始是 NAME 字段，采用了压缩方案存储：
    ```
    001b:   0xc0  0x0c
    ```
  - 按照标签方式，第一个字节应该表示这个标签的长度，但是这个字节 0xc0 的最高两位全为1，所以这里采用的是压缩方案，这个字节的后6位与下个字节一起组成一个偏移，下一个字节是 0x0c，所以偏移应该是：((0xc0 & 0x3f) << 8) + 0x0c = 0x000c，偏移地址 0x000c 处显示的是什么呢？正是 question 部分用标签方式存储的域名 baidu.com
  - 这个例子只是为了直观的说明阳索方案是如何表达一个域名的，实际应用中可能比这个复杂一些，比如可能前面使用标签方式，但最后是一个压缩方案的指针等。
  
## 3. 实现一个DNS客户端
* 我们要实现的这个DNS客户端，仅实现域名 A 记录和 CNAME 记录的查询，这也是最常见的两种DNS记录；
* 通常 DNS 客户端使用 UDP 实现，DNS 协议规定的端口号是 53；
* 实现一个DNS客户端的步骤：
  1. 建立一个UDP socket
  2. 设置socket接收超时，避免在接收应答消息时阻塞
      ```C
      struct timeval timeout;
      timeout.tv_sec  = 5;
      timeout.tv_usec = 0;
      setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      ```
  3. **构建DNS request报文**
      - DNS request报文由(报头 + 域名 + QTYPE + QCLASS)组成，报头的长度为12字节，QTYPE和QCLASS的长度均为2字节，域名为不定长度
      - 一个域名在 request 中占用的字节数为(域名字符串长度 + 2)；域名中每个'.'的位置要换成标签的长度，域名的第一个标签需要增加1字节的长度字节，域名的结束需要填充一个0；
      - 通过以上计算可以得出一个request的长度，合理地分配内存空间
        ```C
        int dns_name_len = strlen(domain_name) + 2;
        int dns_request_len = 12 + dns_name_len + 2 + 2;
        unsigned char *dns_request = malloc(dns_request_len);
        memset(dns_request, 0, dns_request_len);
        ```
      - 将一个域名用标签方式存储，这是构建request最复杂的部分，```struct dnshdr``` 定义了DNS报头结构，详见源程序
        ```C
        uint8_t *dns_name = dns_request + sizeof(struct dnshdr);
        char *p = (char *)dns_name;
        strcpy(p + 1, domain_name);
        char *pdot;
        while ((pdot = index(p + 1, '.')) != NULL) {
            *pdot = 0;
            *p = strlen(p + 1);
            p = pdot;
        }
        *p = strlen(p + 1);
        ```
      - 填写request中的QTYPE和QCLASS
        ```C
        uint16_t *qtype = (uint16_t *)(dns_name + dns_name_len);
        uint16_t *qclass = (uint16_t *)(dns_name + dns_name_len + 2);

        *qtype = htons(1);
        *qclass = htons(1);
        ```
      - 填写DNS报头，ID填任何数字都可以，flags中RD=1表示要求服务器进行递归查询，其他字段均为0，qu_count=1表示有一个查询，要注意的是，request中的存储都必须是网络字节序，所以要使用htons()转换一下
        ```C
        struct dnshdr {
            uint16_t id;
            uint16_t flags;
            uint16_t qu_count;      // Number of questions
            uint16_t an_count;      // Number of answer rr
            uint16_t au_count;      // Number of authority rr
            uint16_t ad_count;      // Number of additional rr
        };

        struct dnshdr *dns_header = (struct dnshdr *)dns_request;
        dns_header->id = htons(0x1234);
        dns_header->flags = htons(0x0100);
        dns_header->qu_count = htons(1);
        dns_header->an_count = 0;
        dns_header->au_count = 0;
        dns_header->ad_count = 0;
        ```

  4. **向DNS服务器发送DNS request报文**
      ```C
      struct sockaddr_in dns_addr;
      memset(&dns_addr, 0, sizeof(struct sockaddr_in));
      dns_addr.sin_family = AF_INET;
      dns_addr.sin_port = htons(53);
      dns_addr.sin_addr.s_addr = inet_addr("114.114.114.114");

      sendto(sockfd, dns_request, dns_request_len, 0, (struct sockaddr *)&dns_addr, sizeof(struct sockaddr));
      ```

  5. **接收来自DNS服务器的应答报文**
      ```C
      uint8_t *buf = malloc(512);
      memset(buf, 0, 512);
      struct sockaddr_in addr;
      unsigned int addr_len = sizeof(struct sockaddr_in);
      recvfrom(sockfd, buf, 512, 0, (struct sockaddr *)&addr, &addr_len);
      ```
      - DNS协议规定，DNS数据包的长度不超过512字节，所以这里仅给接收缓冲区分配512个字节

  6. **解析DNS服务器的应答报文**
      - 从DNS报头获取answer部分的RR数量
        ```C
        struct dnshdr *dns_hdr = (struct dnshdr *)buf;
        uint16_t ancount = ntohs(dns_hdr->an_count);
        ```
      - 找到answer部分的起始位置，在应答报文中，要跳过question部分才是answer部分，但question部分不是固定长度的，所以要费点周折；
        ```C
        uint8_t *p_question = buf + sizeof(struct dnshdr);
        uint8_t *p = p_question;
        while (*p > 0) {
            p += *p;
            p++;
        }
        p++;
        uint8_t *p_answer = p + 2 + 2;
        ```
      - p_question指向question部分的开头，经过一个循环找到QTYPE字段的位置，再加上QTYPE和QCLASS的长度就找到了answer的起始位置；
      - answer中的内容其实就是一个一个的RR，前面我们已经从包头中获得了RR的数量，这里循环就好了
      - 前面介绍过RR的结构，一个RR是由(NAME + TYPE + CLASS + TTL + RDLENGTH + RDATA)组成，其中CLASS永远为IN，TTL是设置DNS cache时用的，这两项我们可以不用管；
      - TYPE决定着RDATA的格式和内容，我们只解析A记录和CNAME记录，所以如果TYPE为其他类型，我们可以放弃
      - 如果是A记录(TYPE=1)，则RDATA中是一个32位的IP地址，占4个字节，NAME中存放着其主域名(不是别名)；
      - 如果是CNAME记录(TYPE=5)，则NAME中存放的是一个别名，RDATA中存放着这个别名的主域名，此时RDATA中的数据采用标签方式或压缩方案存储域名；
      - 在源程序中有一个函数parse_name()专门用于将RR中的name或者RDATA中的name转换成我们可以读懂的域名格式
        ```C
        char *owner_name;
        char *cname;
        int name_len = 0;
        uint16_t ans_type;
        uint16_t rdlength;
        uint8_t *p_rdata;

        owner_name = malloc(256);
        int i;
        for (i = 0; i < ancount; ++i) {
            memset(owner_name, 0, 256);
            name_len = parse_name(buf, p_answer, owner_name, 256);
            ans_type = ntohs(*(int16_t *)(p_answer + name_len));
            rdlength = ntohs(*(int16_t *)(p_answer + name_len + 2 + 2 + 4));

            p_rdata = p_answer + name_len + 2 + 2 + 4 + 2;
            if (ans_type == TYPE_HOST) {
                // host ip in rdata. ip points to rdata.
                printf("The owner name: %s\n", owner_name);
                printf("ip: %d.%d.%d.%d\n", p_rdata[0], p_rdata[1], p_rdata[2], p_rdata[3]);
            } else if (ans_type == TYPE_CNAME) {
                // canonical name in rdata
                cname = malloc(256);
                memset(cname, 0, 256);
                parse_name(buf, p_rdata, cname, 256);
                printf("The alias name: %s\n", owner_name);
                printf("The canonical name: %s\n", cname);
                free(cname);
            }
            // point to next answer
            p_answer = p_answer + name_len + QTYPE_LEN + QCLASS_LEN + TTL_LEN + RDLENGTH_LEN + rdlength;
        }
        free(owner_name);
        ```
      - owner_name是RR中NAME字段中的域名，cname是当TYPE=CNAME时，RDATA中的域名
      - parse_name()是解析name的函数，其返回值为该name在报文中占用的字节数，p_answer是指向answer部分开头的指针，所以p_answer加上parse_name()的返回值就是RR中TYPE字段的位置，再加上TYPE、CLASS和TTL的长度，就是RDLENGTH字段的位置，再加上RDLENGTH的长度，就是RDATA的位置，p_data指针就是这样得到的；

* 完整源程序的文件名为：[dns-client.c][src01](**点击文件名下载源程序**)
* 编译：```gcc -Wall dns-client.c -o dns-client``
* 运行：```./dns-client baidu.com```
  - 查询这个域名通常会返回2条A记录

    ![Screenshot of baidu.com][img01]

* 运行：```./dns-test www.baidu.com```
  - www.baidu.com其实是一个别名，所以这个查询会得到一个CNAME记录和2个A记录

    ![Screenshot of www.baidu.com][img02]


## 4. 后记
* 在日常的网络活动中，有时会遇到DNS污染的困扰，比如在浏览器中输入域名，却无法到达我们期望到达的网站，这有时就是因为我们收到的DNS回应并不是来自一个合法的DNS服务器；当遇到此类问题时，本文的程序可以帮助你判断是否存在DNS污染；
* 如果一个网站没有完成备案，则使用域名访问时也无法到达该网站，这其实也是在DNS上做的文章；
* 本文仅对DNS的A记录和CNAME记录做了解析，其实常用DNS记录还有MX、TXT等；
* DNS的反向查询指的是使用IP地址查询其对应的域名；
* DNS协议的在rfc 1035中定义的反向查询(Inverse Queries)，但是该功能是可选的(Optional)，也就是说DNS服务器可以不具备反向查询的功能，所以使用这个功能可能无法达到预期结果；
* 但是DNS协议在rfc 1035中定义了一个域 **IN-ADDR.ARPA**，利用这个域的查询可以达到和反向查询类似的功能，所以在实际应用中，如果需要做DNS反向查询，通常都是采用 IN-ADDR.PARA 域来完成，更详细的信息请查阅 [rfc 1035][article02] 第3.5节。



## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**





-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[article01]:https://www.ietf.org/rfc/rfc1034.txt
[article02]:https://www.ietf.org/rfc/rfc1035.txt

[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180019/dns-client.c

[img01]:https://whowin.gitee.io/images/180019/screenshot-dns-baidu.com.png
[img02]:https://whowin.gitee.io/images/180019/screenshot-dns-www-baidu-com.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/e2ff0752d649f48e0901e8860ea029cb.png
[img02]:https://img-blog.csdnimg.cn/img_convert/a728716ba050995a057ce144eb10d483.png
-->
