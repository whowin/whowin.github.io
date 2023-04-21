# Intel CPU的CPUID指令（一）  
**2009-02-05 14:16:38**

## 前言
  ```
    Intel有一个超过100页的文档，专门介绍cpuid这条指令，可见这条指令涉及内容的丰富。

    记得去年的时候，曾经有个“英布之剑”问过我这条指令，当时并没有给出一个满意的回答，现在放假，想起来，把资料整理了一下。很久以前确实用过这条指令，其实指令本身并没有什么难的，关键是看你有没有耐心研读完繁琐的资料，当然还得对CPU有一定的了解，如果“英布之剑”看到这篇文章，而且仍然需要更详细的资料，可以给我一个联系方式，或者相互之间可以交流一下。

    cpuid就是一条读取CPU各种信息的一条指令，大概是从80486的某个版本开始就存在了。似乎是从80386开始，当CPU被RESET以后，CPU会在EDX寄存器中返回一个32bits的CPU签名（Processor Identification Signature），但这时候CPU还没有CPUID这条指令，后来出现了这条指令后，软件无需以来CPU复位就可以读出这个CPU签名，同时还可以读出很多CPU的相关信息。

    CPUID这条指令，除了用于识别CPU（CPU的型号、家族、类型等），还可以读出CPU支持的功能（比如是否支持MMX，是否支持4MB的页等等），内容的确是十分丰富。CPUID指令有两组功能，一组返回的是基本信息，另一组返回的是扩展信息，本文介绍基本信息部分，扩展信息部分下篇中介绍。本文所在程序或程序片段，均使用MASM 6.11编译连接，可以在DOS（包括虚拟机的DOS下）运行。
  ```

## 1、如何判断CPU是否支持CPUID指令
* 前面说过，大概是从80486开始才有的cpuid这个指令，是不是所有的80486家族CPU都有这个指令我也不是很清楚，但在EFLAGS中的bit 21可以识别CPU是否支持CPUID指令，如下图：

  <img src="images\Intel CPU的CPUID指令（一）-01.jpg"><br />
  图1

* 在8086和8088CPU中，FLAGS只有16位长，在80386CPU中，bit 21被保留未用，在支持CPUID指令的CPU中，这一位将为1。

## 2、CPUID指令的执行方法
* 把功能代码放在EAX寄存器中，执行CPUID指令即可。例如：
  ```
    mov eax, 1
    cpuid
  ```
* 前面说过CPUID指令分为两组，一组返回基本信息，一组返回扩展信息，当执行返回基本信息的CPUID指令时，EAX中功能代码的bit 31为0，当执行返回扩展信息的CPUID指令时，EAX中的功能代码的bit 31为1。那么不管是那组功能，如何知道EAX中的功能代码最大可以是多少呢？根据Intel的说明，可以用如下方法：
  ```
    mov eax, 0
    cpuid
  ```
* 执行完CPUID指令后，EAX中返回的值就是返回基本信息时，功能代码的最大值，在执行CPUID指令要求返回基本信息时，EAX中的值必须小于或等于该值。
  ```
    mov eax, 80000000h
    cpuid
  ```
* 执行完CPUID指令后，EAX中返回的值就是返回扩展信息时，功能代码的最大值，在执行CPUID指令要求返回扩展信息时，EAX中的值必须小于或等于该值。
* 由于很多编译器都不能编译CPUID指令，所以了解CPUID指令的操作码是必要的，CPUID指令的操作码是：0FA2h

## 3、返回基本信息的功能全貌
* 在实际介绍每一个功能之前，我们先通过一张图了解一下返回基本信息的功能全貌。

  <img src="images\Intel CPU的CPUID指令（一）-02.jpg"><br />
  <img src="images\Intel CPU的CPUID指令（一）-03.jpg"><br />
  图2

## 4、EAX=0：获取CPU的Vendor ID
* Vendor ID这个东西，在以前介绍PCI的文章中应该介绍过，实际上就是制造商的标识，用下面的方法执行该功能：
  ```
    mov eax, 0
    cpuid
  ```
