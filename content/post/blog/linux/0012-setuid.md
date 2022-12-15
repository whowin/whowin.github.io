---
title: "Linux文件权限：setuid、setgid和sticky bit"
date: 2022-08-25T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags:
  - Linux
  - "文件权限"
  - setuid
  - uid
  - euid
  - gid
  - egid
  - setgid
  - "sticky bit"
draft: false
#references: 
# - [Linux中的setuid简介](https://blog.csdn.net/weixin_44575881/article/details/86552016)
# - [getuid、geteuid、getgid和getegid函数](https://blog.csdn.net/qq_33883085/article/details/88799925)
postid: 100012
---

Linux 下的文件权限管理分为三组：拥有者、组、其它用户，文件权限分为读、写、执行，但其实并不仅仅如此，还有 setuid、setgid、sticky bit 这一组标志，本文通过一个可执行文件的权限 4755 展开介绍 setuid、setgid 和 sticky bit 的概念，希望本文对读者理解 Linux 文件权限管理能有所帮助。
<!--more-->

---------------------------
## 1. 概述
* 本文通过一个 4755 权限的可执行文件引出了关于 Linux 下与文件权限相关的一系列概念，包括 uid、euid、gid、egid、setuid、setgid等等；
* 希望通过本文读者可以更好地理解 Linux 下的权限管理方式；
* 本文涉及的概念并不复杂，所需的关联知识也不多，希望对初学者和有一定功底的程序员均能有所帮助；
* Linux 的文件有权限(Permission)和属性(Attribute)，前者用于限制用户对文件的读、写和执行操作，后者也是用于限制对文件的操作，比如限制文件只能追加、限制文件不能压缩存放等，权限(Permission)和属性(Attribute)在中文里有些混淆，本文仅讨论文件的权限(Permission)。

-------------------------
## 2. 问题的提出
> 前不久在折腾 openwrt 时，需要给 openwrt 装上 sudo(openwrt 默认是不安装 sudo 的)，安装很成功，用起来也没什么问题，但是重启了以后就不能用了，后来发现原因之一是这个 openwrt 在启动的时候将 /usr/bin/ 目录下的所有文件的权限(permission)都改为了 0755，这是一个正常的可执行文件的权限(permission)，但是对 sudo 这个可执行文件却是不行的，sudo 的文件权限(permission)必须是 4755；

> 解决这个问题倒是不难，在 openwrt 启动时，执行一下 chmod 4755 /usr/bin/sudo 就行了；

> 这件事让我想写这篇文章，因为我们看惯了诸如 0755、0750等可执行文件属性，这个 4755 的文件属性并不多见。

--------------------------
## 3. 有哪些文件的属性是4755
* **4755 的文件属性在文件列表时的样子**
    ```
    ls -al /usr/bin/sudo
    ```
    
    ![在ubuntu上看到的结果][img01]
    - **图1:4755的文件属性怎么显示**
    ***********************************
    - 可以看到平时常见的 rwx 中的 x 变成了 **s**，这个 **s** 正是 chmod 4755 /usr/bin/sudo 命令中，那个 “**4**” 造成的

* **sudo 这个文件有什么与众不同的特点**
    - 假定我们有一个用户 demo，我们已经把这个用户加入到了 sudoers 中，那么 demo 用户使用 sudo 命令就可以提权了；
    - sudo 这个可执行文件的拥有者(owner)一定是 root，执行这个文件时，至少需要读取用户密码文件 /etc/shadow，这个密码文件的拥有者是 root，权限是 640，所以读取这个文件是要有 root 权限，也就是说，执行 sudo 是需要有 root 权限的；
    - 问题是，当我们执行这个文件时，当前用户是 demo，而 demo 是没有 root 权限的，只有执行 sudo 经过提权才能短暂地拥有 root 权限；
    - 这里产生了一个无法调和的矛盾，demo 用户需要通过执行 sudo 获得 root 权限，但执行 sudo 也需要 root 权限；
    - 这就是为什么 sudo 这个文件的属性是 4755 的原因了，当一个可执行文件的属性是 4755 时，任何用户执行这个文件时，将被以该文件拥有者的权限运行，这个问题后面会有详细的描述；

