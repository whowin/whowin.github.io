---
title: "远程开机-一个简单的嵌入式项目开发"
date: 2022-08-19T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - Linux
  - Embedded
tags: 
  - openwrt
  - 嵌入式
  - 网络唤醒
  - 远程开机
draft: false
#references: 
# https://wiki.wireshark.org/WakeOnLAN
# https://en.wikipedia.org/wiki/Wake-on-LAN
# [Lua教程](https://www.runoob.com/lua/lua-tutorial.html)
# [luasocket官网](https://w3.impa.br/~diego/software/luasocket/home.html)
# [lua手册](https://www.runoob.com/manual/lua53doc/manual.html)
postid: 100009
---

本文通过一个简单的需求介绍了在一个 *ARM* 设备上开发一个程序实现远程打开服务器的过程，通过这个实例大致介绍了一个简单的嵌入式 *Linux* 开发的过程。本文并不会详细介绍网络唤醒的原理以及 *Magic Packet*。
<!--more-->

------------
## 1. 概述
* 本文介绍了一个简单的嵌入式项目的的开发过程；
* 从需求到实践，本文对整个过程做了全面的介绍，本文所介绍的设备容易获得且价格低廉；
* 本文涉及了 *Linux* 下 C 语言下的网络编程、网络广播、*Magic Packet*、内网穿透、反向代理等概念；
* 本文所涉及的一些技术概念读者可以自行参考其它的文章；
* 本文可能并不适合初学者

----------------
## 2. 需求
* 家里放了一台服务器，差不多我所有的东西都在服务器上，不管在家里还是其它地方，都需要连接这台服务器才能做事情；
* 这台服务器白天开着，晚上就关了(省点电:))；每天起床以后要想着按一下服务器的电源开关，每天睡觉前要记得把服务器关了；
* 晚上忘记关服务器，通常不会有什么问题；但有时早上没有打开服务器，可能就要有麻烦了；
* 尴尬的时候就是早上没有打开服务器，然后外出，然后刚好需要登录服务器，这才想起来服务器没开；
* 所以呐，我需要有个机制，可以 **远程打开我的服务器**，这样我就不会再出现尴尬了；

--------------
## 3. 简要解决方案
* 我们把要完成的这个需求当作一个项目来做，可以把这个项目叫做 **服务器远程开机**
* 首先要确保服务器的主板支持网络唤醒，否则不太好办，不过现在的绝大多数主板都是支持的(*PCI 2.2* 以后的主板一般都支持，而 *PCI 2.2* 的标准是1998年提出的)，有些主板可能需要 *BIOS* 设置，请自行搜索解决方案；
* 第二是在家里的局域网上要有一个小设备是 24 小时运行的，通过这台设备在局域网上广播 *Magic Packet* 来唤醒服务器，这台设备应该是一台低功耗设备，越小越简单越好；我们姑且把这个设备叫做 **wakener**；
* 第三是要在这个 **wakener** 上编写一个简单的程序，这个程序可以在局域网上广播 *Magic Packet*，以唤醒服务器，这个程序我们称为 **wakeOnLan**；
* 第四是要有一个机制可以从互联网上访问到这台 **wakener**，只有这样才能从互联网上操控 **wakener** 上的软件，其实这是一个内网穿透的问题，要做到这一点或许需要一台连接到互联网上的轻量级服务器(VPS就可以)；
* 这样我不管在哪里，通过终端(笔记本、台式机、手机、平板等)登录家里24小时开机的 **wakener**，运行 **wakeOnLan**，就可以打开我的服务器；
* 这台24小时运行的设备(*wakener*)有可能为你完成更多的事情，比如 NAS、远程开空调等等，但是重要的是完成眼前这个第一步。

--------------
## 4. 技术要点
> 上面的解决方案显然非常粗糙，但是这个项目本身确实也比较简单，没有必要做非常详尽的设计方案，所以我们下面仅列出一些可能的要点及解决方法

* **本项目的基本网络架构**
  - 下面是一个简单的示意图，表达了这个项目中各个设备是如何连接和互相影响的，其中 *Local Server* 是我们要远程开机的服务器，*Server* 是一台连接在互联网上的 *VPS*，用于内网穿透

    ![wake on lan架构][img05]
    + **图1：wake on lan架构图**

