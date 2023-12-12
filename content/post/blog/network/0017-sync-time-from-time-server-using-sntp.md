---
title: "使用SNTP协议从时间服务器同步时间"
date: 2023-02-13T16:43:29+08:00
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
  - 网络编程
  - NTP
  - SNTP
draft: false
#references: 
# - [Simple Network Time Protocol (SNTP) Version 4](https://www.rfc-editor.org/rfc/rfc4330)
# - [Network Time Protocol Version 4](https://www.rfc-editor.org/rfc/rfc5905)
# - [Network Time Protocol (Version 3)](https://www.rfc-editor.org/rfc/rfc1305)
# - [Network Time Protocol (Version 3) pdf](https://www.rfc-editor.org/rfc/rfc1305.pdf)
# - [NTP授时原理，提高NTP授时精度的方法](https://m.elecfans.com/article/1545275.html)
#     - 其中的NTP原理和授时精度分析有参考价值
# - [Conversion between NTP time and UNIX struct timeval](http://waitingkuo.blogspot.com/2012/06/conversion-between-ntp-time-and-unix.html)
# - [A Very Short Introduction to NTP Timestamps](https://tickelton.gitlab.io/articles/ntp-timestamps/)
#    - 这篇文章解释了NTP时间戳中的fraction的含义
# - Time Server
#   ntp.tencent.com       106.55.184.199
#   ntp1.tencent.com      106.55.184.199
#   ntp.aliyun.com        203.107.6.88
postid: 180017
---

在互联网上校准时间，是几乎连接在互联网上的每台计算机都要去做的事情，而且很多是在后台完成的，并不需要人工干预；互联网上有很多时间服务器可以发布精确的时间，计算机客户端使用NTP(Network Time Protocol)协议与这些时间服务器进行时间同步，使本机得到精确时间，本文简要描述了NTP协议的原理，对NTP协议的时间同步精度做了简要分析，并具体实现了SNTP(Simple Network Time Protocol)下的客户端，本文附有完整的C语言SNTP客户端的源程序。阅读本文只需掌握基本的socket编程即可，本文对网络编程的初学者难度不大。
<!--more-->

## 1. NTP协议和SNTP协议
* SNTP协议使用与NTP协议同样的报文结构和格式，所以仅就从服务器进行时间同步而言，在服务器端看NTP和SNTP没有什么区别，使用SNTP协议的客户端可以从任何一台符合NTP协议的时间服务器上进行时间同步；
* NTP和SNTP协议的区别在于错误检测和时间校准的算法上，这主要体现在客户端的软件上；
* SNTP客户端程序向一台NTP时间服务器发出时间数据包，接收来自服务器的回应，并据此计算本机的时间偏差，从而校准本机时间；
* NTP协议的算法比SNTP复杂得多，NTP通常使用多个时间服务器校验时间，该算法使用多种方法来确定这些获取的时间值是否准确，包括模糊因子和识别与其他时间服务器不一致的时间服务器，然后加速或减慢系统时钟的**漂移率**，使系统时间可以做到
  1. 系统的时间总是正确的；
  2. 在初始校正时间后，系统时间不会再有任何时间跳跃。
* 与NTP客户端不同，SNTP客户端通常只使用一个时间服务器来计算时间，然后将系统时间跳转到计算的时间；为了防止时间服务器出现不可用的情况，SNTP客户端可以有一个或多个备份时间服务器，但不会同时使用多个时间服务器来计算时间；
* SNTP客户端通常按照一个固定的间隔时间去访问时间服务器，在间隔期间则不对系统时间做任何调整，所以，当SNTP访问时间服务器校准时间时，往往会产生时间跳跃；
* 我们可以用一个更形象的例子来说明SNTP客户端，我们把墙上的挂钟当做时间服务器，把我们戴的手表当做客户端
  - 当我们的手表为SNTP客户端时，我们每隔一小时看一下挂钟，并使用挂钟来校准手表；
  - 当手表12:00时，挂钟的时间为11:59:57秒，手表快了3秒钟，所以把手表调慢3秒钟；
  - 在接下来的1个小时里，不会对手表做任何调整，当手表1:00时，挂钟的时间为12:59:57秒，所以我们再次把手表调慢3秒钟；
  - 从手表时间的准确度来说，刚刚校准完时间时是最准确的，然后准确度逐渐变差，在再次调整时间前，其准确度是最差的；
