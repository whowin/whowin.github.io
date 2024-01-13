---
title: "在ubuntu上的18个非常实用的命令行工具软件"
date: 2024-01-11T16:43:29+08:00
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

# - [USEFUL TOOLS THAT YOU CAN FIND ON ANY LINUX DISTRIBUTION](https://devarea.com/useful-tools-that-you-can-find-on-any-linux-distribution/)
# - https://devarea.com/category/linux/

postid: 100010
---

使用Ubuntu的过程中，在终端上使用命令行工具是非常常见的事情，熟练地掌握命令行工具是使用ubuntu必不可少的技能，即便是Ubuntu的初学者，通常也很熟悉诸如`ls`、`rm`、`cp`等一些文件操作工具，当浏览`/bin`目录时，你会发现Ubuntu还有许多工具软件，本文将向读者简单介绍18个在Ubuntu上使用的命令行工具软件，本文不会详细介绍每个命令的用法，有对某个命令感兴趣的读者可以自行查找更详细的资料或者使用man在线手册，本文非常适合初学者阅读。

<!--more-->

------
## 1 `find` - 在文件系统中查找所需的文件
* 在当前目录及其子目录下查找 C 语言的源程序：
    ```
    $ find . -name *.c
    ```
* 在 `/var` 目录及其子目录下查找文件大小大于 `100kb` 的日志文件：
    ```
    $ sudo find /var -name "*.log" -size +100k
    ```
* 在当前目录及其子目录下查找大于 2M 的临时文件，如果其所属用户不是 `developer`，则将其删除：
    ```
    $ find . -name "*tmp" -size +2M ! -user developer -exec rm {} \;
    ```
* 要求在删除文件前确认：
    ```
    $ find . -name "*tmp" -size +2M ! -user developer -ok rm {} \;
    ```

-------
## 2 `grep` - 在文件中查找字符串
* 在当前目录下的文本文件中，查找含有 "hello" 字符串的文件：
    ```
    $ grep "hello" *.txt
    ```
* 在当前目录及其子目录下的所有文件中，查找含有 "hello" 字符串的文件：
    ```
    $ grep -r "hello" ./
    ```
* 在当前目录及其子目录下的 C 语言源代码文件中，查找含有 "hello" 字符串的文件：
    ```
    $ grep -r "hello" ./ --include="*.c"
    ```
* 使用正则表达式查找文件中的字符串：
    ```
    $ grep -r "uid-[0-9]*" ./ --include="*.c"
    ```

-----
## 3 `cut` - 打印文件中指定的字段(列)
* 打印文件 `/etc/fstab` 文件中每行的前 12 个字符
    ```
    $ cut -c1-12 /etc/fstab
    ```
* 打印文件 `/etc/passwd` 中第 1、6、7 字段的内容，字段分隔符为 ":"
    ```
    $ cut -d: -f1,6,7 /etc/passwd
    ```

