---
title: "在vitual box虚拟机上安装dos"
date: 2013-04-23T14:27:38+08:00
author: whowin
sidebar: false
authorbox: false
pager: true
toc: true
categories:
  - "DOS"
tags:
  - "DOS"
  - "Ubuntu"
  - "Virtual Box"
draft: false
postid: 160014
---

现在找一台运行DOS的机器可是太不容易了，如果想玩DOS，只有在虚拟机上安装了，本文介绍如何在Virtual Box虚拟机上安装dos 6.22，本文的实例是在Ubuntu下，在windows基本也是一样的。本文适合初学者阅读。
<!--more-->

## 1. 前言
> 因为最近收到一封email，内容是关于我2008年写的一篇关于DOS的文章，这使我想要在博客上恢复2008年的那些关于DOS的文章，但是现在基本找不到安装DOS的机器了，所以想要写这篇文件，介绍一下如何在虚拟机上安装DOS

* 本文的安装过程，以及所有截图均在ubuntu 20.04下完成；
* 假设你的ubuntu下已经安装了virtual Box，如果没有安装，可以按下面方法安装：
    ```
    sudo apt install virtualbox
    ```

## 2. 下载DOS的镜像
* 我们要安装的DOS版本是6.22，应该是纯DOS的最后一个版本，后面的DOS版本都是搭载在windows下的；
* 我们需要一个DOS 6.22的软盘镜像，在虚拟机上挂载成一个可以启动的软盘
* 恐怕很难找到这样的镜像了，所以只能用我提供的了
    - [dos 6.22启动软盘镜像]
* 从上面的连接上下载文件dos622.img，将其放在某个文件夹下，比如：```~/dos/``` 下，备用。

## 3. 在Virtual Box上安装DOS
* 启动Virtual Box，点击"新建"建立一个新的虚拟电脑
    ![Start Virtual /box][img01]

* 选择"类型"为"Other"，"名称"可以随便填，这里填"dos622"，"版本"选"DOS"
    ![Set Virtual Computer name and type][img02]

* 内存按默认即可
    ![Set memory as default][img03]

* 硬盘大小按照默认即可
    ![Set virtual harddisk as default][img04]

* 硬盘类型按照默认即可
    ![Set virtual disk type as default][img05]

* 建议设定虚拟硬盘为"固定大小"，当然也可以选择"动态分配"
    ![Set virtual disk size as fixed value][img06]

* 文件位置和大小按照默认即可，点击"创建"，创建虚拟电脑
    ![Set file position and size as default][img07]

* 虚拟电脑创建完毕
    ![Virtual computer creation complete][img08]

* 点击"设置"，设置虚拟电脑参数
    ![Setting virtual computer][img09]

* 按图示步骤，设置虚拟软盘
    ![Storage Settings][img10]

* 点击"注册"找到在上一节下载的文件dos622.img，然后点击"选择"
    ![Choice soft disk][img11]

* 至此，已经完成了一个可以从软盘启动dos的虚拟电脑，点击"启动"，启动dos
    ![Start DOS from soft disk][img12]

* 遇到启动菜单时直接回车，则进入DOS提示符
    ![Start for the first time][img13]

* 软盘的盘符是A:，硬盘的盘符是C:，现在的C盘因为还没有分区和格式化，所以还没法用，我们可以用下面的命令试一下，会收到错误信息的
    ```
    A:\>c:

    C:\>dir
    ```
* 按下面图示步骤为硬盘分区，命令：```fdisk```
    ![Create dos partition][img14]

    ![Create primary dos partition][img15]

    ![Set partition size][img16]

    ![Reboot DOS][img17]

* 格式化硬盘，命令：```format c:/s```
    ![Format hard disk][img18]

* 至此，硬盘已经可以使用了，我们首先要把整个DOS的基本文件从软盘拷贝到硬盘上
    ```
    A:\>xcopy *.* c:\*.* /s
    ```
* 现在你的硬盘上就安装好一个完整的dos6.22了，你可以从软盘中取出软盘，然后从硬盘启动dos
    1. 首先退出DOS，退出dos使用"强制退出"即可
        ![Exit from DOS][img19]

    2. 从软驱中取出软盘，在下图的dos622.img上鼠标点右键，选择"删除盘片"
        ![Remove soft disk from drive][img20]

    3. 再次启动dos，将从硬盘启动
        ![Boot DOS from hard disk][img21]

* 至此已经在虚拟机Virtual Box上安装好了一个完整的DOS.






-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[software01]:/software/dos622.img

[img01]:/images/160014/start-vbox.png
[img02]:/images/160014/set-virtual-computer-name-type.png
[img03]:/images/160014/set-memory-as-default.png
[img04]:/images/160014/set-virtual-harddisk-as-default.png
[img05]:/images/160014/set-virtual-disk-type-as-default.png
[img06]:/images/160014/set-disk-size-as-fixed-value.png
[img07]:/images/160014/set-file-position-size-as-default.png
[img08]:/images/160014/complete-virtual-computer.png
[img09]:/images/160014/virtual-computer-setting.png
[img10]:/images/160014/storage-settings.png
[img11]:/images/160014/choice-softdisk.png
[img12]:/images/160014/start-dos-from-softdisk.png
[img13]:/images/160014/dos-first-start.png
[img14]:/images/160014/fdisk-create-dos-partition.png
[img15]:/images/160014/fdisk-create-primary-dos-partition.png
[img16]:/images/160014/fdisk-partition-size.png
[img17]:/images/160014/fdisk-reboot-dos.png
[img18]:/images/160014/format-hard-disk.png
[img19]:/images/160014/exit-from-dos.png
[img20]:/images/160014/remove-softdisk-from-drive.png
[img21]:/images/160014/boot-dos-from-harddisk.png