* **有哪些文件的属性是 4755**
    - 下面指令可以显示 /usr/bin/ 目录下哪些文件的属性是 4755
        ```
        ls -al /usr/bin|grep "^-rws"
        ```
    - 在我的 ubuntu 下会显示出这些文件

        ![/usr/bin/目录下4755属性的文件][img02]
        + **图2：/usr/bin/目录下4755属性的文件**

        *****************************
        ![/bin/目录下4755属性的文件][img03]
        + **图3：/bin/目录下4755属性的文件**
    
    - 还可以用 find 命令查找系统中所有的权限为 4755 的文件
        ```
        sudo find / -perm 4755 -type f
        ```
-------------------------
## 4. uid、euid、gid、egid
* 当我们在 Linux 上创建一个新用户时，系统会分配给这个用户一个 id，我们称为 uid；还会分配给这个用户一个组 id，我们称为 gid；
* 我们可以从 /etc/passwd 文件中看到一个用户的 uid 和 gid，比如我当前的用户是 whowin，我可以这样看到这个用户的 uid 和 gid

    ![当前用户的uid和gid][img04]
    - **图4：当前用户的uid和gid**

    ******************************
    - 图中黄色框中的两组数字，前面的 1000 是 uid，后面的 1000 是 gid
* root 用户的 uid = 0，gid = 0，这个可是不能乱改的；

    ![root用户的uid和gid][img05]
    - **图5：root用户的uid和gid**

* 当一个用户执行一个程序时，需要创建一个进程，Linux 会赋予这个进程一定的权限，当然是赋予执行这个程序的用户所拥有的权限，比如我用 whowin 这个用户运行 touch file.tmp，则 touch 进程获得的是 whowin 的权限，whowin 有在当前目录下创建文件的权限，这个命令创建了一个新文件 file.tmp 该文件的所有人(owner)是 whowin；

    ![当前用户执行touch命令][img06]
    - **图6：whowin用户执行touch命令**

* 同样是 whowin 用户，当执行 cat /etc/shadow 时，权限就不够了，前面说过 /etc/shadow 的权限是 640，只有 root 才能读写该文件，Linux 把用户 whowin 的权限赋予 cat 进程，whowin 没有权限读取 /etc/shadow，所以 cat 进程无法继续执行；

    ![当前用户权限不够][img07]
    - **图7：用户权限不够**

* 以上例子只是要说明，正常情况下 Linux 会把当前用户的权限赋予其创建的进程，如果这个用户没有足够的权限，则其创建的进程也没有足够的权限；
* 但是，Linux 并不是通过判断 uid 和 gid 来确定一个进程的权限的，而是通过 euid(effective uid) 和 egid(effective gid) 来决定赋予进程什么权限，就像我们上面的例子一样，大多数情况下，uid 和 euid 是相同的，gid 和 egid 是相同的；
* 但是，这样的一种机制显然带来了一种可能性，即：uid 和 euid 不同；那么这种不同会带来什么呢？**当一个 uid=1000 的用户运行一个程序 A，如果此时 euid=0，那么程序 A 将获得 root 的权限**。
--------------------------------
## 5. setuid、setgid 和 sticky bit
* setuid、setgid 和 sticky bit 其实都是文件权限的标志位，在详细介绍这几个标志位之前，我们需要先回顾一下一般意义上的文件权限；
* 我们常说一个文件的权限是 0755，其中 7 代表拥有者的权限，中间的 5 代表该文件所在的组的权限，最右边的 5 代表其它用户的权限，那么最前面的 0 是什么意思呢？记得曾经有人告诉我，这个 0 的意思是指权限是使用八进制表示的，听上去有点道理，因为表达八进制数字时确实是需要在前面加上一个 0 的，但这里的这个 0 却有着更深的含义，而且，这个 0 也可以是大于 0 的值，这些我们后面再说；
* 文件权限的确是用八进制数表示的，权限分为三组，分别是：拥有者、组和其它用户，每组的读、写、执行权限用一个八进制数表示(三位二进制数)；

    ![Linux文件权限示意图][img08]
    - **图8：Linux文件权限示意图**

    ------------------------------
