---
title: "在DOS下玩转BT878A进行视频采集"
date: 2008-05-19T13:54:00+08:00
author: whowin
sidebar: false
authorbox: false
pager: true
toc: true
categories:
  - "DOS"
tags:
  - DOS
  - PCI
  - BT878A
  - "video capture"
draft: true
postid: 160015
---

BT878A是目前广泛采用的一种视频采集芯片，市面上可以很容易地买到PCI的以BT878A位核心的视频采集卡，这些卡大都仅提供windows下的驱动，如果你要把这颗芯片用在工控领域，如果你想在DOS下使用它，困难就来了。本文是以前一个使用BT878A芯片用于视频采集的项目的关于BT878A使用的一个总结，主要叙述如何从底层直接对BT878A进行操作和控制，掌握本文介绍的内容，应该说不仅是在DOS下，应该在任何环境下，你都将有能力驾驭这颗芯片。本文并不打算对BT878A的所有功能进行解释，仅对其视频采集部分的基本功能进行说明并实际完成一个例子。**本文是我2008年的作品，2023年重新整理发布，仅为存档，其中的程序并没有再次验证，特此说明。**
<!--more-->

## 1、BT878A介绍
* 该芯片的datasheet可以在 [这里][article01] 下载
* BT878A功能十分强大，下面仅就本文感兴趣的功能作一下介绍：
  - 有PCI接口，可以方便地连接到PCI总线上
  - 有DMA控制器，可以通过PCI总线实现DMA
  - 支持PAL和NTSC制式的输入模拟信号
  - 内部有一套精简指令系统，在实现数据采集和高质量视频流时十分有用
  - 最大分辨率可以达到768X576（实例中我们将在NTSC下实现640X480分辨率）
  - 可以实现32位真彩色（我们实际使用24位真彩）
  - 24位的GPIO，同时有一个I2C接口（将另文介绍使用BT878的I2C接口的体会）
  - 可以调整亮度、对比度等

