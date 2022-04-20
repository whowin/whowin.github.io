---
title: "在 DOS 下的 DJGPP + RHIDE 安装实作"
date: 2008-03-28T13:48:41+08:00
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
  - "RHIDE"
draft: false
postid: 160004
---


本文介绍在一个纯DOS构建一个C、C++和汇编的保护模式开发环境，编写的程序可以在纯DOS环境下运行在保护模式下
<!--more-->
## 1、安装环境
  * 主板：GX1-C19工控主板，AMD GX1 233MHz CPU，64M内存，8GB IBM 3.5英寸硬盘，支持USB
  * 软件：安装前，硬盘中已安装完整的DOS 6.22，DOS下的USB驱动
    - 我的config.sys文件内容如下：
      ```
      DEVICE=C:\DOS\SETVER.EXE
      DEVICE=C:\DOS\HIMEM.SYS
      DOS=HIGH
      FILES=30
      LASTDRIVE=Z
      DEVICE=C:\USB\ASPIOHCI.SYS
      DEVICE=C:\USB\DI1000DD.SYS
      ```
    - 我的autoexec.bat文件内容如下：
      ```
      @ECHO OFF
      PROMPT=$P$G
      set PATH=C:\DOS
      set TEMP=C:\DOS
      doskey
      ```

  * USB驱动程序：如果您需要，请与我联系：hengch@163.com

## 2、下载所需的DJGPP安装包
  * 打开网页：[http://www.delorie.com/djgpp/zip-picker.html](http://www.delorie.com/djgpp/zip-picker.html)
  * 做如下选择，其中[ ... ]部分为我的选择，其他为提示。
    ```
    FTP Site
            Select a suitable FTP site:
                  [Pick one for me]
    ```
    ```
    Basic Functionality
            Pick one of the following:
                  [Build and run programs with DJGP]
    ```
    ```
      Which operating system will you be using? 
                  [MS-DOS, OpenDOS, PC-DOS]
    ```
    ```
      Do you want to be able to read the on-line documentation?
                  [Yes]
    ```
    ```
      Which programming languages will you be using?
                  [C]
                  [C++]
                  [Assembler]
    ```
    ```
      Which IDE(s) would you like?
                  [RHIDE, similar to Borland's IDE, including a built-in editor and debugger.]
    ```
    ```
      Would you like gdb, the text-mode GNU debugger? You don't need it if you get RHIDE. 
                  [No]
    ```
    ```
    Extra Stuff

      Please check off each extra thing that you want.
    ```

  * 选择完毕后点击 **Tell me which files I need** 按钮，则给出如下内容：
    ```
      unzip32.exe to unzip the zip files              95 kb

      v2/copying.dj DJGPP Copyright info               3 kb
      v2/djdev203.zip DJGPP Basic Development Kit    1.5 mb
      v2/faq230b.zip Frequently Asked Questions      664 kb
      v2/readme.1st Installation instructions         22 kb

      v2apps/rhid15ab.zip RHIDE                      6.0 mb

      v2gnu/bnu217b.zip Basic assembler, linker      3.9 mb
      v2gnu/gcc423b.zip Basic GCC compiler           4.3 mb
      v2gnu/gpp423b.zip C++ compiler                 4.5 mb
      v2gnu/mak3791b.zip Make (processes makefiles)  267 kb
      v2gnu/txi411b.zip Info file viewer             888 kb

      Total bytes to download:                   23,102,842
    ```

  * 大致需要下载 23MB 的安装包

## 3、安装
  * 下载内容通过U盘存放到硬盘中。
  * 在硬盘中建立目录：c:\djgpp
    ```
      c:\>md djgpp
    ```
  * 拷贝安装包到c:\djgpp下
    ```
      c:\copy g:. c:\djgpp          (我的USB盘为g)
    ```
  * 解压缩所有安装包
    ```
      c:\>cd\djgpp
      c:\djgpp>unzip32 *.zip
    ```
  * 这个过程比较长，请耐心等待。
  * 修改配置
    ```
      c:\djgpp>cd\
      c:\>edit autoexec.bat
      增加一行：set DJGPP=C:\DJGPP\DJGPP.ENV
      把原来的：set PATH=c:\DOS 改成：set PATH=c:\DOS;c:\djgpp\bin
      存盘退出。
    ```
  * 重新启动
  * 测试 **DJGPP** 的安装情况
    ```
      重新启动后
      c:\>go32-v2
      显示错误提示：Load error: no DPMI - Get csdpmi*b.zip
    ```
  * 这是因为 *go32-v2.exe* 这个程序是32位保护模式下的程序，而我们没有安装 **DPMI** 服务，所以不能运行。
  * 下载 v2misc/csdpmi5b.zip
    - 下载地址：[http://www.delorie.com/pub/djgpp/current/v2misc/csdpmi5b.zip](http://www.delorie.com/pub/djgpp/current/v2misc/csdpmi5b.zip)
  * 安装 **CWSDPMI**
    - 将下载的 *cwdpmi5b.zip* 拷贝到硬盘的 **c:\djgpp** 下：
      ```
      c:\>copy g:cwdpmi5b.zip c:\djgpp
      解压缩
      c:\>cd djgpp
      c:\djgpp>unzip32 cwdpmi.zip
      ```
  * 再次测试 **DJGPP**
    ```
      c:\djgpp>cd\
      c:\>go32-v2
      我们得到提示：
      DPMI memory available: 62401 kb
      DPMI swap space available: 129919 kb
      说明DJGPP安装成功。
    ```
## 4、其他
  * 安装过程中，实际上我们已经安装了一个开发环境：rhide
  * 可以这样测试 rhide 已经安装成功：
    ```
    c:\>rhide
    ```
  * 你可以看到一个类似 turbo C 的界面，这就是 RHIDE，rhide 和 djgpp 配合十分默契。
  * 至此，你已经可以开发 C/C++ 的32位保护模式下的程序了。
