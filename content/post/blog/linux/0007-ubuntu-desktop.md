---
title: "安装一个好用的Ubuntu桌面"
date: 2022-05-06T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags:
  - Ubuntu
  - Desktop
draft: false
#references: 
postid: 100007
---

本文只是记录了安装一个 Ubuntu 桌面及其应用环境的全过程，本文正是在这个环境中写成，仅此希望对准备使用 Ubuntu 桌面的朋友们有所帮助
<!--more-->

## 1. 导言
* 手里一台 Lenovo T430，CPU: *i5-3320M*，*8G DDR3* 内存，*500G* 机械硬盘，废弃许久，准备重新启用
* 我平常的工作方式是一台ubuntu的服务器，所有内容都在这台服务器上，桌面有一个windows，以备不时之需，其它的都是Ubuntu，我不喜欢去哪儿都背个电脑，也不喜欢笔记本的小屏幕，所以我常去的地方都有一个Ubuntu的终端；
* 这次在笔记本上装Ubuntu，我特意做了记录，希望能够给也想使用 Ubuntu 桌面的朋友们做个参考；
* 使用 Ubuntu 最大的乐趣就在满满的掌控欲，一切尽在掌握中，绝大多数问题都能找到原因并找出解决方案；
* 尽管 Ubuntu 已经发布了 Ubuntu 22.04 LTS，但我习惯使用次新版的稳定版，所以我选择 Ubuntu 20.04 LTS
* 目前工作主要是使用 *vscode* 远程连接服务器工作，文档主要是用 *markdown*，有时也会使用思维导图和流程图，这些都会使用 *vscode* 实现；对桌面的其它要求包括：
  - 阅读 windows 下的所有格式的文档，使用 *wps* 实现；
  - 阅读 pdf 文档(包括在 *vscode* 下阅读 *pdf* 文档)，使用 *wps* 和 *vscode* 的 *pdf* 扩展解决；
  - 进行简单的图片处理，使用 *gimp* 实现；
  - 能够对屏幕做任意窗口的截屏(Ubuntu 自带功能)；
  - 搜狗拼音，根据习惯选装；
  - 翻译词典，使用有道词典实现
* 并不建议在 Ubuntu 下安装微信，尽管可以安装，但不好用，而且很耗费资源；我们使用 Ubuntu 桌面的其中一个原因就是节省资源，安装一个不好用的微信显得与初衷不符；
* 但是手机与电脑之间的文件交换还是很有必要的，我们通过安装一个开源软件 *qrcp*，很好地解决了这个问题。

