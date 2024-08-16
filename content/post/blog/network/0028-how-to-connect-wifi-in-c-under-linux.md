---
title: "Linux下使用libiw进行无线信号扫描的实例"
date: 2024-04-13T16:43:29+08:00
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
    - "802.11"
    - wifi
    - "无线网络"
    - ioctl
draft: true
#references: 

postid: 180026
---

打开电脑连接wifi是一件很平常的事情，但这些事情通常都是操作系统下的wifi管理程序替我们完成的，如何在程序中连接指定的wifi其实很少有资料介绍，本文将讨论在Linux下使用C语言连接wifi的方法，本文给出的实例不使用任何第三方软件和库，就可以连接到指定的wifi上，本文将给出完整的源代码，本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；尽管本文内容主要涉及无线网络，但读者并不需要对 802.11 标准有所了解。

<!--more-->

## 1 前言
* 







## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[src01]:https://whowin.gitee.io/sourcecodes/180025/wifi-new-scanner.c

[article01]:https://blog.csdn.net/whowin/article/details/131504380
[article02]:https://blog.csdn.net/whowin/article/details/137711398

[article03]:https://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html
[article04]:https://github.com/HewlettPackard/wireless-tools
[article05]:https://people.iith.ac.in/tbr/teaching/docs/802.11-2007.pdf

[img01]: https://whowin.gitee.io/images/180025/screenshot-of-iwlist-scan.png
[img02]: https://whowin.gitee.io/images/180025/screenshot-of-driver-string.png
[img03]: https://whowin.gitee.io/images/180025/screenshot-of-IE-data.png
[img04]: https://whowin.gitee.io/images/180025/screenshot-of-80211-2007-1.png
[img05]: https://whowin.gitee.io/images/180025/screenshot-of-IE-structure.png
[img06]: https://whowin.gitee.io/images/180025/screenshot-of-rates.png
[img07]: https://whowin.gitee.io/images/180025/wifi-new-scanner.gif
