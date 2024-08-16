---
title: "C语言函数调用时的参数传递机制"
date: 2022-09-20T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "C Language"
tags:
  - C语言
  - 参数传递
  - 函数调用
draft: false
#references: 
# - [c语言中函数调用的原理](https://blog.csdn.net/u011555996/article/details/70211541)
# - [x86-64寄存器参数详解](https://blog.csdn.net/CrystalShaw/article/details/124172238)
postid: 130001
---

本文通过实例验证了 C 语言函数调用时参数传递机制在 32 位和 64 位时的不同；阅读本文不仅需要 C 语言的知识，还需要有一些汇编语言的知识。
<!--more-->

> X86的指令集从 16 位的 8086 指令集开始，经历了 40 多年的发展，现在广泛使用的已经是 64 位的 X86-64 指令集，寄存器也从以前的 16 位变成了现在的 64 位，寄存器的数量也大大增加，gcc 当然也必须随着指令集的变化不断升级，在 64 位的时代，C 语言在函数调用时的参数传递也发生了很大的改变，本文通过把 C 语言程序编译成汇编语言的方式来验证这种改变。阅读本文不仅需要 C 语言的知识，还需要有一些汇编语言的知识。本文所有例子在 Ubuntu 20.04 下验证通过，使用的 gcc 版本为 9.4.0。

## 1. 32位下C语言调用函数时的参数传递
* 32 位时代，C语言在调用函数时，实际是使用堆栈来传递参数的；
* 在执行 call 指令前，会首先把需要传递的参数按反顺序依次压入堆栈，比如有三个参数：func(1, 2, 3)，则先把 3 压入堆栈，再把 2 压入堆栈，然后再把 1 压入堆栈；
* 执行 call 指令时，会首先把函数返回地址(也就是 call 指令的下一条指令地址压入堆栈)，然后将 eip 寄存器设置为函数的起始地址，就完成了函数调用；
* 函数执行完毕执行 ret 指令返回时，会将 call 指令压入堆栈的返回地址放入 eip 寄存器，返回过程就结束了；
* 这个过程和 8086 指令集时基本一样；
* 我们用一段简单 C 语言程序来验证以上的说法，该程序文件名定为：param1.c
  ```
  #include <stdio.h>
  #include <stdlib.h>
  
  int func1(int i, int j, char *p) {
      int i1, j1;
      char *str;
  
      i1 = i;
      j1 = j;
      str = p;
  
      return 0;
  }
  
  int main(int argc, char **argv) {
      char *str = "Hello world.";
  
      func1(3, 5, str);
  
      return 0;
  }
  ```

  <!--```
  001 #include <stdio.h>
  002 #include <stdlib.h>
  003 
  004 int func1(int i, int j, char *p) {
  005     int i1, j1;
  006     char *str;
  007 
  008     i1 = i;
  009     j1 = j;
  010     str = p;
  011 
  012     return 0;
  013 }
  014 
  015 int main(int argc, char **argv) {
  016     char *str = "Hello world.";
  017 
  018     func1(3, 5, str);
  019 
  020     return 0;
  021 }
  ```-->
* 我们把这段程序编译成 32 位汇编语言，编译时带了一些选项，其目的是去掉一些调试信息，使汇编代码看上去更加清爽
  ```
  gcc -S -m32 -no-pie -fno-pic -fno-asynchronous-unwind-tables param1.c -o param1.32s
  ```
* 如果你的 64 位机无法编译 32 位程序，可能你需要安装 32 位支持，参考下面的安装
  ```
  sudo apt install g++-multilib libc6-dev-i386
  ```
