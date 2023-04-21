# 给Allegro增加一个JPEG库  
**2008-05-15 11:42:58**

> Allegro我们已经安装好了，相信应该很好用，我在实际运用中感觉Allegro最遗憾的问题是无法对JPEG图片进行处理，这给我带来了很多麻烦，比如在工控系统下，通常存储空间都很有限，比如使用FLASH memory，或者使用小容量的CF卡（因为要考虑成本）等，当要存储采集到的图片时，当然希望占用的空间越小越好，但Allegro却不能存储成JPEG格式；或者我们通过网络从服务器上获得图片，我们当然希望图片文件小一些，这样传输的可以快一些，但Allegro却不能调入JPEG文件，种种这些麻烦让我下决心找一个好用的JPEG库，好在很幸运，我找到了一个可以和Allegro完美结合的JPEG库：JPGALLEG。

> 本文介绍如何在已经搭建好的平台：DOS+DJGPP+ALLEGRO的环境下，安装JPGALLEG库。

## 1、下载JPGALLEG库
* JPGALLEG库的官方下载地址在: http://wiki.allegro.cc/JpgAlleg
* 这个文件下载下来有些小问题，1是它的文件扩展名是.gz，这是LINUX下常用的压缩文件格式，可能会让一些读者感到疑惑；2是它的文件名太长，全名是jpgalleg-2.5.tar.gz，这对DOS这个不支持长文件名的系统可能有麻烦；另外，解压缩后它的阙上目录名是jpgalleg-2.5，这对不支持长文件名的用户显然也有麻烦；显然这个库不是专门为DOS用户准备的，为此，我从新制作了一个zip文件，修正了这些麻烦，可以在下面网址下载：http://blog.whowin.net/software/jpgalleg/jpgall25.zip

## 2、编译JPGALLEG库
* 由于JPGALLEG库是以源代码形式发行的，所以我们要进行编译后才可以使用。
* 将下载到的jpgall25.zip放到c:\下，键入命令：unzip32 jpgall25
* 会在c:下建立一个新目录c:\jpgall2.5，所有的文件会被解压缩到这个目录下，如果你使用pkunzip解压缩，请不要忘记使用-d选项。
* 进入jpgall2.5目录，执行编译
    ```
    c:\>cd jpgall2.5
    c:\jpgall2.5>fix djgpp
    c:\jpgall2.5>make
    c:\jpgall2.5>make install
    ```
* 一般情况下在执行make这一步骤时，可能会出现一些警告信息，不用管它，正常情况下，不会有问题。
* 安装完毕。

## 3、测试JPGALLEG库
* 首先进入DJGPP安装目录下的lib下，应该可以看到一个库文件libjpgal.a，这基本说明安装成功了。
* 按照JPGALLEG库随带的readme文件看，似乎在编译时加上-ljpgal就可以使用JPGALLEG库，这是gcc的编译方法，在DOS的rhide下并不成功。
* 使用rhide工作时，为编译程序建立工程，哪怕只有一个程序也要建立一个工程（Project），并把刚编译完成的库libjpgal.a作为一个工程项目加到工程中。
  - 在rhide的做如下设置：Option---->Directories---->Libraries---->c:\djgpp\lib
    > 如下面两图：

    ![给Allegro增加一个JPEG库](images\给Allegro增加一个JPEG库-01.jpg)

    ![给Allegro增加一个JPEG库](images\给Allegro增加一个JPEG库-02.jpg)

* 当然，如果你的DJGPP不是安装在c:\djgpp目录下，请按照实际情况设置。
* 此时，我们可以进入到JPGALLEG的目录c:\jpgall2.5，在这个目录下应该可以看到一个叫examples的子目录，进入该子目录，在这个子目录下有ex1.c ex2.c ex3.c ex4.c ex5.c五个范例文件，我们试着编译其中的一个，比如ex1.c。
  - c:\jpgall2.5\examples>rhide ex1
  - 将ex1.c和c:\djgpp\lib\libjpgal.a加入到工程项目中，并按照上一步的方法进行设置，由于需要Allegro库，记得参考设置Allegro的编译选项。
  - 在菜单上选择：Compile---->Make
* 应该完美地完成编译，然后你可以按Ctrl+F9运行以下，屏幕上你应该可以看到一幅漂亮的图片，就像下图：

    ![给Allegro增加一个JPEG库](images\给Allegro增加一个JPEG库-03.jpg)

> 至此，一切已经就绪。Enjoy it!