* 如果这样的情况能够满足你对时间的要求，那就可以使用SNTP协议去校准时间，否则就要考虑使用NTP客户端；
* NTP客户端计算"手表"的时间变化方向和速率，以此为基础来补偿"手表"上的时间漂移，对"手表"进行实时调整，使"手表"一直保持准确；
* 实际上，对大多数PC而言，SNTP都是可以满足要求的，windows的内建程序w32time采用的就是SNTP协议。

## 2. NTP时间同步的基本原理
* NTP的原理是通过一个时间消息包的传送计算出客户端和服务器端的时间偏差，从而校准客户端的时间；
* NTP和SNTP客户端均使用UDP向时间服务器发送消息，IANA为NTP分配的端口号为123，也就是说，NTP/SNTP客户端需要向时间服务器的123端口发送一个符合格式(下一节介绍消息格式)的UDP消息，客户端的接收端口号没有规定；
* 时间服务器是Server，客户端是Client，同步过程如下进行：
  1. Client向Server发送一个消息包，记录发出消息包时的时间戳 **T<sub>1</sub>**(以Client系统时间为准)
  2. Server收到消息包立即记录时间戳 **T<sub>2</sub>**(以Server系统时间为准)
  3. Server向Client返回一个消息包，返回消息包时记录时间戳 **T<sub>3</sub>**(以Server系统时间为准)
  4. Client收到Server返回的消息包，此时记录时间戳 **T<sub>4</sub>**(以Client系统时间为基准
* 过程如下图所示：

  ![Time Synchronization][img01]


* T<sub>4</sub> 和 T<sub>1</sub> 是以Client的时间标准记录的时间戳，其差 T<sub>4</sub> - T<sub>1</sub> 表示整个消息传递过程所花费的总时间；
* T<sub>3</sub> 和 T<sub>2</sub> 是以Server的时间标准记录的时间戳，其差 T<sub>3</sub> - T<sub>2</sub> 表明消息传递过程在Server停留的时间；
* 那么 (T<sub>4</sub> - T<sub>1</sub>) - (T<sub>3</sub> - T<sub>2</sub>) 应该就是信息包的往返时间(总时间-在Server停留的时间)；
* **如果假定信息包从Client到Server和从Server到Client所用的时间一样**，那么，从Client到Server或者从Server到Client信息包的传送时间d为：
  $$
  \large \\ \\ d = {(T_4 - T_1) - (T_3 - T_2) \over 2}
  $$

* 假定Client相对于Server机的时间误差是 **t**(t = 服务器时间戳 - 客户端时间戳)，则有下列等式：
  $$
  \begin{cases}
  T_2 = T_1 + t + d \newline
  T_4 = T_3 - t + d
  \end{cases}
  $$

* 从以上三个等式组成一个方程式：
  $$
  \begin{cases}
  \large d = {(T_4 - T_1) - (T_3 - T_2) \over 2} \newline
  \normalsize T_2 = T_1 + t + d \newline
  T_4 = T_3 - t + d
  \end{cases}
  $$

* 可以解出Client机的时间误差 **t** 为：
  $$
  \large t = {(T_2 - T_1) + (T_3 - T_4) \over 2}
  $$

* 如果一时没有转过来，可以自己在纸上画个图，再细细地琢磨一下，应该没有问题。

## 3. NTP和SNTP协议参考
* 这里列出这两个协议的原件下载地址，有兴趣的读者可以认真读一下；
* NTP最新版目前是NTP v4(RFC5905)，v5还没有正是推出，这里列出NTP v4和v3(RFC1305)的链接，还有一个NTP v3的pdf文档的下载地址；
* SNTP目前的版本是v4(RFC4330)，其实这个版本也是很久以前的，2006年的，SNTP一直没有新的版本。
* [Network Time Protocol Version 4][article01]
* [Network Time Protocol (Version 3)][article02]
* [Network Time Protocol (Version 3) pdf][article03]
* [Simple Network Time Protocol (SNTP) Version 4][article04]

## 4. SNTP(NTP)协议的消息结构
* 下面这张图取自SNTP协议，很直观地显示出NTP消息包的结构，要注意的是，所有字段应该都是网络字节序(big endian)
* NTP时间同步过程中，客户端发送的消息包结构与时间服务器返回的消息包结构是完全一样的，时间服务器会将其中的一些字段改写，然后发回；

  <!--![NTP packet struct][img03]-->


  ```plaintext
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
  
* **LI(Leap Indicator)**
  - 闰秒指示，2 bits，bit 0和bit 1
  - 表示是否警告在当天的最后一分钟插入/删除一个闰秒；通常填0表示不需要警告。
  - 这个值只在服务器端有意义，SNTP协议第4节中对LI可能的取值做了说明

* **VN(Version Number)**
  - NTP/SNTP版本号，3 bits
  - 这里只要填一个服务器支持的版本即可，目前最高的版本为4，填4，估计填3也不会有问题，因为绝大多数的时间服务器应该仍然支持NTP v3，以保证一些古老的客户端仍可以运行；

* **Mode**
  - 工作模式，3 bits，其取值含义如下
    ```plaintext
    Mode  Meaning
    ------------------------------------
     0    reserved
     1    symmetric active
     2    symmetric passive
     3    client
     4    server
     5    broadcast
     6    reserved for NTP control message
     7    reserved for private use
    ```
  - 当访问单台时间服务器同步时间时，仅取值3和4有意义，客户端发送消息时将这个字段填3，表示发送消息者是一个客户端；服务器回复消息时将这个字段改为4，表示回复消息的是服务器；

* **Stratum**
  - 表示当前时间服务器在NTP网络体系中所处的层，是一个8位无符号整数，其值通常为0-15；该字段仅在NTP服务器消息中有意义；
  - NTP 的网络体系是分层(stratum)结构，Stratum-0层设备(包括原子钟和gps钟)是最精确的，但不能通过网络向客户端授时；0级设备通常仅作为1级时间服务器的参考时钟(或同步源)；
  - Stratum-1层设备是可以通过网络授时的最准确的ntp时间源，1层设备通常通过0层参考时钟同步时间；
  - Stratum-2层设备通过网络连接从一级设备同步时间；由于网络抖动和延迟，二级服务器的时间准确度不如一级服务器；从第二层时间源同步的NTP客户端将是Stratum-3设备；
  - 以此类推，层级越高，其时间的精确度和可靠度越低；
  - NTP协议不允许客户端接受来自Stratum-15以上设备的时间，因此Stratum-15是最低的NTP层；

* **Poll Interval**
  - 表示连续时间消息之间的最大间隔，以2的指数表示(比如4则间隔时间为 2<sup>4</sup>)，单位为秒，此值为一个8位无符号整数；
  - 该字段仅在SNTP服务器消息中有意义，其值范围为4(16秒)到17(131,072秒——大约36小时)；

* **Precision**
  - 时间服务器的系统时钟精度，以2的指数表示(比如-10则精度 2<sup>-10</sup>)，单位为秒，此值为一个8位有符号整数；
  - 此字段仅在服务器消息中有意义，其中的值范围从-6(主频时钟)到-20(某些工作站中的微秒时钟)；

* **Root Delay**
  - 表示时间服务器到主参考源的总往返延迟，以秒为单位，是一个32位有符号的定点数，小数点在第bit 15和bit 16之间；
  - 该变量可以取正值，也可以取负值，取决于相对时间和频率偏移量，该字段仅在服务器消息中有意义，其值范围从几毫秒的负值到几百毫秒的正值；

* **Root Dispersion**
  - 表示相对于主要时钟参考源的的最大误差，单位为秒，是一个32位有符号的定点数，小数点在第bit 15和bit 16之间；
  - 其值只能是大于0的正数；

* **Reference Identifier**
  - 用于标识特定的时间参考源，32位串；此字段仅在服务器消息中有意义；
  - 对于Stratum-0和Statum-1，该字段的值为一个四字节的ASCII字符串，左对齐并以0填充到32位；
  - 对于IPv4的Stratum-2时间服务器，该字段的值为时间同步源的32位IPv4地址(IP地址)；

* **Reference Timestamp**
  - 本地时钟最后一次设置或修正时的时间，64位时间戳格式。

* **Originate Timestamp**
  - 前面原理部分说到的 T<sub>1</sub>，也就是消息包从客户端发出时，客户端系统时间戳，由客户端程序填写，64位时间戳格式；

* **Receive Timestamp**
  - 前面原理部分说到的 T<sub>2</sub>，也就是服务器收到客户端消息包时，服务器端系统时间戳，由服务器端程序填写，64位时间戳格式；

* **Transmit Timestamp**
  - 前面原理部分说到的 T<sub>3</sub>，也就是服务器把数据包返回客户端时，服务器端系统时间戳，由服务器端程序填写，64位时间戳格式；

* **Authenticator**
  - 可选项，一般不填；
  - 当采用NTP认证方案时，"Key Identifier"和"Message Digest"字段包含了[《RFC 1305》][article02]附录C中定义的MAC(Message authentication code)信息。

* 关于**64位时间戳**
  - 前面多次提到**64位时间戳**，在NTP/SNTP协议中，对此有专门的定义
  - NTP时间戳表示为 64 位无符号定点数，以秒为单位，相对于1900年1月1日的0时；
  - 整数部分在前32位，小数部分在后32位；在小数部分，没有意义的低位，通常要设置为0；
  - 有关小数部分(Fraction Part)，其单位既不是毫秒(millisecond)，也不是微秒(microsecond)，**其单位为 1/2<sup>32</sup> 秒**，这一点非常重要，但却鲜有文章说明，如果不知道这一点，UNIX时间戳和NTP时间戳之间的转换就搞不明白；
  - 下图摘自SNTP协议

    <!--![NTP timestamp structure][img04]-->

    ```plaintext
                         1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                           Seconds                             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                  Seconds Fraction (0-padded)                  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ```

## 5. SNTP时间同步的精度分析
* 有很多文章中都提到，NTP时间同步的精度可以达到50ms以内，本节将讨论这个精度是如何计算得出的；
* 要注意我们讲网络时间同步的精度并不包括时间服务器本身的时间精度，只是客户端与服务器端同步时间后，客户端与服务器端相对时间误差；
* 本文第2节介绍NTP时间同步的基本原理时，曾经列出过方程式，当时有一个重要的假设为：**假定信息包从Client到Server和从Server到Client所用的时间一样**，这次我们去掉这个假设，重新列出方程式；
* 假定消息包从Client到Server的时长为 d<sub>1</sub>，从Server到Client的时长为 d<sub>2</sub>，d<sub>2</sub> 与 d<sub>1</sub> 的差为  &delta;，其它定义同前，则有下列等式：
  $$
  \begin{cases}
  \delta = d_2 - d_1 \newline
  T_2 = T_1 + t + d_1 \newline
  T_4 = T_3 - t + d_2 \newline
  \end{cases}
  $$
* (将1式带入3式)
  $$
  \begin{cases}
  t = T_2 - T_1 - d_1 \newline
  t = T_3 - T_4 + \delta + d_1 \newline
  \end{cases}
  $$
* 两式相加，得出
  $$
  \large t = {{(T_2 - T_1) + (T_3 - T_4)} \over 2} + {\delta \over 2}
  $$

* 这个结果是精确的，没有任何假设，理论时间同步的精度为 0；
* 当 &delta; 为 **0** 时，相当于 **假定信息包从Client到Server和从Server到Client所用的时间一样**，得出的结果和第 2 节的结果是一样的；
* 由此可见，误差由 &delta; 产生，而 &delta; 的最大值为 d<sub>2</sub> 或者最小值为 -d<sub>1</sub>，假定Client到Server的最大时延为100ms，则 &delta; 的最大值为 100ms，则根据上式，其时间精度的最大误差为 &delta;/2，即 50ms
* 由上面的计算可以得知，NTP协议进行时间同步的精度误差主要来自数据包从Client到Server和从Server到Client的时间不一样，这个差异越大，其误差越大；
* SNTP使用UDP协议发送时间信息包，UDP又是一种无连接的协议，从Client到Server和从Server到Client的路由很可能是不一样的，这无形中会使时间同步的精度变差；
* 由上面的分析可以看出：**找到一个时延比较小的时间服务器可以有效地提高时间同步的精度**。

## 4、SNTP客户端实例
* 说起来一大堆，但实现起来其实并不像说的那么复杂。
* SNTP协议允许使用单播(unicast)、广播(broadcast)和多播(manycast)模式，通常我们只能使用单播模式，广播和多播模式的时间服务器只存在于某个子网中，为有限的用户服务；互联网上并不存在实际的广播或多播模式的时间服务器；
* 根据[《SNTP 协议》][article01]第5节"**SNTP Client Operations**"的说明，使用单播模式进行时间同步时，向时间服务器发送的请求数据包中，除了第一个字节以外，其他字段都可以设为0，也可以将Originate Timestamp(T<sub>1</sub>)填在Transmit Timestamp(T<sub>4</sub>)这个字段上，时间服务器会将Transmit Timestamp(T<sub>4</sub>)字段的内容搬移到Originate Timestamp(T<sub>1</sub>)上，然后填上正确的Transmit Timestamp(T<sub>4</sub>)；
* 下面的例子中就是按照SNTP协议的说法去做的，在发送的请求包中，填了LI、VN、MODE三个字段，并把T<sub>1</sub>(Originate Timestamp)填在了T<sub>4</sub>(Transmit Timestamp)上，从时间服务器返回的数据看，完全印证了SNTP协议中的说法；
* 要注意的是，ntp数据包中的各个字段都是网络字节序(big endian)，而我们使用的电脑都是主机字节序(little endian)，所以相互之间要做转换；
* 再次强调一下，NTP时间戳的fraction字段的单位是 **1/2<sup>32</sup> 秒**；
* 源程序文件名：[sntp-client.c][src01](**点击文件名下载源文件**)

* 编译：```gcc -Wall sntp-client.c -o sntp-client -lm```，因为其中使用了数学函数，所以编译时要加上 "-lm"
* 运行：```./sntp-client ntp.aliyun.com```
* 运行截图：

  ![screenshot of sntp_client][img02]

* 在源程序中，列出了几个时间服务器，可以通过百度或者谷歌找更多的时间服务器进行尝试。


## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

[src01]:https://whowin.gitee.io/sourcecodes/180017/sntp-client.c

[article01]:https://www.rfc-editor.org/rfc/rfc5905
[article02]:https://www.rfc-editor.org/rfc/rfc1305
[article03]:https://www.rfc-editor.org/rfc/rfc1305.pdf
[article04]:https://www.rfc-editor.org/rfc/rfc4330


[img01]:https://whowin.gitee.io/images/180017/process-of-time-synchronization.png
[img02]:https://whowin.gitee.io/images/180017/screenshot-of-sntp-client.png
[img03]:https://whowin.gitee.io/images/180017/ntp-packet-structure.png
[img04]:https://whowin.gitee.io/images/180017/ntp-timestamp-structure.png
