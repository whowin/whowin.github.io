# 在DJGPP+DOS下安装ALLEGRO  
**2008-05-14 17:10:29**

> 我在2017年3月13日开始整理这篇文章时，惊奇地发现ALLEGRO仍然活着，这让我非常高兴，整个项目的网址在：http://liballeg.org/，而且已经发展到了allegro 5.2

> 在整理其他文章时，发现要用到Allegro库，而我还没有介绍Allegro的安装，所以只好停下来先介绍一下Allegro的安装过程。

> Allegro是一个主要用于游戏和多媒体编程的函数库，开始是由一个叫Shawn Hargreaves的人（应该是个英国人）写的，用C+汇编的混合编程，在DJGPP下编译完成。有关Shawn Hargreaves的个人信息，可以在下面网址中看到：http://www.talula.demon.co.uk(2017年3月13日注：这个链接已经失效了)

> 这张照片应该是他本人。

  ![在DJGPP+DOS下安装ALLEGRO](images\在DJGPP+DOS下安装ALLEGRO-01.jpg)

> 我们之所以要安装Allegro是因为它可以让我们在真彩的图形模式下编程，使我们的界面丰富多彩，它的声音功能我倒是不觉得有太大的用，因为它支持的环境我几乎没有。

> Allegro可以提供跨平台支持，目前支持的平台包括：

  1. Unix (Linux, FreeBSD, Irix, Solaris, Darwin)
  2. Windows (MSVC, MinGW, Cygwin, Borland)
  3. BeOS
  4. QNX
  5. MacOS X
  6. Dos (DJGPP, Watcom)

> Allegro有源代码发行版本，如果使用源代码发行版进行安装，需要根据环境首先进行编译，本文只介绍在DOS+DJGPP环境下二进制版的安装方法，有关源代码的编译方法请参考下面网址：

> http://alleg.sourceforge.net/latestdocs/en/readme.html（2017年3月13日注：这个链接已经失效了）

> 新的教程地址在：http://liballeg.org/docs.html

## 1、Allegro需要的软件环境
* DJGPP 2.01以上 (djdev*.zip).
* GCC compiler 2.91.x 以上(gcc*b.zip). 
* Binutils 2.9.x 以上(bnu*b.zip).
* GNU make (mak*b.zip).
* Texinfo (txi*b.zip).
* 可选: sed (sed*b.zip).

> 如果是安装我以前的文章《在DOS下的DJGPP+RHIDE安装实作》的步骤进行的安装，那我们已经具备了上述条件，可选的软件我们放弃，其他的部分我们应该有：

* DJGPP 2.03
* GCC compiler 4.23
* Binutils 2.17
* GNU make 3.79
* Texinfo 4.11

> 完全符合条件。

> 如果有些软件你没有安装，可以在下面网址下找到这些软件：

> 在http://ftp.delorie.com/pub/djgpp/current/v2gnu/下你可以找到：

> (2017年3月14日注：很高兴这个链接还健在，不过为了以防万一，我已经下载了这个地方与djgpp有关的所有东东，稍后会提供有效的下载地址)

* GCC compilier 4.23 (gcc423b.zip)    4422 kb
* Binutils 2.17 (bnu217b.zip)        3962 kb
* GNU make 3.79 (mak3791b.zip)        267 kb
* Texinfo 4.11 (txi411b.zip)         888 kb

> 很抱歉，这些软件的安装不是本文的话题，请自行阅读相关的readme文件进行安装。

> 如果你不确定你安装的DJGPP中是否有以上软件，请按下面方法检验：

* DJGPP的版本

  > 到你安装DJGPP的目录下（比如：c:\djgpp），应该可以看到文件readme.1st，用edit打开，可以看到一行类似这样的话：This is the README.1ST file for DJGPP Version 2.03. 这足以说明你安装的DJGPP的版本。

* GCC的版本

  > 在DOS提示符下键入如下命令：
  ```
  c:>gcc -v
  ``` 
  > 如果没有这个命令，那可能是你根本就没有安装GCC；否则你应该看到几行字显示出来，最下面一行应该是：gcc version 4.2.3
* Binutils的版本
  >Binutils 是一组开发工具，包括连接器，汇编器和其他用于目标文件和档案的工具。

  > 安装下列程序:

    1. addr2line.exe
      > 把程序地址转换为文件名和行号。在命令行中给它一个地址和一个可执行文件名，它就会使用这个可执行文件的调试信息指出在给出的地址上是哪个文件以及行号。
    2. ar.exe
      > 建立、修改、提取归档文件。归档文件是包含多个文件内容的一个大文件，其结构保证了可以恢复原始文件内容。
    3. as.exe
      > 主要用来编译GNU C编译器gcc输出的汇编文件，产生的目标文件由连接器ld连接。
    4. cxxfilt.exe
      > cxxfilt.exe 连接器使用它来过滤 C++ 和 Java 符号，防止重载函数冲突。
    5. gprof.exe
      > 显示程序调用段的各种数据。
    6. ld.exe
      > 是连接器，它把一些目标和归档文件结合在一起，重定位数据，并链接符号引用。通常，建立一个新编译程序的最后一步就是调用ld。
    7. nm.exe
      > 列出目标文件中的符号。
    8. objcopy.exe
      > 把一种目标文件中的内容复制到另一种类型的目标文件中.
    9. objdump.exe
      > 显示一个或者更多目标文件的信息。显示一个或者更多目标文件的信息。使用选项来控制其显示的信息。它所显示的信息通常只有编写编译工具的人才感兴趣。
    10. ranlib.exe
      > 产生归档文件索引，并将其保存到这个归档文件中。在索引中列出了归档文件各成员所定义的可重分配目标文件。
    11. readelf.exe
      > 显示ebf格式可执行文件的信息。
    12. size.exe
      > 列出目标文件每一段的大小以及总体的大小。默认情况下，对于每个目标文件或者一个归档文件中的每个模块只产生一行输出。
    13. strings.exe
      > 打印某个文件的可打印字符串，这些字符串最少4个字符长，也可以使用选项-n设置字符串的最小长度。默认情况下，它只打印目标文件初始化和可加载段中的可打印字符；对于其它类型的文件它打印整个文件的可打印字符，这个程序对于了解非文本文件的内容很有帮助。
    14. strip.exe
      > 丢弃目标文件中的全部或者特定符号。
  > 由于软件包由多个软件组成，不太容易判断当前的版本；但从是否在bin目录下存在这些文件，可以大致判断是否已经安装了该软件包。

