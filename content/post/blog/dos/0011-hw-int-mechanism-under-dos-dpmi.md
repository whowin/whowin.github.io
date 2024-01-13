---
title: "DOS DPMI下的硬件中断机制"
date: 2008-05-07T17:45:03+08:00
author: whowin
sidebar: false
authorbox: false
pager: true
toc: true
categories:
  - "DOS"
tags:
  - DOS
  - DPMI
draft: false
postid: 160011
---

本文旨在探讨DPMI(DOS Protect Mode Interface)环境下的硬件中断机制，以及在此环境下硬件中断的编写方法，本文并不探讨实模式下和保护模式下硬件中断的原理，但文中难免涉及一些这方面的知识，请读者自行阅读相关资料解决。在本文的最后将举一个硬件中断的编写实例。本文主要讨论的环境为：DOS6.22 + DJGPP + CWSDPMI。CWSDPMI为DJGPP自带的DPMI服务，也是一个好用的且免费的DPMI，有关这个环境的搭建，请参阅我以前的博文。**本文是我2008年的作品，2023年重新整理发布，仅为存档，其中的程序并没有再次验证，特此说明。**
<!--more-->

## 1、DPMI下的硬件中断机理

> 实模式下的硬件中断有很多书籍中都有介绍，保护模式下的硬件中断要复杂一些，但也有一些资料。本博客中的大部分文章将介绍在DOS环境下使用DJGPP完成保护模式下的程序，编译出来的程序需要在DPMI的支持下运行在保护模式下，通常情况下，并没有什么，当你需要调用DOS下的实模式下的功能调用时，DPMI会帮助你切换到实模式，完成后再返回保护模式，基本上不需要编程者去做任何处理事情，但在处理硬件中断方面就比较微妙了，我们究竟应该在实模式下完成硬件中断还是在保护模式下，如果原有的硬件中断工作在实模式下，而我却编了一个保护模式下的硬件中断程序，按规则，在我的中断例程完成后，我必须再执行原有的硬件中断，显然这里有些问题。

> 首先，不论是在实模式下还是在保护模式下，硬件中断都会产生，当程序运行在DPMI下时，尽管你的程序是32位的，运行在保护模式下，但当你调用实模式下DOS的功能调用时，DPMI会切换到实模式下，可能一个硬件中断恰恰在这个时候产生了，但不管什么时候发生了硬件中断，只要你的程序运行在DPMI下，DPMI将截获硬件中断，并将首先传送给保护模式下的硬件中断程序执行，然后再转到实模式下的中断例程；但并不是所有的DPMI都将在两种模式下执行中断例程，比如我们本文讨论的环境，CWSDPMI下，DPMI截获中断并交给保护模式下的中断例程，只有在保护模式的中断例程不存在的情况下，DPMI才会去执行实模式下的中断例程，在CWSDPMI下，DPMI只执行一种模式下的中断例程；但WINDOWS 95下的DPMI就不一样，windows宣称会执行把两种模式下的中断例程都执行一遍（抱歉，我并没有验证这一说法）；所以，一定要了解你所处的环境，在下面的叙述中，如果不特别说明，均只在CWSDPMI下，综上所述，在DPMI下，你可以只编写一个保护模式下的中断例程，而无须管实模式下的中断例程。

> 但是，我们毕竟是在DOS下编程，很多资源来自于实模式的DOS，所以如果你的中断程序中需要大量地调用DOS的资源时，你必须要考虑编写实模式的中断程序，否则在你的中断程序中DPMI将不断地在实模式和保护模式之间切换，这将十分麻烦。

## 2、DPMI下编写硬件中断的方法

> 保护模式下的中断程序，按下面方法设置。

* 用汇编语言编写的中断程序，可以按照下面的步骤进行设置：
  - 取得原有的中断程序结构
    > 调用__dpmi_get_protected_mode_interrupt_vector函数，并把返回值存在结构中，以便程序退出时可以恢复原来的中断程序
  - 锁定内存
    > 调用__dpmi_lock_linear_region函数锁定在中断程序中可能用到的数据变量，代码，调用的函数；锁定失败有可能造成你的程序崩溃；
  - 设置新的中断程序
    > 调用__dpmi_set_protected_mode_interrupt_vector，把新中断程序的选择子（selector）和偏移（offset）填入__dpmi_paddr中，传给函数，其中，pm_selector可以通过函数__dpmi_cs()获得
    
