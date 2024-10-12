---
title: "在DOS下进行网络编程（上）"
date: 2008-04-09T10:06:26+08:00
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
  - "network"
  - "packet driver"
  - "watt-32"
draft: false
# https://blog.csdn.net/gaiazhang/article/details/17074091
# http://www.wangchao.net.cn/bbsdetail_147812.html
# https://dankohn.info/projects/Dos_Networking.htm
# https://vetusware.com/    hengch@163.com/NZ52n8
postid: 160007
---

该文介绍了在 DOS 下的进行网络编程的基本方法，该文写于2008年4月，2022年重新发表修了一些修改
<!--more-->

> windows下作网络编程不是一件很难的事，但在DOS下就不是很容易了，对很多人来说甚至是无从下手，本文详细阐述在DOS下进行网络编程的方法，下一篇文章讲给出一个具体的实例。

## DOS 网络编程准备工作
* Packet Driver
  - 要在DOS下进行网络编程，首先要有一个Packet Driver，这是一个与硬件相关的驱动程序，符合FTP Software提出的PC/TCP Packet Driver规范，有兴趣的读者可以在下面地址下载这份规范：[PC/TCP Packet Driver Specification](http://crynwr.com/packet_driver.html)
  - 我使用的机器主板上的网络芯片是rt8139，对应的Packet Driver是rtspkt.exe，驱动方法是在autoexec.bat中加入下面这一行：
    ```
    @rtspkt 0x62
    ```
  - 其中，0x62为中断向量，如果在你的机器上这个中断向量已经被占用，你可以改成其他的未被占用的中断向量号，按照PC/TCP规范，应该在0x60 -- 0x80之间。
* WATT-32库
  - 有了Packet Driver后，我们还需要有一个好用的能够提供 TCP/IP Socket 编程接口的函数库，在 DJGPP 下我们建议使用 WATT-32 库，这个库比 DJGPP 官方网站上提供的 WATTCP 库内容更加丰富，而且文档完整和范例程序丰富，可以在下面网站上下载到：
    ```
    http://www.watt-32.net/
    ```
  - WATT-32 是以源代码的形式发行的，所以在使用前需要自行进行编译链接，整个过程如下（以下步骤是建立在你已经按照前面的博客文章 [《在DOS下的DJGPP+RHIDE安装实作》][article01]正确安装完毕DJGPP和RHIDE）：
    1. 首先从上述网址上下载 WATT-32，共有3个 zip包，如下：
      + watt32b*.zip，watt32s*.zip，watt32d*.zip
      + 其中 "*" 会随版本号不同有所不同。

    2. 通过U盘或其他媒介作为载体把 3 个文件拷贝到要配置的机器上，由于 DOS 不支持长文件名，需要把这三个文件分别改成：
      ```
      watt32b.zip，watt32s.zip 和 watt32d.zip
      ```

    3. 将三个文件解压缩到一个子目录下，例如：c:\net\watt
      ```
      c:\>md net
      c:\>md net\watt
      c:\>unzip32 watt32b.zip -d c:\net\watt
      c:\>unzip32 watt32s.zip -d c:\net\watt
      c:\>unzip32 watt32d.zip -d c:\net\watt
      在解压缩过程中，有一些共用文件会产生覆盖，没有关系，覆盖所有的文件。
      ```

    4. 在环境变量中增加变量：WATT_ROOT
      ```
      需要修改 autoexec.bat，增加下面一行：
      set WATT_ROOT=c:\net\watt
      然后重新启动计算机。
      ```

    5. 产生make文件
      ```
      c:\>cd\net\watt\src
      c:\net\watt\src>configur djgpp
      这一步完成后会看到提示，要求你执行 make -f djgpp.mak，照做就好了。
      ```

    6. 生成WATT-32库
      ```
      照上一步的提示
      c:\net\watt\src>make -f djgpp.mak
      这个步骤时间比较长，需要耐心等待一会。在编译过程中会有一些“警告”出现，不用管它们。
      ```

    7. 为使用 WATT-32 库配置环境变量
      + 在编译完成后，我们还要在 autoexec.bat 里增加四个环境变量，我们在步骤 4 中增加的 WATT_ROOT 环境变量仅在编译的过程中有用，实际使用中并不需要这个环境变量，所以可以去掉（当然，不去掉也没有关系）。
      + 在 autoexec.bat 中增加下面四行：
        ```
        set WATTCP.CFG=c:\net\watt\bin
        set ETC=c:\net\watt\bin
        set C_INCLUDE_PATH=c:/net/watt/inc
        set LIBRARY_PATH=c:/net/watt/lib
        ```
      + WATTCP.CFG 是 WATT-32 的配置文件 wattcp.cfg 所在的位置，你也可以把 wattcp.cfg 放在其他目录下，比如：c:\net\cfg 目录下，但要记得把 set WATTCP.CFG=c:\net\watt\bin 这句改成：
        ```
        set WATTCP.CFG=c:\net\cfg
        ```
      + 至此，安装已经完成，应该可以在 c:\net\watt\lib 目录下看到文件 libwatt.a，这就是我们需要的网络函数库。
* 配置
  - 此时，可能仍然不能进行网络编程，还需要实际配置一下 wattcp.cfg 文件，前面提到，该文件放置在c:\net\watt\bin目录下，我们可以在该目录下看到该文件的样板，至少我们要在配置文件中配置 IP 地址和地址掩码，类似下面的形式：
    ```
    my_ip=192.168.0.10
    mask=255.255.255.0
    ```
  - 有时，还需要配置网关和解析服务器，类似下面：
    ```
    gateway=192.168.0.1
    nameserver=202.106.134.133
    ```
  - nameserver可以写一个或者多个，每个解析服务器占一行。
  - 一般情况下，配置这四个参数就足够了，如果希望配置更多的参数可以参考 wattcp.cfg 中的说明。
## 相关资料
  * 推荐一篇网络编程的经典文章
    - 学习DOS下的网络编程，有一篇文章很值得一读，[《Beej's Guide to Network Programming Using internet Sockets》][article02]，我最初看到这篇文章的版本还是1.5.5，是1999年1月13日写的，写此文时，其最新版本是 2.4.5，是2007年8月5日完成的，2022年的时候，版本已经更新到 3.1.5，是2020年11月更新的；看来这哥儿们一辈子孜孜不倦地在维护这份文档，这真是个好同志
  * 该文以前在网上有不少免费的中译本，现在免费的很难找到了，中文学术界的悲哀。不过我手里倒是还存有两份中译本，可以在 [**这里**][article03] 或者 [**这里**][article04] 下载，只是不知道是哪个版本的。
  * 该文中所提到的数据结构及函数均为 BSD 规范下的网络编程的数据结构和函数，在WATT-32库中均适用。
  * 另外，读者可以在 [**这里**][article05] 下载到关于 WATT-32 库中的所有函数的使用说明，也算是珍藏本了
  * 实际上，WATT-32 并没有出一本完整好用的手册，而是沿用了 WATTCP 的手册，但由于 WATTCP 的手册是收费的，现在收费的也没有了，http://www.erickengelke.com/wattcp/docs.shtml（2017年3月13日注：这个链接已经光荣失效）
  * 在WATT-32的bin目录下还有很多子目录，里面有很多的范例程序，在开始进行网络编程前，可以先看看这些范例程序，下面我们拿其中的一个范例来说明，如何在 RHIDE 下编译含有 WATT-32 库的程序。
  * 我们以 ftpsrv 范例来说明如何在 RHIDE 下编译，首先进入该范例的目录，并用 RHIDE 打开范例程序：
    ```
    c:\>cd\net\watt\bin\ftpsrv
    c:\net\watt\bin\ftpsrv>rhide ftpsrv
    ```
    - 这样，RHIDE会自动建立一个 ftpsrv 的 project，目前该 project 中没有任何项目，按下面步骤把程序 ftpsrv.c 加入到 project 中：
      ```
      alt+p-->选择Add Item-->选择ftpsrv.c-->回车-->按Esc键退出
      ```
    - 这样我们就在屏幕下方的 Project Window 中看到了一个项目：ftpsrv.c，此时如果你选择编译链接（按ALT+C再选择Make），会在链接时产生一些错误，这是由于我们没有把 WATT-32 库链接进去的原因，按下面方法操作：
      ```
      ALT+O-->选择Libraraies-->填入watt-->按SHIFT+TAB(此时光标应停在watt前的[ ]上-->按空格（看到[X]）-->回车
      ```
    - 此时再按如下步骤进行编译链接，就可以生成ftpsrv.exe。
      ```
      ALT+C-->选择Make-->回车
      ```

> 至此，我们学会了在 DJGPP 下安装、配置 WATT-32 的过程，同时学会了在 DJGPP 下使用 RHIDE 编译使用 WATT-32 库的程序，我们已经做好了进行网络编程的准备。

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[article01]:/post/blog/dos/0004-install-djgpp-rhide-under-dos/
[article02]:https://beej.us/guide/bgnet/
[article03]:/references/bgnet.txt
[article04]:/references/Beej-cn-20140429.pdf
[article05]:/references/watt-32.chm
