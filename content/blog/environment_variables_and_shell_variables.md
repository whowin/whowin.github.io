---
title: "bash 的环境变量和 shell 变量"
date: 2022-04-10T16:43:29+08:00
author: 华松青
description: shell变量和环境变量相关的一些概念
sidebar: false
authorbox: false
categories:
  - "Linux"
  - "Shell"
  - "Bash"
tags:
  - shell
  - bash
  - source
  - printenv
  - 环境变量
  - declare
  - export
draft: false
postid: 100001
---

本文简单介绍了bash的启动过程；shell变量和环境变量的区别及相互转换；环境变量传递给子进程的过程；当前环境执行脚本机器实际应用
<!--more-->
## 导言
* shell 是一个 Linux 的命令行解释器，Linux 下有很多 shell，其中 ubuntu 中默认的 shell 是应该是 **dash**，因为我们看到 */bin/sh* 被链接到了 **dash**
  
  ![/bin/sh-->dash][img_01]

  **图1：/bin/sh指向dash**

* 但是当你启动终端时(不管是在桌面还是远程)，默认启动的却是 **bash**(Bourne-Again SHell)，bash 是 GNU (Gnu's Not Unix)开发的
* 使用 *cat /etc/shells* 命令可以看到在你的 Linux 下有那些 shell；使用 *echo $SHELL* 可以看到当前你正在使用的 shell
* 本文中如无特别说明，shell 指的是 bash，所有范例在 ubuntu 16.04 下完成，在更高版本的 ubuntu 上，可能会有些微区别；在较低版本的 ubuntu 上不能保证有相同的效果

## 终端是如何启动 shell 的
* ubuntu 的1号进程是 systemd，这是 ubuntu 在加载完 Linux 内核后启动的第一个进程，是所有其它进程的祖宗
* 你可能用 *ps aux(ps -ef)* 命令发现1号进程是 **/sbin/init**，而不是 **systemd**，但你用 *ls -l /sbin/init* 看一下就会恍然大悟
  
  ![1号进程][img_02]

  **图1：1号进程**
  
  ![init进程指向systemd][img_03]

  **图3：/sbin/init指向systemd**

* 当你在 ubuntu 桌面版上启动一个终端时，会启动一个 **gnome-terminal** 进程，gnome-terminal 进程启动了 **bash** 进程，这样你就看到可爱的提示符了，这个过程使用 *pstree* 命令一目了然

  ![gnome-terminal进程启动bash][img_04]

  **图4：gnome-terminal进程启动bash**

* 当你使用 ssh 客户端远程登录到 Linux 系统时，**bash** 进程是由 **sshd** 进程启动的，这个也可以用 *pstree* 命令看到
  
  ![sshd进程启动bash][img_05]

  **图5：sshd进程启动bash**

* 只有启动了 bash 你才拥有了一个 shell 环境，你才能够在终端上输入命令，你从键盘输入的任何内容都必须由 bash 进行解释并做出进一步的处理

## shell 变量和环境变量
* **shell 变量**
  - shell 管理着一个变量表，这使得用户可以自己定义变量，这些变量是在 shell 下建立，由 shell 管理，在 shell 下使用
  - 在启动 shell(bash) 的时候，shell 会创建一些变量(不同的 shell 创建的变量会有所不同)，同时，shell 在启动过程中还会去执行一些可以由客户自定义的脚本，比如：在启动bash时会执行：**/etc/bash.bashrc**，在登录时会执行：**~/.bashrc**等，这些脚本通常也会建立一些变量
  - 由于有些脚本是在用户登录时执行的，比如：**~/.bashrc**，而这个脚本是放在用户的 **home** 目录下的，这就使得不同的用户在登录时可以执行不同的脚本，从而每个用户在登录后所建立的 shell 变量也可以是不一样的
  - 在 shell 下，可以直接用：*变量名=变量值* 的方法定义或修改 shell 变量；也可以用：*unset 变量名* 来删除变量，按照惯例，shell 变量名使用**大写字母**
  - 使用 *set* 命令可以查看当前所有的 shell 变量；用 *echo $变量名* 可以显示指定变量名的值

* **环境变量**
  - 环境变量也是 shell 变量，但环境变量却不等同于 shell 变量，我们暂时把不是环境变量的 shell 变量称作 **普通 shell 变量**，在 bash 内部，环境变量与普通 shell 变量的区别，仅仅是一个不同的标记而已
  - 环境变量和普通 shell 变量的最主要的区别是，在建立一个子进程时，环境变量会被传递给这个子进程，而普通 shell 变量不会，所以，环境变量可能会对一个子进程的行为产生影响，因为子进程可以根据环境变量的值做出不同的动作
  - 环境变量可以使用 *export 变量名=变量值* 进行设置，可以像普通 shell 变量一样用 *unset 变量名* 进行删除
  - 可以使用 *printenv* 命令查看所有的环境变量；用 *echo $变量名* 可以显示指定环境变量的值；也可以用 *printenv 环境变量名* 来显示一个环境变量的值
  - *env* 如果不带参数的话，也是可以显示环境变量的，但这个命令主要用于在指定环境下执行命令
* 关于 *env* 命令的两个例子
  - *env 环境变量名=变量值 命令* 和 *env -u 环境变量名 命令* 都只能临时在一个新的环境中改变或者删除一个变量，用于在一个指定的环境中执行命令，这一点在很多文章中都没有明确说明
  - 下面的例子设置了一个临时环境变量 **ENV_VAR_1**，并且在这个临时环境中将这个变量的值打印出来
    ```
    $ printenv ENV_VAR_1                           # 当前环境下，不存在环境变量 ENV_VAR_1
    $ env ENV_VAR_1=first_value printenv ENV_VAR_1 # 临时设置环境变量并打印出值
    first_value                                    # 临时设置的环境变量ENV_VAR_1的值
    $ printenv ENV_VAR_1                           # 当前环境下，仍然不存在环境变量 ENV_VAR_1
    $ 
    ```
  - 下面的例子中在临时环境中删除一个在当前环境中存在的环境变量，在临时环境中打印该环境变量为空，确定该环境变量在临时环境中已经不存在
    ```
    $ export ENV_VAR_2=second_value                  # 当前环境下设置环境变量
    $ printenv ENV_VAR_2                             # 打印该环境变量的值
    second_value
    $ env -u ENV_VAR_2 printenv SHELL ENV_VAR_2 USER # 临时删除该环境变量，并在临时环境中打印该变量
    /bin/bash                                        # SHELL变量的值
    demouser                                         # USER变量的值，偏偏没有变量ENV_VAR_2的值
    $ printenv ENV_VAR_2                             # 当前环境下打印环境变量ENV_VAR_2
    second_value
    $ 
    ```

* **环境变量和普通shell变量的转换**
  - 普通shell变量，通过 *export 变量名* 可以转变成环境变量
    ```
    $ set|grep VAR_TO_ENVVAR                # 没有VAR_TO_ENVVAR这个shell变量
    $ VAR_TO_ENVVAR=convert_var_to_envvar   # 设置VAR_TO_ENVVAR
    $ set|grep VAR_TO_ENVVAR                # VAR_TO_ENVVAR是一个shell变量
    VAR_TO_ENVVAR=convert_var_to_envvar
    $ printenv|grep VAR_TO_ENVVAR           # VAR_TO_ENVVAR不是一个环境变量
    $ export VAR_TO_ENVVAR                  # 执行export命令
    $ printenv|grep VAR_TO_ENVVAR           # VAR_TO_ENVVAR已经变成一个环境变量
    VAR_TO_ENVVAR=convert_var_to_envvar
    $ 
    ```
  - 普通shell变量，通过 *declare -x 变量名* 可以转变成环境变量
    ```
    $ unset VAR_TO_ENVVAR                   # 删除变量VAR_TO_ENVVAR
    $ VAR_TO_ENVVAR=convert_var_to_envvar   # 设置VAR_TO_ENVVAR
    $ set|grep VAR_TO_ENVVAR                # VAR_TO_ENVVAR是一个shell变量
    VAR_TO_ENVVAR=convert_var_to_envvar
    $ printenv|grep VAR_TO_ENVVAR           # VAR_TO_ENVVAR不是一个环境变量
    $ declare -x VAR_TO_ENVVAR              # 执行declare命令
    $ printenv|grep VAR_TO_ENVVAR           # VAR_TO_ENVVAR已经变成一个环境变量
    VAR_TO_ENVVAR=convert_var_to_envvar
    $ 
    ```
  - 环境变量，通过 *declare +x 变量名* 可以转变成普通shell变量
    ```
    $ export ENV_VAR_1=testing    # 设置一个环境变量
    $ printenv|grep ENV_VAR_1     # 确认设置成功
    ENV_VAR_1=testing
    $ declare +x ENV_VAR_1        # 执行declare +x命令
    $ printenv|grep ENV_VAR_1     # 该变量已经不再是环境变量
    $ set|grep ENV_VAR_1          # 该变量仍然是一个普通shell变量
    ENV_VAR_1=testing
    $ 
    ```
  - *declare 变量名=变量值* 可以用来设置或修改一个普通shell变量的值；*declare -x 变量名=变量值* 可以用来设置或修改一个环境变量的值；*declare -x 变量名* 可以将普通shell变量变成环境变量；*declare +x 变量名* 可以将环境变量变成普通shell变量

## bash 如何将环境变量传给子进程
  * **bash 如何执行一个命令**
    - shell 在收到一个换行符(new line，ASCII码0x0A)时开始解释命令行的命令
    - shell 查找命令是否有别名(alias)，如果有则用别名代替命令
    - 如果命令中不包含 "/"，shell 首先查找同名函数，如果有，执行这个函数即可
    - 如果没有同名函数则查找内建命令，如果是内建命令，则在bash内部执行即可
    - 如果也不是内建命令，则根据环境变量 **PATH** 的顺序查找命令文件
    - 如果找不到命令文件，则显示错误信息并回到提示符接收下面的命令
    - 如果找到命令文件或者命令中有 "/" 字符，bash 会 fork 一个子进程，自身进程执行 **wait()** 等待子进程结束，然后在子进程中执行 **execve()**，一切的其他工作交给 **execve()** 来处理
  * **环境变量的传递**
    - 我们看一下 **execve()** 的手册
      
      ![execve()说明][img_06]

      **图6：execve()手册**
      
    - 在执行 **execve()** 时，需要传递三个参数过去，filename - 要执行的程序文件名；argv[] - 执行这个程序需要的参数；envp - 环境变量；**环境变量就是这样传递给了可执行程序**
    - 大家可能注意到，shell 不仅可以执行一个二进制的程序，也可以执行一个 shell 脚本(ASCII文本文件)，我们并不需要告诉 shell 我们在执行那种文件，而 shell 却不会搞错，其实这个识别过程也是 **execve()** 的功劳
      > **execve()** 在执行程序时首先要读出文件的前128个字节，用以分析文件的类别，以便用适当的方式执行这个文件；比如：shell脚本文件的前两个字符是 **"#!"**，这一点在手册中有明确说明；ELF文件的前四个字符是：0x45 0x4c 0x46 0x7c等，还有其它不同的可执行格式，这个过程其实还是比较复杂的，但是和 shell 变量毫无关系，所以不在本文讨论的范围内
  * **一个打印环境变量的C程序**
    - 该程序只是验证传递给子进程的环境变量不包括普通shell变量
    - 源代码，文件名：print_env.c
      ```
      #include<stdio.h>

      extern char **environ;
      
      int main() {
          int i;

          for (i = 0; environ[i]; i++)
              printf("*%s\n", environ[i]);
      
          return 0;
      }
      ```
    - 编译执行
      ```
      $ gcc print_env.c -o print_env
      $ ./print_env
      ```
    - 本程序打印出的环境变量与 *printenv* 命令打印出的结果一致
  * **跟踪程序的执行**
    - 用 *strace ./print_env* 可以清楚地看到环境变量被传递给了程序 print_env

      ![跟踪程序print_env][img_07]

      **图7：跟踪程序print_env**
    
    - 我们先用 *printenv|wc -l* 打印出环境变量的数量，然后我们看到有相同数量的变量被传递给了我们的程序 print_env
  * **环境变量对启动程序的影响**
    - 上面这个小程序我们在运行时是使用 *./print_env* 的方式运行的，其中 "./" 表示当前路径，但是这个程序其实就在当前目录下，我们为什么一定要指定路径呢？我们试一下不指定路径会怎样

      ![command not found][img_08]

      + **图8：命令执行失败**

    - 这是因为 shell 是沿着环境变量 **PATH** 的顺序来查找命令文件的，而当前目录 ./ 并不在当前的 **PATH** 中，我们可以试着把 ./ 加入到环境变量 **PATH** 中，然后再运行一下试试

      ![执行print_env成功][img_09]

      + **图9：执行print_env成功**

    - 正如我们所期待的，执行成功了
  * **子进程中无法修改父进程环境的环境变量**
    - shell 传递给的程序的环境仅仅是父进程环境的一个副本，所以我们在程序中改变这个副本中的变量都无法改变父进程的环境，当程序运行结束时，这个环境副本将被销毁
    - 下面这个例子我们首先在 shell 下定义一个环境变量 ENV_VAR_1，然后编写一个脚本修改这个环境变量，在脚本退出后我们再次打印这个环境变量
      > 脚本文件名为：chg_envvar.sh，脚本的代码如下：
      ```
      #!/bin/bash

      echo "Print environment variable - ENV_VAR_1"
      printenv ENV_VAR_1
      echo "Change ENV_VAR_1 to 'second_value'"
      ENV_VAR_1="second value"
      echo "Again, print environment variable - ENV_VAR_1"
      printenv ENV_VAR_1

      exit 0
      ``` 
      > 下面我们完成这个测试
      ```
      whowin@ubuntu:~$ chmod +x chg_envvar.sh
      whowin@ubuntu:~$ export ENV_VAR_1="first value"
      whowin@ubuntu:~$ printenv ENV_VAR_1
      first value
      whowin@ubuntu:~$ ./chg_envvar.sh
      Print environment variable - ENV_VAR_1
      first value
      Change ENV_VAR_1 to 'second_value'
      Again, print environment variable - ENV_VAR_1
      second value
      whowin@ubuntu:~$ printenv ENV_VAR_1
      first value
      whowin@ubuntu:~$
      ```
      > 我们看到，在脚本内部执行 *printenv ENV_VAR_1* 时，打印出来的结果已经是 "second value"，说明我们已经修改成功了这个环境变量的值，但当脚本退出，我们再次打印这个变量时，其值仍然是 "first value"，并没有改变，这说明我们在子进程中对环境的修改并不能影响到父进程

## 在当前环境下运行程序
  * 我们在前面一再强调，shell 在执行一个程序的时候会先 fork 一个子进程，然后在子进程中执行程序，这实际上是 shell 为执行一个程序新建立了一个环境，然后在这个环境中执行程序，当然这个新环境继承了父进程的环境
  * 其实，shell 也可以不 fork 一个子进程，而是直接在当前进程下执行你的程序，shell 下有一个内建命令 *source* 就是为此而设计的，我们先来看看这个命令的手册

    ![source的help手册][img_10]

    **图10：source命令的help手册**

  * 说明已经很清楚了，要说明的是 *source* 命令只能执行脚本文件
  * 我们提出的问题是，既然 *source* 命令是在当前 shell 下执行程序，那是不是意味着上面那个改变环境变量的脚本文件可以改变当前 shell 下的环境变量了呢？我们执行一下试试
  * 我们在前面执行脚本时使用的命令是：*./chg_envvar.sh*，我们已经解释过 "./" 的含义以及为什么要有 "./"，这次我们执行这个脚本准备用 *. ./chg_envvar.sh* 来执行，只是在原来命令的前面多了 ". "，点后面有个空格，其实这个 "." 就等同于 *source*，所以这个命令就相当于 *source ./chg_envvar.sh*
    ```
    whowin@ubuntu:~$ . ./chg_envvar.sh
    ```
  * 当我们用这种方法执行这个脚本的时候，我们发现意外出现了，整个 shell 都退出了，这是为什么呢？
    > 这是因为在我们的脚本的最后一行有一个 **exit 0** 语句，正常情况下，如果我们不使用 source 去运行时，**exit** 会退出 shell 为这个脚本建立的子进程，所以一点问题都没有，但是当用 *source* 去运行这个脚本时，没有建立子进程，那么退出的就是当前 shell 进程，所以你的 shell 就没有了，因为被你运行的脚本退出了；但是如果我们不使用 **exit** 而使用 **return** 退出确实是可以解决在使用 *source* 命令运行的问题，但是不使用 *source* 运行就要出问题了，大家自己可以试一下

    > 那么是不是可以不写 **exit** 和 **return** 呢？当然可以，但是这不是写 shell 脚本的好习惯，因为执行完你的脚本后，可能还要执行下一个脚本，而下一个脚本可能要判断你的这个脚本的返回值，所以在写脚本时返回一个有意义的值是非常良好的习惯

    > 其实我们修改一下这个脚本就可以解决这个问题，这里仅提供源码，解释超出了本文涉及的内容


    ```
    whowin@ubuntu:~$ cat chg_envvar.sh 
    #!/bin/bash

    echo "Print environment variable - ENV_VAR_1"
    printenv ENV_VAR_1
    echo "Change ENV_VAR_1 to 'second_value'"
    ENV_VAR_1="second value"
    echo "Again, print environment variable - ENV_VAR_1"
    printenv ENV_VAR_1

    if [ $0 != "$BASH_SOURCE" ]; then
        return 0
    else
        exit 0
    fi
    whowin@ubuntu:~$ 
    ```
  * 用这种方法，也可以让某些必须用 *source* 运行的程序如果没有在 *source* 下运行，可以给出提示并停止运行
  * 现在回到正题，这个脚本能不能改变当前 shell 下的环境变量呢？答案是肯定的。
    ```
    whowin@ubuntu:~$ export ENV_VAR_1="first value"
    whowin@ubuntu:~$ printenv ENV_VAR_1
    first value
    whowin@ubuntu:~$ source ./chg_envvar.sh 
    Print environment variable - ENV_VAR_1
    first value
    Change ENV_VAR_1 to 'second_value'
    Again, print environment variable - ENV_VAR_1
    second value
    whowin@ubuntu:~$ printenv ENV_VAR_1
    second value
    whowin@ubuntu:~$ 
    ```
  * 很显然，脚本 *chg_envvar.sh* 运行完毕后，我们发现，环境变量 **ENV_VAR_1** 已经发生了改变
  * 实际上这种方法是一种常用的方法，常被用于改变当前环境
    > 在嵌入式开发中，不同的开发板使用的CPU可能不同，这样在交叉编译时的工具链也不同，不同的开发板即便是相同的CPU也可能使用不同的工具链进行编译，比如即便是相同的CPU可能有些需要用软浮点的编译器，有些使用硬浮点的编译器，这时我们可以用上面的方法为每一个交叉编译的工具链写一个脚本，脚本中为某个指定的工具链所需的环境，然后用 *source* 去运行，下面是我的环境下的一个例子

    ```
    whowin@ubuntu:~$ cat a8.sh
    #!/bin/bash
    #A8 arm-linux-gnueabi工具链
    export PATH=/home/whowin/toolschain/4.5.1/bin:$PATH
    whowin@ubuntu:~$ 
    ```
## 结语
  * 启动终端程序时启动了 **bash** 进程，使我们可以在 **shell** 下输入命令
  * 环境变量也是 shell 变量，但又与 shell 变量略有不同
  * 环境变量与普通 shell 变量的主要区别是环境变量会传递给新建的子进程
  * 环境变量和普通 shell 变量之间可以使用 *export* 或 *declare* 进行转换
  * shell 可以在当前进程下运行脚本程序(不创建子进程)，这种运行方式常被用于改变当前运行环境下的环境变量




[img_01]:/images/100001/sh_point_to_dash.png
[img_02]:/images/100001/first_process.png
[img_03]:/images/100001/init_point_to_systemd.png
[img_04]:/images/100001/terminal-bash.png
[img_05]:/images/100001/sshd-bash.png
[img_06]:/images/100001/man-execve.png
[img_07]:/images/100001/strace_print_env.png
[img_08]:/images/100001/command_not_found.png
[img_09]:/images/100001/print_env_successfully.png
[img_10]:/images/100001/source_help.png