* 下面我们用一个实例来说明文件权限为 4755 时的不同
    1. 首先我们编一个打开文件的小程序 open_file，并建立一个只有 root 用户可以读取的文件 rootfile.txt
        + 程序 open_file.c 的源代码
            ```
            #include <stdio.h>
            #include <stdlib.h>

            #include <sys/types.h>
            #include <sys/stat.h>
            #include <fcntl.h>
            #include <unistd.h>

            #include <errno.h>
            #include <string.h>

            int main(int argc, char **argv) {
                int fd = 0;

                if (argc != 2) {
                    printf("Usage: %s [filename]\n", argv[0]);
                    return -1;
                }

                fd = open(argv[1], O_RDONLY);
                if (fd == -1) {
                    fprintf(stderr, "%s\n", strerror(errno));
                    return -1;
                }
                close(fd);

                printf("Open file %s successfully.\n", argv[1]);
                return 0;
            }
            ```
        + 编译这个程序
            ```
            gcc open_file.c -o open_file
            ```
        + 建立一个只有 root 可以读的文件 rootfile.txt
            ```
            echo "This file's owner is root.">rootfile.txt
            chmod 600 rootfile.txt
            sudo chown root:root rootfile.txt
            ls -l rootfile.txt
            cat rootfile.txt
            ```

        ![编译程序并建立一个只有root可读的文件][img09]
        + **图9：编译程序并建立一个只有root可读的文件**
        -------------------------
        + 我们看到当前用户 whowin 是不能读取文件 rootfile.txt 的

    2. 用程序 open_file 打开 rootfile.txt
        ```
        ./open_file rootfile.txt
        ```

        ![程序open_file无法打开文件rootfile.txt][img10]
        + **图10：程序open_file无法打开文件rootfile.txt**
        -----------------------------
        + 权限不够是因为 Linux 在执行 open_file 程序时将当前用户 whowin 的权限赋予了 open_file 进程，而 whowin 没有读取文件 rootfile.txt 的权限；
    3. 将程序 open_file 的所有者改为 root 并再次尝试打开文件 rootfile.txt
        ```
        sudo chown root:root open_file
        ls -l open_file
        ./open_file rootfile.txt
        ```

        ![将程序所有者改为root并再次执行][img11]
        + **图11：将文件所有者改为root并再次执行**
        -----------------------------
        + 权限还是不够，尽管 open_file 的拥有者变成了 root，whowin 仍有执行 open_file 的权限，但读取 rootfile.txt 时仍然使用的是 whowin 的权限，也就是说 **Linux 在执行 open_file 时将 whowin 的权限赋予了 open_file 进程，而没有将这个文件的拥有者 root 的权限赋予 open_file 进程**；
    4. 将 open_file 的权限改为 4755 并再次尝试打开文件 rootfile.txt
        ```
        sudo chmod 4755 open_file
        ls -l open_file
        ./open_file rootfile.txt
        ```

        ![将程序的权限改为4755并再次执行][img12]
        + **图12：将程序的权限改为4755并再次执行**
        -----------------------------
        + 第一个惊喜是，当我们把 open_file 的权限改为 4755 后，ls -l open_file 显示的**权限从 rwx 变成了 rws**
        + 第二个惊喜是 open_file 居然可以打开文件 rootfile.txt 了，这说明 open_file 的权限被改为 4755 后，Linux 在执行 open_file 时不像以前一样把当前用户 whowin 的权限赋予 open_file 进程，**而是把 open_file 的拥有者 root 的权限赋予了 open_file 进程**，这才使得 open_file 进程有足够的权限打开文件 rootfile.txt；

