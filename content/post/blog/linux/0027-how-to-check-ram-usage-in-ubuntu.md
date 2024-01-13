---
title: "在ubuntu上检查内存使用情况的九种方法"
date: 2024-01-09T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags:
  - Linux
  - ubuntu
draft: false
#references: 

# - [How to Check RAM Usage on Ubuntu](https://webhostinggeeks.com/howto/how-to-check-ram-on-ubuntu/)

postid: 100027
---

在 Ubuntu 中，可以通过 GUI(图形用户界面)和命令行使用多种方法来监视系统的内存使用情况，监视 Ubuntu 服务器上的内存使用情况并不复杂；了解已使用和可用的内存量对于故障排除和优化服务器性能至关重要，因为内存对系统 I/O 速度至关重要，定期监控内存使用情况有助于诊断潜在的系统问题和优化服务器性能，还可以帮助使用者确定是否需要扩充内存；本文将简要描述在 Ubuntu 上使用命令和工具监视内存使用情况的各种方法。

<!--more-->

------
## 方法 1：使用 `free` 命令查看内存
* `free` 命令显示系统中可用和已使用的内存量，要使用此命令，请打开终端并键入以下命令：
    ```
    free
    ```
* 输出将显示内存总量、已用内存、可用内存和共享内存，输出还将显示缓冲区和缓存内存；
    ```
    whowin@vm448813:~$ free
                  total        used        free      shared  buff/cache   available
    Mem:         484936      108308       10336         248      366292      366360
    Swap:        239696       25588      214108

    ```
------
## 方法 2：使用 `top` 命令查看内存
* `top` 命令显示系统进程及其资源使用情况，包括内存使用情况，在终端上键入 `top` 即可启动该命令；
    ```
    top
    ```
* 输出将显示系统上运行的进程列表，包括它们的 PID、用户、CPU 使用情况和内存使用情况，内存使用情况(MiB Mem)和交换内存使用情况(MiB Swap)的单位为 MiB 或者 KiB，1 MiB 为 1024<sup>2</sup> bytes，1 KiB 为 1024 bytes；
    ```
    top - 23:07:33 up 144 days, 11:01,  1 user,  load average: 0.20, 0.07, 0.01
    Tasks:  85 total,   1 running,  84 sleeping,   0 stopped,   0 zombie
    %Cpu(s):  4.3 us,  3.7 sy,  0.0 ni, 87.7 id,  0.0 wa,  0.0 hi,  1.7 si,  2.7 st
    MiB Mem :    473.6 total,     32.7 free,     92.4 used,    348.5 buff/cache
    MiB Swap:    234.1 total,    209.1 free,     25.0 used.    370.6 avail Mem 

        PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
         10 root      20   0       0      0      0 I   0.7   0.0 871:36.89 rcu_sched
          1 root      20   0  168804   6408   4172 S   0.3   1.3  27:35.83 systemd
          9 root      20   0       0      0      0 S   0.3   0.0 178:24.79 ksoftirqd/0
      15485 root      20   0    6448    996    780 S   0.3   0.2 242:24.27 qemu-ga
      20887 root      20   0   12184   2264   2092 S   0.3   0.5  32:42.65 sshd
      21836 root      19  -1  195616 114896 114128 S   0.3  23.7 109:38.57 systemd-journal
    1877591 root      20   0       0      0      0 I   0.3   0.0   0:21.95 kworker/0:1-events
          2 root      20   0       0      0      0 S   0.0   0.0   0:09.50 kthreadd
          3 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 rcu_gp
          4 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 rcu_par_gp
          6 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker/0:0H-kblockd
          8 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 mm_percpu_wq
         11 root      rt   0       0      0      0 S   0.0   0.0   1:00.36 migration/0
         12 root     -51   0       0      0      0 S   0.0   0.0   0:00.00 idle_inject/0
         14 root      20   0       0      0      0 S   0.0   0.0   0:00.00 cpuhp/0
         15 root      20   0       0      0      0 S   0.0   0.0   0:00.00 kdevtmpfs
         16 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 netns
         17 root      20   0       0      0      0 S   0.0   0.0   0:00.00 rcu_tasks_kthre
         18 root      20   0       0      0      0 S   0.0   0.0   0:00.00 kauditd
         19 root      20   0       0      0      0 S   0.0   0.0   0:37.21 khungtaskd
         20 root      20   0       0      0      0 S   0.0   0.0   0:00.00 oom_reaper
         21 root       0 -20       0      0      0 I   0.0   0.0   0:00.12 writeback
         22 root      20   0       0      0      0 S   0.0   0.0   0:01.04 kcompactd0
         23 root      25   5       0      0      0 S   0.0   0.0   0:00.00 ksmd
         69 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kintegrityd
         70 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kblockd
    ```

