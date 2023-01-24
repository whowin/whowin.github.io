---
title: "在 DOS 下安装 DJGPP"
date: 2008-03-27T14:27:38+08:00
author: whowin
sidebar: false
authorbox: false
pager: true
toc: true
categories:
  - "DOS"
tags:
  - "DOS"
  - "DJGPP"
  - "保护模式"
  - "DPMI"
draft: false
postid: 160003
---


该文翻译自 **djgpp version 2.03** 的 *.readme* 文件。本文中包含获得、安装、使用 **DJGPP** 的信息，在寻求帮助之前请完整阅读该文档。
<!--more-->

## DJGPP介绍
  * DJGPP是MS-DOS下使用C/C++开发32位保护模式软件的免费环境，点击进入[DJGPP的官方网站](http://www.delorie.com/djgpp/)
  * DJGPP的最新状态及其他信息（在线文档，FAQ、邮件文档）通过DJGPP的官方网站发布。有关DJGPP的使用与特点方面的讨论在DJGPP的新闻组（提交到comp.os.msdos.djgpp）和DJGPP的邮件列表（发信息到djgpp@delorie.com，通过listserv@delorie.com订阅），有关DJGPP的最新版本信息，请登陆网站查看。
  * 在下面网站上有一些获取、设置和使用DJGPP的文档：[DJGPP文档](http://www.delorie.com/djgpp/doc/)
  * **截止到2022年4月11日，DJGPP的网站依然存活**

## 发行
  * DJGPP的发行包，根据内容被分成了几个目录，每个目录下有一个00_index.txt的文件，对这个目录下的文件进行了说明。
  * 以“b”结尾的zip压缩包中包含有二进制文件和在线文档，在写这篇文件时，文件目录清单如下：
    |文件或目录名|说明|
    |:---------|:----|
    |FAQ|一个很小的文件，告诉你应该读的文档在什么地方（包括完整的FAQ）|
    |||
    |v2/||
    |unzip32|一个免费的解压缩程序（与pkunzip类似）|
    |djdev203|DJGPP V2开发包和运行程序|
    |djlsr203|DJGPP V2基本库源程序|
    |djtst203|DJGPP V2测试程序（用于测试C库）|
    |djcrx203|DJGPP V2跨平台编译器支持文件（来自djlsr/djdev）|
    |djtzn203|DJGPP V2 TimeZone文件|
    |djtzs203|DJGPP V2 TimeZone源程序|
    |faq*b|各种格式（info, ascii, html......）的FAQ|
    |faq*s|FAQ文本格式源文件和生成各种格式FAQ的工具|
    |frfaq*|DJGPP FAQ法文版|

    |文件或目录名|说明|
    |:---------|:----|
    |v2apps/|各种用DJGPP编写的应用程序和用于DJGPP的应用程序，如：RHIDE和Tex|
    |v2gnu/|各种用于DJGPP的FSF/GNU程序，如：gcc和binutils|
    |v2tk/|各种与DJGPP一同使用的工具包，如：Allegro和libsocket|
    |v2misc/||
    |csdpmi*|CWSDPMI，Charles Sandmann的免费的DPMI服务器程序和文档|
    |mlp*|ML的可执行packer，二进制文件（“djp”）|
    |pmode*|PMODE stub for djgpp v2|
    |wmemu*|WM的可选的用于DJGPP V2的387仿真文件|
    ||（其他与DJGPP一起使用的东西）|

## 下载
  * 要获得并运行DJGPP，下面是新用户必须了解的内容，更详细的信息请阅读DJGPP文档和FAQ，建议新用户完整阅读一下FAQ。
  * 下载什么？
    - [http://www.delorie.com/djgpp/zip-picker.html](http://www.delorie.com/djgpp/zip-picker.html) 将指引你了解应该下载那些文件，一般情况下，仅下载安装发行包即可，大多数人不需要下载源程序。
    - 要开发C程序，你需要djdev203.zip, gcc*b.zip和bnu*b.zip
    - 如果要开发C++程序，你还需要gpp*b.zip
    - 要阅读在线手册，需要下载txi*b.zip，并且运行“info”，如果你没有安装DPMI服务器，你还需要下载cwdpmi*b.zip。（windows, QDPMI, 386Max, NWDOS, OpenDOS, OS/2, win/NT和linux DOSEmu都提供DPMI服务，在这些环境下你不需要CWSDPMI）。详细信息请下载faq*b.zip（完整FAQ）并阅读第4节“下载什么和在哪里下载？”。
    - 如果你使用的windows Me, 2000或xp是2001年12月以前发行的，可能不能正常运行DJGPP，所以请确认你的windows版本。

## 安装
  1. 为DJGPP建立一个目录，如：c:\djgpp（注意：不能把DJGPP安装在一个目录下，如：c:\dev, d:\dev等，这将使DJGPP无法运行，详细内容请看FAQ）。目录名不能用长文件名，目录名中也不能有空格或其它特殊字符。
    * 如果你以前安装了DJGPP V1.x版，最好是删除“bin/”目录下的内容或者将该目录移到其它目录下（不能是PATH设置的目录），并且删除老的v1.x版安装的其它内容（在用户提交的问题报告中，有一些就是由于无意中把DJGPP v2和老的v1.x混在一起引起的）。v1.x版中唯一可以留下的程序是go32.exe。
    * 如果你要在Windows NT 4.0下运行，在安装之前你需要决定是使用长文件名还是使用DOS 8.3文件名，如果你打算使用长文件名，在解压缩之前你需要先下载并安装长文件名驻留程序（ntlfn*b.zip）。

  2. 从你指定的目录中解压缩所有的文件，注意要保存原目录结构。例如：
    ```
    pkunzip -d djdev203 或者 unzip32 djdev203
    ```
    * 在Windows 9x, Windows/ME, Windows 2000和Windows XP下安装，应使用支持长文件名的解压缩程序，InfoZip的UnZip和PKUnZip，包括WinZip都支持长文件名，从DJGPP网站上得到的unzip32.exe也支持长文件名。确定是否支持长文件名的一种方法可以查找一个文件：include/sys/sysmacros.h，如果你看到的文件名是：sysmacro.h，说明你的解压缩程序不支持长文件名，你需要换一个解压缩程序。
    * 必须保证解压缩时保存了原有的目录结构，如果你使用WinZip，你一定要检查选中“Use folder names”选项，如果使用pkunzip，必须要使用-d开关。
    * 如果不打算在Windows/NT（版本4以下，不是W2K！）下使用长文件名的驻留程序，那么unzip程序将不支持长文件名，没有长文件名的驻留程序，DJGPP不能读取NT4下的长文件名。unzip32.exe可以避免这个问题，所以强烈推荐。

3. 解压缩所有的zip文件后，要将DJGPP的环境变量指到DJGPP安装目录下的DJGPP.ENV文件上，并把BIN子目录加入到PATH设置中。
  * 设置方法要看你使用什么操作系统。
    - Windows 98
      + 点击“开始”
      + 选择：程序->附件->系统工具->系统信息
      + 像下面说明的那样修改autoexec.bat文件

    - Windows ME
      + 点击“开始”，选择“运行”，键入“msconfig.exe”，点击“确定”
      + 点击“环境变量”
      + 修改PATH系统变量加入DJGPP的bin子目录
      + 增加一个新变量DJGPP并且把值设置成DJGPP.ENV的全路径名，参照下面解释

    - Windows NT
      + 右击“我的电脑”，选择“属性”
      + 选择“环境变量”
      + 编辑PATH系统变量增加DJGPP bin子目录
      + 增加一个新变量DJGPP并且把值设置成DJGPP.ENV的全路径名，参照下面解释

    - windows 2000和Windows XP
      + 右击“我的电脑”，选择“属性”
      - 选择“高级”并点击“环境变量”
      + 编辑PATH系统变量增加DJGPP bin子目录
      + 增加一个新变量DJGPP并且把值设置成DJGPP.ENV的全路径名，参照下面解释

  * 对于其它系统（DOS, Windows 3.x和Windows 95），使用任意一种文本编辑软件，比如 *EDIT*，编辑启动盘（通常是C）根目录下的 **autoexec.bat** 文件。
    - 不改变autoexec.bat文件或者不改变全局环境变量，你也可以为DJGPP单独建立一个BAT文件，如果你的系统中有多个编译器，可能需要这种方法。
    - 不管你用什么方法，假定你的DJGPP安装在C:\DJGPP下，则DJGPP和PATH这两个环境变量应该如下设置：
      ```
      set DJGPP=C:\DJGPP\DJGPP.ENV
      set PATH=C:\DJGPP\BIN;%PATH%
      ```

4. 重新启动。
  * 这将使你在 **autoexec.bat** 中加入的两行生效。（在Windows NT，Windows 2000和Windows XP下，设置立即生效，所以不需要重新启动，但要关闭并重新打开DOS窗口）

5. 不带参数运行 *go32-v2.exe* 程序
  ```
  go32-v2
  ```
  * 显示系统中DJGPP可用的DPMI存储器和DPMI交换空间，如下：
    ```
    DPMI memory available: 8020 kb
    DPMI swap space available: 39413 kb
    ```
  * 实际数值会根据你系统中内存大小和空闲磁盘空间大小以及DPMI服务器不同而有所变化。如果两个数值的总和少于4MB，请阅读FAQ的第3.9节，“How to configure your system for DJGPP”。（尽管两数值之和大于4MB，为了使你的系统性能更高，也可以阅读这部分）


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png