* 我们看看编译出来的 32 位的汇编语言是什么样子的
  ```
      .file   "param1.c"
      .text
      .globl  func1
      .type   func1, @function
  func1:
      endbr32
      pushl   %ebp
      movl    %esp, %ebp
      subl    $16, %esp
      movl    8(%ebp), %eax         # 第 1 个参数
      movl    %eax, -12(%ebp)
      movl    12(%ebp), %eax        # 第 2 个参数
      movl    %eax, -8(%ebp)
      movl    16(%ebp), %eax        # 第 3 个参数
      movl    %eax, -4(%ebp)
      movl    $0, %eax              # 返回值
      leave
      ret
      .size   func1, .-func1
      .section  .rodata
  .LC0:
      .string   "Hello world."
      .text
      .globl    main
      .type     main, @function
  main:
      endbr32
      pushl   %ebp
      movl    %esp, %ebp
      subl    $16, %esp
      movl    $.LC0, -4(%ebp)
      pushl   -4(%ebp)              # 第 3 个参数 str
      pushl   $5                    # 第 2 个参数 5
      pushl   $3                    # 第 1 个参数 3
      call    func1                 # 调用函数 func1
      addl    $12, %esp
      movl    $0, %eax
      leave
      ret
      .size     main, .-main
      .ident    "GCC: (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0"
      .section  .note.GNU-stack,"",@progbits
      .section  .note.gnu.property,"a"
      .align    4
      .long     1f - 0f
      .long     4f - 1f
      .long     5
  0:
      .string   "GNU"
  1:
      .align    4
      .long     0xc0000002
      .long     3f - 2f
  2:
      .long     0x3
  3:
      .align    4
  4:
  ```
  <!--```
  001     .file   "param1.c"
  002     .text
  003     .globl  func1
  004     .type   func1, @function
  005 func1:
  006     endbr32
  007     pushl   %ebp
  008     movl    %esp, %ebp
  009     subl    $16, %esp
  010     movl    8(%ebp), %eax         # 第 1 个参数
  011     movl    %eax, -12(%ebp)
  012     movl    12(%ebp), %eax        # 第 2 个参数
  013     movl    %eax, -8(%ebp)
  014     movl    16(%ebp), %eax        # 第 3 个参数
  015     movl    %eax, -4(%ebp)
  016     movl    $0, %eax              # 返回值
  017     leave
  018     ret
  019     .size   func1, .-func1
  020     .section  .rodata
  021 .LC0:
  022     .string   "Hello world."
  023     .text
  024     .globl    main
  025     .type     main, @function
  026 main:
  027     endbr32
  028     pushl   %ebp
  029     movl    %esp, %ebp
  030     subl    $16, %esp
  031     movl    $.LC0, -4(%ebp)
  032     pushl   -4(%ebp)              # 第 3 个参数 str
  033     pushl   $5                    # 第 2 个参数 5
  034     pushl   $3                    # 第 1 个参数 3
  035     call    func1                 # 调用函数 func1
  036     addl    $12, %esp
  037     movl    $0, %eax
  038     leave
  039     ret
  040     .size     main, .-main
  041     .ident    "GCC: (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0"
  042     .section  .note.GNU-stack,"",@progbits
  043     .section  .note.gnu.property,"a"
  044     .align 4
  045     .long     1f - 0f
  046     .long     4f - 1f
  047     .long     5
  048 0:
  049     .string   "GNU"
  050 1:
  051     .align    4
  052     .long     0xc0000002
  053     .long     3f - 2f
  054 2:
  055     .long     0x3
  056 3:
  057     .align    4
  058 4:
  ```-->

* 第 26 行开始是 main 函数的汇编代码，第 31 - 35 行在调用函数 func1(3, 5, str)，其中 32 行把第 3 个参数 str 压入堆栈，33行将第 2 个参数 5 压入堆栈；34 行将第 1 个参数 3 压入堆栈；
* 这段汇编程序有两点要注意，一是参数是按照调用顺序的反方向压入堆栈的，即先把第 3 个参数压入堆栈，再把第 2 个参数压入堆栈，最后把第 1 个参数压入堆栈；二是对于整数变量，是直接把整数值压入堆栈，而不是这个整数值存储的地址；
* 第 5 - 18 行是 func1 函数的汇编代码，第 9 行将堆栈的栈顶扩展了 16 个字节，这块地方用于存储 func1 中定义的变量，也就是 C 代码中的 i1、j1 和 str 三个变量，(ebp - 12)存储变量 i1，(ebp - 8)存储变量 j1，(ebp - 4)存储变量 str；
* 第 10 行从堆栈中取出第 1 个参数放入 eax 寄存器；第 12 行从堆栈中取出第 2 个参数；第 14 行从堆栈中取出第 3 个参数；
* 下面这张图，试图描述调用函数 func1 前后堆栈的变化
  ![调用函数 func1 前后堆栈的变化][img01]
  - **调用函数 func1 前后堆栈的变化**
