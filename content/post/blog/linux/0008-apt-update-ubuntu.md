---
title: "apt update 如何工作"
date: 2022-07-10T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags: 
  - apt
  - ubuntu
  - linux
draft: false
#description: 该文章翻译自 [What does the apt update do on Ubuntu and Debian](https://linuxhint.com/apt-update-command/)
#references: 

postid: 100008
---

本文简单介绍了在 *ubuntu* 的命令行下运行 *apt update* 时，软件包管理工具 *apt* 所执行的实际动作，相信此文会让你对 *ubuntu* 的包管理系统有更好的了解
<!--more-->

## 1. 导言
  * 众所周知，*Linux* 有很多的发行版，*Ubuntu* 是其中之一，*Ubuntu* 使用一个软件包管理系统 *apt* 对软件进行维护，这些软件包包括硬件驱动、*Linux* 内核以及应用软件，所有这些软件包放在一个在线的服务器上，这个服务器被称作 **软件仓库**，使用 *apt* 命令行工具，可以访问软件仓库、建立本地软件仓库，并管理这些软件。本文将讨论在 *ubuntu* 上执行 *apt update* 命令所执行的具体操作。

## 2. 什么是 APT
  * *apt* 是一个在 *Ubuntu*、*Debian* 以及其他 *Debian* 衍生发行版上使用的强大的命令行工具，用于软件包的管理。
  * *APT* 是一个高级包管理工具，可以完成各种任务：
    - 新软件安装
    - 已安装包的升级
    - 更新软件包索引清单(本地软件仓库)
    - 甚至可以使用 *apt* 命令升级 *Ubuntu* 系统

## 3. 软件仓库和软件源
  * 前面说过，存储 *ubuntu* 软件包的在线服务器就是软件仓库；*Ubuntu* 会在本地磁盘中存放一个软件仓库中的软件包清单，这个清单又称为**本地软件仓库**；下面是一些软件仓库的地址：
    - http://cn.archive.ubuntu.com/ubuntu/ 
    - http://mirrors.aliyun.com/ubuntu/
    - https://mirrors.163.com/ubuntu/

  * 在一个 *ubuntu* 软件仓库中实际存储着各种版本的 *ubuntu* 发行版的软件包，还包括软件包的源代码；但是，我们实际正在使用的 *ubuntu* 其实只是其中的一个版本而已，而且大多数情况下我们也并不需要这些软件包的源代码，所以实际我们只是需要软件仓库中一部分的资源；在文件 */etc/apt/sources.list* 中定义了我们需要软件仓库中的哪些资源，其中的每一行定义了一种资源，我们把每一个定义又称为**软件源**
  * 除了在 */etc/apt/sources.list* 定义了软件源外，*/etc/apt/sources.list.d* 目录下的文件中也会定义一些第三方的软件源或者叫 *PPA(Personal Package Archives)*

## 4. apt update 是什么
  * ***apt update*** 命令从软件仓库获取最新版的所需的软件包清单信息，并以此更新本地软件仓库
    ```
    sudo apt update
    ```

    ![apt update][img01]
    - **图1：执行 apt update**

  * 运行 *apt update* 需要使用 *root* 用户登录，或者有 *sudo* 权限

## 5. ubuntu 下执行 apt update 实际执行的操作
  * *apt update* 命令会从所有软件源中下载最新的软件包信息，包括软件包的名称、版本信息和依赖关系等，并更新本地软件仓库；
  * *apt upgrade* 这类的命令只会从本地软件仓库中获取信息，并不会直接从软件仓库中下载软件包信息，所以在更新软件前如果不更新本地软件仓库，有可能 *apt upgrade* 命令并不会把软件升级到最新版本。
  * 在执行 *apt update* 时，针对每个软件源，会首先从软件仓库中下载一个 *InRelease* 文件，并通过电子签名验证其正确性，该文件中包含有所有软件包索引文件的 *hash* 校验值
  * 然后 *apt* 会下载每一类软件包的软件包索引文件 *Packages*，下载前，*apt* 会首先检查本地软件仓库中是否存在准备下载的文件，如果有则检查其 *Hash* 校验值并和 *InRelease* 中给出的值做比较，如果一致则不再下载该文件，否则从软件仓库中下载该文件
  * 计算从软件仓库中下载文件的 *hash* 校验值，并使用 *InRelease* 文件中给出的该文件的 *hash* 校验值验证其正确性，使用经验证正确的文件更新本地软件仓库
  * *InRelease*、*Packages* 和一些其它文件(比如国际版本用的 *Translation* 文件)组成了本地软件仓库
  * *apt update* 只会更新本地的软件包清单(本地软件仓库)，并不会升级任何软件，所以，运行 *apt update* 是绝对安全的，不会对你现有系统和应用软件做任何修改；即便是执行 *apt update* 时意外中断也不会出现问题，只需要再次执行即可
  * 在 *ubuntu* 系统中，本地软件清单(本地软件仓库)存放在 */var/lib/apt/lists* 这个目录下，实际上，如果你删除这个目录下的所有文件(也就是清空了本地软件仓库)，运行 *apt update* 后会重建你的本地软件仓库

## 6. 执行 apt update 时 Hit、Ign、Err和Get分别代表什么
  * 执行 *apt update* 更新本地软件仓库时，在终端上显示的信息中，每行的开头会有一个关键字(参见图1)，它们的含义如下：
    1. **Hit(命中)：**
        > *apt* 发现某个 *Packages* 文件(软件清单索引文件)的 *Hash* 校验值与最新的 *InRelease* 文件中给出的 *Hash* 校验值一致，所以无需再下载
    2. **Ign(忽略)：**
        > *apt* 在下载一个文件时出错，但是，这个文件并不重要，所以这个错误被忽略掉，*apt* 会继续下一个动作
    3. **Err(错误)：**
        > *apt* 在下载一个文件时发现了一个严重错误，无法再继续执行命令，比如在验证 *InRelease* 文件的电子签名时找不到所需的公钥时就会出现这个错误
    4. **Get(获取)：**
        > *apt* 已经从软件仓库下载了一个正确的软件包索引文件，并且更新了本地软件仓库

## 7. 结语
  * *Ubuntu* 上的 *apt update* 命令会从软件仓库上更新本地软件仓库，本文描述了其基本过程
  * *apt update* 仅会更新本地的软件包索引文件(本地软件仓库)，不会实际更新或升级任何软件，运行是安全的
  * *apt update* 会保证本地软件仓库为最新版，所以在准备更新软件前先运行 *apt update* 是非常必要的
  * *apt update* 的运行依赖在 *Ubuntu* 上配置的软件源文件 */etc/apt/sources.list*
  * 本地软件仓库存放在 *Ubuntu* 上的 */var/lib/apt/lists/* 目录下

<!--实际使用-->
[img01]:/images/100008/apt-update-screen.png

<!--编辑时使用-->
<!--
[img01]:../../../../static/images/100008/apt-update-screen.png
-->