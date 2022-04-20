---
title: "ANSI的ESC转义序列"
#lead: ""
date: 2022-04-06T14:00:00+08:00
#thumbnail:
#  src: "img/placeholder.png"
#  visibility: 
#    - list
#    - post
author: whowin
authorbox: false
sidebar: false
pager: true
toc: true
categories:
  - "Linux"
tags:
  - "ANSI转义"
  - "CSI序列"
  - "SGR参数"
  - "颜色编码"
#weight: 2
#menu: main
#references:
#  - "[ANSI escape code](https://en.wikipedia.org/wiki/ANSI_escape_code)"
#  - "[ANSI转义序列](https://zh.wikipedia.org/wiki/ANSI%E8%BD%AC%E4%B9%89%E5%BA%8F%E5%88%97)"
#  - "[XTerm Control Sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html)"
postid: 100003
---

本文较完整第介绍了ANSI的转义序列，大多数都可以用于在普通终端上显示出较为漂亮的界面，本文可以作为一个资料备查。
<!--more-->

## CSI序列
* **CSI**(Control Sequence Introducer)序列由 ```ESC [``` 开始，后面跟若干个参数字节(可以没有)，规定范围为 ```0x30-0x3F(ASCII 0–9:;<=>?)```；再跟若干个中间字节，规定范围为 ```0x20-0x2F(ASCII 空格以及!"#$%&'()*+,-./)```；然后再跟一个结束字符(单个字符)，规定范围为 ```0x40-0x7E(ASCII @A–Z[\]^_`a–z{|}~)```
  |组成部分|字符范围|ASCII|
  |:----:|:----:|:----|
  |参数字节|0x30–0x3F|```0–9:;<=>?```|
  |中间字节|0x20–0x2F|```空格、!"#$%&'()*+,-./```|
  |最终字节|0x40–0x7E|@A–Z[\]^_`a–z{|}~|
* 常见的序列通常是把一些数字用分号分隔开，例如 1;2;3，缺少的数字视为 0(比如：1;;3被视为1;0;3)，```ESC[m``` 这样没有参数的情况相当于参数为 0；
* 某些序列（如下表中的CUU - 光标上移一行）把0视为1，以使在缺少参数的情况下有意义；
* 下表摘自维基百科中 [CSI序列部分](https://zh.wikipedia.org/wiki/ANSI%E8%BD%AC%E4%B9%89%E5%BA%8F%E5%88%97#CSI%E5%BA%8F%E5%88%97)
* 一些ANSI控制序列（不完整列表）
  |编号|代码|名称|作用|
  |:--:|:--------|:----|:----|
  |1|CSI n A|CUU – 光标上移(Cursor Up)|光标向指定的方向移动n(默认1)格。如果光标已在屏幕边缘，则无效|
  |2|CSI n B|CUD – 光标下移(Cursor Down)|同上|
  |3|CSI n C|CUF – 光标前移(Cursor Forward)|同上|
  |4|CSI n D|CUB – 光标后移(Cursor Back)|同上|
  |5|CSI n E|CNL – 光标移到下一行(Cursor Next Line)|光标移动到下面第n(默认1)行的开头。(非ANSI.SYS)|
  |6|CSI n F|CPL – 光标移到上一行(Cursor Previous Line)|光标移动到上面第n(默认1)行的开头。(非ANSI.SYS)|
  |7|CSI n G|CHA – 光标水平绝对(Cursor Horizontal Absolute)|光标移动到第n(默认1)列。(非ANSI.SYS)|
  |8|CSI n ; m H|CUP – 光标位置(Cursor Position)|光标移动到第n行、第m列。值从1开始，且默认为1(左上角)。例如CSI ;5H和CSI 1;5H含义相同；CSI 17;H、CSI 17H和CSI 17;1H三者含义相同|
  |9|CSI n J|ED – 擦除显示(Erase in Display)|清除屏幕的部分区域。如果n是0(或缺失)，则清除从光标位置到屏幕末尾的部分。如果n是1，则清除从光标位置到屏幕开头的部分。如果n是2，则清除整个屏幕(在DOS ANSI.SYS中，光标还会向左上方移动)。如果n是3，则清除整个屏幕，并删除回滚缓存区中的所有行(这个特性是xterm添加的，其他终端应用程序也支持)|
  |10|CSI n K|EL – 擦除行(Erase in Line)|清除行内的部分区域。如果n是0(或缺失)，清除从光标位置到该行末尾的部分。如果n是1，清除从光标位置到该行开头的部分。如果n是2，清除整行，光标位置不变|
  |11|CSI n S|SU – 向上滚动(Scroll Up)|整页向上滚动n(默认1)行。新行添加到底部。(非ANSI.SYS)|
  |12|CSI n T|SD – 向下滚动(Scroll Down)|整页向下滚动n（默认1）行。新行添加到顶部。(非ANSI.SYS)|
  |13|CSI n ; m f|HVP – 水平垂直位置(Horizontal Vertical Position)|同CUP|
  |14|CSI n m|SGR – 选择图形渲染(Select Graphic Rendition)|设置SGR参数，包括文字颜色。<br/>CSI后可以是0或者更多参数，用分号分隔。如果没有参数，则视为CSI 0 m(重置/常规)|
  |15|CSI 5i|打开辅助端口|启用辅助串行端口，通常用于本地串行打印机|
  |16|CSI 4i|关闭辅助端口|禁用辅助串行端口，通常用于本地串行打印机|
  |17|CSI 6n|DSR – 设备状态报告(Device Status Report)|以ESC[n;mR(就像在键盘上输入)向应用程序报告光标位置(CPR)，其中n是行，m是列|
  |18|CSI s|SCP – 保存光标位置(Save Cursor Position)|保存光标的当前位置|
  |19|CSI u|RCP – 恢复光标位置(Restore Cursor Position)|恢复保存的光标位置|

## SGR参数
* SGR(Select Graphic Rendition)参数是ANSI转义序列中一组用于控制光标和字体的控制代码
* 以下内容摘自维基百科中[SGR参数部分](https://zh.wikipedia.org/wiki/ANSI%E8%BD%AC%E4%B9%89%E5%BA%8F%E5%88%97#%E9%80%89%E6%8B%A9%E5%9B%BE%E5%BD%A2%E5%86%8D%E7%8E%B0%EF%BC%88SGR%EF%BC%89%E5%8F%82%E6%95%B0)

  |代码|作用|备注|
  |:--:|:----|:----|
  |0|重置/正常|关闭所有属性|
  |1|粗体或增加强度|
  |2|弱化（降低强度）|未广泛支持|
  |3|斜体|未广泛支持；有时视为反相显示|
  |4|下划线||
  |5|缓慢闪烁|低于每分钟150次|
  |6|快速闪烁|MS-DOS ANSI.SYS；每分钟150以上；未广泛支持|
  |7|反显|前景色与背景色交换|
  |8|隐藏|未广泛支持|
  |9|划除|字符清晰，但标记为删除。未广泛支持|
  |10|主要（默认）字体||
  |11–19|替代字体|选择替代字体n-10|
  |20|尖角体|几乎无支持|
  |21|关闭粗体或双下划线|关闭粗体未广泛支持；双下划线几乎无支持|
  |22|正常颜色或强度|不强不弱|
  |23|非斜体、非尖角体||
  |24|关闭下划线|去掉单双下划线|
  |25|关闭闪烁||
  |27|关闭反显||
  |28|关闭隐藏||
  |29|关闭划除||
  |30–37|设置前景色	参见下面的颜色表|
  |38|设置前景色|下一个参数是5;n或2;r;g;b，见下|
  |39|默认前景色|由具体实现定义（按照标准）|
  |40–47|设置背景色|参见下面的颜色表|
  |48|设置背景色|下一个参数是5;n或2;r;g;b，见下|
  |49|默认背景色|由具体实现定义（按照标准）|
  |51|Framed||
  |52|Encircled||
  |53|上划线||
  |54|Not framed or encircled||
  |55|关闭上划线||
  |60|表意文字下划线或右边线|几乎无支持|
  |61|表意文字双下划线或双右边线||
  |62|表意文字上划线或左边线||
  |63|表意文字双上划线或双左边线||
  |64|表意文字着重标志||
  |65|表意文字属性关闭|重置60–64的所有效果|
  |90–97|设置明亮的前景色|aixterm（非标准）|
  |100–107|设置明亮的背景色|aixterm（非标准）|

## 颜色编码
* 以下内容摘自维基百科中[颜色部分](https://zh.wikipedia.org/wiki/ANSI%E8%BD%AC%E4%B9%89%E5%BA%8F%E5%88%97#%E9%A2%9C%E8%89%B2)
* 3bit 和 4bit 色彩
  - 因为最早的终端的颜色在硬件上只有 3bits，颜色只有 8 种，所以早期的规范中只给这 8 种颜色做了命名；SRG参数的 30-37 用于选择前景色，40-47 用于选择背景色；SGR 编码中 1 表示字体加粗，但那时许多终端并不是把粗体字(SGR码为 1)当成一个字体，而是使用更明亮的色彩来实现所谓"粗体"，通常你无法把这种加亮的色彩用作背景色
  - 例如：使用 ```ESC[30;47m``` 是白底黑字的效果，使用 ```ESC[31m``` 变成红字，使用 ```ESC[1;31m``` 会变成加亮的红字；```ESC[39;49m``` 将恢复到默认的前景、背景色(有些终端不支持)，使用 ```ESC[0m``` 关闭所有属性也可以恢复默认设置
  - 后来随着技术的进步，终端都增加了直接设置高亮前景色和背景色的功能，前景色使用代码 90-97，背景色使用代码 100-107
  - 随着硬件的进步，开始使用 8bits 的 DAC(Digital-to-Analog Converters 数模转换器)，许多软件开始使用 24bit 的颜色编码
    |颜色名称|前景色代码|背景色代码|
    |:----:|:----:|:----:|
    |黑|30|40|
    |红|31|41|
    |绿|32|42|
    |黄|33|43|
    |蓝|34|44|
    |品红|35|45|
    |青|36|46|
    |白|37|47|
    |亮黑（灰）|90|100|
    |亮红|91|101|
    |亮绿|92|102|
    |亮黄|93|103|
    |亮蓝|94|104|
    |亮品红|95|105|
    |亮青|96|106|
    |亮白|97|107|
  - 这部分在维基百科中看会更加直观
* 8bit 色彩
  - 随着 256 色的图形卡越来越普遍，相应的转义序列也跟着做了变更，可以从预定义的 256 种颜色中选择色彩
    ```
    ESC[38;5;⟨n⟩m ---- 选择前景色
    ESC[48;5;⟨n⟩m ---- 选择背景色
      0-  7:  标准色彩 (相当于 ESC [ 30–37 m)
      8- 15:  高亮色彩 (相当于 ESC [ 90–97 m)
    16-231:  6 × 6 × 6 RGB (216 种颜色): 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)
    232-255:  从黑到白工 24 级灰度
    ```
* 24bit色彩
  - 随着有着 16-24bit 种颜色的"真彩"图像卡越来越普遍，应用程序开始支持 24bit 的色彩，Xterm 等终端仿真器开始支持使用转义序列设置24位前景和背景颜色
    ```
    ESC[ 38;2;⟨r⟩;⟨g⟩;⟨b⟩ m ---- 选择 RGB 前景色
    ESC[ 48;2;⟨r⟩;⟨g⟩;⟨b⟩ m ---- 选择 RGB 背景色
    ```

## OSC 序列
* 以下内容译自wikipedia[OSC (Operating System Command) sequences](https://en.wikipedia.org/wiki/ANSI_escape_code#OSC_(Operating_System_Command)_sequences)
* 在另一篇文章[《关于bash下变量PS1的完整理解》][article01]会用到这个说明
* 大多数 OSC(Operating System Command)序列都是由 Xterm 定义的，但许多其他仿真终端也支持 OSC 序列。由于历史原因，Xterm 可以使用 BEL(ASCII 007) 结束命令，也可以使用标准 ST(ESC \) 结束命令。例如，Xterm 允许使用 **```\033]0；window title\007```** 设置窗口标题

[article01]: ../0001-environment_variables_and_shell_variables/
