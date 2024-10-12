---
title: "使用libnm扫描wifi信号的实例"
date: 2024-07-10T16:43:29+08:00
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
    - libnm
draft: true
#references:
# - [Connect to WIFI from C program](https://stackoverflow.com/questions/57586732/connect-to-wifi-from-c-program)
# - (https://cgit.freedesktop.org/NetworkManager/NetworkManager/tree/examples/C/glib/add-connection-libnm.c) 
# - [libnm Reference Manual](https://networkmanager.dev/docs/libnm/latest/)
# - [Using libnm](https://networkmanager.dev/docs/libnm/latest/usage.html)
# - [glib C examples](https://github.com/cyphermox/NetworkManager/tree/master/examples/C/glib)
# - [NetworkManager](https://networkmanager.dev/)
# - [GLib – 2.0](https://docs.gtk.org/glib/index.html)
# - [Gio – 2.0](https://docs.gtk.org/gio/index.html)


postid: 180028
---

打开电脑连接wifi是一件很平常的事情，这个过程通常并不需要在程序中完成，操作系统会为我们提供完备的工具软件，但在某些应用场景下我们仍然需要了解现场的无线网络情况，这时就需要扫描wifi信号，前面已经有三篇文章介绍了基于WE(Wireless Extensions)扫描wifi信号的方法，本文将讨论在Linux下基于libnm库扫描wifi的方法，本文将给出完整的源代码，本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；尽管本文内容主要涉及无线网络，但读者并不需要对 802.11 标准有所了解。

<!--more-->

## 1 前言
* NetworkManager 是标准的 Linux 网络配置工具套件；它支持从桌面到服务器和移动设备的各种网络设置，并与流行的桌面环境和服务器配置管理工具完美集成。
* NetworkManager 在系统总线上提供 D-Bus 接口。您可以使用此接口查询网络状态和网络接口的详细信息（例如当前 IP 地址或 DHCP 选项），以及激活、停用、创建、编辑和删除已保存的网络连接。
* 关于 D-Bus 的概念可以参考另一个专栏 [『进程间通信专栏』](https://blog.csdn.net/whowin/category_12404164.html)
* libnm 将 D-Bus API 包装在易于使用的 GObjects 中，并且对于基于 glib 的应用程序来说，使用起来通常更加简单。

libnm 的 API 提供了两个核心方面。一个是NMConnection和NMSetting 类型，它们有助于处理 NetworkManager 的连接配置文件。另一个是 NMClient，它是来自 D-Bus API 的 D-Bus 对象的缓存。





## 函数说明

* `NMClient *nm_client_new(GCancellable *cancellable, GError **error);`
    - 功能：建立一个新的 NMClient 实例，这是一个阻塞函数，一直要完成对新 NMClient 完成初始化后才能返回
    - 参数：
        + cancellable - 是 Gio 中的一个类，在这个函数调用中通常为 NULL
        + error - 是 Glib 中的一个结构，用于表示错误代码和错误信息，定义如下：
            ```C
            struct GError {
                GQuark domain;
                gint code;
                gchar* message;
            }
            ```
    - 返回：NMClient 指针

* `const GPtrArray *nm_client_get_all_devices(NMClient *client);`
    - 功能：获取 NetworkManager 下管理的所有设备
    - 参数：
        + client - 使用 nm_client_new() 建立的 NMClient
    - 返回：包含所有 NMDevice 的 GPtrArray；返回的数组归 NMClient 对象所有，不要修改。

* 宏定义：`g_ptr_array_index(array, index_)`
    - 功能：返回指针数组(array)中给定索引(index_)处的指针
    - 参数：
        + array - GPtrArray
        + index_ - 数组索引值
    - 返回：数组索引项的指针
    - 说明：该宏不会去检查数组边界，所以调用者要保证 index_ 不要越界








## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/180025/wifi-new-scanner.c

[article01]:/post/blog/network/0022-how-to-scan-wifi-signal/
[article02]:/post/blog/network/0025-another-wifi-scanner-example/

<!--CSDN
[article01]:https://blog.csdn.net/whowin/article/details/131504380
[article02]:https://blog.csdn.net/whowin/article/details/137711398
-->

[article03]:https://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html
[article04]:https://github.com/HewlettPackard/wireless-tools
[article05]:https://people.iith.ac.in/tbr/teaching/docs/802.11-2007.pdf

[img01]: /images/180025/screenshot-of-iwlist-scan.png
[img02]: /images/180025/screenshot-of-driver-string.png
[img03]: /images/180025/screenshot-of-IE-data.png
[img04]: /images/180025/screenshot-of-80211-2007-1.png
[img05]: /images/180025/screenshot-of-IE-structure.png
[img06]: /images/180025/screenshot-of-rates.png
[img07]: /images/180025/wifi-new-scanner.gif
