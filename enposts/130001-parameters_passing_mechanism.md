---
title: "Parameter passing mechanism of function call in C language"
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

This article verifies the difference between 32-bit and 64-bit parameter passing mechanism when calling C language functions through examples. reading this article requires not only knowledge of C language, but also some knowledge of assembly language.
<!--more-->

> The X86 instruction set started with the 16-bit 8086 and has experienced more than 40 years of development. The 64-bit X86-64 instruction set is now widely used. The registers have also changed from the previous 16-bit to the current 64-bit. The number of registers is also greatly increased. Of course gcc must also be constantly upgraded as the instruction set changes. In the 64-bit era, the parameter passing of C language in function calls has also undergone great changes. This article verifies this change by compiling C language programs into assembly language. Reading this article requires not only knowledge of C language, but also some knowledge of assembly language. All examples in this article are verified under Ubuntu 20.04, and the gcc version used is 9.4.0.

## 1. Parameter transfer for calling functions in C language under 32-bit.
* In the 32-bit era, C language actually used the stack to pass parameters when calling functions.
* Before executing the call instruction, the parameters to be passed will be pushed onto the stack in reverse order. For example, if there are three parameters: func(1, 2, 3), then push 3 onto the stack first, then push 2 onto the stack, and then push 1 onto the stack.
* When the call instruction is executed, the function return address (that is, the address of the next instruction of the call instruction) will be pushed onto the stack first, and then the eip register will be set to the starting address of the function to complete the function call.
* When the function executes the ret instruction, it will put the return address pushed onto the stack when the call instruction is executed into the eip register, and the return process is over.
* This process is basically the same as the 8086 instruction set.
* We use a simple C language program to verify the above statement, the program file name is: param1.c.
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
* We compiled this program into 32-bit assembly language with some options, the purpose of which is to remove some debugging information and make the assembly code look cleaner.
  ```
  gcc -S -m32 -no-pie -fno-pic -fno-asynchronous-unwind-tables param1.c -o param1.32s
  ```
* If your 64-bit computer cannot compile 32-bit programs, you may need to install 32-bit support. Refer to the installation instructions below.
  ```
  sudo apt install g++-multilib libc6-dev-i386
  ```
* Let's see what the compiled 32-bit assembly language looks like.
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
      movl    8(%ebp), %eax         # 1st parameter
      movl    %eax, -12(%ebp)
      movl    12(%ebp), %eax        # 2nd parameter
      movl    %eax, -8(%ebp)
      movl    16(%ebp), %eax        # 3rd parameter
      movl    %eax, -4(%ebp)
      movl    $0, %eax              # return value
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
      pushl   -4(%ebp)              # 1st parameter - str
      pushl   $5                    # 2nd parameter - 5
      pushl   $3                    # 3rd parameter - 3
      call    func1                 # call function func1()
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

* Beginning on line 26 is the assembly code for the main function. Lines 31-35 call the function func1(3, 5, str), where line 32 pushes the third parameter str onto the stack, and line 33 pushes the second parameter 5 onto the stack. Line 34 pushes the first parameter 3 onto the stack.
* There are two points to note in this assembler. One is that the parameters are pushed into the stack in the reverse order of the calling sequence. That is, push the third parameter onto the stack first, then push the second parameter onto the stack, and finally push the first parameter onto the stack. Second, for integer variables, the integer value is directly pushed onto the stack, not the address where the integer value is stored.
* Lines 5 - 18 are the assembly code for the func1 function. Line 9 extends the top of the stack by 16 bytes. This memory area is used to store the variables defined in func1, that is, the three variables i1, j1 and str in the C code. (ebp - 12) stores variable i1. (ebp - 8) stores variable j1. (ebp - 4) stores the variable str.
* Line 10 takes the 1st parameter from the stack and puts it into the eax register. Line 12 pops the second parameter from the stack. Line 14 takes the third parameter from the stack.
* The following diagram describes the changes in the stack before and after calling the function func1.

  ![The changes in the stack before and after calling the function func1][img01]
  - **The changes in the stack before and after calling the function func1**
----------------