* GNU make的版本
  > 在DOS提示符下键入如下命令：
    ```
    c:>make -v
    ```
  > 如果没有这个命令，那可能是你根本就没有安装GNU make；否则你应该看到几行字显示出来，第一行应该是：GNU make version 3.79.1

* texinfo的版本
  > Texinfo 软件包包含读取、写入、转换 Info 文档的程序。
  > 它包括下面这些软件：info.exe、infokey.exe、install-info.exe、makeinfo.exe、texi2dvi、texi2pdf、texindex.exe
    1. info.exe
      > 用于读取 Info 文档。
    2. infokey.exe
      > 把包括 Info 设置的源文件编译成二进制格式
    3. install-info.exe
      > 安装 info 文档，它会更新
    4. makeinfo.exe
      > 将 Texinfo 源文档翻译成不同的格式，包括html、info文档、文本文档。
    5. texi2dvi
      > 把给定的 Texinfo 文档格式化成可打印的设备无关的文件
    6. texi2pdf
      > 将 Texinfo 文档转化成 PDF 文件
    7. texindex.exe
      > 对 Texinfo 索引文件进行排序
  > 由于软件包由多个软件组成，不太容易判断当前的版本；但从是否在bin目录下存在这些文件，可以大致判断是否已经安装了该软件包。
  > 如果你是按照常规方法进行的安装，在DJGPP安装目录下应该有一个gnu目录，进入该目录可以看到一些子目录，从这些子目录名上可以很容易地判断出我们已经安装的gnu软件及其版本，例如我得机器中在gnu目录下有这些子目录：
    ```
    [BINUTL-2.17]  [GCC-4.23]  [MAKE-3.791]  [TEXINFO4.11]
    ```

## 2、下载Allegro
* 在下面网址可以下载到完整的Allegro
  ```
    http://ftp.delorie.com/pub/djgpp/current/v2tk/allegro/
  ```
* 我使用的版本是4.22版，对应的文件是all422x.zip，我们可以看到，这样的文件有四个，分别是：
  1. all422a.zip
    > Allegro二进制库文件和包含文件
  2. all422b.zip
    > Allegro二进制执行文件
  3. all422d.zip
    > Allegro的文档
  4. all422s.zip
    > Allegro的所有源程序,演示源程序及范例源程序

* 一般情况下，我们安装Allegro的目的是为了编程，实际上我们只安装all422a.zip就可以了；all422b.zip中是一些可执行文件，通常我们用不上；all422d.zip中是Allegro的文档，肯定是有用的，但不一定安装在这台机器上，可以放在平常你用的windows的机器上以备查看；all422s.zip中为源代码，其中的范例程序源代码对我们学习Allegro会很有帮助，建议您下载并放在DOS+DJGPP的这台机器上。

* 鉴于此，建议你不妨把这4个文件都下载下来，但实际只安装all422a.zip和all422s.zip。

## 3、Allegro的安装
* Allegro的安装及其简单，按以下步骤：
  - 把all422a.zip和all422s.zip拷贝到DJGPP的安装目录下，比如c:\djgpp
  - 解压缩这两个文件
    ```
    c:\djgpp>unzip32 all422a.zip
    .........
    c:\djgpp>unzip32 all422s.zip
    ```
    + 如果你使用pkunzip，千万不要忘记加-d选项。
    + 如果你没有unzip32，但希望使用这个解压缩程序，可以在下面地址下载：http://ftp.delorie.com/pub/djgpp/current/unzip32.exe
    + 解压缩过程中可能会有重复文件所以提示你是否覆盖，选全部覆盖，没有关系。
  - 安装完毕。

## 4、验证Allegro的安装
> 由于我们安装了源代码，所以在DJGPP安装目录下应该有这个目录，allegro\examples，比如：c:\djgpp\allegro\examples，进入该目录可以看到很多范例程序，由一个最简单的程序叫exhello.c，我们试着编译以下。
* 进入rhide
  > 这个在安装DJGPP的时候我们已经安装好了。

  > c:\djgpp\allegro\examples>rhide
* 打开源文件exhello.c
  > 在rhide下的菜单中选择：File---->Open

  > 选择exhello.c文件并打开
* 设置带allegro库德编译参数
  >在rhide菜单下选择：Option---->Libraries

  > 如下图，填入alleg并使该项有效[X]，然后回车

  ![在DJGPP+DOS下安装ALLEGRO](images\在DJGPP+DOS下安装ALLEGRO-02.jpg)

* 编译
  > 在rhide菜单中选择：Compile---->Make

  >如果没有错误，说明Allegro安装成功。

* 运行
  >在rhide菜单中选择：Run---->Run
  
  >你应该可以看到运行结果。

> 至此，Allegro全部安装完毕，关于Allegro的使用方法，请参阅你下载的文档。