----------------

## 2. 64位下C语言调用函数时的参数传递
* 64 位时代，CPU 通用寄存器的数量已经从 32 位时的 6 个(不含 eip, esp, ebp)增加到了 14 个(不含rip, rsp, rbp)；
* 为了提高调用函数的性能，gcc 在调用函数时会尽量使用寄存器来传递参数，而不是像 32 位指令集那样全部使用堆栈来传递参数；
* 当需要传递的参数少于 6 个时，使用 rdi, rsi, rdx, rcx, r8, r9 这六个寄存器来传递参数；
* 当传递的参数多于 6 个时，前 6 个参数使用寄存器传递，6 个以上的参数仍然像 32 位时那样使用堆栈传递参数；
* 这一规则提示我们，在编写 C 语言程序时，调用函数时的**参数应该尽量少于 6 个**；
* 下面我们仍然用一个简单程序来验证上面的说法，该程序文件名定为：param2.c
  ```
  #include <stdio.h>
  #include <stdlib.h>
  
  int func2(char *p, int var1, int var2, int var3, int var4, int var5, int var6, int var7) {
      char *str;
      int int_arr[7];
  
      int_arr[0] = var1;
      int_arr[1] = var2;
      int_arr[2] = var3;
      int_arr[3] = var4;
      int_arr[4] = var5;
      int_arr[5] = var6;
      int_arr[6] = var7;
  
      return 0;
  }
  
  int main(int argc, char **argv) {
      char *str = "Hello world.";
  
      func2(str, 2, 3, 4, 5, 6, 7, 8);
  
      return 0;
  }
  ```
  <!--```
  001 #include <stdio.h>
  002 #include <stdlib.h>
  003 
  004 int func2(char *p, int var1, int var2, int var3, int var4, int var5, int var6, int var7) {
  005     char *str;
  006     int int_arr[7];
  007 
  008     int_arr[0] = var1;
  009     int_arr[1] = var2;
  010     int_arr[2] = var3;
  011     int_arr[3] = var4;
  012     int_arr[4] = var5;
  013     int_arr[5] = var6;
  014     int_arr[6] = var7;
  015 
  016     return 0;
  017 }
  018 
  019 int main(int argc, char **argv) {
  020     char *str = "Hello world.";
  021 
  022     func2(str, 2, 3, 4, 5, 6, 7, 8);
  023 
  024     return 0;
  025 }
  ```-->
* 我们把这段程序编译成 64 位汇编语言，编译时所带的选项，与编译 32 位汇编时相比增加了一个 -fno-stack-protector，禁用了堆栈保护，同样是为了让汇编代码看上去更清爽；
  ```
  gcc -S -no-pie -fno-pic -fno-asynchronous-unwind-tables -fno-stack-protector param2.c -o param2.64s
  ```
