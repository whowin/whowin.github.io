---
title: "bash下变量PS1的完整理解"
date: 2022-04-18T13:30:29+08:00
author: 华松青
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
  - "Shell"
  - "Bash"
tags:
  - "shell"
  - "bash"
  - "PS1"
  - "PS2"
  - "PS3"
  - "PS4"
  - "环境变量"
  - "debian_chroot"
draft: false
#references: 
#  - [Why env does not print PS1 variable?](https://stackoverflow.com/questions/41931855/why-env-does-not-print-ps1-variable)
#  - [How to turn off the color in my $PS1?](https://unix.stackexchange.com/questions/669664/how-to-turn-off-the-color-in-my-ps1)
#  - [What does "${debian_chroot:+($debian_chroot)}" do in my terminal prompt?](https://askubuntu.com/questions/372849/what-does-debian-chrootdebian-chroot-do-in-my-terminal-prompt#:~:text=The%20file%20%2Fetc%2Fdebian_chroot%20is%20when%20you%20have%20a,whether%20you%20are%20in%20the%20chroot%20or%20not.)
#  - [What does an if statement with only a variable evaluate to in bash?](https://stackoverflow.com/questions/54699956/what-does-an-if-statement-with-only-a-variable-evaluate-to-in-bash)
#  - [6.3. Xterm Title Bar Manipulations](https://tldp.org/HOWTO/Bash-Prompt-HOWTO/xterm-title-bar-manipulations.html)
#  - [Environment variable vs Shell variable, what's the difference?](https://askubuntu.com/questions/26318/environment-variable-vs-shell-variable-whats-the-difference)
#  - [Understand this .bashrc script](https://stackoverflow.com/questions/16410740/understand-this-bashrc-script-curly-braces-eval/16411133#16411133)
#  - [What is $debian_chroot in .bashrc?](https://unix.stackexchange.com/questions/3171/what-is-debian-chroot-in-bashrc/3174#3174)
#  - [Environment variable definition From Wikipedia](https://en.wikipedia.org/wiki/Environment_variable)
#  - [中文维基百科中环境变量的定义](https://zh.wikipedia.org/wiki/%E7%8E%AF%E5%A2%83%E5%8F%98%E9%87%8F)
#  - [Shell Variables and Environment Variables](https://web.archive.org/web/20170713001430/http://sc.tamu.edu/help/general/unix/vars.html)
#  - [GNU Coreutils](https://www.gnu.org/software/coreutils/manual/html_node/)
#  - [chroot](https://wiki.debian.org/chroot)
#  - [建立 debian 的 chroot 環境](https://datahunter.org/chroot)
#  - [chroot环境搭建](https://www.cnblogs.com/abnk/p/15127456.html)
postid: 100004
---


本文并不会讲解如何设置PS1以获得你喜欢的提示符；本文会围绕PS1这个变量，就其涉及到的一些概念展开讨论
<!--more-->

## 导言
  * ubuntu 的默认 shell 是 **bash**，bash 下有个变量 **PS1**，我们在 linux 的终端上用这个变量来设置命令行界面的提示符，这是一件很平常的事情
  * 很多很多文章在提到 **PS1** 时都称其为环境变量，但是有一天我使用 *env* 显示环境变量时却发现并没有 PS1，用 *printenv* 也是一样，认真分析了一下 PS1，觉着可以写一篇文章
  * 关于 Linux 下设置 PS1 的文章可以找到很多，但大多数毫无新意，讲一个浮于表面的设置方法而已，其实围绕这个变量可以有很多可谈的话题。本文不敢讲深入，力求能够全面第帮助读者理解这个变量，进而能够对 Linux 的强大和灵活有更深的认识。
  * 本文中如无特别说明，shell 指的是 **bash**，所有范例在 ubuntu 16.04 下完成，在更高版本的 ubuntu 上，可能会有些微区别；在较低版本的 ubuntu 上不能保证有相同的效果