* 执行CPUID指令后，AX中返回的内容前面已经说过了，返回的Vendor ID固定为12个ASCII字符依次存放在EBX、EDX、ECX中，对于Intel的CPU，返回的字符串永远是：GenuineIntel。对应在三个寄存器中的值如下：
  ```
    EBX=756E6547h，EDX=49656E69h，ECX=6C65746Eh

    大家可以参考图2。
  ```
* 尽管本文是介绍Intel的CPUID指令，但下面还是尽我所知，列出其它厂家生产的IA-32架构CPU的Vendor ID，希望能对需要这些资料的人有所帮助。
  - AMDisbetter! ---- 早期AMD K5芯片的工程样品芯片
  - AuthenticAMD ---- AMD
  - CentourHauls ---- Centour
  - CyrixInstead ---- Cyrix
  - GenuineTMx86 或 TransmetaCPU ---- Transmeta
  - Geode by NSC ---- National Semiconductor
  - NexGenDriven ---- NexGen
  - SiS SiS SiS  ---- SiS
  - RiseRiseRise ---- Rise
  - UMC UMC UMC  ---- UMC
  - VIA VIA VIA  ---- VIA

## 5、EAX=1：处理器签名（Processor Signiture）和功能（Feature）位
  ```
    mov eax, 1
    cpuid
  ```
* 执行完成后，处理器签名放在EAX中，功能位及其它杂七杂八的内容分别放在EBX、ECX和EDX中。
* 处理器签名（Processor Signiture）：
  - 返回在EAX中，定义如下：

    <img src="images\Intel CPU的CPUID指令（一）-04.jpg"><br />

  - 图中的灰色区域表示没有定义。前面说过，当CPU复位时，会在EDX中返回处理器签名，从80486以后，这个签名和上面的定义完全一样，只是放在不同的寄存器中而已。前面还提到过，80386在复位时也返回处理器签名，但80386返回的签名格式是和上面不同的，后面可能会提到。
  - 通过处理器签名，可以确定CPU的具体型号，以下是部分Intel CPU的处理器签名数据（资料来自Intel）：

    <img src="images\Intel CPU的CPUID指令（一）-05.jpg"><br />

  - 前面说过，80386尽管没有CPUID指令，但在复位时也是可以返回处理器签名的，下面是80386返回的处理器签名的格式：

    <img src="images\Intel CPU的CPUID指令（一）-06.jpg"><br />

  - 下面是80386处理器签名的识别方法（资料来自Intel）：

    <img src="images\Intel CPU的CPUID指令（一）-07.jpg"><br />

* 关于Stepping的说明：
  ```
    Intel和AMD都有Stepping的概念，用来标识一款同样规模的微处理器从一开始到你用的这款处理器经历的设计过程，用一个字母和一个数字表示。一般来说，一款同样规模的微处理器的第一个版本是A0，如果改进了设计，则可以通过改变字母或者数字进行标识，如果仅仅改变数字（比如改成A3），说明进行了一些辅助的改进，如果字母和数字都改变，说明改动较大，Stepping可以使用户可以识别微处理器的版本。下面是一个Stepping的例子（不知道为什么穿上来后图这么小）。
  ```

  <img src="images\Intel CPU的CPUID指令（一）-08.jpg"><br />

* 当处理器签名一样时的处理
  ```
     有时候，从处理器签名上仍然不能识别CPU，比如根据Intel提供的资料，Pentium II, Model 5、Pentium II Xeon, Model 5和Celeron?, Model 5的处理器签名完全一样，要区别他们只能通过检查他们的高速缓存（Cache）的大小，后面将介绍使用CPUID指令获得CPU高速缓存信息的方法，如果没有高速缓存，则是Celeron?处理器；如果L2高速缓存为1MB或者2MB，则应该是Pentium II Xeon处理器，其它情况则应该是Pentium II处理器或者是只有512KB高速缓存的Pentium II Xeon处理器。
  ```
  ```
    有些情况下，如果从处理器签名上不能区分CPU，也可以使用Brand ID(在EBX的bit7:0返回）来区分CPU，比如Pentium III, Model 8、Pentium III Xeon, Model 8和Celeron?, Model 8三种处理器的处理器签名也是一样的，但它们的Brand ID是不同的。
  ```