---------
## 方法 3：使用 `htop` 命令查看内存 
* `htop` 命令是 `top` 命令的增强版本，它以更加人性化的方式显示系统进程及其资源使用情况；
* 使用如下命令安装 `htop`：
    ```
    sudo apt install htop
    ```
* 安装好后在终端上键入 `htop` 即可启动；
* 输出将显示系统上运行的进程列表，包括它们的 PID、用户、CPU 使用情况和内存使用情况；

    ![Screenshot of htop][img01]

------
## 方法 4. 使用 `vmstat` 命令查看内存
* `vmstat` 是一个报告虚拟内存统计信息的工具，它提供有关进程、内存、分页、块 I/O、陷阱和 CPU 活动的信息；
* 在终端上键入 `vmstat` 即可使用；
* 查看 "swpd"(已使用的交换区)和 "free"(可用内存)列，了解有关内存使用情况的详细信息；
    ```
    whowin@vm448813:~$ vmstat
    procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
     r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
     0  0  25844  18916  69112 302020    0    0     2    14    1    3  0  1 96  0  3
    ```

------
## 方法 5. 通过文件 `/proc/meminfo` 查看内存
* `/proc/meminfo` 文件包含有关系统内存使用情况的详细信息，使用下面命令查看其内容：
    ```
    cat /proc/meminfo
    ```
* 该命令显示系统内存的详细分类，包括总内存、可用内存、已用内存和其他与内存相关的统计信息；
    ```
    whowin@whowin-ThinkPad-T14s:~$ cat /proc/meminfo
    MemTotal:       15090452 kB
    MemFree:         7713808 kB
    MemAvailable:   11334744 kB
    Buffers:          155260 kB
    Cached:          3976592 kB
    SwapCached:            0 kB
    Active:          1509760 kB
    Inactive:        5128712 kB
    Active(anon):       3864 kB
    Inactive(anon):  2842564 kB
    Active(file):    1505896 kB
    Inactive(file):  2286148 kB
    Unevictable:           0 kB
    Mlocked:               0 kB
    SwapTotal:       2097148 kB
    SwapFree:        2097148 kB
    Dirty:                16 kB
    Writeback:             0 kB
    AnonPages:       2506628 kB
    Mapped:          1368216 kB
    Shmem:            339912 kB
    KReclaimable:     163640 kB
    Slab:             349152 kB
    SReclaimable:     163640 kB
    SUnreclaim:       185512 kB
    KernelStack:       22048 kB
    PageTables:        52072 kB
    NFS_Unstable:          0 kB
    Bounce:                0 kB
    WritebackTmp:          0 kB
    CommitLimit:     9642372 kB
    Committed_AS:   14348500 kB
    VmallocTotal:   34359738367 kB
    VmallocUsed:       59772 kB
    VmallocChunk:          0 kB
    Percpu:            17856 kB
    HardwareCorrupted:     0 kB
    AnonHugePages:         0 kB
    ShmemHugePages:        0 kB
    ShmemPmdMapped:        0 kB
    FileHugePages:         0 kB
    FilePmdMapped:         0 kB
    HugePages_Total:       0
    HugePages_Free:        0
    HugePages_Rsvd:        0
    HugePages_Surp:        0
    Hugepagesize:       2048 kB
    Hugetlb:               0 kB
    DirectMap4k:      545488 kB
    DirectMap2M:     9693184 kB
    DirectMap1G:     5242880 kB
    ```