* 如果中断程序用C语言写成，可以按照下面方法设置：
  - 取得原有的中断程序结构
    > 调用_go32_dpmi_get_protected_mode_interrupt_vector，将返回值存入适当的结构中，以便程序退出时恢复原有的中断程序
  - 对中断程序进行再包装
    > 调用_go32_dpmi_allocate_iret_wrapper函数，按照要求传递参数，该函数将按照中断程序的规范建立一个小的汇编函数对你的C语言写的中断程序进行包装，该函数填充的结构_go32_dpmi_seginfo将作为下面函数调用的参数。
  - 设置新的中断程序
    > 调用_go32_dpmi_set_protected_mode_interrupt_vector函数，把上一步骤返回的_go32_dpmi_seginfo作为参数传给该函数。
  - 链接原有中断
    > 如果需要在执行完你的中断程序后执行原有的中断程序，不必执行前面两个步骤，直接调用_go32_dpmi_chain_protected_mode_interrupt_vector即可，就是说不用调用_go32_dpmi_allocate_iret_wrapper和_go32_dpmi_set_protected_mode_interrupt_vector这两个函数，_go32_dpmi_chain_protected_mode_interrupt_vector将完成所有的工作，在填结构_go32_dpmi_seginfo时，把中断程序的偏移放到pm_offset中，把_my_cs()的返回值放到pm_selector中即可；用这种方式设置的中断你只能通过DJGPP的退出代码进行释放，别无他法。

> 用C语言写中断程序的主要问题是你根本没有办法完全锁定你的中断程序所用到的存储器，所以，这种方法只适用于那些较小的程序，你可以确认你的程序不会被page到磁盘上去；这一点要特别引起注意，一般情况下，中断程序尽量使用汇编语言完成比较好。

## 3、DPMI下一个时钟中断的实例

* 老实说，没有什么很好的例子可以给大家演示中断程序，本来想结合前面博文中的AC97的内容编写一个PCI的中断例子，但复杂一些，涉及的问题也比较多，好在比较幸运的是我找到了一个int 8h的例子，个人感觉写的比较简单，好懂，对这段代码做了大量的修改，原程序接管了int 8h，我修改后的程序接管了int 8h和int 70h两个中断，我觉得这样更能说明问题。

* int 8h(IRQ 0)是时钟中断，来自可编程的时钟芯片8254，现在的机器中已经没有了这颗芯片，给集成到了别的芯片中，不过保留了原来的I/O端口及8254的控制方法，所以仍然可以使用控制8254的方法控制时钟；要说明这个例子，我们不得不先简单介绍一下8254这颗芯片。

* 8254定时器这颗芯片有三个定时通道，每个通道有一个I/O地址，timer0 - timer2对应的I/O地址为40h - 42h，这个I/O通道是8位的，通过这个8位的I/O通道可以对8254进行编程
* 8254内部有一个16位的锁存寄存器，编程时送入的计数值首先进入这个锁存寄存器，然后被复制到计数寄存器，当接到8254上的时钟信号每来一个脉冲时，这个计数寄存器中的数值将减一，当减到0时，8254将输出一个脉冲，同时再次将锁存寄存器中的内容复制到计数寄存器中，开始下一轮的计数。
* 所以，8254芯片的每个通道有两个输入信号：一个是时钟信号，一个是门控信号，用于控制该通道工作或者不工作(后面我们会提到)；一个输出信号，即计数完毕后的输出信号。
* 一般情况下，timer0用于产生每秒18.2次的系统时钟中断(int 8h)，这就是我们下面例子中要编的中断；timer1用于存储器刷新(现在应该不用了？)；timer2连接扬声器，可以产生方波使扬声器发声。这里说到的知识可能有点老，尤其是timer1的用法，不过不妨碍我们的例子。
* 8254还有一个控制寄存器，I/O地址为43h，8位通道，该寄存器的定义如下：

  ![DPMI下的硬件中断][img01]

