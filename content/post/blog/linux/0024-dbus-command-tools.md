---
title: "IPC之十四：与 D-Bus 相关的若干命令行工具介绍"
date: 2023-12-10T16:43:29+08:00
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
  - 进程间通信
  - "D-Bus"
  - IPC
draft: true
#references: 

# - [D-Bus : Transmit a Data Array in Simple and Useful Form](http://gaiger-programming.blogspot.com/2015/08/d-bus-simple-and-useful-example-to-send.html)
#   - 里面有一个很不错异步处理信号的例子
# - [D-Bus Tools](https://www.freedesktop.org/wiki/Software/DbusTools/)
# - [dbus-send](https://dbus.freedesktop.org/doc/dbus-send.1.html)
# - [dbus-monitor](https://dbus.freedesktop.org/doc/dbus-monitor.1.html)
# 

postid: 100024
---


IPC 是 Linux 编程中一个重要的概念，D-Bus 是 IPC 的一种重要实现，在 IPC 系列文章中，已经对 D-Bus 做过介绍，本文主要介绍几个与 D-Bus 相关的命令行工具，通过这些工具我们可以在不编程的情况下，查看到 D-Bus 的状态，向 D-Bus 总线发送消息等；本文给出了众多的应用实例；本文不适合 Linux 编程的初学者阅读。

<!--more-->

* busctl
    > busctl introspect org.freedesktop.systemd1 /org/freedesktop/systemd1/unit/ssh_2eservice


* dbus-send

* dbus-monitor

* D-Feet
    - https://wiki.gnome.org/action/show/Apps/DFeet?action=show&redirect=DFeet


* bustle
    - https://gitlab.freedesktop.org/bustle/bustle


* gbus





-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

[article01]: https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
[article02]: https://whowin.gitee.io/post/blog/linux/0011-ipc-examples-of-fifo/
[article03]: https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/
[article04]: https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/
[article05]: https://whowin.gitee.io/post/blog/linux/0015-systemv-semaphore-sets/
[article06]: https://whowin.gitee.io/post/blog/linux/0016-posix-semaphores/
[article07]: https://whowin.gitee.io/post/blog/linux/0017-systemv-shared-memory/
[article08]: https://whowin.gitee.io/post/blog/linux/0018-posix-shared-memory/
[article09]: https://whowin.gitee.io/post/blog/linux/0019-ipc-with-unix-domain-socket/
[article10]: https://whowin.gitee.io/post/blog/linux/0020-ipc-using-files/
[article11]: https://whowin.gitee.io/post/blog/linux/0021-ipc-using-dbus/

[dbus_webpage]: https://www.freedesktop.org/wiki/Software/dbus/
[libdbus_api]: https://dbus.freedesktop.org/doc/api/html/group__DBus.html
[dbus_specification]: https://dbus.freedesktop.org/doc/dbus-specification.html


