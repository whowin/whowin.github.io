# 在DR-DOS下建立DJGPP的开发环境  
**2011-04-15 12:03:46**

* 前面的博文中，我们已经在虚拟机上安装了DR-DOS7.03，这篇文章里我们要在这个刚刚安装好的DR-DOS上建立DJGPP+RHIDE+ALLEGRO+WATTCP-32的开发环境。
* 以前博文中的有关在DOS6.22下建立开发环境的文章完全适用，这些文章如下：
  - 安装DJGPP
  - 在DOS下的DJGPP+RHIDE安装实作
  - 在DJGPP+DOS下安装ALLEGRO
  - 给ALLEGRO增加一个JPEG库
  - 在DOS下进行网络编程（上）
```
下面是我在实际安装过程中遇到的一些麻烦，供大家借鉴。
```
```
在实际操作之前，应该先把光驱驱动起来，在DR-DOS中没有找到光驱的驱动程序，所以我们需要借助DOS6.22下的驱动，在我们驱动光驱的时候，我们只有软驱，所以只能通过软驱把这个驱动拷贝到DR-DOS中。
```
* 在《在虚拟机下安装DOS 6.22(上)》这篇文章中，我们有一张DOS 6.22启动盘的镜像下载，这张盘上有我们需要的东西，这里重新写一下下载地址，以方便大家：http://blog.whowin.net/software/dos622.img
* 将这张DOS6.22的启动盘挂在软驱上，然后把其中的IDE目录整个拷贝到DR-DOS下，我们需要的驱动就有了。
  ```
    修改DR-DOS中的config.sys，在config.sys加入一行：
        device=c:\ide\ide.sys /D:MSCD000
    修改DR-DOS中的autoexec.bat，在autoexec.bat最后加入一行：
        nwcdex /d:mscd000
    这里的nwcdex是DR-DOS提供的程序，相当于MSDOS中的mscdex，在drdos目录下可以找到。
    重新启动DR-DOS，记得启动前把软驱中的DOS 6.22启动盘取出来，再启动以后，就可以驱动光驱了。
    顺便提一句，DR-DOS下的edit尽管和dos6.22下的edit界面不同，但使用方法几乎一样。
  ```
* 在《在虚拟机下安装DOS 6.22(上)》这篇文章中，我推荐了一张自己制作的DOS软件的光盘镜像，这里面包括了大量你用得着的DOS软件，最好先下载下来备用：http://blog.whowin.net/software/dossoft.iso
* 在编译JPGALEG库的时候过不去，明确提示是由于gcc的-mpentium参数，想尽办法解决不了，最后只好修改了相应的makefile文件，如果你的等待编译的JPGALEG放在jpgall2.5目录下，那么这个makefile文件应该是：\jpgall2.5\makefile.dj，修改内容如下图：
  在DR-DOS下建立DJGPP的开发环境 - whowin - DOS编程技术
* 将其中蓝线的那一行前面加上“#”注释掉，并将这句改成红线的那一行，没有改什么，只是把参数-mpentium去掉了而已。
* 如果你安装DR-DOS的步骤完全按照我的博文中的步骤，那么你的DR-DOS在安装完成后应该没有装载多任务管理器而且DPMI也是关闭的，在安装软件的过程中最好是保证DPMI关闭并且不加载多任务管理器，因为根据资料显示，这两个东西比较容易引起不兼容的问题，另外，我在编译WATT-32库的时候的确是发生了错误，但我把DPMI关闭后，一切就正常了。
* 确认你没有加载多任务管理器可以利用DR-DOS的setup程序，setup程序可以帮助你配置DR-DOS，在DOS提示符下输入setup即可启动该程序，这个程序可以在drdos目录下找到，setup程序会根据你的选择帮助你修改config.sys和autoexec.bat文件。
* etup启动后界面如下：

  在DR-DOS下建立DJGPP的开发环境 - whowin - DOS编程技术

* 按TAB键可以选择光标的位置，然后通过上下箭头键移动光标位置，我们选择“Task Management”，然后回车进入：
  在DR-DOS下建立DJGPP的开发环境 - whowin - DOS编程技术
 
* 要确认上图中红圈内的两项均没有被选中，任何一项被选中都会自动启动DPMI。
* 要确认DPMI没有开启，建议看一下config.sys
  在DR-DOS下建立DJGPP的开发环境 - whowin - DOS编程技术

* 一定要确认上图中红线部分为：DPMI=OFF，如果不是，请更改成DPMI=OFF，然后存盘退出。

* 也许你不够幸运，在安装软件的过程中还遇到了一些我没有提到的问题，你只有想办法自己解决了，当然你也可能足够幸运，安装十二分顺利，什么麻烦都没有，那就恭喜你了。
* 最后要说的是，要在虚拟机上把网络驱动起来，除了正确的设置外，你还需要一个PacketDriver，如果你使用的是VirtualBox，缺省的虚拟网卡应该是PCnet-FAST III，这个网卡的Packet Driver在我提供的dossoft.iso中有，文件名为：pcntpk.com，把这个文件拷贝到c:根目录下，然后在autoexec.bat中加上一句：pcntpk int=0x60，就可以驱动起你的网卡了，如果有中断冲突，可以调整一下0x60这个数字。
* 至于虚拟机的网络设置，还是看图吧，按照图中去设置，应该就OK了。
  在DR-DOS下建立DJGPP的开发环境 - whowin - DOS编程技术

* 通常你的网卡和我的很可能不一样，所以，“界面名称”这一项会和图中不同，还有MAC地址也有可能不一样，其他的应该是一样的就好了，这样设置，可以让你的DR-DOS通过你本机的网卡直接连接到网络上，当然，你的DR-DOS中的wattcp.cfg中的IP和gateway等要设置对才行。
* 顺便还有一句要说明一下，在DR-DOS的setup中，也有关于网络的配置，但是这些配置及所加载的驱动程序都是基于NOVELL的网络结构，包括加载的网卡驱动，也是NOVELL的ODI格式，对通常的网络编程并不适用，所以，不要随便尝试去配置它。
 