* 关于处理器类型的定义
  ```
    在处理器签名中的bit12:13返回的是处理器类型，其定义如下

    Value    Descriptor
    ---------------------------------------------
     00      以前的OEM处理器
     01      OverDrive?处理器
     10      多处理器（指可用于多处理器系统）
  ```

* 功能标志（Feature Flag）
  ```
    在EDX和ECX中返回的功能标志表明着该CPU都支持那些功能，EDX定义如下（资料来源与Intel）：
  ```
  ```
    bit  Name  Description
    --------------------------------------------
     00  FPU   FPU On-chip
     01  VME   Virtual Mode Extended
     02  DE    Debugging Extension
     03  PSE   Page Size Extension
     04  TSC   Time Stamp Counter
     05  MSR   Model Specific Registers 
     06  PAE   Physical Address Extension
     07  MCE   Machine-Check Exception
     08  CX8   CMPXCHG8 Instruction
     09  APIC  On-chip APIC Hardware
     10        Reserved
     11  SEP   Fast System Call
     12  MTRR  Memory Type Range Registers
     13  PGE   Page Global Enable
     14  MCA   Machine-Check Architecture
     15  CMOV  Conditional Move Instruction
     16  PAT   Page Attribute Table
     17  PSE-36 36-bit Page Size Extension
     18  PSN   Processor serial number is present and enabled
     19  CLFSH CLFLUSH Instruction
     20        Reserved
     21  DS    Debug Store
     22  ACPI  Thermal Monitor and Software Controlled Clock Facilities
     23  MMX   MMX technology
     24  FXSR  FXSAVE and FXSTOR Instructions
     25  SSE   Streaming SIMD Extensions
     26  SSE2  Streaming SIMD Extensions 2
     27  SS    Self-Snoop
     28  HTT   Multi-Threading
     29  TM    Thermal Monitor
     30  IA64  IA64 Capabilities
     31  PBE   Pending Break Enable
  ```
  ```
    ECX定义如下（资料来自Intel）：
  ```
  ```
     bit   Name     Description
    -------------------------------------------------------
      00   SSE3     Streaming SIMD Extensions 3
      01            Reserved
      02   DTES64   64-Bit Debug Store
      03   MONITOR  MONITOR/MWAIT
      04   DS-CPL   CPL Qualified Debug Store
      05   VMX      Virtual Machine Extensions
      06   SMX      Safer Mode Extensions
      07   EST      Enhanced Intel SpeedStep? Technology
      08   TM2      Thermal Monitor 2
      09   SSSE3    Supplemental Streaming SIMD Extensions 3
      10   CNXT-ID  L1 Context ID
    12:11           Reserved
      13   CX16     CMPXCHG16B
      14   xTPR     xTPR Update Control
      15   PDCM     Perfmon and Debug Capability
    17:16           Reserved
      18   DCA      Direct Cache Access
      19   SSE4.1   Streaming SIMD Extensions 4.1
      20   SSE4.2   Streaming SIMD Extensions 4.2
      21   x2APIC   Extended xAPIC Support
      22   MOVBE    MOVBE Instruction
      23   POPCNT   POPCNT Instruction
    25:24           Reserved
      26   XSAVE    XSAVE/XSTOR States
      27   OSXSAVE
    31:28           Reserved
  ```
* 下面是在DEBUG中当EAX=0时执行CPUID指令时的情况：

  <img src="images\Intel CPU的CPUID指令（一）-09.jpg"><br />

* 下面是在DEBUG中当EAX=1时执行CPUID指令时的情况

  <img src="images\Intel CPU的CPUID指令（一）-10.jpg"><br />

    （未完待续） 