* 对这个控制寄存器，还是简单说明一下：
  - bit [6:7]
    > 准备对那个定时器通道进行操作，timer0 or timer1 or timer2
  - bit [4:5]
    ```
    读/写当前计数值
    00：读当前计数值，先读低8位，后读高8位，要两次in指令
    01：只写高8位，一次out指令
    10：只写低8位，一次out指令
    11：先写低8位，后写高8位，要两次out指令
    ```
  - bit [1:3]
    > 工作模式，有6种工作模式，其中模式0（000）为计数结束后产生中断；模式3（003）为方波频率发声器，这两种模式后面要用
  - bit 0
    > 写入值的格式，0--二进制格式，1--BCD码格式

* 前面说到，8254的输入信号里还有一个门控信号，对于timer0和timer1，这个信号永远是选通的，但timer2的门控信号是可控的，由I/O端口61h的bit 0和bit 1控制，该I/O端口的bit0为1则关断timer2的门控信号，为0开启门控信号，bit1和可以控制timer2和扬声器的通断，我们不需要使用这个信号。
* pc机种还有一个实时时钟中断，int 70h(IRQ 8)，由以前的MC146818这颗芯片产生，现在这颗芯片也已经集成到其他芯片中了，不过操作和控制方法还是相同，MC146818可以产生多种类型的中断，在初始状态都是关闭的，我们仅用它的周期中断，缺省为每秒1024次，我们并没有更改任何参数，仅仅是在中断中增加了一个中断计数，我们希望以此中断来检验我们接管的int 8h的正确性。
* 好了，基本知识介绍完了。下面是程序清单：
  ```
  001 #include <stdio.h>
  002 #include <dos.h>
  003 #include <string.h>
  004 #include <dpmi.h>
  005 #include <pc.h>

  006 void pm_new8h(void);
  007 void lock_pm_new8h(void);
  008 void pm_new70h(void);
  009 void lock_pm_new70h(void);
  010 char *get_cmostime(void);

  011 #define IRQ0                 0x8       // 8254 time interrupt
  012 #define IRQ8                 0x70      // Real time interrupt
  013 #define PIT0                 0x40      // 8254 timer0
  014 #define PITMODE              0x43      // 8254 control word
      /* 8254 controll word
          bit 7:6 timer number 00--timer0  01--timer1  10--timer2
          bit 5:4 latch,read format;
                  00--latch current count;
                  01--write low byte(no latching);
                  10--write high byte(no latching);
                  11--write low,then high byte
          bit 3:1 mode number;
                  000--interrupt on terminal count;
                  001--programmable one-shot;
                  010--rate generator
                  011--sequare wave generator;
                  100--software triggered strobe;
                  101--hardware triggered strobe
          bit 0   count byte; 0--binary 1--bcd
      */
  015 #define IMR0                 0x21       // IMR of 8259-0
  016 #define IMR1                 0xa1       // IMR of 8259-1
  017 #define RTC_ADDR             0x70
  018 #define RTC_DATA             0x71
  019 #define PITCONST             1193180L   // Frequence of 8254
  020 #define PIT0DEF              18.2067597 // ticks per sec of previous int
  021 #define RT_MS_PER_TICK       0.9765625
  022 #define NEW8H                1          // flag. new int 8h valiable.

  023 static float tick_per_ms    = 0.0182068;  // ticks per ms
  024 static float ms_per_tick    = 54.9246551; // interval between two ticks
  025 static float freq8h         = 18.2067597; // frequency of system timer
  026 static unsigned char flag8h = 0;
  027 static _go32_dpmi_seginfo old_handler_8h, new_handler_8h;
  028 static _go32_dpmi_seginfo old_handler_70h, new_handler_70h;
  029 static _go32_dpmi_registers r;
  030 unsigned long int ticks_8h  = 0;
  031 unsigned long int ticks_70h = 0;

      /***********************************************
      * New int 8h initialization
      * It chains int routine of protected mode and
      * real mode
      ***********************************************/
  032 void pctimer_init(unsigned int Hz) {
  033     unsigned int pit0_set, pit0_value;

  034     if (flag8h != NEW8H) {      // Current int 8h is old.
  035         disable();                // Disable interrupts

  036         lock_pm_new8h();         // Lock int routine in protected mode
  037         lock_pm_new70h();        // Lock int routine in protected mode

              // Lock some var that will be used in int routine
  038         _go32_dpmi_lock_data(&ticks_8h, sizeof(ticks_8h));
  039         _go32_dpmi_lock_data(&ticks_70h, sizeof(ticks_70h));
  040         _go32_dpmi_lock_data(&r, sizeof(r));

              // Get the previous int routine in protected mode
              // and chain a new routine
  041         _go32_dpmi_get_protected_mode_interrupt_vector(IRQ0, &old_handler_8h);
  042         new_handler_8h.pm_offset = (int)pm_new8h;
  043         new_handler_8h.pm_selector = _go32_my_cs();
  044         _go32_dpmi_chain_protected_mode_interrupt_vector(IRQ0, &new_handler_8h);

  045         _go32_dpmi_get_protected_mode_interrupt_vector(IRQ8, &old_handler_70h);
  046         new_handler_70h.pm_offset   = (int)pm_new70h;
  047         new_handler_70h.pm_selector = _go32_my_cs();
  048         _go32_dpmi_allocate_iret_wrapper(&new_handler_70h);
  049         _go32_dpmi_set_protected_mode_interrupt_vector(IRQ8, &new_handler_70h);
              // initial RTC
  050         outportb(RTC_ADDR, 0x0b);
  051         outportb(RTC_DATA, 0x42);
              // initial 8259-2
  052         outportb(IMR1, 0x0c);
              // initial Timer0 of 8254
  053         outportb(PITMODE, 0x36);      // 8254 command register
                                            // 00110110b.timer0,write low and high
                                            // sequare wave generator,binary
  054         pit0_value = PITCONST / Hz;
  055         pit0_set = (pit0_value & 0x00ff);
  056         outportb (PIT0, pit0_set);
  057         pit0_set = (pit0_value >> 8);
  058         outportb (PIT0, pit0_set);
  059         // initial vars for int
  060         ticks_8h      = 0;
  061         flag8h        = NEW8H;               // new int 8h
  062         freq8h        = Hz;                  // frequence of new int
  063         tick_per_ms   = freq8h / 1000;       // ticks per ms
  064         ms_per_tick   = 1000 / freq8h;       // ms of per tick

  065         enable();                           // enable interrupts
  066     }
  067 }

  068 void pctimer_exit(void) {
  069     unsigned int pit0_set, pit0_value;
  070     unsigned long tick;
  071     char *cmostime;

  072     if (flag8h == NEW8H) {
  073         disable();

  074         outportb(PITMODE, 0x36);
  075         outportb(PIT0, 0x00);
  076         outportb(PIT0, 0x00);

  077         outportb(RTC_ADDR, 0x0b);
  078         outportb(RTC_DATA, 0x02);
  079         outportb(IMR1, 0x0d);

  080         _go32_dpmi_set_protected_mode_interrupt_vector(IRQ0, &old_handler_8h);
  081         _go32_dpmi_free_iret_wrapper(&new_handler_70h);
  082         _go32_dpmi_set_protected_mode_interrupt_vector(IRQ8, &old_handler_70h);

  083         enable();

  084         cmostime = get_cmostime();
  085         tick = PIT0DEF *
                  (
                      (((float) *cmostime) * 3600) +
                      (((float) *(cmostime + 1)) * 60) +
                      (((float) *(cmostime + 2)))
                  );
  086         biostime(1, tick);

  087         flag8h = 0;
  088         freq8h = PIT0DEF;
  089         tick_per_ms = freq8h / 1000;
  090         ms_per_tick = 1000 / freq8h;
  091     }
  092 }
      /****************************************
      * New int 8h routine in protected mode
      ****************************************/
  093 void pm_new8h(void) {
  094     ticks_8h++;
  095 }
      /************************************************
      * Lock the memory of new int 8h routine region
      * in protected mode
      ************************************************/
  096 void lock_pm_new8h(void) {
  097     _go32_dpmi_lock_code(pm_new8h, (unsigned long)(lock_pm_new8h - pm_new8h));
  098 }
      /****************************************
      * New int 70h routine in protected mode
      ****************************************/
  099 void pm_new70h(void) {
  100     disable();
  101     ticks_70h++;
  102     outportb(RTC_ADDR, 0x0c);
  103     inportb(RTC_DATA);
  104     outportb(0xa0, 0x20);
  105     outportb(0x20, 0x20);
  106     enable();
  107 }
      /************************************************
      * Lock the memory of new int 70h routine region
      * in protected mode
      ************************************************/
  108 void lock_pm_new70h(void) {
  109     _go32_dpmi_lock_code(pm_new70h, (unsigned long)(lock_pm_new70h - pm_new70h));
  110 }
      /*****************************************
      * Sleep delayms ms
      *****************************************/
  111 void pctimer_sleep(unsigned int delayms) {
  112     unsigned long int delaytick;

  113     delaytick = delayms * tick_per_ms + ticks_8h;
  114     do {
  115     } while (ticks_8h <= delaytick);
  116 }

  117 char *get_cmostime(void) {
  118     char *buff;
  119     static char buffer[6];
  120     char ch;

  121     buff = buffer;
  122     memset(&r, 0, sizeof (r));
  123     r.h.ah = 0x02;
  124     _go32_dpmi_simulate_int(0x1a, &r);

  125     ch = r.h.ch;
  126     buffer[0] = (char)((int)(ch & 0x0f) + (int)((ch >> 4) & 0x0f) * 10);
  127     ch = r.h.cl;
  128     buffer[1] = (char)((int)(ch & 0x0f) + (int)((ch >> 4) & 0x0f) * 10);
  129     ch = r.h.dh;
  130     buffer[2] = (char)((int)(ch & 0x0f) + (int)((ch >> 4) & 0x0f) * 10);
  131     buffer[3] = r.h.dl;
  132     buffer[4] = (char)(r.x.flags & 0x0001);
  133     buffer[5] = 0x00;

  134     return(buff);
  135 }
      // Main program
  136 int main(void) {
  137     int i;
  138     unsigned long int start, finish;
  139     int elapsedtime[20];
  140     float average_error, sum = 0;

  141     printf("Press any key to begin the test. ");
  142     printf("(It lasts for 20 seconds.)\n\n");
  143     getch();
  144     printf("Test in progress.  Please wait...\n");

  145     pctimer_init(1000);         // 1000Hz, 1000 ticks per sec

  146     for (i = 0; i < 10; i++) {
  147         start = ticks_70h;
  148         pctimer_sleep(1000);
  149         finish = ticks_70h;
  150         elapsedtime[i] = (int)(finish - start);
  151     }

  152     pctimer_exit();

  153     printf("Test finished.\n");
  154     printf("Press any key to display the result...\n");
  155     getch();

  156     for (i = 0; i < 10; i++) {
  157         sum += (float)((elapsedtime[i] - 1024) * RT_MS_PER_TICK);
  158         printf("iteration:%2d  expected:1024  observed:%d  difference:%d\n",
                      i + 1, elapsedtime[i], elapsedtime[i] - 1024);
  159     }

  160     average_error = (float)sum / 10;
  161     printf("\ntotal error:%.2f ms  average error:%.2f ms\n",
                  sum, average_error);
  162     return 0;
  163 }
  ```
