---
title: "给Allegro增加一个JPEG库"
date: 2008-05-15T11:42:58+08:00
author: whowin
sidebar: false
authorbox: false
pager: true
toc: true
categories:
  - "DOS"
tags:
  - DOS
  - DJGPP
  - ALLEGRO
  - jpgalleg
draft: false
postid: 160013
---

Allegro本身不能对jpeg图片进行处理，比如把生成的图形存储成jpeg文件，所以给Allegro配上一个合适的jpeg库是很有必要的，本文详细介绍了如何在Allegro上安装一个叫做jpgalleg的jpeg库。**本文是我2008年的作品，2023年重新整理发布，仅为存档，其中的程序并没有再次验证，特此说明。**
<!--more-->

## 前言
> Allegro我们已经安装好了，相信应该很好用，我在实际运用中感觉Allegro最遗憾的问题是无法对JPEG图片进行处理，这给我带来了很多麻烦，比如在工控系统下，通常存储空间都很有限，比如使用FLASH memory，或者使用小容量的CF卡(因为要考虑成本)等，当要存储采集到的图片时，当然希望占用的空间越小越好，但Allegro却不能存储成JPEG格式；或者我们通过网络从服务器上获得图片，我们当然希望图片文件小一些，这样传输的可以快一些，但Allegro却不能调入JPEG文件，种种这些麻烦让我下决心找一个好用的JPEG库，好在很幸运，我找到了一个可以和Allegro完美结合的JPEG库：JPGALLEG。

> 本文介绍如何在已经搭建好的平台：DOS+DJGPP+ALLEGRO的环境下，安装JPGALLEG库。

## 1. 下载JPGALLEG库
* JPGALLEG库的官方下载地址在: ```http://wiki.allegro.cc/JpgAlleg```，但是这个网址现在已经失效了(2023年4月21日)；
* 好在我保留了jpgalleg 2.5，可以在 [**这里**][web02] 下载

## 2. 编译JPGALLEG库
* 由于JPGALLEG库是以源代码形式发行的，所以我们要进行编译后才可以使用。
* 将下载到的jpgall25.zip放到c:\下，键入命令：```unzip32 jpgall25```
* 会在c:下建立一个新目录c:\jpgall2.5，所有的文件会被解压缩到这个目录下，如果你使用pkunzip解压缩，请不要忘记使用-d选项。
* 进入jpgall2.5目录，执行编译
    ```
    c:\>cd jpgall2.5
    c:\jpgall2.5>fix djgpp
    c:\jpgall2.5>make
    c:\jpgall2.5>make install
    ```
* 一般情况下在执行make这一步骤时，可能会出现一些警告信息，不用管它，正常情况下，不会有问题。
* 安装完毕。

## 3、测试JPGALLEG库
* 首先进入DJGPP安装目录下的lib下，应该可以看到一个库文件libjpgal.a，这基本说明安装成功了。
* 按照JPGALLEG库随带的readme文件看，似乎在编译时加上-ljpgal就可以使用JPGALLEG库，这是gcc的编译方法，在DOS的rhide下并不成功。
* 使用rhide工作时，为编译程序建立工程，哪怕只有一个程序也要建立一个工程（Project），并把刚编译完成的库libjpgal.a作为一个工程项目加到工程中。
  - 在rhide的做如下设置：```Option---->Directories---->Libraries---->c:\djgpp\lib```
    > 如下面两图：

    ![给Allegro增加一个JPEG库][img01]

    ![给Allegro增加一个JPEG库][img02]

* 当然，如果你的DJGPP不是安装在c:\djgpp目录下，请按照实际情况设置。
* 此时，我们可以进入到JPGALLEG的目录c:\jpgall2.5，在这个目录下应该可以看到一个叫examples的子目录，进入该子目录，在这个子目录下有ex1.c ex2.c ex3.c ex4.c ex5.c五个范例文件，我们试着编译其中的一个，比如ex1.c。
  - ```c:\jpgall2.5\examples>rhide ex1```
  - 将ex1.c和c:\djgpp\lib\libjpgal.a加入到工程项目中，并按照上一步的方法进行设置，由于需要Allegro库，记得参考设置Allegro的编译选项。
  - 在菜单上选择：```Compile---->Make```
* 应该完美地完成编译，然后你可以按Ctrl+F9运行以下，屏幕上你应该可以看到一幅漂亮的图片，就像下图：

    ![给Allegro增加一个JPEG库][img03]

> 至此，一切已经就绪。Enjoy it!

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[web01]:http://wiki.allegro.cc/JpgAlleg/
[web02]:/software/jpgalleg/jpgall25.zip

[img01]:/images/160013/add-jpgalleg-under-rhide.jpg
[img02]:/images/160013/set-lib-path-under-rhide.jpg
[img03]:/images/160013/jpgalleg-logo.jpg