-----
## 4 `bc` - 命令行上的计算器
* 这是一个命令行下的交互式计算器，可以使用变量，还可以使用数学函数，`quit` 命令退出交互；
    ```
    $ bc -l
    bc 1.07.1
    Copyright 1991-1994, 1997, 1998, 2000, 2004, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
    This is free software with ABSOLUTELY NO WARRANTY.
    For details type `warranty'. 
    102+240*3.5
    942.0
    a=102
    b=240
    c=3.5
    a+b*c
    942.0
    b/c
    68.57142857142857142857
    sqrt(a)
    10.09950493836207795336
    quit
    $
    ```
* 也可以在脚本中使用：
    ```
    $ x=100
    $ y=20
    $ echo "$x/$y" | bc -l
    5.00000000000000000000
    $ echo "$x/$y" | bc
    5
    $
    ```

-----
## 5 `comm` - 比较两个已排序的文件
* 为了演示这个命令，先制作两个简单的文本文件：
    ```
    $ echo -e "avi\ndani\nrina\nzina">student1
    $ echo -e "avi\ndina\nmeni\nzina">student2
    ```
* 用 comm 比较这两个文件：
    ```
    $ cat student1
    avi
    dani
    rina
    zina
    $ cat student2
    avi
    dina
    meni
    zina
    $ comm student1 student2
                    avi
    dani
            dina
            meni
    rina
                    zina
    $
    ```
* 第 1 列是第 1 个文件的内容，第 2 列是第 2 个文件内容，第 3 列是两个文件中相同的内容；

-----
## 6 `diff` - 逐行比较两个文件
* 使用上面使用过的文件 student1 和 student2 进行比较：
    ```
    $ diff student1 student2
    2,3c2,3
    < dani
    < rina
    ---
    > dina
    > meni
    $
    ```
* 其中 `2,3c2,3` 的含义是：将第 1 个文件中的第 2、3 行改为第 2 个文件的第 2、3 行，两个文件就一样了；

-----
## 7 `df` - 文件系统的磁盘空间使用情况
    ```
    $ df
    Filesystem     1K-blocks    Used Available Use% Mounted on
    udev              211908       0    211908   0% /dev
    tmpfs              48496     712     47784   2% /run
    /dev/vda1        6105676 4262704   1522648  74% /
    tmpfs             242468       0    242468   0% /dev/shm
    tmpfs               5120       0      5120   0% /run/lock
    tmpfs             242468       0    242468   0% /sys/fs/cgroup
    tmpfs              48492       0     48492   0% /run/user/1000
    $    
    ```

-----
## 8 `du` - 估算文件系统的磁盘使用情况
* 使用这个命令通常都要加上参数 `-h`，这样显示的结果才比较清楚：
    ```
    $ du -h
    4.0K    ./.config/procps
    8.0K    ./.config
    28K     ./.mitmproxy
    4.0K    ./.cache
    22M     ./frp
    51M     .
    $
    ```
* 可以不显示子目录，仅显示汇总结果：
    ```
    $ du -h -s
    51M     .
    $
    ```
* 还可以指定子目录的深度，比如：
    ```
    $ du -h -d 1
    8.0K    ./.config
    28K	    ./.mitmproxy
    4.0K    ./.cache
    22M     ./frp
    51M     .
    $
    ```
* 与前面的 `du -h` 相比，这次没有显示目录 `./.config/procps`，这是因为 `-d 1` 指定了目录深度只有 1 层。

----
## 9 `file` - 查看文件类型
```
$ file mytest.c
mytest.c: C source, ASCII text, with CRLF line terminators
$
```
```
$ file /bin/cp
/bin/cp: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=421e1abd8faf1cb290df755a558377c5d7def3b1, for GNU/Linux 3.2.0, stripped
$
```
```
$ file /lib64/ld-linux-x86-64.so.2
/lib64/ld-linux-x86-64.so.2: symbolic link to /lib/x86_64-linux-gnu/ld-2.31.so
$
```

-----
## 10 `fold` - 打印文件内容时，在指定宽度位置添加"换行"
* 为演示 `fold` 命令，先制作一个文件：
    ```
    $ echo "id-2093384 id-8984773 id-8725536 id-9828835 id-6455351 id-9873773 ">data1
    $
    ```
* 执行下面命令看一下打印效果：
    ```
    $ cat data1
    id-2093384 id-8984773 id-8725536 id-9828835 id-6455351 id-9873773 
    $ fold -w 11 data1
    id-2093384 
    id-8984773 
    id-8725536 
    id-9828835 
    id-6455351 
    id-9873773 
    $
    ```

-----
## 11 `head/tail` - 显示文件的第一行/最后一行
* 显示 `/etc/passwd` 文件的最前面的 2 行：
    ```
    $ head -2 /etc/passwd
    root:x:0:0:root:/root:/bin/bash
    daemon:x:1:1:daemon:/usr/sbin:/usr/sbin/nologin
    $
    ```
* 显示 `/etc/passwd` 文件的最后 2 行：
    ```
    $ tail -2 /etc/passwd
    snap_daemon:x:584788:584788::/nonexistent:/usr/bin/false
    glances:x:128:135::/var/lib/glances:/usr/sbin/nologin
    $
    ```
* 显示 `/etc/passwd` 文件的前 50 个字符：
    ```
    $ head -c 50 /etc/passwd
    root:x:0:0:root:/root:/bin/bash
    daemon:x:1:1:daemo$
    ```

-----
## 12 `join` - 将两个具有公共字段的文件合并在一起
* 为了演示 `join` 命令，先制作 2 个文件：
    ```
    $ echo -e "avi haifa\ndani aco\nrina tel aviv\nzina ny">s1
    $ echo -e "avi 1002\ndani 2000\nrina 3000\nzina 4255">s2
    $
    ```
* 显示这两个文件
    ```
    $ cat s1
    avi haifa
    dani aco
    rina tel aviv
    zina ny
    $ cat s2
    avi 1002
    dani 2000
    rina 3000
    zina 4255
    $
    ```
* 可以看到这两个文件的第一列是一样的，这就是这两个文件的公共字段，这样的文件就可以使用 `join` 合并：
    ```
    $ join -j1 1 -j2 1 s1 s2
    avi haifa 1002
    dani aco 2000
    rina tel aviv 3000
    zina ny 4255
    $ 
    ```
* 其中 `-j1 1` 表示第一个文件的第一个字段，`-j2 1` 表示第二个文件的第一个字段。

-------
## 13 `od` - 以各种不同的格式显示文件
* 先制作一个文件
    ```
    $ echo -e "hello">a1
    ```
* 正常显示文件
    ```
    $ cat a1
    hello
    $
    ```
* 使用 `od` 按八进制和字符显示文件
    ```
    $ od -bc a1
    0000000 150 145 154 154 157 012
              h   e   l   l   o  \n
    0000006
    $
    ```
* 按 16 进制字和字符显示文件
    ```
    $ od -xc a1
    0000000    6568    6c6c    0a6f
              h   e   l   l   o  \n
    0000006
    $
    ```
* 按 16 进制字节和字符显示文件
    ```
    $ od -t x1c a1
    0000000  68  65  6c  6c  6f  0a
              h   e   l   l   o  \n
    0000006
    $
    ```

-----
## 14 `paste` - 合并文件行
* 制作一个新文件 s3
    ```
    $ echo -e "20003 france\n09388 uk\n20019 italy\n98377 spain">s3
    ```
* 使用 `paste` 命令将前面用过的文件 s2 和 s3 文件合并
    ```
    $ cat s2
    avi 1002
    dani 2000
    rina 3000
    zina 4255
    $ cat s3
    20003 france
    09388 uk
    20019 italy
    98377 spain
    $ paste s2 s3
    avi 1002	20003 france
    dani 2000	09388 uk
    rina 3000	20019 italy
    zina 4255	98377 spain
    $
    ```

-----
## 15 `sort` - 对文件进行排序
* 使用 s3 文件演示 `sort` 的作用：
    ```
    $ cat s3
    20003 france
    09388 uk
    20019 italy
    98377 spain
    $ sort s3
    09388 uk
    20003 france
    20019 italy
    98377 spain
    $
    ```

-----
## 16 `uniq` - 从一个已排序的文件中删除重复的行
* 以前面用过的 s2 文件演示 `uniq` 命令
    ```
    $ echo -e "dani 2000\nrina 3000">>s2
    $ cat s2
    avi 1002
    dani 2000
    rina 3000
    zina 4255
    dani 2000
    rina 3000
    $ sort s2>s4
    $ cat s4
    avi 1002
    dani 2000
    dani 2000
    rina 3000
    rina 3000
    zina 4255
    $ uniq s4
    avi 1002
    dani 2000
    rina 3000
    zina 4255
    $
    ```

-----
## 17 `split` - 将文件分割为多个部分
* 以 s2 文件为例，下面 `split` 命令的意思是：将 s2 文件按照 2 行一个文件分割为若干个文件，新文件名称以 `gen_` 开头
    ```
    $ cat s2
    avi 1002
    dani 2000
    rina 3000
    zina 4255
    dani 2000
    rina 3000
    $ split -2 s2 gen_
    $ cat gen_aa
    avi 1002
    dani 2000
    $ cat gen_ab
    rina 3000
    zina 4255
    $ cat gen_ac
    dani 2000
    rina 3000
    $
    ```
* 因为 s2 文件有 6 行，所以被分割成了 3 个文件，文件名分别是：`gen_aa`、`gen_ab` 和 `gen_ac`

-----
## 18 `wc` - 打印文件中的字符数、单词数和行数
* 默认情况下，打印出文件的行数、单词数和字符数：
    ```
    $ wc /etc/passwd
      51   90 3070 /etc/passwd
    ```
* 其中：`51` 是文件行数，`90` 是文件中的单词数，`3070` 是该文件的字符数；
* 可以使用参数 `-l` 表示行数，`-w` 表示单词数，`-c` 表示字符数；
    ```
    $ wc -l /etc/passwd
    51 /etc/passwd
    $ wc -c /etc/passwd
    3070 /etc/passwd
    $ wc -w /etc/passwd
    90 /etc/passwd
    ```
* 还可以这样用：
    ```
    $ ps aux | wc -l
    381
    ```
* 表示 `ps aux` 这个命令的输出有 381 行。






## **欢迎订阅 [『网络编程专栏』](https://blog.csdn.net/whowin/category_12180345.html)**
## **欢迎订阅 [『进程间通信专栏』](https://blog.csdn.net/whowin/category_12404164.html)**



-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