* 我们看看编译出来的 64 位的汇编语言是什么样子的
  ```
      .file   "param2.c"
      .text
      .globl  func2
      .type   func2, @function
  func2:
      endbr64
      pushq   %rbp
      movq    %rsp, %rbp
      movq    %rdi, -40(%rbp)     # (rbp - 40)存放变量str，rdi 为第 1 个参数
      movl    %esi, -44(%rbp)     # (rbp - 44)临时存放 rsi 中的第 2 个参数
      movl    %edx, -48(%rbp)     # (rbp - 48)临时存放 rdx 中的第 3 个参数
      movl    %ecx, -52(%rbp)     # (rbp - 52)临时存放 rcx 中的第 4 个参数
      movl    %r8d, -56(%rbp)     # (rbp - 56)临时存放 r8 中的第 5 个参数
      movl    %r9d, -60(%rbp)     # (rbp - 60)临时存放 r9 中的第 6 个参数
      movl    -44(%rbp), %eax     # 取出第 2 个参数
      movl    %eax, -32(%rbp)     # (rbp - 32)存放变量int_arr[0]
      movl    -48(%rbp), %eax     # 取出第 3 个参数
      movl    %eax, -28(%rbp)     # (rbp - 28)存放变量int_arr[1]
      movl    -52(%rbp), %eax     # 取出第 4 个参数
      movl    %eax, -24(%rbp)     # (rbp - 24)存放变量int_arr[2]
      movl    -56(%rbp), %eax     # 取出第 5 个参数
      movl    %eax, -20(%rbp)     # (rbp - 20)存放变量int_arr[3]
      movl    -60(%rbp), %eax     # 取出第 6 个参数
      movl    %eax, -16(%rbp)     # (rbp - 16)存放变量int_arr[4]
      movl    16(%rbp), %eax      # 第 7 个参数是从堆栈中传过来的
      movl    %eax, -12(%rbp)     # (rbp - 12)存放变量int_arr[5]
      movl    24(%rbp), %eax      # 第 8 个参数是从堆栈中传过来的
      movl    %eax, -8(%rbp)      # (rbp - 8)存放变量int_arr[6]
      movl    $0, %eax
      popq    %rbp
      ret
      .size     func2, .-func2
      .section  .rodata
  .LC0:
      .string   "Hello world."
      .text
      .globl    main
      .type     main, @function
  main:
      endbr64
      pushq   %rbp
      movq    %rsp, %rbp
      subq    $32, %rsp
      movl    %edi, -20(%rbp)
      movq    %rsi, -32(%rbp)
      movq    $.LC0, -8(%rbp)
      movq    -8(%rbp), %rax
      pushq   $8                  # 要传递的第 8 个参数，压入堆栈
      pushq   $7                  # 要传递的第 7 个参数，压入堆栈
      movl    $6, %r9d            # 要传递的第 6 个参数，使用 r9 寄存器
      movl    $5, %r8d            # 要传递的第 5 个参数，使用 r8 寄存器
      movl    $4, %ecx            # 要传递的第 4 个参数，使用 rcx 寄存器
      movl    $3, %edx            # 要传递的第 3 个参数，使用 rdx 寄存器
      movl    $2, %esi            # 要传递的第 2 个参数，使用 rsi 寄存器
      movq    %rax, %rdi          # 要传递的第 1 个参数，使用 rdi 寄存器
      call    func2
      addq    $16, %rsp
      movl    $0, %eax
      leave
      ret
      .size     main, .-main
      .ident    "GCC: (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0"
      .section  .note.GNU-stack,"",@progbits
      .section  .note.gnu.property,"a"
      .align    8
      .long     1f - 0f
      .long     4f - 1f
      .long     5
  0:
      .string   "GNU"
  1:
      .align    8
      .long     0xc0000002
      .long     3f - 2f
  2:
      .long     0x3
  3:
      .align    8
  4:
  ```
  <!--```
  001     .file   "param2.c"
  002     .text
  003     .globl  func2
  004     .type   func2, @function
  005 func2:
  006     endbr64
  007     pushq   %rbp
  008     movq    %rsp, %rbp
  009     movq    %rdi, -40(%rbp)     # (rbp - 40)存放变量str，rdi 为第 1 个参数
  010     movl    %esi, -44(%rbp)     # (rbp - 44)临时存放 rsi 中的第 2 个参数
  011     movl    %edx, -48(%rbp)     # (rbp - 48)临时存放 rdx 中的第 3 个参数
  012     movl    %ecx, -52(%rbp)     # (rbp - 52)临时存放 rcx 中的第 4 个参数
  013     movl    %r8d, -56(%rbp)     # (rbp - 56)临时存放 r8 中的第 5 个参数
  014     movl    %r9d, -60(%rbp)     # (rbp - 60)临时存放 r9 中的第 6 个参数
  015     movl    -44(%rbp), %eax     # 取出第 2 个参数
  016     movl    %eax, -32(%rbp)     # (rbp - 32)存放变量int_arr[0]
  017     movl    -48(%rbp), %eax     # 取出第 3 个参数
  018     movl    %eax, -28(%rbp)     # (rbp - 28)存放变量int_arr[1]
  019     movl    -52(%rbp), %eax     # 取出第 4 个参数
  020     movl    %eax, -24(%rbp)     # (rbp - 24)存放变量int_arr[2]
  021     movl    -56(%rbp), %eax     # 取出第 5 个参数
  022     movl    %eax, -20(%rbp)     # (rbp - 20)存放变量int_arr[3]
  023     movl    -60(%rbp), %eax     # 取出第 6 个参数
  024     movl    %eax, -16(%rbp)     # (rbp - 16)存放变量int_arr[4]
  025     movl    16(%rbp), %eax      # 第 7 个参数是从堆栈中传过来的
  026     movl    %eax, -12(%rbp)     # (rbp - 12)存放变量int_arr[5]
  027     movl    24(%rbp), %eax      # 第 8 个参数是从堆栈中传过来的
  028     movl    %eax, -8(%rbp)      # (rbp - 8)存放变量int_arr[6]
  029     movl    $0, %eax
  030     popq    %rbp
  031     ret
  032     .size     func2, .-func2
  033     .section  .rodata
  034 .LC0:
  035     .string   "Hello world."
  036     .text
  037     .globl    main
  038     .type     main, @function
  039 main:
  040     endbr64
  041     pushq   %rbp
  042     movq    %rsp, %rbp
  043     subq    $32, %rsp
  044     movl    %edi, -20(%rbp)
  045     movq    %rsi, -32(%rbp)
  046     movq    $.LC0, -8(%rbp)
  047     movq    -8(%rbp), %rax
  048     pushq   $8                  # 要传递的第 8 个参数，压入堆栈
  049     pushq   $7                  # 要传递的第 7 个参数，压入堆栈
  050     movl    $6, %r9d            # 要传递的第 6 个参数，使用 r9 寄存器
  051     movl    $5, %r8d            # 要传递的第 5 个参数，使用 r8 寄存器
  052     movl    $4, %ecx            # 要传递的第 4 个参数，使用 rcx 寄存器
  053     movl    $3, %edx            # 要传递的第 3 个参数，使用 rdx 寄存器
  054     movl    $2, %esi            # 要传递的第 2 个参数，使用 rsi 寄存器
  055     movq    %rax, %rdi          # 要传递的第 1 个参数，使用 rdi 寄存器
  056     call    func2
  057     addq    $16, %rsp
  058     movl    $0, %eax
  059     leave
  060     ret
  061     .size     main, .-main
  062     .ident    "GCC: (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0"
  063     .section  .note.GNU-stack,"",@progbits
  064     .section  .note.gnu.property,"a"
  065     .align    8
  066     .long     1f - 0f
  067     .long     4f - 1f
  068     .long     5
  069 0:
  070     .string   "GNU"
  071 1:
  072     .align    8
  073     .long     0xc0000002
  074     .long     3f - 2f
  075 2:
  076     .long     0x3
  077 3:
  078     .align    8
  079 4:
  ```-->

* 汇编代码中的注释已经可以很清楚的看到，在调用 func2 前准备传递参数时，使用堆栈来传递第 7、8 两个参数(48、49行)，使用 rdi, rsi, rdx, rcx, r8, r9 来传递前 6 个参数(50 - 55行)；
* 和 32 位汇编代码一样，对于整数参数是直接把整数值压入堆栈或者放如寄存器，而不是使用指针进行传递；
* 在函数 func2 的汇编代码中，也可以清楚地看到其取参数的规则，与 main 中传递参数的规则一致(9 - 28行)；
* 再次强调，**在编写 x86-64 的 C 语言程序，把函数调用时的参数限制在 6 个以内，将可以在一定程度上提高程序的性能**


-------------
**欢迎访问我的博客：https://whowin.cn**
**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[img01]:/images/130001/0001-parameters_passing_mechanism.png

<!-- CSDN
[img01]:https://img-blog.csdnimg.cn/img_convert/f11bb43ff6ecc66daa43d5147ac8ddc6.png
-->