* **Magic Packet**
  - 远程唤醒其实是网卡的一个功能，在 *PCI 2.2* 之后，信号线上多了一个 *PME* 信号，主机关闭电源后进入休眠状态，可以继续为网卡供电，网卡在收到一个叫做 *Magic Packet* 的数据包后，检测出该数据包是发给自己的，则会在 *PCI* 上发出 *PME* 信号，从而控制电脑启动，这个功能被称为 **网络唤醒**
  - 网络唤醒其实并不复杂，在局域网中，从一台电脑以广播的形式发送 *Magic Packet*，那么在 *Magic Packet* 中指定的 *MAC* 地址对应的电脑就会被唤醒；
  - *Magic Packet* 就是一个指定格式的数据包，其格式为：6 个 *0xff*，然后 16 组需要被网络唤醒的电脑的 *MAC* 地址，比如需要被唤醒的电脑的 *MAC* 为：*00:e0:2b:69:00:03*，则 *Magic Packet* 为(16进制表述)：
    ```
    ff ff ff ff ff ff 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    ```
  - 通常 *Magic Packet* 的广播使用 *UDP* 发送，端口号可以任意，通常使用 7 或者 9
  - 关于 *Magic Packet* 的更多的信息请参考
    + [wikipidia-网络唤醒](https://zh.wikipedia.org/zh-cn/%E7%B6%B2%E8%B7%AF%E5%96%9A%E9%86%92)
    + [wikipedia-Wake-On-Lan](https://en.wikipedia.org/wiki/Wake-on-LAN)
  - 从图1中可以看出，*Wakener* 通过路由器向局域网内广播 *Magic Packet*，从而唤醒 *Local Server*；
  - 要注意的是，*Magic Packet* 只对有线网卡有效，所以，服务器要使用网线连接到路由器上；

* **内网穿透**
  - 在这个项目里，**内网穿透**指的是：使用一定的技术手段让我们可以从互联网上直接访问到家里的一台设备上，这台设备通过普通家用宽带连接互联网，家用宽带可能没有公网 IP；
  - 如果家里的宽带有公网 IP，穿透内网并不是一个很困难的事，大致需要三个步骤：
    1. 在家里路由器上设置一个 *NAT* 让外网的访问直接转发到局域网内某个指定设备的指定端口上；
    2. 局域网中的一个设备(或者路由器本身)向某个有权限的公网服务器发送心跳包，使这个服务器可以知道家里宽带的公网 IP；
    3. 互联网上的终端设备通过服务器获知家里宽带的公网 IP，直接访问即可；
  - 如果家里的宽带没有公网 IP(大多数宽带应该是这样的)，穿透内网就要麻烦一些，通常需要使用**反向代理**来实现，这需要一个公网服务器；
    + 公网服务器上安装有支持反向代理的服务器端软件，比如 *sshd*；
    + 局域网的设备上装有反向代理的客户端软件，比如 *ssh*，通过向公网服务器发送反向代理的指令可以建立一个反向代理隧道(*tunnel*)，*tunnel* 建立起来以后，访问公网服务器的某个指定端口将被映射到访问局域网中某个 IP 地址下的某个端口，从而实现内网穿透；
  - 目前有很多的内网穿透的工具，其原理其实都是反向代理，但通常比直接用 *ssh* 要好用的多，至少 *ssh* 在意外断开后不会自动重连，这些工具都会解决这些问题；
  - 本文所使用的嵌入式 *Linux* 系统将是 *openwrt*，理论上可以使用向日葵的内网穿透插件，愿意折腾的读者可以尝试折腾一下向日葵插件；
  - 本文在内网穿透上将使用一个叫 *frp* 的开源项目，后面会给出这个项目的具体网址

* **其它要面对的麻烦**
  - 为硬件烧录或编译一个定制的 *Linux* 操作系统，还好本文的例子并不需要自己编译一个 *Linux*，烧录就好了；
  - 交叉编译环境，这个是做嵌入式开发必须面对的，无法回避。

------------------
## 5. 简单的嵌入式开发步骤
1. 需求分析
2. 概要设计和详细设计
3. 硬件开发及验证
4. 编译与硬件相适应的操作系统及所需工具
5. 建立在相应硬件上进行软件开发的交叉编译环境；
6. 嵌入式软件开发；
7. 调试

> 下面我们会依照这个开发步骤去实现我们的方案

-----------------------------------
## 6. 具体实践
* **需求分析和设计**
  > 本项目的开发的需求分析和设计已经在前面完成，鉴于该项目比较简单，就不做更详细设计了

* **硬件开发和验证**
  - 要找一个**24小时运行的设备**
    + 我有一个早就不用的 *迅雷一代赚钱宝(以下简称**赚钱宝**)*，不知道为何物的自己去百度一下
    + 赚钱宝这个玩意 *CPU* 用的是 *Amlogic S805*，*ARM Cortex-A5* 架构，4核1.5GHz，功耗非常低，记住这个 CPU 的型号，记住这个 CPU 是32位的；
    
    ![一代赚钱宝外观][img01]
    + **图2：赚钱宝外观**

    *********************************

    ![S805主板][img02]
    + **图3：赚钱宝主板**

    ***************************

    + 这个玩意有 *256M* 内存，1G 的 *Flash*，*100M* 网口和一个 *USB* 口，足够用了；以前有一些厂家用这个 *CPU* 做网络机顶盒，现在应该没人用了；
    + 淘宝上查了一下，赚钱宝一代是买不到了，但三代(玩客云)还有二手卖，大概在45 - 55元，用的CPU和一代一样，只是内存大了，如果你想折腾，也可以买一个来玩；
  - 这个设备的硬件显然不需要验证，迅雷已经帮我们验证了；
  - 实际上，这个设备可以有多种选择，如果你的路由器是 OEM 的，上面通常运行的都是 openwrt，那么你可以直接使用它；或者你手头有闲置的机顶盒等，都有可能用得上；但是通常不建议使用平板，一是太耗电，二是把安卓刷成 *Linux* 有难度。

*******************************
* **操作系统**
  - 赚钱宝上原有的 *Linux* 应该是迅雷自己编译的，很难知道迅雷在这个系统上做了什么，所以还是不用为好，需要刷个新的系统，*CPU* 为 *S805* 的设备通常都是支持 *USB* 刷机的，所以其实根本不用把它拆开，直接一条 *USB* 线就可以刷新系统了；
  - 刷个 *openwrt* 是比较现实的，有现成的教程，而且 *openwrt* 资料丰富，便于今后折腾；
  - 刷机教程：[赚钱宝一代刷OpenWrt固件](https://post.smzdm.com/p/axlkkz64/)；
  - 刷机包及相应软件：[百度网盘](https://pan.baidu.com/s/1E4Ls05lPHHHhv0Ou8fb7GA)；提取码：ow2l
  - 刷机包提供了 *openwrt 18* 和 *19* 两个，建议刷 *openwrt 19*(因为我刷的是 *openwrt 19*)
  - 具体刷机过程自己去享受吧；
  - 刷好的机器应该是可以使用 *ssh* 登录的
    > 首先要从你的路由器上找到这台 *openwrt* 的 IP 地址，比如为：*192.168.2.100*，然后用 *ssh* 登录，新刷的 *openwrt* 没有密码

    ```
    ssh root@192.168.2.100
    ```

    > 我这里登录以后的样子，登录以后一定要改一下 root 的密码

    ![ssh登录openwrt][img06]
    + **图4：ssh登录openwrt**
    **********************************
  - *openwrt* 还会有一个 *web* 界面，直接在浏览器上输入 IP 即可进入

    ![登录openwrt的web][img07]
    + **图5：登录openwrt的web界面**

    ![openwrt的web界面][img08]
    + **图6：openwrt的web界面**

    ******************************

  - 这台设备最好不要使用 *DHCP*,而是使用固定 IP，这样便于以后远程登录，有三种方法设置固定 IP
    1. **直接修改配置文件**
        > *openwrt* 的网络配置文件放在 */etc/config/network* 文件中，可以直接修改这个文件，改成下面这个样子：

        ![固定IP][img09]
        + **图7：openwrt 改成固定 IP**

    **************
    2. **使用 *openwrt* 的 *web* 界面修改**
        > 从浏览器登录 *openwrt* 的 *web* 页面，其密码与 *ssh* 的密码一致，选择：*Network --> Interfaces --> Edit*

        ![通过openwrt的web界面修改固定IP][img10]
        + **图8：通过openwrt的web界面修改固定IP**

        ![通过openwrt的web界面修改固定IP][img11]
        + **图9：通过openwrt的web界面修改固定IP**

        ![通过openwrt的web界面修改固定IP][img12]
        + **图10：通过openwrt的web界面修改固定IP**

    **************
    3. **在路由器上用 *MAC* 地址绑定 IP**
        > 实际上就是在路由器上设置 *DHCP* 每次分配 IP 时，给指定 *MAC* 地址的设备分配一个固定的 IP，这样，你的 *openwrt* 设备就不必设置固定 IP了，每种路由器的设置方法不一样，我的路由器的界面像这个样子
        
        ![通过路由器设定openwrt的固定IP][img13]
        + **图11：通过路由器设定openwrt的固定IP**

*************************
* **建立交叉编译环境**
  - 要在赚钱宝上写程序，需要我们在本地电脑完成编程，然后用交叉编译的工具编译成在赚钱宝上可以运行的程序，传到赚钱宝上才可以在赚钱宝上运行；
  - 所以我们需要有一个工具链，对我们编写的程序进行交叉编译；
  - 这个工具链不是放在 *wakener* 上的，因为 *wakener* 通常性能比较差，而且为了节省存储空间，上面通常只放一些运行时(*runtime*)库，不具备开发的能力；所以这个工具链要放在另外一台运行着 *Linux* 的机器上，也可以运行在虚拟机上，我们把这台电脑叫做 **开发机**，建议在**开发机**上运行 *ubuntu*；
  - 下面是建立这个编译环境的过程
  - 首先 *ssh* 登录你刚刷的 *openwrt*，查看你刷的 *openwrt* 的版本号：

    ![openwrt的版本号][img03]
    - **图12：查看openwrt的版本号**
    *****************************

  - 去 [openwrt官网](https://downloads.openwrt.org) 找到你所需要的版本号下的 *sdk*，要选择 *at91/sama5* 目录下的，这个是 *cortex-A5* 架构(*CPU S805* 的架构)的工具链
    + [我的工具链的下载地址](https://downloads.openwrt.org/releases/19.07.7/targets/at91/sama5/openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64.tar.xz)
  - 注意这个工具链只能运行在 *Linux* 下，我是运行在 *Ubuntu* 下，而且 *openwrt-19.07.7* 的 *SDK* 只有 64 位 X86 版本；
  - 先将这个工具链下载到 *~/*Downloads/* 目录下
    ```
    wget https://downloads.openwrt.org/releases/19.07.7/targets/at91/sama5/openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64.tar.xz -C ~/Downloads/
    ```
  - 再将其解压出来
    ```
    cd ~/Downloads
    tar -xvf openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64.tar.xz
    ```
  - 在 *~/Downloads/* 目录下会建立一个新目录 *openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64*，进入到这个目录，可以看到一个 **staging_dir** 目录
    ```
    cd openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64/
    ls
    ```
  - 这个 **staging_dir** 目录下的所有内容就是一个完整的工具链，我们可以把这个工具链单独拿出来使用；
  - 我把这个目录拷贝到了 *~/tooschain/openwrt-a5/* 下，我们之所以放到 *~/toolschain/* 下，是因为我们还可能有别的设备的工具链，都放到这个目录下便于管理
    ```
    mkdir -p ~/toolschain/openwrt-a5/
    cp -fr ~/Downloads/openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64/staging_dir/* ~/toolschain/openwrt-a5/
    ```
  - 现在我们已经有了一个完整的工具链，但这个工具链基本上是没办法用的，我们需要简单的配置一下，其实就是设置一些环境变量；
    + 在 *HOME* 目录下编辑一个文件 *a5.sh*，内容如下:
      ```
      #!/bin/bash

      #A5 arm-linux-musl-eabi工具链，用于一代赚钱宝openwrt
      export PATH=/home/whowin/toolschain/openwrt-a5/toolchain-arm_cortex-a5+vfpv4_gcc-7.5.0_musl_eabi/bin:$PATH
      export STAGING_DIR=/home/whowin/toolschain/openwrt-a5
      ```
    + 其中 */home/whowin* 为我的 *HOME* 目录，你要更改为你的 *HOME* 目录；
    + 其实这个文件就是修改了环境变量 *PATH*，然后增加了一个环境变量 *STAGING_DIR*，这些设置都是为了能够正常使用这个工具链；
    + 将这个文件设置为可执行，然后把这个文件拷贝到 */bin/* 下
      ```
      cd ~
      vi a5.sh
      (编辑内容并存盘)

      chmod 755 a5.sh
      sudo cp a5.sh /bin/
      ```
    + 放到 */bin/* 目录下只是为了用起来方便，并没有特别的含义；
    + 下面我们可以试一下这个工具链
      ```
      source /bin/a5.sh
      arm-openwrt-linux-gcc -v
      ```
    + 下面是在我的机器上的输出
      
      ![测试一下工具链][img04]
      + **图13：测试工具链**

*************************
* **源程序**
  - 有了工具链就可以编程了，编程的过程要在**开发机**上完成，不是在 *openwrt* 下，但是用这个源程序编译出来的可执行文件是要在 *openwrt* 下运行的；
  - 下面是这个项目中需要在 *openwrt* 下运行的程序 *wakeOnLan* 的源程序

    ```
    /******************************************************************************
      File Name: wakeOnLan.c
      Description:  向局域网中的计算机发出远程唤醒的指令
                    该程序将运行在迅雷一代赚钱宝上(已刷openwrt)，用于唤醒主机
      
      Compile:  source /bin/a5.sh
                arm-openwrt-linux-gcc -Wall wakeOnLan.c -o wakeOnLan

      Usage: wakeOnLan [boradcast IP] [MAC]
      Example: sudo ./wakeOnLan 192.168.2.255 00:e0:2b:68:00:03
      
      Date: 2022-07-26
    *******************************************************************************/
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <signal.h>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>   ////inet_ntop
    #include <netinet/in.h>  //inet_addr

    #define  BIND_PORT         7                   // 端口号
    #define  MSG_LEN           102                 // magic packet的长度
    #define  USAGE             "wakeOnLan [broadcast IP] [MAC]\n" \
                               "Example: sudo ./wakeOnLan 192.168.1.255 00:e0:2b:69:00:03\n"

    /*****************************************************
    * Function: int is_IP(char *IP)
    * Description: 检查IP是否为一个合法的IPv4
    * Return: 1    合法的IPv4
    *         0    非法的IPv4
    *****************************************************/
    int is_IP(char *IP) {
        int a, b, c, d;
        char e;

        if (4 == sscanf(IP, "%d.%d.%d.%d%c", &a, &b, &c, &d, &e)) {
            if (a >= 0 && a < 256 &&
                b >= 0 && b < 256 &&
                c >= 0 && c < 256 &&
                d >= 0 && d < 256) {
                return 1;
            }
        }
        return 0;
    }

    /*************************************************************************
    * Function: int is_MAC(char *mac_str, char *mac)
    * Description: 检查MAC是否为合法的MAC格式，如果合法，将其转换成6组数字放到mac中
    * 
    * Return: 1    合法的MAC，char *mac中为转换后的mac地址
    *         0    非法的MAC
    *************************************************************************/
    int is_MAC(char *mac_str, char *mac) {
        int temp[6];
        char e;
        int i;

        if (6 == sscanf(mac_str, "%x:%x:%x:%x:%x:%x%c", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &e)) {
            for (i = 0; i < 6; ++i) {
                if (temp[i] < 0 || temp[i] > 255) break;
            }
            if (i == 6) {
                for (i = 0; i < 6; ++i) {
                    mac[i] = temp[i];
                }
                return 1;
            }
        }
        return 0;
    }

    // ===================主程序================================================
    int main(int argc, char *argv[]) {
        struct sockaddr_in sin;
        char *broadcast_ip;
        char wol_msg[MSG_LEN + 2];                           // magic packet
        char mac[6];
        int socket_fd;
        int i, j;
        int on = 1;
        
        // 检查参数数量
        if (argc < 3) {
            // 参数数量不对
            printf("Incorrect input parameters.\n");
            printf(USAGE);
            return -1;
        }

        // 检查第一个参数是否为一个IP地址
        if (! is_IP(argv[1])) {
            printf("%s is an invalid IPv4.\n", argv[1]);
            printf(USAGE);
            return -1;
        }

        // 检查第二个参数是否为一个MAC地址
        if (! is_MAC(argv[2], mac)) {
            printf("%s is an invalid MAC address.\n", argv[2]);
            printf(USAGE);
            return -1;
        }
        printf("MAC is %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]);

        // 建立socket
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            printf("Can not set up socket. Program exits(%d).\n", socket_fd);
            return -1;
        }

        // 为IP地址分配内存
        broadcast_ip = calloc(strlen(argv[1]) + 1, sizeof(char));
        if (! broadcast_ip) {
            printf("Can not allocate memory. Program exits\n");
            return -1;
        }
        // 允许在 socket_fd 上发送广播消息
        setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
        
        memset((void *)&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = inet_addr(broadcast_ip);       // 广播IP
        sin.sin_port = htons(BIND_PORT);                     // 端口号

        // 6个 ff
        for (i = 0; i < 6; i++) {
            wol_msg[i] = 0xFF;
        }
        // 16遍 mac 地址
        for (j = 0; j < 16; j++) {
            for (i = 0; i < 6; i++) {
                wol_msg[6 + j * 6 + i] = mac[i];
            }
        }
        // 发送 magic packet
        sendto(socket_fd, &wol_msg, MSG_LEN, 0, (struct sockaddr *)&sin, sizeof(sin));
        close(socket_fd);
        // 释放前面分配的内存空间
        free(broadcast_ip);

        printf("Magic Packet has been sended.\n");

        return 0;
    }
    ```
  - 这个程序本身比较简单，注释比较完整，也没什么好说明的

***********************
* **交叉编译**
  - 交叉编译当然也是在**开发机**上完成；
  - 假定我们把上面这个源程序放在 *~/wake_on_lan/* 目录下，下面是交叉编译的过程
    ```
    cd ~/wake_on_lan
    source /bin/a5.sh
    arm-openwrt-linux-gcc -Wall wakeOnLan.c -o wakeOnLan
    ```
  - 交叉编译的过程难免出错，请自行排错；
  - 交叉编译完成的程序是不能在**开发机**上运行的，需要拷贝到 *openwrt* 上才能运行，仍然假定 *openwrt* 的 IP 地址为：*192.168.2.100*，则可以这样将已经编译好的程序拷贝到 *openwrt* 上，操作在开发机上完成
    ```
    cd ~/wake_on_lan
    scp wakeOnLan root@192.168.2.100:/root/
    ```

*******************
* **在 openwrt 上运行程序**
  - 用 *ssh* 登录到 *openwrt*，登录后应该就在 */root/* 目录下，因为 */root/* 是 *root* 用户的 *HOME* 目录，然后运行程序
    ```
    ssh root@192.168.2.100

    ./wakeOnLan 192.168.2.255 00:e0:2b:69:00:03
    ```
  - 第一个参数是局域网上的广播 IP，第二个参数是要被远程唤醒的机器的 *MAC* 地址，请根据你的具体情况进行修改
  - 在我的机器上运行效果是这样的

    ![在openwrt上运行wakeOnLan][img14]
    + **图14：在openwrt上运行wakeOnLan**

***********************
* **软件调试**
  - 这个程序的调试主要是确保程序能够正确地发出 *magic packet*，需要在局域网上找另一台机器进行数据包的监听，这台监听的机器既可以运行 *windows* 也可以运行 *Linux*，最好是使用准备远程唤醒机器作为监听的机器，我们以一台运行 *ubuntu* 的机器为例来完成调试
  - 使用 *ubuntu* 下的工具 *tcpdump* 来进行数据包的监听，*tcpdump* 必须在 *root* 权限下运行；
  - 首先在监听机器上运行 *tcpdump*
    ```
    sudo tcpdump -vv -x udp port 7
    ```
  - 这行命令的意思就是监听 *udp* 端口 7 的数据包，*-vv* 的意思是显示详细的信息，*-x* 的意思是按照 16 进制显示，这两个参数也可以写成 *-vvx*
  - 在 *openwrt* 上运行 *wakeOnLan*
    ```
    ./wakeOnLan 192.168.2.255 00:e0:2b:69:00:03
    ```
  - 其中的广播 IP 和 *MAC* 地址请按照实际情况填写
  - 正常情况下，在监听机器上可以看到程序发出的 *Magic Packet*，仔细看一下这个数据包的格式是否正确
  - 在我的环境下，看到的输出如下：
    
    ![侦听magic packet][img15]
    + **图15：侦听Magic Packet**

  - 其中黄线标识的部分是 IP 头，占 20 个字节；绿线标识的部分是 UDP 头，占 8 个字节，剩下的就是 *Magic Packet*；
  - 如果你正常地侦听到了一个完整且正确的 *Magic Packet*，那么恭喜你，就快要成功了；
  - 如果你使用 *windows* 侦听 *Magic Packet* 数据包，通常使用著名的 *Wireshark*

*******************
* **局域网内的网络唤醒测试**
  - 把需要网络唤醒的机器关机，如果需要设置 *BIOS*，要先设置好 *BIOS* 再关机；
  - 使用局域网内的另一台电脑 *ssh* 登录 *openwrt*，或者使用手机登录 *openwrt*，如果使用手机登录，需要在手机上安装一个终端 *app*，我使用安卓手机，安装的 *app* 叫 *ConnetcBot*，推荐大家试一下；
  - 在 *openwrt* 运行你编写的程序 *wakeOnLan*，如果你运气好，你那台刚刚关机的电脑应该被默默的打开了电源
  - 但是，通常都没有那么好的运气，那么可能的问题如下：
    1. 程序里的广播 IP 是不是正确？
    2. *Magic Packet* 中的 *MAC* 地址是否正确？
    3. *Magic Packet* *的格式是否正确？
    4. 被唤醒的机器是否与 *openwrt* 在同一个网段？
    5. 路由器是否限制了 *UDP* 的端口 7？
    6. 如果以上都没有问题，恐怕只能怀疑你那台要被唤醒的机器不支持网络唤醒，或者 *BIOS* 设置的不正确

*******************************
* **内网穿透**
  - 做到现在这样，我们已经完成了大部分工作，下面唯一要做的是如何从外网上访问到这台 *openwrt* 的设备，这就是前面说过的 **内网穿透**；
  - 搞内网穿透是需要在互联网上有一台服务器(*VPS*)的，可以是那种很便宜性能很弱的VPS，因为我们不干别的事，就是转发一下数据而已；
  - 我自己使用的是一台俄罗斯的 *VPS*，价格只有 **US$13.04/年**，512M内存，5G的SSD，运行 *ubuntu 20.04*，虽然配置低，但是用起来感觉还是不错的，我很乐意推荐给大家：
    + [俄罗斯VPS](https://justhost.ru/?ref=149230)
  - 前面说过我使用的内网穿透的工具是一个开源项目，叫做 *frp*，项目地址如下：
    + [frp内网穿透项目](https://github.com/fatedier/frp)
    + 该项目有中文文档，大家可以按照文档下载适当的 *release*，其服务器软件 *frps* 运行在服务器(VPS)上，客户端 *frpc* 运行在 *openwrt* 上；
    + 通常服务器端软件都是 64 位的 X86 架构，比较容易搞定；
    + 要注意的是，要看清楚运行 *openwrt* 的设备是什么架构，是 32 位的还是 64 位的，比如本文中的设备 *CPU* 为 *S805*，就是一个 32 位的 *arm* 架构，否则你下载的客户端软件可能无法运行；
    + 这个软件的设置还是要费一些功夫，请认真阅读该项目的文档，并参考其范例；这里我给出我的实例

      > 服务器端配置文件：*frps.ini*，其中的 *xxx.xxx.xxx.xxx* 请按照实际情况设置，*frp_log_path* 请指向实际存放 *frp* 日志的目录

      ```
      [common]
      bind_addr = xxx.xxx.xxx.xxx
      bind_port = 57000
      bind_udp_port = 57001
      kcp_bind_port = 57000
      proxy_bind_addr = xxx.xxx.xxx.xxx
      vhost_http_port = 58080
      vhost_https_port = 58443

      log_file = /frp_log_path/frps.log
      log_level = info
      log_max_days = 3
      disable_log_color = false

      detailed_errors_to_client = true

      authentication_method = token
      authenticate_heartbeats = false
      authenticate_new_work_conns = false
      token = skyline.admin

      oidc_client_id =
      oidc_client_secret = 
      oidc_audience = 
      oidc_token_endpoint_url = 

      allow_ports = 58000-59000,50000-53000
      max_pool_count = 15
      max_ports_per_client = 0
      tls_only = false
      subdomain_host = frps.com
      tcp_mux = true
      ```
      > 客户端配置文件：*frpc.ini*，其中的 *xxx.xxx.xxx.xxx* 请按照实际情况设置；*openwrt.aaa.com* 是一个 A 记录指向 *VPS* 的域名(子域名)，也要根据实际情况进行设置

      ```
      [common]
      server_addr=xxx.xxx.xxx.xxx
      server_port=57000
      log_file=/tmp/frpc.log
      log_level=info
      log_max_days=3
      disable_log_color=false
      token=skyline.admin
      pool_count=5
      tcp_mux=true
      user=whowin
      login_fail_exit=false
      protocol=tcp
      tls_enable=falset
      dns_server=8.8.8.8
      admin_addr=127.0.0.1
      admin_port=7400
      admin_user=skyline
      admin_pwd=admin

      [ssh]
      type=tcp
      local_ip=192.168.2.100
      local_port=22
      use_encryption=false
      use_compression=false
      custom_domain=openwrt.aaa.com
      remote_port=52998
      health_check_type=tcp
      health_check_timeout_s=3
      health_check_max_failed=10
      health_check_interval_s=30

      [openwrt_web]
      type=http
      local_ip=192.168.2.100
      local_port=80
      use_encryption=false
      use_compression=true
      custom_domains=openwrt.aaa.com
      header_X-From-Where=frp           
      health_check_type=http    
      health_check_url=/        
      health_check_interval_s=90 
      health_check_max_failed=3
      health_check_timeout_s=3
      ```
  - 启动 *VPS* 上的 *frps*，*path_to* 指向实际路径
    ```
    /path_to/frps -c /path_to/frps.ini &
    ```
  - 启动 *openwrt* 上的 *frpc*，*path_to* 指向实际路径
    ```
    /path_to/frpc -c /path_to/frpc.ini &
    ```
  - 如遇问题，强烈建议认真查看 *frps.log* 和 *frpc.log*；
  - 正常情况下，现在你已经可以在互联网上通过 *frp* 访问你家里的 *openwrt* 了，像这样：
    ```
    ssh root@xxx.xxx.xxx.xxx -p 52998
    ```
  - 其中：*xxx.xxx.xxx.xxx* 为 *VPS* 的 IP 地址，*52998* 是在 *frpc.ini* 中设置的端口号，也可以设置一个域名指向 *VPS* 的 IP，比如设置 *vps.aaa.com* 的 A 记录指向 *VPS*，则可以这样登录 *openwrt*：
    ```
    ssh root@vps.aaa.com -p 52998
    ```
  - 同样，按照上面的设置，如果要访问 *openwrt* 的 *web* 界面，在浏览器上输入：*openwrt.aaa.com:58080* 即可，*58080* 是在 *frps.ini* 中设置的一个端口号；
  - 特别要注意的是，要在 *VPS* 上设置好防火墙，放开可能用到的端口，否则 *frp* 将无法正常工作

*************************
* **远程开机测试**
  - 测试远程开机使用的电脑、平板或者手机，不能连接到家里的 *wifi* 上，建议使用手机，关掉 *wifi*，打开 *ConnectBot*
  - 设置连接如下：

    ![在ConnectBot上设置连接][img16]
    + **图16：在ConnectBot上设置连接**
    *******************************
  - 通过 *frp* 连接到 *openwrt*

    ![在ConnectBot上连接openwrt][img17]
    + **图17：在ConnectBot上连接openwrt**
    *********************
  - 在 *openwrt* 上运行 *wakeOnLan*

    ![在openwrt上运行wakeOnLan][img18]
    + **图18：在openwrt上运行wakeOnLan**
    **********************
  - 正常情况下，*MAC* 地址指定的机器应该已经开机了；为了运行方便，你可以在 *openwrt* 上写一个 *shell* 脚本，免得每次都要输入 IP 地址和 *MAC* 地址，像下面这样，注意，*openwrt* 下的 *shell* 是 *ash*
    ```
    $ cat wakeOnLan.sh 
    #!/bin/ash

    /home/whowin/wakeOnLan 192.168.2.255 00:e0:2b:68:00:03
    $
    ```

------------------------------
## 7. 后记
> 前面提过，*Magic Packet* 可以在任意端口发送，通常使用端口 7 或 9，我们这个例子使用端口 7 发送，读者可以试一下从其他端口发送，比如端口 1234；

> 实际上，*openwrt* 是有现成的**网络唤醒**模块的，可以在 *openwrt* 的 *web* 界面上搜索 *luci-app-wol*，安装这个软件的同时，还会安装一个 *etherwake* 的软件，安装好以后，可以使用类似 *etherwake -b AA:BB:CC:DD:EE:FF* 的命令发送 *Magic Packet* 唤醒指定的机器，也可以通过 *web* 界面唤醒指定的电脑

  ![在web界面上发送Magic Packet][img19]
  * **图19：在web界面上发送Magic Packet**

> 至此，我们的这个项目就算做完了，我们基本上经历了一个嵌入式开发的全过程，只是每一个阶段都比较简单而已，嵌入式开发，貌似复杂，其实并没有想象的那么难；

> 在嵌入式项目的设计阶段，我们需要有足够的知识储备以便为我们的需求提出最合理的方案，在这个项目的设计中，正是由于我们储备了 *Magic Packet* 和反向代理的知识，才可以为我们的需求提出这样一个软硬件结合的方案；

> 我们这个项目基本没有硬件开发，但是通常情况下，嵌入式开发的软件工程师是需要参与到硬件开发中去的，包括芯片方案的选择等都要参与意见，以便设计开发的硬件产品在今后的软件开发中可以比较顺利，所以嵌入式软件工程师同样要具备阅读芯片的 *datasheet* 的能力和看懂硬件原理图的能力，好的嵌入式软件工程师也可以拥有非常不错的焊接技能；

> 嵌入式开发的最关键的地方还是软件开发，但比普通的软件开发所要了解的知识要多很多，比如在我们这个项目中，我们必须要知道我们所用的硬件的 *CPU* 是什么，甚至要知道这个 *CPU* 的架构，否则，我们无法为这个硬件构建正确的操作系统，也无法在开发机上为这个硬件构建正确的交叉编译环境；

> 如果你喜欢做嵌入式开发，希望这篇文章能给予你帮助。



<!-- 用于实际发布 -->
[img01]:/images/100009/money_maker.png
[img02]:/images/100009/money_maker_board.jpg
[img03]:/images/100009/openwrt_version.png
[img04]:/images/100009/toolschain_test.png
[img05]:/images/100009/wake_on_lan_architecture.png
[img06]:/images/100009/login_openwrt_first.png
[img07]:/images/100009/web_openwrt_1.png
[img08]:/images/100009/web_openwrt_2.png
[img09]:/images/100009/openwrt_fixed_ip_config.png
[img10]:/images/100009/openwrt_fixed_ip_web_1.png
[img11]:/images/100009/openwrt_fixed_ip_web_2.png
[img12]:/images/100009/openwrt_fixed_ip_web_3.png
[img13]:/images/100009/openwrt_fixed_ip_router.png
[img14]:/images/100009/openwrt_run_wakeonlan.png
[img15]:/images/100009/listen_magic_packet.png
[img16]:/images/100009/connectbot_setting.jpg
[img17]:/images/100009/connectbot_login_openwrt.jpg
[img18]:/images/100009/connectbot_run_wakeonlan.jpg
[img19]:/images/100009/luci_wake_on_lan.png
<!-- -->

<!--用于本地预览
[img01]:../../../../static/images/100009/money_maker.png
[img02]:../../../../static/images/100009/money_maker_board.jpg
[img03]:../../../../static/images/100009/openwrt_version.png
[img04]:../../../../static/images/100009/toolschain_test.png
[img05]:../../../../static/images/100009/wake_on_lan_architecture.png
[img06]:../../../../static/images/100009/login_openwrt_first.png
[img07]:../../../../static/images/100009/web_openwrt_1.png
[img08]:../../../../static/images/100009/web_openwrt_2.png
[img09]:../../../../static/images/100009/openwrt_fixed_ip_config.png
[img10]:../../../../static/images/100009/openwrt_fixed_ip_web_1.png
[img11]:../../../../static/images/100009/openwrt_fixed_ip_web_2.png
[img12]:../../../../static/images/100009/openwrt_fixed_ip_web_3.png
[img13]:../../../../static/images/100009/openwrt_fixed_ip_router.png
[img14]:../../../../static/images/100009/openwrt_run_wakeonlan.png
[img15]:../../../../static/images/100009/listen_magic_packet.png
[img16]:../../../../static/images/100009/connectbot_setting.jpg
[img17]:../../../../static/images/100009/connectbot_login_openwrt.jpg
[img18]:../../../../static/images/100009/connectbot_run_wakeonlan.jpg
[img19]:../../../../static/images/100009/luci_wake_on_lan.png
-->