* 为了说明方便，源程序中加了行号，所有的注释语句没有行号。
* 先大概说一下这个程序完成的事情
  > PC机系统的时钟中断是18.2次/秒，本程序重新编写了系统时钟中断int 8h，试图做到1ms中断一次，链接到原中断上，这样int 8h(IRQ 0)的中断频率将从18.2Hz变成1000Hz，这势必影响到PC机的时钟，因为很多资源是依赖这个时钟中断的，比如时间；同时我们接管了实时时钟中断int 70h(IRQ 8)，并启动了它的周期中断，中断频率应该是1024Hz，在这个中断中我们仅仅作了一个中断计数；主程序中在启动中断后做了10次的循环，每次循环前记录int 70h的中断次数计数值，然后等待int 8h中断1000次(应该刚好1秒钟)，然后再记录int 70h的中断次数计数值，理论上说，int 8h中断1000次，int 70h应该刚好中断1024次，如果数值吻合，说明int 8h完全按照我们的预期在运行。

* 常量、变量说明：
  - ticks_8h：int 8h的中断次数计数
  - ticks_70h：int 70h的中断次数计数
  - PIT0：8254 timer0的端口地址，我们要修改timer0的设置
  - PITMODE：8254的控制寄存器
  - IMR1：第二颗8259中断控制器的中断屏蔽寄存器地址，因为IRQ 8连在第2颗8259上
  - RTC_ADDR：实时时钟芯片（MC146818）地址寄存器地址
  - RTC_DATA：实时时钟芯片（MC146818）数据寄存器地址