## 2、对BT878A编程及相关寄存器介绍
* 前面提到，BT878A可以很方便地连接到PCI总线上，BT878A实现了PCI的配置空间，所以有关BT878的基地址等一系列信息要通过PCI的配置寄存器获得，这个方法在我以前的博文里有介绍，请参阅博文：[《遍历PCI设备》][article02] 和 [《在DOS下针对AC'97编程》][article03]；
* BT878A是CONEXANT的产品，该公司的Vendor ID是0x109E，Function 0的Device ID是0x036E，Function 1的Device ID是0x0878，我们可以使用这些数据来搜寻你的PCI总线上是否有这个设备。

* BT878A的寄存器非常多(具体多少我没有数，偏移地址从0x000--0x2FF，中间有些地方不连续)；
* 寄存器是以存储器地址映射的，从PCI配置空间得到基地址后，加上偏移地址，使用内存的访问方法就可以访问到所有的寄存器(我们只用FUNCTION 0的寄存器)；
* 由于涉及到许多视频的知识，有些寄存器的作用我也没有完全搞清楚，好在很多寄存器并不需要我们过多地关心；
* 所有寄存器的作用，请看BT878A的datasheet的5-3 Local Registers，这里只就我们比较关心的一些寄存器以功能为序进行一下介绍。

* 芯片的软件复位：
  - SRESET——Software Reset Register（0x07c）
  - 向SRESET寄存器写入任何数据会复位BT878的视频部分，同时所有寄存器将复位成缺省值。
  - WriteLocalDWORD(SRESET,oxff)

* 停止RISC的运行
  - 涉及寄存器：CAP_CTL: Capture Control Register（0x0dc）
    + Bit0：CAPTURE_EVEN：为1时，在偶域可以向FIFO写数据
    + Bit1：CAPTURE_ODD：为1时，在奇域可以想FIFO写数据
    + Bit2：CAPTURE_VBI_EVEN：为1时，在偶域可以采集VBI数据并写入FIFO
    + Bit3：CAPTURE_VBI_ODD：为1时，在奇域可以采集VBI数据并写入FIFO
    + Bit4：DITH_FRAME： 0—抖动矩阵应用于一个域的连续行上；1—全屏幕方式
    + 其它位备用
  - WriteLocalDWORD(CAP_CTL,0x00)
* 初始化GPIO和DMA
  - 涉及寄存器：GPIO_DMA_CTL:GPIO and DMA Control Register （0x10c）
  - 设置成Packed方式，一次传递8 WORDS，故bit3，bit2设置成01
  - bit0：FIFO_ENABLE,为1时，允许FIFO数据
  - bit1：RISC_ENABLE:为1时，开始运行RISC
  - 启动RISC时要将这两位置1
  - WriteLocalDWORD(GPIO_DMA_CTL, 0x04)
* 设置中断寄存器
  - INT_MASK:Interrupt Mask Register（0x104）
  - 不使用中断，所以此寄存器设置为0
  - WriteLocalDWORD(INT_MASK,0X00)
* 设置彩色格式
  - Color_FMT:Color Format Register(0x0D4)
  - Bit[3:0]:偶域的色彩 bit[7:4]:奇域的色彩
    ```
    0000=RGB32        0001=RGB24          0010=RGB16          0011=RGB15
    0100=YUV2 4:2:2   0101=BtYUV 4:1: 1   0110=Y8(Gray scale) 0111=RGB(Dithered)
    1000=YcrCb 4:2:2 planar(YUV12)        1001=YcrCb 4:1:1 plannar (YUV9)
    1010—1101=备用    1110=Raw 8xData    1111=备用
    ```
  - 我们使用RGB16，故设为0010
  - WriteLocalDWORD(COLOR_FMT,0x22)
* 设置ADC
  - ADC：ADC Interface Register （0x068）
  - Bit[7:6]=10  bit5:备用
  - bit4：0—AGC Enabled   1—AGC disabled
  - Bit3：CLK_SLEEP，0—普通时钟操作，1—关闭系统时钟
  - Bit2：Y_SLEEP，0—普通Y ADC操作，1—停止Y ADC操作，
  - Bit1：C_SLEEP,0—普通C ADC操作，1—停止C ADC操作，
  - Bit0：0—无自适应AGC  1—自适应AGC
  - WriteLocalDWRD(ADC,ox82)设置成缺省值 
* 色度设置
  - YUV简介
    > YUV（亦称YCrCb）是被欧洲电视系统所采用的一种颜色编码方法（属于PAL）。YUV主要用于优化系统视频信号的传输，使其向后兼容老式黑白电视机，与RGB视频信号传输相比，它最大的优点在于只需占用极少的带宽（RGB要求三个独立的视频信号同时传输）。其中Y表示明亮度（Luminance或Luma），也就是灰阶值；而U和V表示的则是色度（Chrominance或Chroma），作用是描述影像色彩和饱和度，用于指定像素的颜色。“亮度”是通过RGB输入信号来创建的，方法是将RGB信号的特定部分叠加到一起。“色度”则定义了颜色的两个方面—--色调与饱和度，分别用Cr和Cb来表示，其中，Cr反映了RGB输入信号红色部分与RGB信号亮度值之间的差异。而Cb反映的是GRB输入信号蓝色部分与RGB信号亮度值之间的差异。

  - 色度设置涉及到4个寄存器
    1. SAT_U_LO:Chroma（U）Gain Register，Lower Byte（0x034）
    2. SAT_V_LO:Chroma（V）Gain Register，Lower Byte（0x038）
    3. E_CONTROL: Miscellaneous Control Register （Even Field）（0x02C）
    4. O_CONTROL: Miscellaneous Control Register （Odd Field）（0x0AC）
  - 其中E_CONTROL 和O_CONTROL的Bit0为色度调节（v）的高位，bit1为色度调节（U）的高位，所以，色度调节寄存器共有9位，设备为缺省值即可：SAT_U_LO为0xfe，SAT_V_LO为0xb4
  - WriteLocalDWORD(SAT_U_LO, 0xfe)
  - WriteLocalDWORD(STA_V_LO, 0xb4)
* 亮度调节
  - BRIGHT:Brightness Control Register（0x028）亮度值设为0x20
  - WriteLocalDWORD(BRIGHT, 0x20)
* 对比度设置
  - 涉及三个寄存器
    1. CONTRAST_LO:Luma Gain Register，Lower Byte（0x030）
    2. E_CONTROL: Miscellaneous Control Register （Even Field）（0x02C）
    3. O_CONTROL: Miscellaneous Control Register （Odd Field）（0x0AC）
  - 其中E_CONTROL和O_CONTROL的bit2为亮度调节的高位，故亮度调节共有9位。
  - 亮度设为0x30，WriteLocalDWORD（CONTRAST_LO, 0x030）
* 色彩控制
  - COLOR_CTL:Color Control Register（0x0D8）
  - 具体说明请看Page5—29
  - 设为0x10，WriteLocalDWORD（COLOR_CTL，0x10）
* 输入格式设置
  - IFORM:Input Format Register（0x004）
  - Bit[2:0]输入格式
    ```
    000—自动控制格式       001—NTSC(M)         010—NTSS(JAPAN)
    011—PAL(B,G,H,I)     100—PAL(M)          101—PAL(N)
    110—SECAM            111—PAL(N—联合)
  - bit[6:5]输入端口
    ```
    00—MUX3      01—MUX2      10—MUX0      11—MUX1
    ```
  - 其他位备用
  - 本机使用MUX0，输入格式为NTSC(M)
  - bit[4:3]:必须设为11
  - WriteLocalDWORD(IFORM, 0x59)（01011001B）
* AGC延迟和Burst延迟
  - ADELAY:AGC Delay Register（0x060）
  - BDELAY:Burst Delay Register（0x064）
  - ADELAY的计算公式：$ ADELAY = (6.8\mu s \times 4 \times Fsc) + 15 $
  - 对于NTSC输入信号：$ ADELAY = (6.8\mu s \times 4 \times 14.32MHz) + 15 = 112(0x70) $
  - BDELAY的计算公式：$ BDELAY = (6.5\mu s \times 4 \times Fsc) $
  - 对于NTSC输入信号：$ BDELAY = (6.5\mu s \times 4 \times 14.32MHz) = 93(0x5D) $
  - 这两个值的含义不很清楚，故按缺省值设置
  - WriteLocalDWORD(ADELAY,0x70)
  - WriteLocalDWORD(BDELAY,0x50)
* 缩放比例设置
  - 涉及寄存器
    1. E_HSCALE_HI: Horizontal Scale Register（Even Field），Upper Byte（0x020）
    2. O_HSCALE_HI: Horizontal Scale Register（Odd Field），Upper Byte（0x0A0）
    3. E_HSCALE_HI: Horizontal Scale Register（Even Field），Lower Byte（0x024）
    4. O_HSCALE_HI: Horizontal Scale Register（Odd Field），Lower Byte（0x0A4）
    5. E_VSCALE_HI: Vertical Scaling Register（Even Field），Upper Byte（0x04c）
    6. O_ VSCALE _HI: Vertical Scaling Register（Odd Field），Upper Byte（0x0AC）
    7. E_ VSCALE _LO: Vertical Scaling Register（Even Field），Lower Byte（0x050）
    8. O_ VSCALE_LO: Vertical Scaling Register（Odd Field），Lower Byte（0x0D0）
  - 首先说HSCALE，定义横向的缩放比例，对NTSC而言，BT878A每行产生910个像素，PAL产生1135像素，HSCALE的计算公式如下：
    + $NTSC:HSCALE = [(910\:/\:Pdesird) - 1] \times 4096$
    + $PAL：HSCALE = [(1135\:/\:Pdesird) - 1] \times 4096$
    + 其中Pdesired为你希望得到的像素值，其中包括active，sync或blanking由于sync或blanking的值不好掌握，还有下面的公式
      * $NTSC:HSCALE = [(754\:/\:HACTIVE) - 1] \times 4096$
      * $PAL:HSCALE = [(922\:/\:HACTIVE) - 1] \times 4096$
      * HACTIVE：每行像素值，不包含sync或blanking，这个公式与前面的公式相比会略有差别，只能粗略地反映实际情况。
    + 实际设置我们以后面的公式计算。NTSC,HACTIVE = 640最后得出: HSCALE = 729.6(0x2D9)
  - WriteLocalDWORD(O_HCALE_HI, 0x02)
  - WriteLocalDWORD(O_HCALE_LO, 0xD9)
  - WriteLocalDWORD(E_HCALE_HI, 0x02)
  - WriteLocalDWORD(E_HCALE_LO, 0xD9)
* 垂直缩放VSCALE
  - 公式如下: $ VSCALE = (0x10000 - {[(scale - ratio) - 1] \times 512} + 0x1fff) $
  - 要注意的是VSCALE只有13位有效位，即其MSB中只有5位有效，其高三位用于其它控制，在设置VSCALE时不要破坏。在这里垂直方向比例为1：1，故而SCALE设为0
  - WriteLocalDWORD（O_VSCALE_LO,0）
  - WriteLocalDWORD（O_VSCALE_HI,0）
  - WriteLocalDWORD（E_VSCALE_LO,0）
  - WriteLocalDWORD（E_VSCALE_HI,0）
  - 在Page2—16有各种情况下HSCALE和VSCALE的表格可供参考
* 图像裁减设置
  - 涉及寄存器
    + HDELAY: Horizontal Delay Register，Lower Byte
      > E_HDELAY_LO(0x18)        O_HDELAY_LO(0x098)
    + MSB Cropping Register的bit[3:2]
      > E_CROP(0x00C)            O_CROP(0x010)
    + VDELAY:Vertical Delay Register Lower Byte
      > E_VDELAY_LO(0x090)       O_HACTIVE_LO(0x010)
    + MSB Cropping Register的bit[7:6]
    + HDELAY: Horizontal Delay Register，Lower Byte
      > E_HDELAY_LO(0x01c)       O_HDELAY_LO(0x09c)
    + MSB Cropping Register的bit[1:0]
    + VDELAY:Vertical Delay Register Lower Byte
      > E_VDELAY_LO(0x014)       O_HACTIVE_LO(0x094)
    + MSB Cropping Register的bit[5:4]
    + HDELAY:水平同步信号到显示的第一个像素之间像素数
    + HACTIVE:实际要显示的水平像素数
    + VDELAY:锯齿状脉冲的结束到显示的第一行之间的行数的一半
    + VACTIVE:实际要显示的行数
  - 详细描述请看P2—18，P2—19
  - 其中，HACTIVE和VACTIVE比较好设置，但HDELAY和VDELAY不太好办。
  - HDELAY的计算公式

    $HDELAY(NTSC) = (135\:/\:754 \times HACTIVE)\:\&\:0x3FE$

    $HDELAY(PAL) = (186\:/\:922 \times HACTIVE)\:\&\:0x3FE$
  - 可以适当增加HDELAY或者减少HACTIVE,但是要遵从下面的原则：

    $NTSC  HDELAY + HACTIVE ≤ 889 \times HSCALE$

    $PAL   HDELAY + HACTIVE ≤ 1108 \times HSCALE$
  - 如果HDELAY + HACTIVE太大，将会看到Front Porch或Back Porch像素
  - 实际应用中
    ```
    HACTIVE = 640(0x280)
    VACTIVE = 480(0x1E0)
    HDELAY = 0x72
    VDELAY = 0x1d
    ```
  - WriteLocalDWORD(E_HACTIVE_LO, 0x80)
  - WriteLocalDWORD(O_HACTIVE_LO, 0x80)
  - WriteLocalDWORD(E_VACTIVE_LO, 0xE0)
  - WriteLocalDWORD(O_VACTIVE_LO, 0xE0)
  - WriteLocalDWORD(E_CROP, 0x12)
  - WriteLocalDWORD(O_CROP, 0x12)
  - WriteLocalDWORD(E_HDELAY, 0x72)
  - WriteLocalDWORD(O_HDELAY, 0x72)
  - WriteLocalDWORD(E_VDELAY, 0x1D)
  - WriteLocalDWORD(O_VDELAY, 0x1D)
* SC LOOP设置
  - SC_LOOP：SC LOOP Control Register
    > E_SCLOOP(0x040)      O_SCLOOP(0x0c0)

    >对此寄存器的认识不是很清楚，对整体的影响也不大详细描述在Page5—19，实际应用中设为0x60

  - WriteLocalDWORD(O_SCLOOP,0x60)
  - WriteLocalDWORD(E_SCLOOP,0x60)
* PLL设置
  - 涉及寄存器
    1. TGCTRL:Timing Generator Controll Register（0x084）
    2. PLL_F_LO:PLL Reference Maltiplier Register (0x0F0)
    3. PLL_F_HI:PLL Reference Maltiplier Register (0x0F4)
    4. PLL_XCI:PLL Integer Register (0x0F8)
    5. PLL:phase Lock Loop(锁相环)
  - Bt878A的内部锁相环使其可以使用一只晶振就可以解码NTSC和PAL的图像，晶振频率为28.636363精度为50ppm，产生的频率接如下公式计算：
    $Frequency = (F_INPUT \div PLL_X) \times PLL_I.LL_F \div PLL_C$
    ```
    F_INPUT = 28.636363MHz
    PLL_X = 预分频（除以2）（为1或2）
    PLL_I = 整数部分
    PLL_F = 小数部分
    PLL_C = 被除数（为4或6）
    PLL_X:PLL_XCI寄存器的bit7，bit7=0则PLL_X=1，Bit7=1则PLL_X=2
    PLL_I:PLL_XCI寄存器的bit[5:0]取值范围6—63，如果为0则PLL不工作，注意PLL_I.PLL_F = 6.8000(最小值)
    PLL_F:由PLL_F_LO和PLL_F_HI组成，共16位
    PLL_C:PLL_XCI寄存器的 bit6,bit6=0则PLL_C=6，bit6=1则PLL_C=4
    ```
    + 注意，Frequency是所需频率的2倍（CLKx2）如NTSC时Frequency=28.636363（14.32x2）
    + PAL时Frequency=35.46895（17.73x2）
    + 当为NTSC时，所有值可以设为0，当为PAL时做如下设置如下：
      ```
      PLL_X = 1      PLL_I = 0x0E         PLL_F = 0xDCF9     PLL_C=0
      ```
    + 设置PLL要按如下步骤：
      1. 在TGCTRL寄存器中的TGCKI设为Normal XTALO/XTALI Mode即TGCKI=00
      2. PLL寄存器的值变化后，DSTAUS寄存器的PLOCK位会被拉低，直至PLL被锁住（大约500ms）
      3. 将TGCKI设置为PLL方式，即TGCKI=01
      4. 通常不必等PLOCK位变高，简单的方法是将TGCKI = 00然后设置PLL寄存器，再延迟1秒后将TGCKI = 01即可，实际应用中由于使用NTSC，所以PLL寄存器可以设为0
  - WriteLocalDWORD(TGCTRL, 0x00)
  - WriteLocalDWORD(PLL_F_LO, 0x00)
  - WriteLocalDWORD(PLL_F_HI, 0x00)
  - WriteLocalDWORD(PLL_XCI, 0x00)
  - Sleep（1）
  - WriteLocalDWORD(TGCTRL,0x08)
  - TGCTRL的说明情况Page5—26其中TGCKI为bit[4：3]
* 启动RISC
  - 涉及寄存器
    1. INT_STAT:/Interrupt status Register （0x100）中断状态寄存器的详细说明Page5—32
    2. RISC_START_ADD:RISC Program Status Address Register（0x114）
    3. GPIO_DMA_CTL:Capture Contrd Register(0x0DC)
  - GPIO_CTL说明见Page 5—29
  - 启动RISC按如下步骤：
    1. 中断状态寄存器设置 WriteLocalDWORD(INT_STAT,0xC000)，bit[32:31]必须为11
    2. 设置RISC程序地址WriteLocalDWORD(RISS_STRT_ADD,RISCAddr)
    3. 设置CAP_CTL寄存器，启动奇偶行采集WriteLocalDWORD(CAP_CTL, 0x03)
    4. 设置GPIO_DMA_CTL，启动RISC,启动FIFO，一次传8DWORDS
  - WriteLocalDWORD(GPIO_DMA_CTL, 0x07)
* TDEC设置
  - 当带宽不够或系统限制时，可能系统无法完成所有帧的传输，此时bt878A有一个减少传输帧的解决方案，通过设置TDEC寄存器来完成。
  - TDEC:Temporal Decimation Register（0x008）
  - TDEC的值可以是1—60（NTSC）或者是1—50（PAL/SECAM），这个值的含义在60帧（NTSC）或50帧（PAL/SECAM）中，丢掉的帧数（或域数）
    + Bit7=0 丢掉帧   bit7=1 丢掉域
    + Bit6=0，从奇域开始丢弃 bit6=1从偶域开始丢弃
    + Bit[5:0]丢掉的帧或域数
    + 实际应用中设为0
  - WriteLocalDWORD(TDEC,0x00)
* 色温调节
  - HUE:Hue Control Register(0x03c)
  - 色温可以有256级，每级为0.7℃，调节范围为 -90°— +89.3°此寄存器为有符号数
  - 实际应用中设为0，WriteLocalDWORD(HVE,0x00)
* 白平衡调节
  - 涉及寄存器
    1. WC_UP:White Crush Up Register（0x044）
    2. WC_UP:White Crush Down Register（0x078）
    3. WC_UP寄存器说明见Page5—20
    4. Bit[7:6]：确定比较点
      ```
      00—最大亮度值的3/4  01—最大亮度值的1/2  10—最大亮度值的1/4  11—自动
      ```
    5. bit[5:0],可以设定为0—63，当比较点的大多数像素的值低于此值时，将适当增加AGC的值
  - 实际应用中使用缺省值0xcF
  - WriteLocalDWORD( WC_UP，0xCF)
* WC_DN寄存器的含义不是很清楚，使用缺省值0x7F
  - WriteLocalDWORD(WC_UP，0x7F)
* 输出色彩范围
  - OFORM:Output Format Register（0x048）
    ```
    Bit7=0   普通操作（亮度范围16—253，色度范围2—253）
                     （Luma：16—253，chroma：2—253）
             Y=16为黑色（基色） Cr，Cb=128为无色
    Bit7=1   全范围输出（Luma：0—255，chroma：2—253）
             Y=0为黑色   Cr，Cb=128为无色
    Bit[6:5]:当中心区为Luma值小于设置值时，其Luma值将强行为0
             00—无效  01—8   02—16  03—32
    ```
  - 使用缺省值0，WriteLocalDWORD(OFORM,0x00)
* 音频复位
  - ARESET:Audio Reset Register（0x05B）
  - Bit7从高跳变到低将复位音频电路
  - 没有使用该寄存器
* VTC控制
  - VTC:Vidio Timing Register
  - O_VTC(0x6c)          E_VTC(0Xec)
  - 此寄存器含义不明，使用缺省值
  - WriteLocalDWORD( O_VTC，0x00)
  - WriteLocalDWORD( E_VTC，0x00)
* TGLB
  - TGLB：Timing Generator Load Byte（0x080）
  - 含义不明，不做设置

* 垂直同步计数调节
  - VTOTAL:Total Line Count Register
  - VTOTAL_LO(0x0B0)  VTOTAL_HI(0x0B4)
  - 当VTOTAL中的值不为0时，解码器的垂直同步线计数将从原来的525/625线变成设定值
    + VTOTAL_HI只有bit0和bit1有效，VTOTAL共10位
    + VTOTAL = （水平视频线数 / 帧数） - 1
    + VTOTAL = （number of horizontal video lines / frame） - 1
  - 实际应用中没有设定

## 3、BT878的精简指令系统
* BT878A内置了一个DMA控制器，使其性能大大增加；
* 它的精简指令引擎可以执行放在内存中的精简指令，从而使DMA控制器可以自动地把图像数据及时地移动到内容缓冲区中，也可以直接移动到显示缓冲区，而这一切不需要CPU的干预；
* 有关这部分的详细内容请看datasheet的2.12 DMA Controller。前面介绍寄存器时提到的启动和停止RISC，指的就是精简指令。下面仅就相关概念做一下介绍。

* 经过解码的复合视频数据存储在BT878A的FIFO中，精简指令放在内存中，软件把精简指令的起始地址放在BT878A的寄存器中；
* 通过设置寄存器启动DMA，DMA控制器从精简指令中逐条读取指令，并根据指令读取FIFO中的数据，然后把数据存储到适当的存储器地址上；
* 整个精简指令集只有五类指令：WRITE、WRITEC、SKIP、SYNC和JUMP。

* 说到这里可能不得不介绍一下878的VBI(Vertical Blanking Interval)数据格式，一帧视频图像，NTSC制式有525线，PAL制式有625线；
* 实际BT878A的每帧视频数据的格式(NTSC制式)如下图：

  ![在DOS下玩转BT878A进行视频采集][img01]

* 我们会注意到，在NTSC制式的525行中，1--20行、264--283行并不是视频数据，实际的视频数据只有21--263行以及284--525行，一共只有485行；
* 另外一点就是实际数据流是分奇和偶的，奇数行放在21--263行共243行，偶数行放在284--525行，共242行；
* PAL制式的情况与NTSC类似，可以自行看下图(这些图均出自datasheet)

  ![在DOS下玩转BT878A进行视频采集][img02]

* 以下我们均以NTSC制式为例，1--9行和264--272行是垂直同步信号数据，这个应该是我们所需要的，不然我们不知道一帧的开始位置；
* 10--20行和273--283行中没有视频数据，但可能会有其他数据，如果只是为了软件编程，我们不必过于深究这个部分；
* 我们更关心的是数据最后存到FIFO里的格式，因为DMA控制器是从FIFO里取数据，对这部分有兴趣的读者可以仔细阅读datasheet。

* 视频数据采集的时序图如下，本文就不过多解释了。 

  ![在DOS下玩转BT878A进行视频采集][img03]

* 下面这幅图对我们也有一定的意义，所以我们把它截过来，从图中看出，采集的视频数据并不时直接放到了FIFO中，中间还要经过一个视频数据格式转换处理，这一点我们了解就可以了，总之，FIFO中的数据是经过了转换后的数据，相对当然要规范一些。

  ![在DOS下玩转BT878A进行视频采集][img04]

* 现在应该可以说FIFO中的数据了，在BT878A中有三个FIFO，其结构如下：
  ```
  FIFO1 ：70 X 36 bits
  FIFO2 ：35 X 36 bits
  FIFO3 ：35 X 36 bits
  ```

* 我们发现，FIFO都是36 bits的，而一个DWORD的视频数据只有32位，那么另外4位被用来存储数据的状态，这四位的含义如下表：

  ![在DOS下玩转BT878A进行视频采集][img05]

* 在正常视频数据的前面是FIFO模式，也就是FM1或FM3，正常视频数据的第一组数据的状态字是SOL，最后一组视频数据的状态字是EOL，当奇数域完毕时存一个VRO，偶数域完毕后存一个VRE。
* 我们根据这些就可以考虑我们启动DMA的精简指令应该怎样写。

* 在继续往下看之前，希望能认真看一下BT878A的datasheet中第2-38页的2.12.3 RISC Instructions这一节，了解一下BT878A的精简指令，并记得在看到不明白的指令时回来查一下，否则可能会不知所云。

> 我们假定BT878A的FIFO工作在packed data模式。首先，一帧数据从奇数行开始，奇数行开始前一定是偶数行结束，偶数行结束时的状态字是VRE，而奇数行开始时应该先有FM1，然后是我们要的数据，这些数据的第一个应该带有SOL，而最后一个应该带有EOL，好了，大致思路应该是这样，用SYNC指令先找到VRE，然后再用一次SYNC指令找到FM1，然后用WRITE指令把SOL和EOL中的数据写我们需要的部分到内存里我们已经申请到的地址中，则奇数行完毕；偶数行类似，用SYNC指令先找到VRO，然后再用一次SYNC指令找到FM1，然后用WRITE指令把SOL和EOL中的数据写我们需要的部分到内存里我们已经申请到的地址中，至于如何在内存中存放数据，当然随自己需要了，当把奇数行和偶数行都搬移一边后，一帧数据处理完毕，我们可以用JUMP指令跳转到开头进行下一帧就可以了。

## 4、实际范例

> 这个例子的条件大致是这样的，我们从BT878A的MUX0输入一个NTSC制式的视频信号(实际来自一个摄像头)，我们要把摄像头的视频信号实时地显示在电脑的屏幕上，不做缩放和剪裁，图像分辨率为640 X 480像素，色彩为16位，我们使用BT878A的精简指令通过DMA直接把图像数据写到显存里，按任意键，程序将退出。

> 程序使用C++完成，在DJGPP下编译完成，再看这段程序之前，希望读者能了解一些有关VESA的知识，否则其中有些地方可能会不大明白，而且本程序需要在支持VESA的机器上运行，现在一般的机器都是支持VESA的，你的机器是否支持VESA，可以下载下面这个程序检查一下。

> https://whowin.gitee.io/software/vesa.exe

> 不带参数运行vesa.exe可以看到你的机器支持的VESA的版本号，也可以看到你的机器支持的所有显示模式，我们的范例中用的显示模式号是：111h，所以要确认在你的显示模式中有这个号，这个模式是640 X 480，16 bits彩色，我们之所以选用这个模式是因为我的测试环境仅支持VESA 1.2，不支持24 bits的彩色模式。运行vesa a可以看到你的机器支持的所有显示模式的详细信息。

> 好了，范例程序的源代码可以在这里下载
可以在下面地址上下载：http://blog.whowin.net/source/bt878.zip

> 程序由三个文件组成：test878.cc、bt878.h、test878.h，其中bt878.h中只是定义了所有的BT878A的寄存器的偏移；test878.h定义了一些数据结构，两个类：DOS_MEM和VIRTUAL_MEM，以及关于RISC的相关定义。主要的程序代码均在test878.cc中，下面我们的解释主要围绕着test878.cc展开，以思路为主，具体细节请自行看源程序。

> 为了方便，程序中的有些地方调用了实模式下的功能，但涉及BT878A的操作部分都是32位的。

> 主程序开始CheckBIOS检查机器的BIOS是否支持PCI BIOS，如果不支持，后面函数中的一些调用将不成立；然后调用FindDevice搜寻是否有BT878A芯片，其中的0x036e和0x109e分别是BT878A的device ID和Vendor ID，最后一个参数是device index，BT878A是个多功能设备，但我们范例中只使用function 0，然后调用ReadConfigDWORD读出PCI配置空间的0x10位置的内容，从配置空间的定义看，这是BT878A的基地址，实际上，前面的所有工作就是为了读出这个基地址，通过这个基地址再加上偏移，我们就可以以访问存储器的方式访问所有的BT878A的内部存储器（BT878A是存储器映射的），然后我们用这个基地址初始化了一个VIRUAL_MEM类Bt878MapedAddr，通过这个类，我们将可以访问BT878A的寄存器。

> 下面SetMode我们把屏幕的显示模式设成0x111，这个我们在上面已经说过了，这是一个640 X 480，16 bits的显示模式，然后用GetModeInfo调用得到关于这种显示模式下的显示缓冲区的物理地址（32位地址，不是实模式下的段:偏移模式），最后我们以显示缓冲基地址初始化了一个VIRUAL_MEM类scr_virtual，通过这个类，我们可以访问到显示缓冲区，我们对这个类定义的大小是640 X 480 X 2 + 4096 + 16384，前面的640 X 480 X 2是这种显示模式所需要的空间大小，每个象素占2个字节，4096是为了防止显示缓冲区溢出特别增加的4K空间，最后的16384这16K的空间我们准备用来存放RISC指令。

> 至此我们有了几个重要的变量，如下：
  ```
    VesaPhysAddr          显示缓冲区的物理地址
    Bt878PhisicalAddr     BT878A的基地址
    Bt878MapedAddr        BT878A的寄存器映射类
    scr_virtual           显示缓冲区及RISC虚拟空间类
  ```

> 下面的很多代码我们在初始化BT878A的寄存器，以期适应我们的需要，这部分请参阅前面有关BT878A寄存器的介绍，

> 特别要注意的是create_risc_WxH函数，建立了RISC的指令序列，这个指令序列的思路在前面已经有介绍，请参照这个介绍以及BT878A的datasheet的有关章节来理解这段代码。

> 在设置完BT878A的寄存器，建立完RISC的指令序列后，我们启动RISC，你会在屏幕上看到摄像头拍到的连续图像，按任意键将停止RISC的运行，并推出图形显示模式，并退出程序。


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[article01]:/specification/bt878.pdf
[article02]:/post/blog/dos/0005-traverse-pci-devices/
[article03]:/post/blog/dos/0010-ac97-programing-in-dos/

[img01]:/images/160015/ntsc-video-frame.jpg
[img02]:/images/160015/pal-video-frame.jpg
[img03]:/images/160015/vbi-timing.jpg
[img04]:/images/160015/video-data-format-converter.jpg
[img05]:/images/160015/status-bits.jpg