## 2. Parameter passing when calling a function in C language under 64-bit.
* In the 64-bit era, the number of CPU general-purpose registers has increased from 6 (excluding eip, esp, ebp) to 14 (excluding rip, rsp, rbp) in 32-bit.
* In order to improve the performance of calling functions, gcc will try to use registers to pass parameters when calling functions, instead of using the stack to pass parameters like the 32-bit instruction set.
* When less than 6 parameters need to be passed, use the six registers rdi, rsi, rdx, rcx, r8, r9 to pass parameters;
* When more than 6 parameters are passed, the first 6 parameters are passed using registers, and the parameters after the 6th parameter are still passed using the stack as in 32-bit.
* This rule reminds us that when writing C language programs, the parameters when calling functions should be less than 6 as far as possible.
* Below we still use a simple program to verify the above statement. The program file name is: param2.c
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
* We compile this program into 64-bit assembly language. The compile-time option adds a -fno-stack-protector compared to when compiling 32-bit assembly, which disables stack protection, also to make the assembly code look cleaner.
  ```
  gcc -S -no-pie -fno-pic -fno-asynchronous-unwind-tables -fno-stack-protector param2.c -o param2.64s
  ```
* Let's see what the compiled 64-bit assembly language looks like.
  ```
      .file   "param2.c"
      .text
      .globl  func2
      .type   func2, @function
  func2:
      endbr64
      pushq   %rbp
      movq    %rsp, %rbp
      movq    %rdi, -40(%rbp)     # (rbp - 40) stores variable str, rdi is the 1st parameter
      movl    %esi, -44(%rbp)     # (rbp - 44) stores temporarily the 2nd parameter in rsi register
      movl    %edx, -48(%rbp)     # (rbp - 48) stores temporarily the 3rd parameter in rdx register
      movl    %ecx, -52(%rbp)     # (rbp - 52) stores temporarily the 4th parameter in rcx register
      movl    %r8d, -56(%rbp)     # (rbp - 56) stores temporarily the 5th parameter in r8 register
      movl    %r9d, -60(%rbp)     # (rbp - 60) stores temporarily the 6th parameter in r9 register
      movl    -44(%rbp), %eax     # takes the 2nd parameter
      movl    %eax, -32(%rbp)     # (rbp - 32) stores variable int_arr[0]
      movl    -48(%rbp), %eax     # takes the 3rd parameter
      movl    %eax, -28(%rbp)     # (rbp - 28) stores variable int_arr[1]
      movl    -52(%rbp), %eax     # takes the 4th parameter
      movl    %eax, -24(%rbp)     # (rbp - 24) stores variable int_arr[2]
      movl    -56(%rbp), %eax     # takes the 5th parameter
      movl    %eax, -20(%rbp)     # (rbp - 20) stores variable int_arr[3]
      movl    -60(%rbp), %eax     # takes the 6th parameter
      movl    %eax, -16(%rbp)     # (rbp - 16) stores variable int_arr[4]
      movl    16(%rbp), %eax      # The 7th parameter is passed from the stack
      movl    %eax, -12(%rbp)     # (rbp - 12) stores variable int_arr[5]
      movl    24(%rbp), %eax      # The 8th parameter is passed from the stack
      movl    %eax, -8(%rbp)      # (rbp - 8) stores variable int_arr[6]
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
      pushq   $8                  # Push the 8th parameter to be passed on the stack
      pushq   $7                  # Push the 7th parameter to be passed on the stack
      movl    $6, %r9d            # r9 register stores the 6th parameter to be passed
      movl    $5, %r8d            # r8 register stores the 5th parameter to be passed
      movl    $4, %ecx            # rcx register stores the 4th parameter to be passed
      movl    $3, %edx            # rdx register stores the 3rd parameter to be passed
      movl    $2, %esi            # rsi register stores the 2nd parameter to be passed
      movq    %rax, %rdi          # rdi register stores the 1st parameter to be passed
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

* The comments in the assembly code can clearly see that when preparing to pass parameters before calling func2, use the stack to pass the 7th and 8th parameters (lines 48 and 49), use rdi, rsi, rdx, rcx, r8, r9 to pass the first 6 arguments (lines 50 - 55).
* As with 32-bit assembly code, for integer parameters, the integer value is pushed directly onto the stack or into a register, rather than using a pointer to pass it.
* In the assembly code of the function func2, you can also clearly see the rules for taking parameters, which are consistent with the rules for passing parameters in main (lines 9-28).
* Again, ** When writing x86-64 C language programs, limiting the parameters of function calls to less than 6 will improve the performance of the program**.


[img01]:/images/130001/0001-parameters_passing_mechanism_en.png