## 如何在线获得帮助
  * 我个人在线获得权威帮助的途径通常有下面几个：
    1. man 手册，在 Linux 下工作的程序员都应该熟悉
    2. help，在命令行下使用 help 可以查到 shell 的内建命令
    3. info 查看一些 info 文档，比如: info coreutils
    4. [gnu coreutils的在线帮助文档](https://www.gnu.org/software/coreutils/manual/html_node/index.html)，因为大部分的 shell 外部命令都来自 gnu coreutils
    5. [中文维基百科](https://zh.wikipedia.org/)，[wikipedia](https://en.wikipedia.org)；维基百科上对很多概念的定义是比较权威的
    6. [ubuntu官方文档(ubuntu Official Documentation)](https://help.ubuntu.com/)
    7. [ubuntu Community Help Wiki](https://help.ubuntu.com/community/CommunityHelpWiki)
  * *man PS1* 是找不到关于PS1的说明的
  * *man bash* --> 查找 PS1 或者 PROMPTING --> 下一个... --> 找到 PROMPTING 条目；不过在 *man bash* 中仅介绍了 PS1 的控制码，本文**附录1**中的控制码就是从 man 手册中获得的
  * 顺便提一句，在 man 手册中查找字符串和在 vim 下查找字符串差不多
  * *info bash* 可以获得与 *man bash* 相同的内容

## PS1 不是 shell 的环境变量
  * 如何显示环境变量？
    - 实际上 bash 专门用来显示环境变量的命令是 *printenv*，但是很多文章中使用 *env* 来显示环境变量，当然也没有错，但是 Linux 下这两个命令的设计初衷确实完全不同的，让我们来分别看一下相应的 man 手册

      ![env命令man手册][img01_man_env_1]

      **图1：env命令的man手册**

    - *env* 这个命令主要是用来执行命令的，它可以在一个指定的环境中(意为不是当前的环境)去执行一个命令；比如当前登录用户为 demouser，那么肯定环境变量 USER=demouser，我们可以临时将 USER 改为 testuser 去运行程序；
      ```
      env USER=testuser <你要运行的程序名>
      ```
    - 下面这个例子首先用 *printenv USER* 显示当前环境变量 USER 的值，为 demouser，然后我们用 *env* 来执行这个命令，在执行前将环境变量临时改为 testuser，这时在 *env* 命令下执行的 *printenv USER* 显示的结果就变成了 testuser；命令退出后再次执行 *printenv USER*，可以看到显示的还是 demouser
      ```
      demouser@ubuntu:~$ printenv USER
      demouser
      demouser@ubuntu:~$ env USER=testuser printenv USER
      testuser
      demouser@ubuntu:~$ printenv USER
      demouser
      demouser@ubuntu:~$ 
      ```
    - 但是 *env* 这个命令确实可以显示出所有的环境变量来，在 man 手册的后面有这样一行字

      ![env命令man手册][img02_man_env_2]

      **图2：env命令的man手册**

    - 但是 *env* 命令并不能向 *printenv* 命令那样显示指定环境变量的值，比如下面例子
      ```
      demouser@ubuntu:~$ printenv USER
      demouser
      demouser@ubuntu:~$ env USER
      env: "USER": 没有那个文件或目录
      demouser@ubuntu:~$ 
      ``` 
    - 我个人并不反对使用 *env* 显示所有的环境变量，只是希望大家不要忘记另一个命令：*printenv*
    - 我们使用命令 *printenv PS1*(或者 *env | grep PS1*)，我们发现在环境变量里找不到这个 **PS1**，这并不是我们哪里做错了，这是因为 PS1 并不是一个环境变量
  * PS1 并不是一个环境变量(后面会慢慢解释)，否则 *printenv* 不可能打印不出来

## shell 变量
  * 有人可能说，你用 *set* 命令就可以显示出 PS1 了，是的，没错，但是 *set* 命令显示的是什么呢？我们来看手册，因为 *set* 是 bash 的内建命令，所以需要用 *help* 来显示它的手册

    ![set命令的help手册][img03_set_help]

    **图3：set命令的help手册**

  * *set* 的手册中强调，*set* 显示的是 shell 变量，而非环境变量，既然用 *set* 可以显示出 PS1，那 PS1 肯定是一个 shell 变量
  * shell 变量真的没有什么好解释的，在 shell 下建立的变量都是 shell 变量；环境变量也是 shell 变量，只不过和普通的 shell 变量有一点不同，下面会说到

## shell 的环境变量
  * 我理解环境变量有一个显著的特征，就是它会传递到子进程中去，所以环境变量不仅会对当前进程，也可能会对其建立的子进程产生影响；而shell变量是不会传递到子进程中去的，所以它仅对当前进程产生影响，无法影响其建立的子进程
  * 一个 shell 变量可以通过 *export* 命令成为环境变量，一个环境变量本身也是一个 shell 变量，仅仅是带有一个环境变量的标志而已，使用 *declare +x* 命令可以去掉这个环境变量的标志，使其不再是环境变量，而仅仅是一个 shell 变量
    - 下面这个过程演示了 PS1 这个变量的变化过程
      ```
      $ printenv PS1
      $ echo $PS1
      \[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]$
      $ export PS1
      $ printenv PS1
      \[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]$
      $ declare +x PS1
      $ printenv PS1
      $ echo $PS1
      \[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]$
      $
      ```
      + 起初 PS1 只是一个 shell 变量，不是环境变量，因为使用 *printenv* 无法显示这个变量，但是 *echo* 可以显示这个变量
      + 然后用 *export* 将 PS1 转换成环境变量，这时使用 *printenv* 就可以显示这个变量了
      + 最后使用 *declare +x* 将 PS1 在变成一个普通的 shell 变量，此时用 *printenv* 就又无法显示这个变量了，但是这个变量显然还存在(可以用 *echo $PS1* 显示出来)
    - 有人可能说用 *unset* 也可以把一个环境变量删除，不一定要用 *declare*；是的，*unset* 会让一个变量不复存在，而 *declare +x* 只是让一个变量不再是环境变量，但这个变量还依然存在
    - 小提示：*declare -x* 命令和 *export* 效果是一样的，也可以把一个变量转变成环境变量，你可以试试看
    - 关于环境变量和 shell 变量的一些概念，可以参考我的另外一篇文章[**《环境变量和 shell 变量》**][article01]

  * 很多很多的文章中都把 PS1 当做环境变量，其实这是不正确的；
  * 但是 PS1 是不是环境变量并不怎么重要，重要的是这其中的很多概念混淆了会给我们带来不少困惑和麻烦

## 理解当前的 PS1
* 先看一下我的 ubuntu 终端上看到的 PS1 是什么内容
  ```
  $ echo $PS1
  \[\e]0;\u@\h: \w\a\]${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$
  ```
  > 要看懂这一大串乱七八糟的东西先要了解 [《PS1 的控制码》][article02]以及 [《ANSI 的 ESCAPE 转义》][article03]，这两篇文章在附录里有链接，可以先看一下
* **CSI 序列的开始字符**
  - 在 [《ANSI 的 ESC 转义》][article03] 中提到，CSI 序列是由 "**ESC [**" 开始的，字符 "[" 我们都认识，ESC 是什么呢？
  - ESC 其实就是你键盘上左上角的 ESC 键产生的 ASCII 码，十进制为27，十六进制是0x1B，八进制是033，是不是在上面显示的 PS1 里面有好几个 033 呢？但是每个 033 前面都有一个斜杠 "\"，这个斜杠又是什么呢？
  - 在 [《PS1 的控制码》][article02] 中，看第 24 个控制码，**\nnn** 表示八进制数 nnn 表示的字符，所以可以肯定 **\033** 正是 **ESC**
  - 有没有发现，所有 **\033** 的后面都跟着一个字符 "**[**"，组成了 CSI 序列的开始字符串
  - 再来看 [《PS1 的控制码》][article02] 中的第 4 个控制符，"**\e**" 也是ASCII 033，也就是 ESC，所以 "**\e[**" 同样也是一个 CSI 的开始字符串
* **CSI 序列的结束字符**
  - 在 [《ANSI 的 ESC 转义》][article03] 中提到，CSI 序列分为四部分，开始字符(已经介绍过)、若干个参数字节，范围为 *0x30-0x3F(ASCII 0–9:;<=>?)*、若干个中间字节，范围为 *0x20-0x2F(ASCII 空格以及!"#$%&'()\*+,-./)*、单个结束字节，范围为 *0x40-0x7E(ASCII @A–Z[\]^_`a–z{|}~)*；其参数字节、中间字节和结束字节分属于不同的编码范围，互相并没有交集
  - 我们注意到，在所有 "\\033]" 后面都有一个 "**m**" 字符，而这个字符又恰恰符合结束字节的范围，可以断定这个 "**m**" 就是 CSI 序列的结束字符
* **关于 ```\[...\]```**
  - 除了 "\\033["、"\\e[" 这两个 CSI 开始字符串中有 " [ " 以外，整个 PS1 中还有很多 "```\[```" 和 "```\]```"，这些又是什么呢？
  - 再来看 [《PS1 的控制码》][article02] 中的第 26、27 控制符，包含在 "```\[```" 和 "```\]```" 之间的是不打印字符，也就是控制符
  - 我们理一下整个 PS1 中有多少组这种控制序列，从左向右找：
    + 第 1 组：**```\[\e]0;\u@\h: \w\a\]```**
    + 第 2 组：**```\[\033[01;32m\]```**
    + 第 3 组：**```\[\033[00m\]```**
    + 第 4 组：**```\[\033[01;34m\]```**"
    + 第 5 组：**```\[\033[00m\]```**"
    + 后面我们会按照这五组的顺序说明
* **终端窗口的标题**
  - 打开一个终端窗口(不论是桌面还是仿真终端远程登录)，窗口上都有一个标题

    ![终端窗口标题][img04_terminal_title]

    **图4：终端窗口标题**

  - 上面提到的第 1 组控制序列正是控制这个窗口标题的，我们称之为 **窗口标题控制序列**，这部分的资料非常少，我也只在 **wikipedia** 中找到了一句关于这组控制码的说明，具体出处请看我的另一篇文章[《ANSI 的 ESC 转义》][article03]，在最后一部分"OSC 序列"中有说明
  - 定义终端窗口标题的序列：**```\[\e]0;控制码\a\]```**；其中控制码部分可以使用文章 [《PS1 的控制码》][article02] 中的大部分控制码
  - 在定义窗口标题的第 1 组序列中，控制码 "**\\u**" 表示当前用户名(user)；"**\\h**" 表示当前主机名(hostname)；"**\\w**" 表示当前工作目录(work directory)；这些均在文章 [《PS1 的控制码》][article02] 中有说明，还有一个 "@" 字符会直接显示出来，看看你的窗口标题是不是和定义的一样
  - 改变一下终端标题：
    + 因为还没有讲到提示符的定义，我们暂且把提示符简单定义为："$ "，先试一下对窗口标题的修改
    + 下面例子将终端窗口标题改为：HELLO TITLE
      ```
      PS1="\[\e]0;HELLO TITLE\a\]$ "
      ```
      ![终端原标题][img05_terminal_original_title]

      **图5：终端窗口原有标题**

      ![终端新标题][img06_terminal_hello_title]

      **图6：终端窗口标题被改变**

    + 下面例子将标题改为：用户名@当前时间
      ```
      PS1="\[\e]0;\u@\t\a\]$ "
      ```

      ![终端新标题][img07_terminal_title_time]

      **图7：终端窗口标题被改变成窗口打开时间**

    + 我们注意到，窗口上显示的时间并不会变化，实际上这个时间只是窗口的打开时间
    + 这两个例子中的控制码都在文章 [《PS1 的控制码》][article02] 中有说明，读者可以根据说明中的控制码自行修改成你喜欢的窗口标题

* **提示符的设定**
  - **窗口标题控制序列**之后的所有序列均为设置提示符的控制序列
  - 在**窗口标题控制序列**之后紧跟着的字符串 **\${debian_chroot:+(\$debian_chroot)}** 我们暂且忽略不提(后面再讲)
  - 第 2 组到第 5 组的控制序列中，都是以 **```\[\033[```** 开头，以 **```m\]```** 结束的，从文章 [《ANSI 的 ESCAPE 转义》][article03] 中可以查到，请看该文中CSI序列的第14个控制序列 - **CSI n m**；其格式与第 2 - 5 组控制序列相符，所以这是在设置 SGR 参数
  - SGR 参数
    + 设置 SGR 参数在这里实际就是在设置字符的前景、背景色等一些属性
    + 设置 SGR 参数的控制序列 **CSI n m** 中，主要内容就在其中的 **n** 上，文章 [《ANSI 的 ESCAPE 转义》][article03] 中说的很清楚，"CSI后可以是0或者更多参数，用分号分隔。"
    + 先来看第 2 组控制序列 - **```\[\033[01;32m\]```**，其中的 *n* 就是 "01;32"，其含义仍然需要在文章 [《ANSI 的 ESCAPE 转义》][article03] 中查，在【SGR 参数】一节中可以查到，"1" - 表示粗体或高亮；"31-37" - 前景色，再查颜色表，可以查到，"32" - 绿色；所以第 2 组控制序列的含义就是：以加粗(高亮)绿色显示文字
    + 同样地，请自行研究第 3 - 5 组控制序列的含义
    + 第 3 组控制序列 - **```\[\033[00m\]```**，含义是去掉前面设置的属性，恢复到默认属性
    + 第 4 组控制序列 - **```\[\033[01;34m\]```**，含义是以加粗(高亮)蓝色显示文字
    + 第 5 组控制序列与第 3 组控制序列含义相同
  - 设定的提示符是什么样的
    + 我们把设定提示符的控制序列重新写在这里：**```\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]$ ```**
    + 如果我们把所有的控制序列都去掉的话变成这样：\\u@\\h:\\w\$
    + **```\u```**、**```\h```**、**```\w```** 在前面讲窗口标题时已经介绍过，所以，暂且不提字体颜色的话，我们设置的提示符是：用户名@主机名:<当前工作路径>$
    + 我们可以实际试一下
      ```
      PS1="\u@\h:\w$ "
      ```
    + 注意：$后面有一个空格，仅仅是为了美观一点

      ![设置PS1][img08_terminal_PS1_1]

      **图8：初次设置PS1**
    
    + 结合前面的分析，实际设置的提示符应该是：[ *绿色字* ]用户名@主机名[ *默认颜色* ]:[ *蓝色字* ]<当前工作路径>[ *默认颜色* ]$
    + 看一下实际情况

      ![提示符][img09_terminal_PS1_2]

      **图9：当前提示符实际效果**

* 关于 **\${debian_chroot:+(\$debian_chroot)}**，我们会在后面单独拿出来一节说明

## bash启动过程中 PS1 的设置过程
* bash的启动过程
  - 我们用 *info bash* (或者 *man bash*)查看 bash 的在线手册，可以找到 bash 启动时执行脚本的过程，在此我们仅讨论交互式 shell 的启动过程，其实，交互式 shell 还分为登录(interactive login shell)和非登录(non-interactive login shell)，这些都不在本文的讨论范围内
    
    ![交互式bash启动时执行脚本的过程][img10_man_bash_1]

    ![交互式bash启动时执行脚本的过程][img11_man_bash_2]

    ![交互式bash启动时执行脚本的过程][img12_man_bash_3]

    **图10：交互式bash启动时执行脚本的顺序**

  - 由此可见，在交互式登录 shell 时(interactive login shell)， bash 在启动时执行脚本的顺序为：
    1. **/etc/profile**
    2. **~/.bash_profile**
    3. **~/.bash_login**
    4. **~/.profile**
  - 在交互式非登录 shell 时(non-interactive login shell)， bash 在启动时执行脚本的顺序为：
    1. **/etc/bash.bashrc**
    2. **~/.bashrc**
  - 但是实际上，在交互式 shell 下，不论是登录还是非登录模式，bash 启动执行的脚本区别不大，我们先来看交互式登录 shell 启动时需要执行的第一个文件 **/et/profile**
    ```
    whowin@whowin-ubuntu:~$ cat -n /etc/profile
      1  # /etc/profile: system-wide .profile file for the Bourne shell (sh(1))
      2  # and Bourne compatible shells (bash(1), ksh(1), ash(1), ...).
      3  
      4  if [ "$PS1" ]; then
      5      if [ "$BASH" ] && [ "$BASH" != "/bin/sh" ]; then
      6          # The file bash.bashrc already sets the default PS1.
      7          # PS1='\h:\w\$ '
      8          if [ -f /etc/bash.bashrc ]; then
      9              . /etc/bash.bashrc
      10         fi
      11     else
      12        if [ "`id -u`" -eq 0 ]; then
      13            PS1='# '
      14        else
      15            PS1='$ '
      16        fi
      17     fi
      18 fi
      19  
      20 if [ -d /etc/profile.d ]; then
      21     for i in /etc/profile.d/*.sh; do
      22         if [ -r $i ]; then
      23             . $i
      24         fi
      25     done
      26     unset i
      27 fi
    whowin@whowin-ubuntu:~$ 
    ```
  - 很显然，正常情况下，/etc/profile 会去执行 **/etc/bash.bashrc**(第 9 行)，这个脚本正是交互式非登录 shell 启动时执行的第一个脚本
  - 在我的系统中，没有 **```~/.bash_profile```** 和 **```~/.bash_login```** 这两个文件，按照 bash 官方文档的说明，交互式非登录 shell 启动时在执行完 /etc/profile 后会去执行 ```~/.profile```，让我们来看看这个文件
    ```
    whowin@whowin-ubuntu:~$ cat -n ~/.profile
      1    # ~/.profile: executed by the command interpreter for login shells.
      2    # This file is not read by bash(1), if ~/.bash_profile or ~/.bash_login
      3    # exists.
      4    # see /usr/share/doc/bash/examples/startup-files for examples.
      5    # the files are located in the bash-doc package.
      6    
      7    # the default umask is set in /etc/profile; for setting the umask
      8    # for ssh logins, install and configure the libpam-umask package.
      9    #umask 022
      10    
      11   # if running bash
      12   if [ -n "$BASH_VERSION" ]; then
      13       # include .bashrc if it exists
      14       if [ -f "$HOME/.bashrc" ]; then
      15           . "$HOME/.bashrc"
      16       fi
      17   fi
      18    
      19   # set PATH so it includes user's private bin directories
      20   PATH="$HOME/bin:$PATH"
    whowin@whowin-ubuntu:~$ 
    ```
  - 很显然，正常情况下，会执行 **```~/.bashrc```**(第 15 行)，这个脚本正是交互式非登录 shell 启动时执行的第二个脚本
  - 至此，研究在 bash 启动过程中 PS1 变量的设置过程，只需研究 **/etc/bash.bashrc** 和 **```~/.bashrc```** 这两个脚本即可
  - **/etc/bash.bashrc**
    + 自行打印这个文件的源码，下面是我的文件中关于 PS1 部分的代码片段，为方便起见，打印时加了行号
      ```
      whowin@whowin-ubuntu:~$ cat -n /etc/bash.bashrc
      ......
      13  # set variable identifying the chroot you work in (used in the prompt below)
      14  if [ -z "${debian_chroot:-}" ] && [ -r /etc/debian_chroot ]; then
      15      debian_chroot=$(cat /etc/debian_chroot)
      16  fi
      17
      18  # set a fancy prompt (non-color, overwrite the one in /etc/profile)
      19  PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
      ......
      ```
    + 抛开 "\${debian_chroot:+(\$debian_chroot)}" 不管(下面会专门讲)，我们看到 PS1 在第 19 行被设置为 "\u@\h:\w\$ "，这其中的含义我们在前面已经介绍过
    > 因为下面还要执行一个 ```~/.bashrc``` 脚本，所以其实这里不用设置 PS1 ，只需要在后面执行的文件中设置即可，但我们要清楚，这个文件是放在 /etc 目录下的，需要有比较高权限的用户才可以修改，而 ```~/.bashrc``` 这个文件是放在用户目录的，用户自己是可以随便修改的，如果在这里没有设置 PS1，而用户自己又修改了 ```~/.bashrc``` 文件，导致在 ```~/.bashrc``` 中也没有设置 PS1 的话，PS1 这个变量会不存在，这就有些尴尬了，因为提示符没有了，你可以试一下在你的终端上执行 *unset PS1*，看看会发生什么
  - **```~/.bashrc```**
    + 自行打印这个文件的源码，下面是我的文件中关于 PS1 部分的代码片段，为方便起见，打印时加了行号
      ```
      whowin@whowin-ubuntu:~$ cat -n ~/.bashrc
      ......
      59  if [ "$color_prompt" = yes ]; then
      60      PS1='${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
      61  else
      62      PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
      63  fi
      64  unset color_prompt force_color_prompt
      65  
      66  # If this is an xterm set the title to user@host:dir
      67  case "$TERM" in
      68  xterm*|rxvt*)
      69      PS1="\[\e]0;${debian_chroot:+($debian_chroot)}\u@\h: \w\a\]$PS1"
      70      ;;
      71  *)
      72      ;;
      73  esac
      ......
      ```
    + 正常情况下，现在的终端都是支持彩色的，所以会执行第 60 行；而我们运行的终端，目前大多都是 xterm 标准的，所以会执行第 68 行
    + 与前面我们看到的本机的 PS1 是完全一致的，为方便起见，我们在这里再次打印本机的 PS1
      ```
      $ echo $PS1
      \[\e]0;\u@\h: \w\a\]${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$
      ```
## debian_chroot
* 要解释 **\${debian_chroot:+(\$debian_chroot)}** 不得不先简单说明一下 **chroot** 是个什么东西
* chroot
  - 中文维基百科 [关于chroot的解释](https://zh.wikipedia.org/wiki/Chroot)
  - 英文维基百科 [chroot](https://en.wikipedia.org/wiki/Chroot)
  > 正常的 linux 启动后根目录在 "/"，下面有一些其它必须的目录，比如：**/dev**、**/etc**、**/bin**、......等等，这个文件系统让你可以做很多操作，启动各种应用程序；但是 Linux 有一个非常有趣且重要的功能，就是改变根目录，比如，我们可以在 /home/ 目录下建立一个新目录 new_root，然后通过命令 *chroot /home/new_root* 把文件系统的根目录变成 /home/new_root/，执行成功后，当你执行 *ls /* 命令时，实际显示的是 /home/new_root/ 目录下的文件，当然，在 **chroot** 之前你必须在 /home/new_root/ 下建立一个与原文件系统类似的文件系统。比如有 dev 目录， bin 目录，lib 目录等，否则你会什么也干不了；当你 **chroot** 成功后，显然你将只能访问原来文件系统中 /home/new_root/ 目录及其子目录，原文件系统中的其它目录将无法访问；不知道是否解释清楚了 chroot
  - 那么，chroot 有什么用呢？维基百科中做了介绍，我个人觉着常用于下面三个方面
    + 软件测试
      > 我们用于开发的系统往往由于安装了一些软件而变得不那么干净，我们开发的应用程序在你用于开发的环境下可以正常运行并不表示在一个标准的干净的环境中也可以正常运行，这时，我们可以在某个目录下建立一个干净的文件系统，然后 chroot 过去，在这个干净的环境中进行测试
    + 软件打包
      > 开发的应用程序有时需要打包发布，打包时需要把这个应用程序所需的依赖都打到一起，但由于我们用于开发的系统并不干净，我们无法准确地判断出哪些库文件需要打包，为此我们可以建立一个 chroot 环境，环境中仅存放应用程序所需的依赖，在确保程序能够正常运行的情况下，可以很方便地打包
    + chroot jail
      > 有些文章翻译成 "chroot监狱"，我感觉有些别扭，就没有采用；这个东西的最典型的应用是 FTP；因为很多服务器使用 ftp 向用户提供资源，为了安全，当一个 guest 用户(或普通用户)登录 ftp 服务器时，被 chroot 到一个特定的目录下，无法访问服务器上的其他目录，这种限制方式被称为 **chroot jail**，比如著名的 vsftpd 这个 FTP 服务器端软件，在配置文件里有一个选项 **chroot_local_user**，当该选项为 "YES" 时，可以为每个用户指定其根目录 "/"，从而将用户关闭在这个 **chroot jail** 中
  - 讨论 chroot 超出了本文的内容
* bash 语法 **```${parameter:-word}```** 和 **```${parameter:+word}```**
  - 这两种不常用的语法在后面的讨论中会使用，在 *man bash*(或 *info bash*) 中，搜索 ```${parameter``` 字符串可以找到这类语法的说明
  - **```${parameter:-word}```**：如果 parameter 不存在或者为 null，则返回 word，否则，返回 parameter
  - **```${parameter:+word}```**：如果 parameter 存在且不为 null，则返回 word，否则返回一个空值(nothing)
* /etc/bash.bashrc 脚本对 debian_chroot 变量的操作
  - 请自行返回到前面查看 /etc/bash.bashrc 脚本的源代码
  - 第 14 - 16 行，如果变量 debian_chroot 不存在，文件 /etc/debian_chroot 存在且可读，则将变量 debian_chroot 设置为文件 /etc/debian_chroot 的内容
* ```~/.bashrc``` 脚本对 debian_chroot 变量的操作
  - 请自行返回到前面查看 ~/.bashrc 脚本的源代码
  - 第 60、62、69 行，\${debian_chroot:+(\$debian_chroot)} 表示：如果变量 debian_chroot 存在，则返回这个变量的值，否则返回一个空值
* ```${debian_chroot:+($debian_chroot)}``` 到底是什么？
  - 综上，**```${debian_chroot:+($debian_chroot)}```** 返回什么取决于文件 **/etc/debian_chroot** 是否存在，如果文件存在，则文件中的内容就是 **```${debian_chroot:+($debian_chroot)}```** 的值
  - 通常情况下，是不会有 /etc/debian_chroot 这个文件的，所以 **```${debian_chroot:+($debian_chroot)}```** 肯定是空值，
* 那么，文件 /etc/debian_chroot 有什么用？
  - 实际上，这个文件需要我们自己来建立，当我们 chroot 到一个特定的目录下时，如果没有提示，我们甚至可能会忘记我们是在 chroot 下工作，那么这时我们可以通过建立一个 /etc/debian_chroot 文件使得 **chroot** 在提示符上有所体现，避免我们因为忘记在 chroot 下而对许多情况感到莫名其妙
  - 下面我举一个实际的例子来看一下
    + 我现在用的电脑系统是 ubuntu 20.04，我在这台电脑的 /home/whowin/chroot-fs/ubuntu-xenial-16.04/ 目录下做了一个 ubuntu 16.04 的 chroot 目录，并且在 /home/whowin/chroot-fs/ubuntu-xenial-16.04/etc/ 目录下建立了一个文件 debian_chroot
    + 我们先来看看这个文件里有什么

      ![debian_chroot][img13_debian_chroot]

      **图11：文件debian_chroot**
    + 现在我们 chroot 到这个目录下，我们看一下会有什么不同

      ![chroot提示符][img14_chroot_prompt]

      **图12：chroot提示符**

    + 我们看到在提示符的最前面多出了 "(u16.04)"，而 "u16.04" 正是文件 debian_chroot 中的内容
* 至此，我觉着关于当前 PS1 的讨论应该可以告一段落

## PS1的有趣设置
* 在提示符上显示当前时间
  - 命令：```PS1="\u@\h [\t]$ "```
  - **```\t```** - 表示当前时间
    ```
    whowin@whowin-ubuntu:~$ 
    whowin@whowin-ubuntu:~$ echo $PS1
    \[\e]0;\u@\h: \w\a\]${debian_chroot:+($debian_chroot)}\u@\h:\w\$
    whowin@whowin-ubuntu:~$ PS1="\u@\h [\t]$ "
    whowin@whowin-ubuntu [17:27:46]$ 
    whowin@whowin-ubuntu [17:27:49]$ 
    whowin@whowin-ubuntu [17:27:53]$ 
    ```
* 显示内核版本及其他
  - 命令：```PS1="\#|\h|$(uname -r)|\$?$ "```
  - **```\#```** - 输入的命令数；$(uname -r) - 内核版本号；```\$?``` - 最后一个命令的返回码(正常返回应为0，非0表示出错)
    ```
    whowin@whowin-ubuntu:~$ 
    whowin@whowin-ubuntu:~$ 
    whowin@whowin-ubuntu:~$ PS1="\#|\h|$(uname -r)|\$?$ "
    3|whowin-ubuntu|4.15.0-175-generic|0$ echo "hello"
    hello
    4|whowin-ubuntu|4.15.0-175-generic|0$ LS
    程序“LS”尚未安装。 如需运行 'LS'，请要求管理员安装 'sl' 软件包
    5|whowin-ubuntu|4.15.0-175-generic|127$ uname -r
    4.15.0-175-generic
    6|whowin-ubuntu|4.15.0-175-generic|0$ 
    ```
  - 我们看到当我们输入一个错误命令 *LS* 时，返回的状态码变成了 **127**
* 在 PS1 中使用 shell 函数
  - 我们写一个 shell 函数显示逻辑 cpu 的数量，然后在 PS1 中调用这个函数使得在提示符中可以显示 cpu 数量
  - 如下输入函数及 PS1 设置命令
    ```
    whowin@whowin-ubuntu:~$ function cpucount {
    > cat /proc/cpuinfo|grep processor|wc -l
    > }
    whowin@whowin-ubuntu:~$ PS1="\u@\h:[`cpucount`]$ "
    whowin@whowin-ubuntu:[6]$ 
    ```
  - 我们可以写更复杂的函数，并把这个函数及PS1的设置命令放到 ```~/.bashrc``` 脚本中，使你的提示符与众不同
* 下划线和闪烁
  - 在提示符中使用颜色实际上是用的 ANSI 的 ESC 转义(见文章[《ANSI 的 ESCAPE 转义》][article03])，其实 ESC 转义还有很多，多数都是可以用的
  - 给用户名加上下划线```(\e[4m)```，让 "$" 提示符闪烁```(\e[5m)```
    ```
    PS1="\e[4m\u\e[0m@\h:\w\e[5m$\e[0m "
    ```
  - 可以和颜色组合起来用，提示符会更加绚丽多彩

## 关于PS2、PS3、PS4
* 既然有 PS1 变量，自然还有 PS2 变量，实际上还有 PS2、PS3 和 PS4，这些变量又是做什么的呢？先看 man 手册中的说明

  ![PS2,PS3,PS4][img15_ps2_3_4]

  **图13：man手册中关于PS2，PS3，PS4的说明**

* PS2 变量
  - 先来看一下 PS2 的值

    ![PS2 提示符][img16_ps2_value]

    **图14：PS2变量的值**

  - PS2 是一个二级提示符，在交互式 shell 中允许在命令行中输入脚本(比如循环、条件)，我们可以在 shell 提示符下试一下下面一段脚本
    ```
    whowin@whowin-ubuntu:~$ if who | grep -q whowin
    > then
    >     echo whowin is working online
    > else
    >     echo whowin is a lazy boy
    > fi
    whowin is working online
    whowin@whowin-ubuntu:~$ 
    ```
  - 其中的提示符 ">" 就是由 PS2 变量定义的，我们可以试着改一下 PS2 变量卡按一下效果
    ```
    whowin@whowin-ubuntu:~$ PS2="# "
    whowin@whowin-ubuntu:~$ if who |grep -q whowin
    # then
    #     echo whowin is working online
    # else
    #     echo whowin os a lazy boy
    # fi
    whowin is working online
    whowin@whowin-ubuntu:~$ 
    ```
  - 很显然，二级提示符由 "> " 变成了 "# "
* PS3 变量
  - PS3 变量定义的是使用 select 命令时的提示符，select 是 bash 的一个内建命令，但其具体用法不在本文的讨论范围内
  - 先来看一下 PS3 的值
    
    ![PS3提示符][img17_ps3_value]
    
    **图15：PS3变量并不存在**

  - PS3 并不存在，我们暂时不管这个，直接运行一个例子，下面例子在交互式 shell 下写了一段 select 的脚本

    ![select的例子][img18_ps3_example_1]

    **图16：PS3的默认值**

  - 按照 man 手册的说明，标记成黄色的 "#? " 就应该是 PS3 的值，可是前面看到明明 PS3 不存在呀，实际上，当 PS3 不存在时，会使用其默认值 "#? " 来代替 PS3
  - 为了明显起见，我们姑且先把 PS3 指定一个值 "## " (有别于其默认值 "#? ")
    ```
    PS3="## "
    ```
  - 我们重新运行上面那段脚本
    ```
    whowin@whowin-ubuntu:~$ set|grep PS3
    whowin@whowin-ubuntu:~$ PS3="## "
    ```
    ![select的例子][img19_ps3_example_2]

    **图17：select的例子**

  - 在这个例子中，红框内的是 PS2 定义的二级提示符，绿框内的数字是从键盘输入的，黄色的 "## " 就是 PS3 定义的提示符
  - 但是我们如果把这段脚本写入一个脚本文件运行，可能又有些不同
    ```
    whowin@whowin-ubuntu:~$ cat ps3.sh
    #!/bin/bash

    select command in who ps quit
    do
        case $command in
        who)
            who
            ;;
        ps)
            ps
            ;;
        quit)
            break
            ;;
        esac
    done
    exit 0

    whowin@whowin-ubuntu:~$ chmod +x ps3.sh
    whowin@whowin-ubuntu:~$ PS3="## "
    ```
    ![select的例子][img20_ps3_example_3]

    **图18：在脚本文件中运行的 select 的例子**

  > 这个地方就有点怪了，尽管我们在运行脚本文件前已经把 PS3 设置成 "## "，但运行起来时 select 的提示符仍然是默认的 "#? "；其实这没什么奇怪的，我们前面在交互式 shell 命令符下运行这段脚本时，是在当前环境下，我们在当前环境下设置了 PS3，然后又在当前环境下运行了脚本；而在脚本文件中运行时，bash 会首先为脚本建立一个子进程，然后在这个新环境中运行这个脚本文件，而我们并没有把 PS3 设置成环境变量，所以 PS3 的值并不会传递到新环境中，那么在运行脚本文件的新环境下实际上就是没有 PS3 的，那自然就会使用 PS3 的默认值去显示了；解决的办法也很简单，一是执行 *export PS3* 把 PS3 变成环境变量；二是在脚本文件里添加 PS3="## " 的语句。

    ```
    export PS3
    ./ps3.sh
    ```
    ```
    whowin@whowin-ubuntu:~$ cat ps3.sh
    #!/bin/bash

    PS3="## "

    select command in who ps quit
    do
        case $command in
        who)
            who
            ;;
        ps)
            ps
            ;;
        quit)
            break
            ;;
        esac
    done
    exit 0

    whowin@whowin-ubuntu:~$ 
    ```

* PS4 变量
  - 按照 man 手册的说明，PS4 定义的提示符会在跟踪脚本时，在显示 bash 命令前显示这个提示符，这个可能需要进一步说明一下
  - bash 在执行的时候，有一种调试(跟踪)模式，在这种模式下，bash 从脚本文件中读出一行，先把读出的命令显示出来，然后再显示执行结果，PS4 定义的提示符就会放在显示命令的这一行的开头
  - PS4 的默认值是 "+ "
  - 先看一下当前 PS4 的值
    
    ![PS4的值][img21_ps4_value]

    **图19：PS4 的值**

  - 我们使用调试模式运行前面的脚本 ps3.sh，在运行之前，先打印 PS4 的值和 ps3.sh 的内容
    ```
    whowin@whowin-ubuntu:~$ set|grep PS4
    PS4='+ '
    whowin@whowin-ubuntu:~$ cat -n ps3.sh
      1  #!/bin/bash
      2  
      3  PS3="## "
      4  
      5  select command in who ps quit
      6  do
      7      case $command in
      8      who)
      9          who
      10          ;;
      11      ps)
      12          ps
      13          ;;
      14      quit)
      15          break
      16          ;;
      17      esac
      18  done
      19  exit 0
      20  
    whowin@whowin-ubuntu:~$ 
    ```

    ![调试模式下运行脚本][img22_trace_mode]

    **图20：调试模式下运行脚本**

  - *bash -x* 是一种以调试模式运行脚本的方法，所有标记为黄色的地方就是 PS4 提示符，可以把这些行与 ps3.sh 文件内容比较一下，应该就能明白 PS4 提示符的作用

## 结语
  > **总结一下本文涉及的内容**

  1. PS1 是一个 shell 变量，并不是环境变量
  2. PS1 的设置遵循文章[《PS1 的完整控制码》][article02]和[《ANSI 的 ESCAPE 转义》][article03]中的规范
  3. 在 bash 启动过程中，设置 PS1 的主要有两个文件 /etc/bash.bashrc 和 ~/.bashrc
  4. PS1 中的一部分用于定义终端窗口标题
  5. PS1 中的 debian_chroot 可用于 chroot 文件系统的标识
  6. PS1 在设置时除了要遵守上面提到的规范，也可以执行 bash 函数或内建命令
  7. PS2, PS3 和 PS4 各有其用

## 附录
1. [《PS1 的完整控制码》][article02]
2. [《ANSI 的 ESCAPE 转义》][article03]


[article01]: ./0001-environment_variables_and_shell_variables.md
[article02]: ./0002-PS1_control_codes.md
[article03]: ./0003-ANSI-escape-code.md


<!-- -->
[img01_man_env_1]:/images/100004/man_env_1.png
[img02_man_env_2]:/images/100004/man_env_2.png
[img03_set_help]:/images/100004/set_help.png
[img04_terminal_title]:/images/100004/terminal_title.png
[img05_terminal_original_title]:/images/100004/terminal_original_title.png
[img06_terminal_hello_title]:/images/100004/terminal_hello_title.png
[img07_terminal_title_time]:/images/100004/terminal_title_time.png
[img08_terminal_PS1_1]:/images/100004/terminal_PS1_1.png
[img09_terminal_PS1_2]:/images/100004/terminal_PS1_2.png
[img10_man_bash_1]:/images/100004/man_bash_1.png
[img11_man_bash_2]:/images/100004/man_bash_2.png
[img12_man_bash_3]:/images/100004/man_bash_2.png
[img13_debian_chroot]:/images/100004/debian_chroot.png
[img14_chroot_prompt]:/images/100004/chroot_prompt.png
[img15_ps2_3_4]:/images/100004/ps2_3_4.png
[img16_ps2_value]:/images/100004/ps2_value.png
[img17_ps3_value]:/images/100004/ps3_value.png
[img18_ps3_example_1]:/images/100004/ps3_example_1.png
[img19_ps3_example_2]:/images/100004/ps3_example_2.png
[img20_ps3_example_3]:/images/100004/ps3_example_3.png
[img21_ps4_value]:/images/100004/ps4_value.png
[img22_trace_mode]:/images/100004/trace_mode.png
<!-- -->

<!-- 
[img01_man_env_1]:../../../static/images/100004/man_env_1.png
[img02_man_env_2]:../../../static/images/100004/man_env_2.png
[img03_set_help]:../../../static/images/100004/set_help.png
[img04_terminal_title]:../../../static/images/100004/terminal_title.png
[img05_terminal_original_title]:../../../static/images/100004/terminal_original_title.png
[img06_terminal_hello_title]:../../../static/images/100004/terminal_hello_title.png
[img07_terminal_title_time]:../../../static/images/100004/terminal_title_time.png
[img08_terminal_PS1_1]:../../../static/images/100004/terminal_PS1_1.png
[img09_terminal_PS1_2]:../../../static/images/100004/terminal_PS1_2.png
[img10_man_bash_1]:../../../static/images/100004/man_bash_1.png
[img11_man_bash_2]:../../../static/images/100004/man_bash_2.png
[img12_man_bash_3]:../../../static/images/100004/man_bash_2.png
[img13_debian_chroot]:../../../static/images/100004/debian_chroot.png
[img14_chroot_prompt]:../../../static/images/100004/chroot_prompt.png
[img15_ps2_3_4]:../../../static/images/100004/ps2_3_4.png
[img16_ps2_value]:../../../static/images/100004/ps2_value.png
[img17_ps3_value]:../../../static/images/100004/ps3_value.png
[img18_ps3_example_1]:../../../static/images/100004/ps3_example_1.png
[img19_ps3_example_2]:../../../static/images/100004/ps3_example_2.png
[img20_ps3_example_3]:../../../static/images/100004/ps3_example_3.png
[img21_ps4_value]:../../../static/images/100004/ps4_value.png
[img22_trace_mode]:../../../static/images/100004/trace_mode.png
<!-- -->
