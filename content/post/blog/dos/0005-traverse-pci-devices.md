---
title: "遍历 PCI 设备"
date: 2008-04-02T09:15:23+08:00
author: whowin
sidebar: false
authorbox: false
pager: true
toc: true
categories:
  - "DOS"
tags:
  - "DOS"
  - "DJGPP"
  - "保护模式"
  - "RHIDE"
  - "PCI"
draft: false
postid: 160005
---

PC机在启动的时候，都会看到一个PCI设备清单，可以看到机器中的所有PCI设备，其实搜索PCI设备的程序并不难编，本文通过一个实例说明如何遍历PCI设备。本文程序的工作环境：MS-DOS 6.22，djgpp+RHIDE。本文初次发表于2008年4月2日，2011年3月11日做过修改，将其中的代码片段换为完整代码。
<!--more-->


## 1、了解PCI设备

  > PCI的含义是外设部件互连（Peripheral Component Interconnect），PCI局部总线（Local Bus）是1991年由Intel定义的，现在PCI局部总线已经成为了PC机中不可缺少的外围设备总线，几乎所有的外部设备都连接到PCI局部总线上，我们说的PCI设备，实际上就是指连接在PCI局部总线上的设备。

## 2、你的 BIOS 是否支持 PCI BIOS 服务
  > 在我的另一篇博客文章：[《32位BIOS说明》][article01]中，说明了如何判断 **BIOS** 是否是32位的，以及是否支持PCI BIOS服务，实际上，并没有那么麻烦，因为在绝大多数有 PCI 插槽的 PC 机中，BIOS 都是32位的，同时也是支持 PCI BIOS 的，在我的另一篇博客文章：[《调用PCI BIOS》][article02]中，介绍了PCI BIOS中常用的功能调用，其中的第一个调用就是检查 **PCI BIOS的存在性**，通常我们利用这个功能调用来作为 PCI BIOS 的存在检测已经足够了，方法如下：
  ```
    汇编语言：

        MOV  AX, 0B01H
        INT 1AH
  ```
  ```
    C语言（DJGPP下）：

        long unsigned int    i;
        __dpmi_regs           r;

        r.x.ax = 0xb101;
        __dpmi_int(0x1a, &r);
        i = r.x.flags;
        if ((i & 0x01) == 0) printf("\nSupport PCI BIOS");
        else printf("\nNot Suport PCI BIOS");
  ```

  > 根据返回参数就可以判断 **PCI BIOS** 是否存在，具体方法请参阅：[《调用PCI BIOS》][article02]这篇博客文章，如果你的BIOS不支持PCI BIOS，那么本文介绍的方法可能不完全适用你。

## 3、了解PCI配置空间
  * 学习PCI编程，不了解PCI的配置空间是不可能的，配置空间是一块容量为 **256** 字节并具有特定记录结构或模型的地址空间，通过配置空间，我们可以了解该 **PCI** 设备的一些配置情况，进而控制该设备，除主总线桥以外的所有 **PCI** 设备都必须事先配置空间，本节仅就一些配置空间的共有的规定作一些说明，更加具体和详细的信息请参阅其他书籍及相应的芯片手册。
  * 配置空间的前 64 个字节叫头标区，头标区又分成两个部分，第一部分为前 16 个字节，在各种类型的设备中定义都是一样的，其他字节随各设备支持的功能不同而有所不同，位于偏移 0EH 的投标类型字段规定了是何种布局，目前有三种头标类型，头标类型 1 用于 PCI-PCI 桥，头标类型 2 用于 PCI-CARDBUS 桥，头标类型 0 用于其他 PCI 设备，下图为头标类型 0 的头标区布局。

    ![遍历PCI设备][img01]

    **图1：遍历 PCI 设备**

  * 头标区中有 5 个字段涉及设备的识别。
    1. 供应商识别字段（Vendor ID）
        > 偏移：00H。该字段用以标明设备的制造者。一个有效的供应商标识由PCI SIG来分配，以保证它的唯一性。0FFFFH是该字段的无效值。

    2. 设备识别字段（Device ID）
        > 偏移：02H。用以标明特定的设备，具体代码由供应商来分配。

    3. 版本识别字段（Revision ID）
        > 偏移：08H。用来指定一个设备特有的版本识别代码，其值由供应商提供，可以是0。

    4. 头标类型字段（Header Type）
        > 偏移：0EH。该字段有两个作用，一是用来表示配置空间头标区第二部分的布局类型；二是用以指定设备是否包含多功能。位7用来标识一个多功能设备，位7为0表明是单功能设备，位7为1表明是多功能设备。位0-位6表明头标区类型。

    5. 分类代码字段（Class Code）
        > 偏移：09H。标识设备的总体功能和特定的寄存器级编程接口。该字节分三部分，每部分占一个字节，第一部分是基本分类代码，位于偏移0BH，第二部分叫子分类代码，位于偏移0AH处，第三部分用于标识一个特定的寄存器级编程接口（如果有的话）。

  * 这部分的代码定义很多，请自行查阅 **PCI规范** 相关文档。我们在实际应用中会对一些用到的代码加以说明。

