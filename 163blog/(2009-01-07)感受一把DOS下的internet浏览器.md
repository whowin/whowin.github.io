# 感受一把DOS下的internet浏览器  
**2009-01-07 10:17:13**

  > 前面的几篇文章，我们已经安装了VirtualBox，并且在VirtualBox下安装了MS-DOS 6.22，我们还成功第实现了host-guest之间的文件共享，这篇文章，我们将在这个DOS平台下安装一个internet浏览器，感受一把在DOS下上网的乐趣。

  > 说到DOS下的浏览器，不得不提到两个著名的浏览器，一个是lynx，另一个是arachne，lynx是一个纯文本的浏览器，所以我们这次不准备安装它，archne是一个相对功能要完整一些的浏览器，好像是几个捷克人做的，不过现在也停止更新了，作为DOS下的图形浏览器，arachne应该是现在最好的了，上面提到的两个浏览器都是GPL软件，特别要说明的是，好像他们都不支持中文，所以本文仅仅是带着大家玩玩，长长见识而已。

  > arachne是由一个叫Arachne Labs负责维护的，Arachne Labs的官方网站是：http://www.arachne.cz（2017年3月17日注：这个链接已经失效了，它搬到了这里http://www.glennmcc.org/）

  > 从其官方网站上可以找到其发布的最后一个arachne brower的版本是1.90（http://www.arachne.cz/?page=soft），但网站上提供的几个链接都打不开，所以没有下载到，我手里的arachne的版本是1.70，该软件已经放在我给大家提供的dossoft.iso的虚拟光盘上了。

  > 在仔细看过arachne的说明后发现，arachne要求使用Packet Driver，而不是前面为共享磁盘安装的NDIS2驱动，所以我们首先要找到我们虚拟网卡PCnet的Packet Driver，这个driver我已经从AMD的网站上找到了，在我们的dossoft.iso这张虚拟光盘上。

  >大家都用过浏览器，显然，在浏览器上操作没有鼠标实在是太不舒服了，所以我们还要有一个DOS下的鼠标驱动程序，我们就用常用的mouse.com就可以了，这个软件也在那张虚拟光盘上了。

  > 软件都找齐了，但还不是那么简单，经过多次测试，我发现网卡的Packet Driver和网卡的NDIS2 Driver是有冲突的，所以非常遗憾的是，如果我们要装Packet Driver就不得不去掉NDIS2 Driver，而且NDIS2 Driver非常吃内存，而arachne的运行，需要很大的常规内存，所以要运行arachne，去掉NDIS2 Driver也是不得已的。

* 下面先说说我们要用到的三个软件在虚拟光盘的什么地方。
  - arachne ---- 根目录下文件archn170.exe
  - PCnet Packet Driver ---- 根目录下文件pcntpk.com
  - 鼠标驱动 ----根目录下mouse.com

* 下面我们安装Packet Driver、鼠标驱动，并去除NDIS2驱动。
  - 从光盘拷贝三个文件到硬盘
    ```
    copy d:\mouse.com c:\dos
    copy d:\pkntpk.com c:\
    copy d:\archn170.exe c:\
    ```
  - 编辑C盘根目录下的autoexec.bat
    * C:\>edit autoexec.bat

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-01.jpg)

    * 把最后三行前面加上“rem ”，并在最后加上两行
      ```
      c:\dos\mouse
      c:\pcntpk int=0x60
      ```

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-02.jpg)

    * 然后存盘退出即可
  - 编辑config.sys
    * C:\>edit config.sys

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-03.jpg)

    * 在device=C:\MSNET\ifship.sys前面加上“rem”

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-04.jpg)

    * 然后存盘退出就好了。
  > 至此，你已经基本准备就绪了，再安装上arachnea一切就OK了！在进行安装之前，我们先把你的虚拟DOS重新启动一下，启动以后的样子大致如下，可以很清晰第看到鼠标驱动和Packet Driver都顺利第启动起来了。
    
    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-05.jpg)

  > 在开始安装arachne之前，你最好先搞清楚你的网络环境
    * 如何上网，拨号？宽带？局域网？小区宽带？不管怎么上网，先让你的HOST挂在网上。
    * 是否使用DHCP？
    * 如果不用DHCP，那么网关的IP？DNS？DOS guest下网卡的IP？子网掩码？
    * 如果是使用DHCP，那问题就简单的多了，如果没有使用DHCP，那一定要搞清楚上面的那些问题，我的网络环境就没有使用DHCP，而且局域网内部的IP段没有使用192.168.x.x，而是使用了100.100.0.x，至于为什么这么做不在今天的讨论范围，这么做并不一定好，大家不必模仿；下面我公布一下我的网络情况
      - 局域网上网
      - 网关：192.168.0.1
      - DOS guest网卡IP：192.168.0.61，子网掩码：255.255.255.0
      - DNS：210.51.176.71   202.106.196.152