## 2. 在 Ubuntu 下制作一个 Ubuntu 20.04 的安装 U 盘
* 你需要有一个正常的Ubuntu系统，运行在虚拟机上的也可以，任何版本的都可以，但最好是官方仍然支持的版本
* 在 Ubuntu 下做一个 Ubuntu 的安装 U 盘还是非常简单的
  1. 下载一个 Ubuntu 20.04 的 iso 镜像
      - [Ubuntu 中文官网](https://cn.ubuntu.com/)
      - [Ubuntu 桌面版的下载页面](https://cn.ubuntu.com/download/desktop)
      - [Ubuntu 20.04.4 桌面版（64位）下载种子](https://releases.ubuntu.com/20.04/ubuntu-20.04.4-desktop-amd64.iso.torrent) 
  2. 插上 U 盘
    > 正常情况下，Ubuntu 会自动识别出 U 盘
  3. 找到 U 盘的设备名
    ```
    sudo fdisk -l
    ```

    > *fdisk -l* 会列出所有的存储设备，包括你刚刚插上的 U 盘

  4. 将 iso 的镜像写入 U 盘
    > 假定 iso 文件放在当前用户 HOME 目录下的 Downloads 目录下，也就是 *~/Downloads/*(或者 *$HOME/Downloads/*) 下，文件名为：*ubuntu-20.04.4-desktop-amd64.iso*；U 盘的设备名(fdisk -l列出的)为：*/dev/sdd*；则下面命令会把 iso 镜像写入 U 盘

    ```
    sudo dd if=~/Downloads/ubuntu-20.04.4-desktop-amd64.iso of=/dev/sdd
    ```

* 等到命令执行完毕(可能时间会比较长)，Ubuntu 的安装 U 盘就做好了

## 3. 使用 U 盘安装 Ubuntu 20.04
* 这个过程没有什么可说的，Ubuntu 的安装还是很友好的，并没有什么难度
* 安装完毕，已经有桌面了
* 先做一些简单的设置，以便用起来顺手些
  - 设置屏幕分辨率以适应你的屏幕，这个比较容易，不予赘述
  - 将系统语言设为 **汉语**
    > *设置 --> region & language --> language --> **汉语** --> Select --> Restart*

      ![Region & Language][img01]

      - **图1：在 Region & Language 中选择 Language**

      ![在语言中选择汉语][img02]

      - **图2：在 Select Language 中选择 汉语**

      ![Region & Language中选择Restart][img03]

      - **图3：在 Region & Language 中选择 Restart**
    >
  - **安装语言支持**
    > *设置 --> region & language --> Manage Installed Language --> 安装 --> 应用到整个系统 --> Close*

      ![Region & Language][img04]

      - **图4：在 Region & Language 中选择 Manage Installed Language**
      
      ![安装语言支持][img05]

      - **图5：安装语言支持**

      ![应用到整个系统][img06]

      - **图6：应用到整个系统**

    > 

  - **设置输入法为拼音，以便能够输入汉字**
    + 此时再进入设置界面应该就全部显示汉字了，像下图一样，如果没有全部显示汉字可以重启一下

      ![设置界面全部显示为中文][img07]

      - **图7：设置界面全部显示中文**

      >
    
    + **特别建议**：建议保留 HOME 目录下的英文文件名
      > 系统语言修改为中文后，当系统启动时，会提示是否将 HOME 目录下的默认目录改为中文，建议**仍然保留英文目录**，因为在以后的操作中，目录名为中文会带来很多麻烦；但这个不是必须的，使用中文做目录名不会有任何问题

      ![建议保留英文目录名][img08]

      - **图8：建议保留英文目录名**

    >

    > *设置 --> 区域与语言 --> + --> 汉语 --> 中文(智能拼音) --> 添加*
    
      ![区域和语言中选择+][img09]

      - **图9：在区域与语言中选择 +**

      ![选择汉语][img10]

      - **图10：在添加输入源中选择汉语**

      ![选择拼音][img11]

      - **图11：在汉语中选择中文(智能拼音)**

  >

  - **将 Dock 移到底部并修改图标大小**
    + Ubuntu 把快捷图标栏称作 **Dock**，Dock 就是下图红框里的图标，Ubuntu 默认放在左边

      ![Dock][img12]

      - **图12：Ubuntu的Dock**

      >

    + 可以把 Dock 移到屏幕的底部，这样和 windows 有点像
      > *设置 --> 外观 --> Dock --> 屏幕上的位置 **底部***
      
      ![把Dock移到底部][img13]

      - **图13：把 Dock 移到底部**

      >

    + 将图标大小设置为 **32**，看上起顺眼一些，见上图

* 在安装 *chromium* 浏览器之前，*Firefox* 浏览器是唯一的选择，后面的下载工作主要靠 *Firefox*

## 4. 安装 Qv2ray
* v2ray 是一个科学上网工具，在 Linux 下工作，为了能访问 google、github 等，科学上网是必要的
* Qv2ray 是一个用 qt 开发的 v2ray 客户端软件，开源的，使用它只是出于习惯，你可以有别的选择
* 安装 Qv2ray 有两个步骤，第一步是安装 v2ray-core，第二步才是安装 Qv2ray；Qv2ray 只是给 v2ray-core 加了个好用的壳
* 安装 v2ray-core
  - [v2ray-core: https://github.com/v2fly/v2ray-core/](https://github.com/v2fly/v2ray-core/)
  - 下载 release 4.45.0，文件名：v2ray-linux-64.zip，通常使用 *Firefox* 下载会放在 *~/Downloads/* 目录下
  - 解压到目录 *~/v2ray-linux-64/* 下备用
    ```
    unzip ~/Downloads/v2ray-linux-64.zip -d ~/v2ray-linux-64
    ```

* 安装 Qv2ray
  - [Qv2ray: https://github.com/Qv2ray/Qv2ray](https://github.com/Qv2ray/Qv2ray)
  - 下载最新版本的文件，目前是：*Qv2ray-v2.7.0-linux-x64.AppImage*，通常使用 *Firefox* 下载会放在 *~/Downloads/* 目录下
  - 将下载的文件放在目录 *~/qv2ray* 中
    ```
    mkdir -p ~/qv2ray
    mv ~/Downloads/Qv2ray-v2.7.0-linux-x64.AppImage ~/qv2ray/
    ```
  - 文件 *Qv2ray-v2.7.0-linux-x64.AppImage* 属性设为可执行
    ```
    chmod +x ~/qv2ray/Qv2ray-v2.7.0-linux-x64.AppImage
    ```

* 运行并设置 qv2ray
  - 从桌面打开目录，找到 *~/qv2ray* 目录，双击文件 *Qv2ray-v2.7.0-linux-x64.AppImage* 运行
    
    ![运行qv2ray][img14]

    - **图14：双击图标运行qvray**

    >

  - *qv2ray* 主界面

    ![qv2ray主界面][img15]

    - **图15：Qv2ray 主界面**

    >

  - 内核设置
    > *首选项 --> 内核设置 --> V2Ray资源目录*

      ![qv2ray内核设置][img16]

      - **图16：Qv2ray 内核设置**

      >

    > 如上图，你看到的目录应该是：*~/.config/q2ray/vcore*，这个目录其实并不存在，我们需要自己建这个目录并把 *v2ray-core* 放到这个目录下

      ```
      mkdir -p ~/.config/qv2ray/vcore
      mv ~/v2ray-linux-64/* ~/.config/qv2ray/vcore/
      ```

  - 除此之外没有必须的设置，建议做如下设置：
    + *首选项 --> 常规设置 --> 登录时启动 --> **√ 已启用***
    + *首选项 --> 常规设置 --> 自动连接 --> **记忆上次连接***

      ![qv2ray常规设置][img17]

      - **图17：Qv2ray 常规设置**

      >

  - 建议其它的设置都不动
  - 如果你有相应的科学上网的账号，这时可以新建一个连接，并设置一个订阅地址，然后用这个新建的连接更新订阅

    ![订阅地址设置][img18]

    - **图18：Qv2ray 订阅地址设置**

    >

## 5. 安装 chromium 浏览器
* 如果你喜欢使用 *Firefox* 浏览器，不一定非要安装 *Chromium*
* 没有安装 *chrome*，而是选择安装 *chromium*，是因为在 Ubuntu 的软件中心中是没有 *Chrome* 的，但是有 *Chromium*；所以，安装 *Chromium* 很容易，但是安装 *Chrome* 就要大费周折了
* 安装 *Chromium*
  ```
  sudo apt install chromium-browser
  ```
* 将 *Chromium* 的快捷方式放到桌面上
  ```
  cp /usr/share/applications/chromium-browser.desktop ~/Desktop/
  ```
  - 在 */usr/share/applications/* 目录下放着所有应用程序的快捷方式，我们只需要把你需要摆在桌面上的快捷方式拷贝到 *~/Desktop/* 即可，这就是上面这条命令的作用
  - 经过上面一步后，你就可以在桌面上看到 chromium 的图标了，但是双击还是不能启动，需要你在图标上用鼠标点右键，然后选择 **允许启动**，然后再双击就可以启动了
  - 后面安装的软件都有可能做这个步骤，再不做解释

## 6. 安装 vscode
* [VSCODE 官网](https://code.visualstudio.com/)
* *vscode* 已经加入到 Ubuntu 软件中心，所以安装起来非常方便
  ```
  sudo apt install code
  ```
* 安装 vscode 本地扩展应根据自己的工作状况，我在本地安装如下扩展(还有一些扩展已在远端安装)
  - Remote-SSH
  - Remote-Development
  - Draw.io.Integration
  - vscode-pdf
  - Markdown All in One

* 将 vscode 图标放到桌面，记得将将图标设置为 **允许启动**
  ```
  cp /usr/share/applications/code.desktop ~/Desktop/
  ```

## 7. 安装搜狗拼音
* 如果你能够适应 Ubuntu 自带的输入法，不必安装新的输入法，目前能够在 Ubuntu 下运行的输入法并不多，搜狗算一个
* [搜狗输入法 for Linux](https://pinyin.sogou.com/linux)
* [ubuntu安装指南](https://pinyin.sogou.com/linux/guide)
* 从官网下载搜狗拼音输入法的 deb 包，目前的文件是：sogoupinyin_4.0.1.2123_amd64.deb
* 搜狗输入法使用 fcitx 输入法系统，但 Ubuntu 默认的输入法系统中，没有fcitx，需要先安装：
  ```
  sudo apt install fcitx
  sudo apt autoremove
  ```
* 输入法系统选择 *fcitx*
  > *设置 --> 区域与语言 --> 管理已安装的语言 --> 键盘输入法系统 --> 选择 fcitx --> 应用到全局*

    ![选择 fcxtx 输入法系统][img19]

    - **图19：选择 fcxtx 输入法系统**

    >

* 设置 *fcitx* 开机自启动
  ```
  sudo cp /usr/share/applications/fcitx.desktop /etc/xdg/autostart/
  ```

* 卸载系统 *ibus* 输入法框架
  ```
  sudo apt purge ibus
  ```
  > **特别提醒：** 在卸载完 *ibus* 以后，再次进入 *设置 --> 区域与语言 --> 管理已安装的语言* 时，会提示 **语言支持没有安装完整**，此时**一定**要选择 **稍后提醒**，否则会重新把 *ibus* 装上，以后会有麻烦

    ![不要安装语言支持][img20]

    - **图20：不要安装语言支持**

    >


* 安装搜狗拼音输入法以及依赖
  ```
  sudo dpkg -i sogoupinyin_4.0.1.2123_amd64.deb
  sudo apt install libqt5qml5 libqt5quick5 libqt5quickwidgets5 qml-module-qtquick2
  sudo apt install libgsettings-qt1
  ```
* 重新启动 Ubuntu

## 8. 安装向日葵
* 安装 [向日葵](https://sunlogin.oray.com/) 是考虑到可能需要远程控制运行 *windows* 的设备，在 *teamviewer* 收费以后，[向日葵](https://sunlogin.oray.com/) 算是比较好用的一个选择，比 anydesk 似乎好用些
* 在 *Ubuntu* 下控制 *windows*，没有什么更好的办法，远程桌面比较简单；也可以试试 *ssh*，不过稍有些麻烦，后面会提到 
* [向日葵官网](https://sunlogin.oray.com/)
* [安装说明](https://service.oray.com/question/8364.html)
* [下载地址](https://sunlogin.oray.com/download)
* 当前下载文件名：SunloginClient_11.0.1.44968_amd64.deb
* 安装，可以按照说明去安装，不过我更喜欢用命令行
  ```
  sudo dpkg -i SunloginClient_11.0.1.44968_amd64.deb
  ```
* 将快捷方式拷贝到桌面，并将其属性改为 **允许执行**
  ```
  cp /usr/share/applications/sunlogin.desktop ~/Desktop/
  ```

## 9. 安装有道词典
* [有道词典官网](https://dict.youdao.com/)
* [linux版下载页面](http://cidian.youdao.com/multi.html#linuxAll)
* 当前下载文件名：youdao-dict_6.0.0-ubuntu-amd64.deb
* 安装
  ```
  sudo dpkg -i youdao-dict_6.0.0-ubuntu-amd64.deb
  ```
* 这一步安装会执行失败，因为依赖问题，*dpkg* 是不能自动解决依赖问题的，所以我们要先解决依赖问题
  ```
  sudo apt install -f -y
* 这一步会自动解决上一个安装命令产生的依赖问题，然后我们需要再次安装
  ```
  sudo dpkg -i youdao-dict_6.0.0-ubuntu-amd64.deb
  ```
* 将快捷方式放到桌面，并设置为 **允许启动**
  ```
  cp /usr/share/applications/youdao-dict.desktop ~/Desktop/
  ```

## 10. 安装 wps
* [wps官网](https://www.wps.cn/)
* [linux版wps下载页面](https://linux.wps.cn/)
* 当前下载文件：wps-office_11.1.0.10976_amd64.deb
* 安装
  ```
  sudo dpkg -i wps-office_11.1.0.10976_amd64.deb
  ```
* 将快捷方式拷贝到桌面，并将其属性改为 **允许执行**
  ```
  cp /usr/share/applications/wps-office-* ~/Desktop/
  ```

## 11. 安装 gimp
* [gimp官网](https://www.gimp.org/) - 一个类似 photoshop 的软件
* [【GIMP教程】基础入门](https://mdnice.com/writing/c4815f6d995a4fec99749a72c473cb8e)
* gimp 已经被收录到 Ubuntu 的软件中心中，所以安装非常容易
  ```
  sudo apt install gimp
  ```
* 将快捷方式拷贝到桌面，并将其属性改为 **允许执行**
  ```
  cp /usr/share/applications/gimp.desktop ~/Desktop/
  ```

## 12. 安装 qrcp
* qrcp 是一个可以方便地和手机互传文件的工具；这个工具可以在屏幕上显示一个二维码，手机通过扫描二维码来完成与电脑之间的文件互传；前面提到过，不建议在 Ubuntu 下安装微信，这个工具可以代替一部分微信的作用
* [qrcp项目](https://github.com/claudiodangelis/qrcp)
* [qrcp当前最新版本0.9.1](https://github.com/claudiodangelis/qrcp/releases)
* 下载release文件： qrcp_0.9.1_linux_x86_64.tar.gz
  ```
  cd ~
  mkdir qrcp
  tar -zxvf ~/Downloads/qrcp_0.9.1_linux_x86_64.tar.gz -C ./qrcp
  sudo mv ./qrcp/qrcp /usr/local/bin
  ```
* Ubuntu 接收手机发送的文件
  ```
  qrcp receive
  ```
* Ubuntu 发送文件到手机
  ```
  qrcp send -k [ 文件名 ]
  ```
  > *-k* 的含义是保持 server 不退出，否则 server 会马上退出导致传输失败

## 13. 安装 FileZilla
* 这是一个好用的 *ftp、sftp* 的开源软件，我喜欢用它在各种 *linux/windows* 之间传递文件
* *FileZilla* 既有客户端也有服务器版本，我只用它的客户端
* [FileZilla官网](https://filezilla-project.org/)
* 安装
  ```
  sudo apt install filezilla
  ```
* 将快捷方式拷贝到桌面，并将其属性改为 **允许执行**
  ```
  cp /usr/share/applications/filezilla.desktop ~/Desktop/
  ```

## 14. 其它
* 把 *terminal* 放到桌面上，记得把桌面图标设置为 **允许启动**
  ```
  cp /usr/share/applications/org.gnome.Terminal.desktop ~/Desktop/
  ```

* 把 *截图工具* 放到桌面上，记得把桌面图标设置为 **允许启动**
  ```
  cp /usr/share/applications/org.gnome.Screenshot.desktop ~/Desktop/
  ```

* *vi* 无法使用
  - Ubuntu 默认会只安装 *vi*，不会装 *vim*，但是你启动 *vi* 后，会发现根本没法用，几乎所有的按键都不对
  - 这是因为尽管安装了 *vi*，但是并没有配置文件，解决这个问题最简单的办法是安装一个 *vim*
    ```
    sudo apt install vim
    ```
  - 如果你就是不想装 *vim*，也不要紧，把配置文件拷贝到适当的地方就可以了
    ```
    cp /etc/vim/vimrc ~/.vimrc
    cp /etc/vim/vimrc /root/.vimrc
    ```
  - 之所以在 */root* 目录下也拷贝了一份 *vi* 的配置，是为了保证你在 *sudo vi* 的时候也是正常的

## 15. 后记
* 至此，整个的环境到这里就安装完成了，希望能给你带来一点帮助
* 你现在看到的这篇文章就是在这个环境下使用 *vscode* 远程完成的，实际的文章放在服务器上，图片是在本机使用 *Ubuntu* 截图工具获取的，然后用 FileZilla* 传到服务器
* 我使用的所有软件都是在 *Ubuntu* 和 *windows* 上均可以使用的开源软件，*Qv2ray、chromium、vscode、搜狗拼音、向日葵、有道词典、wps、gimp、qrcp和FileZilla*，只有 *gimp* 和 *qrcp* 在我的 *windows* 桌面上没有，其它在两种桌面都是一样的，这样可以在不同的桌面保持相同的使用习惯
* 通常情况下，我的所有 *Linux* 机器的 *sshd* 都是开着的，*windows* 上也会安装一个 *openssh*，然后打开 *sshd* 服务；这样各个机器之间就无需各种各样的通信协议去完成共享了
  - Linux 之间使用 ssh 共享不必多说；如果你安装一个 sshfs，还可以方便地把一个远程设备的目录映射到本地
    + 安装 sshfs
      ```
      sudo apt install sshfs
      ```
    + 挂载远程设备
      ```
      sudo sshfs [username]@[remote host]:[remote directory] [local mountpoint]
      ```
    + 卸载远程设备
      ```
      sudo fusermount [local mountpoint]
      ```

  - *Linux* 访问 *windows* 的 *sshd* 是没有问题的；但是用 *sshfs* 挂载 *windows* 下的文件夹始终没有搞定；所以实际在 *Ubuntu* 下向 *windows* 传文件时，都是使用 *sftp*，也就是 *FileZilla*
  - 实际上 *sshfs* 也有 *windows* 版本，而且还有不少，下面是我使用过的
    + [sshfs-win-manager](https://github.com/evsar3/sshfs-win-manager)
    + 用这个软件在 *windows* 上挂载 *Linux* 下的目录还是很方便的




<!-- 本地使用的图片 --
[img01]: ../../../../static/images/100007/select-language-region-language.png
[img02]: ../../../../static/images/100007/select-language.png
[img03]: ../../../../static/images/100007/select-language-restart.png
[img04]: ../../../../static/images/100007/region-language-1.png
[img05]: ../../../../static/images/100007/install-language-support.png
[img06]: ../../../../static/images/100007/apply-whole-system.png
[img07]: ../../../../static/images/100007/region-language-2.png
[img08]: ../../../../static/images/100007/keep-old-names.png
[img09]: ../../../../static/images/100007/region-language-3.png
[img10]: ../../../../static/images/100007/add-input-source.png
[img11]: ../../../../static/images/100007/add-pinyin.png
[img12]: ../../../../static/images/100007/dock.png
[img13]: ../../../../static/images/100007/dock-on-bottom.png
[img14]: ../../../../static/images/100007/qv2ray-doubleclick.png
[img15]: ../../../../static/images/100007/qv2ray-new-connection.png
[img16]: ../../../../static/images/100007/qv2ray-core-settings.png
[img17]: ../../../../static/images/100007/qv2ray-regular-settings.png
[img18]: ../../../../static/images/100007/qv2ray-subscribe-setting.png
[img19]: ../../../../static/images/100007/sogou-fcitx.png
[img20]: ../../../../static/images/100007/not-install-language-support.png
!-- -->

<!-- blog上使用的图片 -->
[img01]: /images/100007/select-language-region-language.png
[img02]: /images/100007/select-language.png
[img03]: /images/100007/select-language-restart.png
[img04]: /images/100007/region-language-1.png
[img05]: /images/100007/install-language-support.png
[img06]: /images/100007/apply-whole-system.png
[img07]: /images/100007/region-language-2.png
[img08]: /images/100007/keep-old-names.png
[img09]: /images/100007/region-language-3.png
[img10]: /images/100007/add-input-source.png
[img11]: /images/100007/add-pinyin.png
[img12]: /images/100007/dock.png
[img13]: /images/100007/dock-on-bottom.png
[img14]: /images/100007/qv2ray-doubleclick.png
[img15]: /images/100007/qv2ray-new-connection.png
[img16]: /images/100007/qv2ray-core-settings.png
[img17]: /images/100007/qv2ray-regular-settings.png
[img18]: /images/100007/qv2ray-subscribe-setting.png
[img19]: /images/100007/sogou-fcitx.png
[img20]: /images/100007/not-install-language-support.png
<!-- -->