* **setuid**
    > setuid 是文件权限的一个标志位，当这个标志位置为 1 时，Linux 在执行这个文件时将会把这个文件的**所有者**的权限赋予这个程序的进程，而不是像普通可执行文件那样将执行该文件的用户的权限赋予这个程序的进程；

    > 换句话说，当一个文件设置了 setuid 标志后，用户执行这个文件时，在这个程序的进程中 uid 为用户的 uid，euid 为这个文件拥有者的 uid；

    - 只有这个文件的拥有者有可执行权限时，setuid 才有意义；
    - 设置这个标志可以使用下面这些命令：
        ```
        chmod u+s [文件名]
        ```
        ```
        chmod 4755 [文件名]
        ```
    - 用 ls -l 显示这个文件时，当该文件的拥有者有可执行权限时，**x 将被改为 s**；否则，**- 将被改为 S**；我们用文件 rootfile.txt 看一下
        ```
        ls -l rootfile.txt
        sudo chmod u+s rootfile.txt
        ls -l rootfile.txt
        sudo chmod u-s rootfile.txt
        ls -l rootfile.txt
        ```

        ![设置非可执行文件的setuid][img13]
        + **图13：设置非可执行文件的setuid**
        -----------------------------
    - 前面提到过 sudo 的例子，正是 setuid 的作用使得普通用户可以通过执行 sudo 进行提权；另一个例子是 passwd 命令，普通用户可以通过这个命令修改自己的密码，但这个文件是要读写 /etc/shadow 文件的，而只有 root 才有这个权限，所以执行 passwd 是需要 root 权限的，普通用户可以正常使用它也是由于这个文件设置了 setuid 标志位；
    - 所以，用户在执行 sudo 时，sudo 进程中的 euid=0；同样，用户在执行 passwd 时，paswd 进程中的 euid=0

* **setgid**
    > 理解了 setuid 之后 setgid 就比较容易理解了，setgid 也是文件权限的一个标志位，当设置该标志位后，用户执行该文件时，将把该文件所在的组的权限赋予这个程序进程的组权限，而不是使用当前用户的组权限去执行该程序；

    > 换句话说，当一个文件设置了 setgid 标志后，用户执行这个文件时，在这个程序的进程中 gid 为用户的 gid，egid 为这个文件拥有者的 gid；

    - 和 setuid 一样，只有这个文件的用户组有可执行权限时，setgid 才有意义
    - 设置这个标志可以使用下面这些命令：
        ```
        chmod g+s [文件名]
        ```
        ```
        chmod 2755 [文件名]
        ```
    - 用 ls -l 显示这个文件时，当该文件用户组有可执行权限时，**x 将被改为 s**；否则，**- 将被改为 S**；读者可以参照 setuid 自己试一下；

* **sticky bit**
    > sticky bit 是一个比较特殊的东西，这个标志只有用于目录时才有效，当一个目录被设置 sticky bit 后，在这个目录下的所有文件只有文件的拥有者和 root 可以删除，不管这个文件的权限是什么(哪怕是 0777 的权限)，所以 sticky bit 又可以被称为 “**限制删除标志**”
    
    - 最典型的例子是 /tmp 目录，每个用户都可以向这个文件中写入文件，假定有 whowin 和 demo 用户都在这个目录下写了文件，如果 demo 用户删除了 whowin 用户写入的文件，就有可能出现问题，所以这个目录被设置了 sticky bit，其中的文件只有拥有者和 root 可以删除；
    - 设置 sticky bit 可以使用以下命令
        ```
        chmod +t [目录名]
        ```
        ```
        chmod 1777 [目录名]
        ```
    - 当一个目录被设置 sticky bit 后，在其他用户权限中的可执行位将被改为 **t**

        ![显示设置sticky bit的目录][img14]
        - **图14：显示设置sticky bit的目录**
        ----------------------
* setuid、setgid 和 sticky bit 组成了文件权限的另一个八进制数

    ![Linux文件权限标志示意图][img15]
    - **图15：Linux文件权限标志示意图**
    ------------------------
* 这三个标志位又凑够了一个八进制数，和文件权限的 3 个八进制合在一起组成了 4 个八进制数，文章开头提到的 sudo 文件权限 4755 中的 4 就是这几个标志位组成的八进制数；
* 理论上，这三个标志是可以同时设置的，但这样设置未必有什么意义，我们可以用上面曾用到的 sticky 目录试一下

    ![在sticky目录上同时设置三个标志][img16]
    - **图16：在sticky目录上同时设置三个标志**
    ----------------