* 主程序从136行开始，145行调用pctimer_init初始化两个硬件中断，传递的参数1000表明把int 8h的中断频率设成1000Hz；146行--151行是测试部分的主循环，前面有过介绍，pctimer_sleep(1000)可以等待int 8h中断1000次，最后把int 70h在此期间的中断次数记录在数组elapsedtime中，共后面显示结果用；循环完毕后，调用pctimer_exit恢复以前的中断，然后显示测试记录。
* 比较复杂的部分是函数pctimer_init，从32行--67行。
  - 35行关中断
  - 36、37行锁定了两个中断程序的代码
  - 38--40行锁定了中断程序中要用到的数据
  - 41--44行设置了int 8h（IRQ 0）
  - 45--49行设置了int 70h（IRQ 8），注意这两个中断的设置方法是有所不同的，我们把int 8h与原来的中断链接到了一起，而int 70h没有链接原来的中断，读者可以通过这段代码体会其中的不同
  - 50、51行设置了实时时钟芯片的状态寄存器B，目的是开启周期中断，读者可以参考与实时时钟芯片相关的资料
  - 52行重新设置了第二颗8259中断控制器，目的是清除对IRQ 8的屏蔽，这里要注意的是，在我做测试的机器上，我读取了第一颗8259的中断屏蔽寄存器，知道其用于级联第二颗8259的IRQ 2并没有被屏蔽，所以我并没有设置第一颗8259的中断屏蔽寄存器，如果你的机器与我的不同，请注意设置第一颗8259的中断屏蔽寄存器，打开IRQ 2，另外，我知道我的机器第二颗8259的中断屏蔽寄存器的缺省设置值是0dh，刚好IRQ 8被屏蔽了，所以我直接送了一个0ch到第二颗8259的中断屏蔽寄存器，以打开IRQ 8，在你的机器上可能也有些不同，请按实际情况设置，只要打开IRQ 8即可，不要改变其他的屏蔽位，以免麻烦，由于此处并非本文的重点，所以在代码上没有作更多处理
  - 54--58行初始化8254的timer0，请参考前面关于8254芯片的介绍部分，另外在14和15行之间的注释也有一个对8254的简单介绍
  - 60--64行设置了一些变量，明白变量的含义就好了，源程序中有注释
  - 65行打开中断，此时两个新的中断开始运行了

* 93--95行是int 8h的中断程序，没有什么好说的，只是不断递增ticks_8h这个变量，注意int 8h这个中断程序是要链接到原有中断上的，所以最后不需要向8259发出中断结束的指令
* 99--107行是int 70h的中断程序，需要做一些说明，首先102、103行读取了实时时钟芯片的状态寄存器C，这是必须要做的，否则芯片认为你没有响应前一个中断，不会发出下一个中断；另外，这个中断和int 8h不同，没有链接到原有中断，所以你必须向8259发出中断完成的指令，又因为IRQ 8连接在第二颗8259上，而第二颗8259通过第一颗8259级联，所以必须向两颗8259均发出中断完成的指令，这就是104、105行的作用
* 117--135行这段程序对本文不重要，这是因为我们改变了系统时钟中断（int 8h）的中断频率，这势必导致本机时间混乱，所以在退出时，读取以下实时时钟芯片中的实时时间，然后重新设置一下时间
* 68--92行这段程序基本上是pctimer_init这个函数的逆动作，也不需要过多说明
* 至此，程序解释完了，希望能给你一些帮助。

-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]


[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[img01]:https://whowin.gitee.io/images/160011/dpmi-controller_registers.jpg


