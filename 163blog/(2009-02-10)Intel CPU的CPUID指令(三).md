# Intel CPU的CPUID指令(三)
**2009-02-10 11:23:14**

* 本文主要介绍CPUID指令返回扩展信息的部分
## 8、EAX=80000001h：最大扩展功能号
  ```
    mov eax, 80000001h
    cpuid

    该功能除了能够向（一）中介绍的那样返回CPU支持的最大扩展功能号外，并没有其它作用，EBX、ECX、EDX都不返回有意义的信息。
  ```
## 9、EAX=80000002h：返回CPU支持的扩展功能
  ```
    mov eax, 80000002h
    cpuid

    执行CPUID指令后，扩展功能标志在EDX和ECX中返回，EDX中的定义如下：

     Bit    Name      Description
    -------------------------------------------------------------------
    10:00             Reserved
      11    SYSCALL   SYSCALL/SYSRET
    19:12             Reserved
      20    XD        Bit Execution Disable Bit
    28:21             Reserved
      29    Intel? 64 Intel? 64 Instruction Set Architecture
    31:30             Reserved

    返回在ECX中的位定义：
     Bit    Name      Description
    -------------------------------------------------------------------
      0     LAHF      LAHF / SAHF
    31:01             Reserved
  ```

## 10、EAX=80000002h、80000003h、80000004h：返回处理器名称/商标字符串
  ```
    mov eax, 80000002h
    cpuid
    ......
    mov eax, 80000003h
    cpuid
    ......
    mov eax, 80000004h
    cpuid

    每次调用CPUID分别在EAX、EBX、ECX、EDX中返回16个ASCII字符，处理器名称/商标字串最多48个字符，前导字符为空格，结束字符为NULL，在寄存器中的排列顺序为little-endian（即低字符在前），下面程序可以在DOS下显示处理器名称/商标字串(使用MASM 6编译)。
  ```
  ```
                    .model tiny
                    .386
    cseg            segment para public 'code'
                    org     100h
                    assume  cs:cseg, ds:cseg, es:cseg
    cpuid           macro
                    db      0fh
                    db      0a2h
    endm
    begin:
                    mov     eax, 80000000h
                    cpuid
                    cmp     eax, 80000004h
                    jb      not_supported
                    mov     di, offset CPU_name
                    mov     eax, 80000002h
                    cpuid
                    call    save_string
                    mov     eax, 80000003h
                    cpuid
                    call    save_string
                    mov     eax, 80000004h
                    cpuid
                    call    save_string
                    mov     dx, offset crlf
                    mov     ah, 9
                    int     21h
                    cld
                    mov     si, offset CPU_name
    spaces:
                    lodsb
                    cmp     al, ' '
                    jz      spaces
                    cmp     al, 0
                    jz      done
    disp_char:
                    mov     dl, al
                    mov     ah, 2
                    int     21h
                    lodsb
                    cmp     al, 0
                    jnz     disp_char
    done:
                    mov     ax, 4c00h
                    int     21h
    not_supported:
                    jmp     done
    save_string:
                    mov     dword ptr [di], eax
                    mov     dword ptr [di + 4], ebx
                    mov     dword ptr [di + 8], ecx
                    mov     dword ptr [di + 12], edx
                    add     di, 16
                    ret
    crlf            db      0dh, 0ah, '$'
    CPU_name        db      50 dup(0)
    cseg            ends
                    end     begin
  ```
## 11、EAX=80000005h：备用

## 12、EAX=80000006h：扩展L2高速缓存功能
  ```
    mov eax, 80000006h
    cpuid

    执行完CPUID指令后，相应信息在ECX中返回，以下是ECX中返回信息的定义：

     Bits    Description
    -----------------------------------------------------------
    31:16    L2 Cache size described in 1024-byte units.
    15:12    L2 Cache Associativity Encodings
               00h Disabled
               01h Direct mapped
               02h 2-Way
               04h 4-Way
               06h 8-Way
               08h 16-Way
               0Fh Fully associative
    11:8     Reserved
     7:0     L2 Cache Line Size in bytes. 
  ```
## 13、EAX=80000007h：电源管理
  ```
    mov eax, 80000007h
    cpuid

    执行CPUID指令后，是否支持电源管理功能在EDX的bit8中返回，其余位无意义。
  ```

## 14、EAX=80000008h：虚拟地址和物理地址大小
  ```
    mov eax, 80000008h
    cpuid

    执行CPUID指令后，物理地址的大小在EAX的bit[7:0]返回，虚拟地址的大小在EAX的bit[15:8]返回，返回的内容为虚拟（物理）地址的位数。例如在我的机器上的运行结果如下：
  ```

  <img src="images\Intel CPU的CPUID指令(三)-01.jpg"><br />
  ```
    表明我的机器支持36位的物理地址和48位的虚拟地址。 
  ```

 