* 尽管我们设置成功了，但是可以想一下 setuid 和 setgid 在这个目录上的意义何在；

* 清除标志的命令，前面的例子中已经出现过
    ```
    chmod u-s [文件名]
    chmod g-s [文件名]
    chmod -t [目录名]
    ```
* chmod 0755 并不一定能清除标志，我们还以 sticky 目录为例

    ![chmod 0755全部无法清除标志][img17]
    - **图17：chmod 0755全部无法清除标志**
    -----------------------
* chmod 0755 仅清除了 sticky bit；chmod 00755 清除了所有三个标志
---------------------------------
## 6. uid 和 euid 的实验
* setuid 之所以能有上面提到的效果，实际上取决于 Linux 在打开一个程序文件创建进程时如何设置 euid，Linux 发现程序文件有 setuid 标志时，则将该文件拥有者的 uid 赋予 euid，否则将当前用户的 uid 赋予 euid；
* 下面我们编一个小程序直观地看一下 uid 和 euid 的变化；
* 程序 get_euid.c 的源代码
    ```
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/types.h>

    int main(void) {
        uid_t uid;
        uid_t euid;

        uid = getuid();
        euid = geteuid();

        printf("uid = %d\neuid = %d\n", uid, euid);

        return 0;
    }
    ```
* 编译该程序
    ```
    gcc get_euid.c -o get_euid
    ```
* 将该程序的拥有者变成 root(不设置 setuid)，运行一下看看 uid 和 euid 都是什么

    ![没有设置setuid时的euid][img18]
    - **图18：没有设置setuid时的euid**

-----------------
* 将该程序设置 setuid，再运行一下看看 uid 和 euid 都是什么

    ![设置setuid后的euid][img19]
    - **图19：设置setuid后的euid**
------------------
* 很明显，当我们设置了 setuid 以后，尽管 uid 还是 1000(whowin 用户的 uid)，但 euid 已经变成了 0(root 的 uid)，Linux 会根据 euid 为进程赋予权限；
----------------------------
## 7. 后记
* 其实写这篇文章时，感觉还有好多地方可以展开，但是如果真的展开了就失去重点了，所以只能收着点，免得文章又臭又长，如果有时间或许今后会专门写一篇关于 Linux 文件权限和文件属性的文章；
* Linux 的文件目录其实也是一个文件，只是一般情况下无法像普通文件一样打开罢了，如果能很容易地打开并读出，就可以很清楚地看到文件权限在文件目录中的存储方式；
* C 语言中有 getuid()、setuid()、geteuid()、seteuid()等与本文有关的函数，可以自己编程试一下；
* 希望本文能够对读者理解 Linux 的权限管理有所帮助；
* **欢迎访问我的博客：https://whowin.cn**





[img01]:/images/100012/ls_show_file_attr_4755.png
[img02]:/images/100012/ls_usr_bin_files_4755.png
[img03]:/images/100012/ls_bin_files_4755.png
[img04]:/images/100012/whowin_uid_gid.png
[img05]:/images/100012/root_uid_gid.png
[img06]:/images/100012/whowin_execute_touch.png
[img07]:/images/100012/whowin_cat_shadow.png
[img08]:/images/100012/permission_diag.png
[img09]:/images/100012/file_readonly_root.png
[img10]:/images/100012/cant_open_rootfile.png
[img11]:/images/100012/change_owner_root_execute.png
[img12]:/images/100012/change_mode_4755_execute.png
[img13]:/images/100012/ls_mode_non_executable_file.png
[img14]:/images/100012/ls_sticky_folder.png
[img15]:/images/100012/permission_flag_diag.png
[img16]:/images/100012/set_all_flags_folder.png
[img17]:/images/100012/clear_all_flags_00755.png
[img18]:/images/100012/show_euid_without_setuid.png
[img19]:/images/100012/show_euid_with_setuid.png