* 下面我们开始安装arachne
  - 我已经将安装文件拷贝到了C盘的根目录下，运行一下就可以了。
  - 在回答了两个“Y”以后，安装界面大致如下：

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-06.jpg)

  - 安装很快就结束了，然后进入设置界面，两个问题是memory type和video card，都回车就好了

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-07.jpg)

  - 此时，返回到了DOS提示符下，你的arachne已经被成功第安装到了c:\arachne目录下，现在只要输入arachne就可以启动浏览器了。
* 使用arachne
  - 在第一次使用时，arachne会要求先进行设置，前两个问题和刚才的一样，都回车就好了，然后我们会看到下面的界面：

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-08.jpg)

  - 这个时候，我们用鼠标就比较方便了，你可以用鼠标在里面随便点一下，会看到下面的界面

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-09.jpg)

  - 以前遇到这个界面大家总是按“取消”，这次请按“捕获”，这样你的鼠标就可以使用了，记得，当希望退出鼠标在guest中的使用时，请按右边的“ALT”键

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-10.jpg)

  - 如果你的屏幕支持更高的分辨率，可以点1024 X 768，然后点击下面的“Try selected graphics mode”按钮

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-11.jpg)

  - 我们点击“Packet Wizard”

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-12.jpg)

  - 点击“Detect packet driver”

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-13.jpg)

  - 如果你看到这个界面，说明你的Packet Driver安装是正确的，祝贺你，点击“Continue”

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-14.jpg)

  - 如果你使用DHCP，直接点击“BOOTP or DHCP”，你的设置就完成了，如果你不使用DHCP，请点击“Manual Setup”按钮，下面的界面

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-15.jpg)

  - 这里是设置的主要地方，按照我们在安装前收集的信息进行填写后，按“Continue”按钮

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-16.jpg)

  - 如果你当初选择使用DHCP的话，也会看到这个界面，直接按“Finish”就好了
  - 到了这一步，你可能会出现死机，不要紧张，根据经验，如果出现这种情况，基本可以肯定是你的常规内存太小了，不要紧，按照以前的方法重新启动你的DOS guest，我们需要修改一下autoexec.bat，使你的常规内存大一些。
  - 启动以后，输入edit autoexec.bat，修改后的autoexec.bat大致如下：
    ```
    rem C:\DOS\SMARTDRV.EXE /L /X
    C:\DOS\mscdex.exe /d:mscd000
    PATH C:\MSNET;C:\DOS
    rem C:\MSNET\net initialize
    rem C:\MANET\nwlink
    rem C:\MSNET\net start

    c:\dos\mouse
    c:\pcntpk int=0x60
    ```
  - 修改完后重新启动DOS guest，输入mem看看内存情况，大致如下：

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-17.jpg)

  - 我们可以看到，空闲的常规内存有535KB，应该足够了，这时我们可以再次启动arachne了。
    ```
    C:\>cd\arachne
    C:\ARACHNE>arachne
    ```
  - 你应该看到下面的界面：

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-18.jpg)

  - 这是由于你上次没有关闭arachne强行启动机器造成的，点击“Ignore”即可，然后你就可以在地址栏里输入网址浏览网页了，下面是yahoo的页面

    ![感受一把DOS下的internet浏览器](images\感受一把DOS下的internet浏览器-19.jpg)

  - 再次说一下，arachne不支持中文，浏览中文网页会看到乱码，不过arachne是个开源项目，有兴趣有能力的读者不妨试试让它支持汉字。
  - arachne中还有一些其它有助于DOS使用的功能，自己去摸索吧。

  