-------
## 方法 6：使用系统监视器查看内存
* 系统监视器是一个图形工具，显示系统进程和资源使用情况；
* 在桌面上点击 "应用程序" 菜单，再单击 "工具"，然后选择 "系统监视器"；

    ![Screenshot of application and utilities][img02]

    ![Screenshot of system monitor][img03]

* 在系统监视器中，点击 "资源" 选项卡，可以看到内存使用情况和其他资源使用信息；

    ![screenshot-of-resources][img04]

--------
## 方法 7. 使用 `glances` 查看内存
* `glances` 是一种先进的系统监控工具，可提供各种系统资源(包括内存)的全面信息；
* 用下面命令安装 `glances`：
    ```
    sudo apt install glances
    ```
* 在终端上键入 `glances` 即可启动；

    ![Screenshot of glances][img05]

-----
## 方法 8. 使用 `nmon` 查看内存
* `nmon` 是另一个系统监视工具，它提供有关各种系统资源(包括内存)的信息；
* 使用下面命令安装 `nmon`：
    ```
    sudo apt install nmon
    ```
* 安装完成后，在终端键入 `nmon` 即可启动；

    ![Screenshot of nmon][img06]

* 启动会，按 "m" 可查看内存使用情况；

    ![Screenshot of nmon][img07]

-------
## 方法 9. 使用 `smem` 查看内存
* `smem` 提供内存使用情况报告，它能够更准确地表示应用程序和进程正在使用的物理内存； 
* 使用下面命令安装 `smem`：
    ```
    sudo apt install smem
    ```
* 运行 `smem`：
    ```
    whowin@whowin-ThinkPad-T14s:~$ smem
      PID User     Command                         Swap      USS      PSS      RSS 
     2383 whowin   /usr/bin/fcitx-dbus-watcher        0      224      246     1412 
     2436 whowin   /usr/libexec/gnome-session-        0      480      528     5300 
     2623 whowin   /usr/libexec/gsd-screensave        0      728      789     6424 
     2535 whowin   /usr/libexec/xdg-permission        0      744      801     6388 
     2379 whowin   /usr/bin/dbus-daemon --sysl        0      684      802     3796 
     2262 whowin   /usr/lib/gdm3/gdm-x-session        0      760      815     6616 
     2410 whowin   /usr/bin/dbus-daemon --conf        0      692      818     4512 
     2601 whowin   /usr/libexec/gsd-a11y-setti        0      772      848     7052 
     3532 whowin   /snap/chromium/2724/usr/lib        0      304      867     3096 
    ......
    ```
* USS(Unique Set Size)：唯一集大小，即进程独自占用物理内存，只计算进程独自占用的物理内存大小，不包含任何共享的部分；
* PSS(Proportion Set Size)：比例集大小，使用某个共享内存的所有进程均分该共享内存的大小，加上该进程独自占用的内存(USS)，即为比例集的大小；
* RSS(Resident Set Size)：驻留集大小，即进程所使用的非交换区的物理内存的大小，该进程独占内存(USS)，加上该进程使用的共享内存大小(非均分共享内存)，即为驻留集大小。




## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**
## **欢迎订阅 [『进程间通信专栏』](https://blog.csdn.net/whowin/category_12404164.html)**



-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png

[img01]: https://whowin.gitee.io/images/100027/screenshot-of-htop.png
[img02]: https://whowin.gitee.io/images/100027/screenshot-of-application-menu.png
[img03]: https://whowin.gitee.io/images/100027/screenshot-of-system-monitor.png
[img04]: https://whowin.gitee.io/images/100027/screenshot-of-resources.png
[img05]: https://whowin.gitee.io/images/100027/sreenshot-of-glances.png
[img06]: https://whowin.gitee.io/images/100027/screenshot-of-nmon-1.png
[img07]: https://whowin.gitee.io/images/100027/screenshot-of-nmon-2.png