## 4、配置寄存器的读写
  * x86的CPU只有内存和I/O两种空间，没有专用的配置空间，PCI协议规定利用特定的I/O空间操作驱动PCI桥路转换成配置空间的操作。目前存在两种转换机制，即配置机制1#和配置机制2#。配置机制2#在新的设计中将不再被采用，新的设计应使用配置机制1#来产生配置空间的物理操作。这种机制使用了两个特定的32位I/O空间，即CF8h和CFCh。这两个空间对应于PCI桥路的两个寄存器，当桥路看到CPU在局部总线对这两个I/O空间进行双字操作时，就将该I/O操作转变为PCI总线的配置操作。寄存器CF8h用于产生配置空间的地址（CONFIG-ADDRESS），寄存器CFCh用于保存配置空间的读写数据（CONFIG-DATA）。
  * 将要访问配置空间寄存器的总线号、设备号、功能号和寄存器号以一个双字的格式写到配置地址端口 (CF8H-CFBH)，接着执行配置数据端口 (CFCH)的读和写，向配置数据口写数据即向配置空间写数据，从配置数据口读数据即从配置空间读数据。
  * 配置地址端口（CF8H）的格式定义如下：
    ```
      bit  31  30...24 23......16 15......11 10......8 7..........2   1  0
            |    保留     总线号     设备号    功能号      寄存器号     0  0
            +----使能位，1有效，0无效

      寄存器号：选择配置空间中的一个双字（32位）
      功能号：选择多功能设备中的某一个功能，有八种功能，0--7
      设备号：在一条给定的总线上选择32个设备中的一个。0--31
      总线号：从系统中的256条总线中选择一条，0--255
    ```

  * 尽管理论上可以有 **256** 条总线，但实际上 PC 机上 *8PCI** 插槽的总线号都是 **1**，有些工控机的总线号是 2 或者 3，所以我们只需要查找 **0--4** 号总线就足够了。
  * PCI规范规定，功能 **0** 是必须实现的，所以，如果功能 **0** 的头标类型字段的位 7 为 0，表明这是一个单功能设备，则没有必要再去查其他功能，否则要查询所有其他功能。

## 5、遍历PCI设备
  * 至此，我们掌握的有关 PCI 的知识已经足够我们遍历 PCI 设备了，其实便利方法非常简单就是按照总线号、设备号、功能号的顺序依次罗列所有的可能性，读取配置空间头标区的供应商代码、及设备代码，进而找到所有 PCI 设备。

    ```
    #include <stdio.h>
    #include <dpmi.h>

    typedef unsigned long      UDWORD;
    typedef unsigned short int UWORD;

    int main() {
        UDWORD i, busNo, deviceNo, funcNo, regVal, retVal, baseAddr;
        UWORD  vendorID, devID, class1, class2, class3;

        __dpmi_regs r;

        r.x.ax = 0xb101;
        __dpmi_int(0x1a, &r);
        i = r.x.flags;
        if ((i & 0x01) == 0) printf("\nSupport PCI BIOS.");
        else printf("\nNot Support PCI BIOS.");

        printf("\nNo.  Vendor/Device  Bus No.  Dev No.  Func No.  Class");
        i = 0;
        for (busNo = 0; busNo < 5; busNo++) {                      // bus No
            for(deviceNo = 0; deviceNo < 32; deviceNo++) {         // device no
                for (funcNo = 0; funcNo < 8; funcNo++) {           // Function No
                    //j = 0x80000000 + i * 2048;
                    regVal = 0x80000000                            // bit31 使能
                            + (busNo << 16)                        // Bus No
                            + (deviceNo << 11)                     // Device No
                            + (funcNo << 8);                       // Function No
                    outportl(0xCF8, regVal);
                    retVal = inportl(0xCFC);                       // 得到配置空间偏移为0的双字
                    if (retVal != 0xffffffff) {                    // 设备存在
                        i++;
                        vendorID = retVal & 0xffff;                // 得到供应商代码
                        devID    = (retVal >> 16) & 0xffff;        // 得到设备代码
                        regVal += 0x08;                            // 得到配置空间偏移为08H的双字
                        outportl(0xCF8, regVal);
                        retVal = inportl(0xCFC);
                        retVal = retVal >> 8;                      // 滤掉版本号
                        class3 = retVal & 0x0FF;                   // 得到三个分类代码
                        class2 = (retVal >> 8) &0x0FF;
                        class1 = (retVal >> 8) &0x0FF;
                        printf("\n%02d   %04x/%04x       %02x       %02x       %02x      %02x-%02x-%02x",
                              i, vendorID, devID, busNo, deviceNo, funcNo, class1, class2, class3);
                        if (funcNo == 0) {                         // 如果是单功能设备，则不再查funcNo>0的设备
                            regVal = (regVal & 0xFFFFFFF0) + 0x0C; // 配置空间偏移0X0C，
                            outportl(0xCF8, regVal);
                            retVal = inportl(0xCFC);
                            retVal = retVal >> 16;                 // 偏移0X0E为投标类型字段
                            if ((retVal & 0x80) == 0) funcNo = 8;  // bit 7为0表示为单功能设备，不再查该设备的其他功能
                        }
                    }
                }
            }
        }
    }   // end of main
    ```
  * 这是一个完整的遍历PCI设备的程序，本代码可以DJGPP下编译通过并正常执行（测试环境：DOS 6.22 DJGPP2.2+RHIDE1.5）。

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png



[article01]:../0001-32bit-bios_introduction/
[article02]:../0002-invoking-pci-bios/

[img01]:/images/160005/traverse_PCI_devices_01.jpg
[-img01]:../../../static/images/160005/traverse_PCI_devices_01.jpg
