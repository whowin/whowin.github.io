# Intel CPU的CPUID指令（二）  
**2009-02-06 23:12:55**

* 我们接着上篇继续。

## 6、EAX=2：高速缓存描述符（Cache Descriptor）
  ```
    mov eax, 2
    cpuid

    执行完CPUID指令后，高速缓存描述符和TLB(Translation Lookable Buffer)特性将在EAX、EBX、ECX和EDX中返回，每个寄存器中的4个字节分别表示4个描述符，描述符中不同的值表示不同的含义（后面有定义），其中EAX中的最低8位（AL）的值表示要得到完整的高速缓存的信息，需要执行EAX=2的CPUID指令的次数（一般都为1，在我这里的数台机器里，还没有为2的），同时，寄存器的最高位（bit 31）为0，表示该寄存器中的描述符是有效的，下面是描述符值的定义（资料来源与Intel）：
  ```
  ```
    Value   Cache or TLB Descriptor Description
    ----------------------------------------------------------------------------------------
     00h    Null
     01h    Instruction TLB: 4-KB Pages, 4-way set associative, 32 entries
     02h    Instruction TLB: 4-MB Pages, fully associative, 2 entries
     03h    Data TLB: 4-KB Pages, 4-way set associative, 64 entries
     04h    Data TLB: 4-MB Pages, 4-way set associative, 8 entries
     05h    Data TLB: 4-MB Pages, 4-way set associative, 32 entries
     06h    1st-level instruction cache: 8-KB, 4-way set associative, 32-byte line size
     08h    1st-level instruction cache: 16-KB, 4-way set associative, 32-byte line size
     09h    1st-level Instruction Cache: 32-KB, 4-way set associative, 64-byte line size
     0Ah    1st-level data cache: 8-KB, 2-way set associative, 32-byte line size
     0Ch    1st-level data cache: 16-KB, 4-way set associative, 32-byte line size
     0Dh    1st-level Data Cache: 16-KB, 4-way set associative, 64-byte line size, ECC
     21h    256-KB L2 (MLC), 8-way set associative, 64-byte line size
     22h    3rd-level cache: 512-KB, 4-way set associative, sectored cache, 64-byte line size
     23h    3rd-level cache: 1-MB, 8-way set associative, sectored cache, 64-byte line size
     25h    3rd-level cache: 2-MB, 8-way set associative, sectored cache, 64-byte line size
     29h    3rd-level cache: 4-MB, 8-way set associative, sectored cache, 64-byte line size
     2Ch    1st-level data cache: 32-KB, 8-way set associative, 64-byte line size
     30h    1st-level instruction cache: 32-KB, 8-way set associative, 64-byte line size
     39h    2nd-level cache: 128-KB, 4-way set associative, sectored cache, 64-byte line size
     3Ah    2nd-level cache: 192-KB, 6-way set associative, sectored cache, 64-byte line size
     3Bh    2nd-level cache: 128-KB, 2-way set associative, sectored cache, 64-byte line size
     3Ch    2nd-level cache: 256-KB, 4-way set associative, sectored cache, 64-byte line size
     3Dh    2nd-level cache: 384-KB, 6-way set associative, sectored cache, 64-byte line size
     3Eh    2nd-level cache: 512-KB, 4-way set associative, sectored cache, 64-byte line size
     40h    No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache
     41h    2nd-level cache: 128-KB, 4-way set associative, 32-byte line size
     42h    2nd-level cache: 256-KB, 4-way set associative, 32-byte line size
     43h    2nd-level cache: 512-KB, 4-way set associative, 32-byte line size
     44h    2nd-level cache: 1-MB, 4-way set associative, 32-byte line size
     45h    2nd-level cache: 2-MB, 4-way set associative, 32-byte line size
     46h    3rd-level cache: 4-MB, 4-way set associative, 64-byte line size
     47h    3rd-level cache: 8-MB, 8-way set associative, 64-byte line size
     48h    2nd-level cache: 3-MB, 12-way set associative, 64-byte line size, unified on-die
     49h    3rd-level cache: 4-MB, 16-way set associative, 64-byte line size(Intel Xeon
            processor MP, Family 0Fh, Model 06h) 2nd-level cache: 4-MB, 16-way set associative,
            64-byte line size
     4Ah    3rd-level cache: 6-MB, 12-way set associative, 64-byte line size
     4Bh    3rd-level cache: 8-MB, 16-way set associative, 64-byte line size
     4Ch    3rd-level cache: 12-MB, 12-way set associative, 64-byte line size
     4Dh    3rd-level cache: 16-MB, 16-way set associative, 64-byte line size
     4Eh    2nd-level cache: 6-MB, 24-way set associative, 64-byte line size
     50h    Instruction TLB: 4-KB, 2-MB or 4-MB pages, fully associative, 64 entries
     51h    Instruction TLB: 4-KB, 2-MB or 4-MB pages, fully associative, 128 entries
     52h    Instruction TLB: 4-KB, 2-MB or 4-MB pages, fully associative, 256 entries
     55h    Instruction TLB: 2-MB or 4-MB pages, fully associative, 7 entries
     56h    L1 Data TLB: 4-MB pages, 4-way set associative, 16 entries
     57h    L1 Data TLB: 4-KB pages, 4-way set associative, 16 entries
     5Ah    Data TLB0: 2-MB or 4-MB pages, 4-way associative, 32 entries
     5Bh    Data TLB: 4-KB or 4-MB pages, fully associative, 64 entries
     5Ch    Data TLB: 4-KB or 4-MB pages, fully associative, 128 entries
     5Dh    Data TLB: 4-KB or 4-MB pages, fully associative, 256 entries
     60h    1st-level data cache: 16-KB, 8-way set associative, sectored cache, 64-byte line size
     66h    1st-level data cache: 8-KB, 4-way set associative, sectored cache, 64-byte line size
     67h    1st-level data cache: 16-KB, 4-way set associative, sectored cache, 64-byte line size
     68h    1st-level data cache: 32-KB, 4 way set associative, sectored cache, 64-byte line size
     70h    Trace cache: 12K-uops, 8-way set associative
     71h    Trace cache: 16K-uops, 8-way set associative
     72h    Trace cache: 32K-uops, 8-way set associative
     73h    Trace cache: 64K-uops, 8-way set associative
     78h    2nd-level cache: 1-MB, 4-way set associative, 64-byte line size
     79h    2nd-level cache: 128-KB, 8-way set associative, sectored cache, 64-byte line size
     7Ah    2nd-level cache: 256-KB, 8-way set associative, sectored cache, 64-byte line size
     7Bh    2nd-level cache: 512-KB, 8-way set associative, sectored cache, 64-byte line size
     7Ch    2nd-level cache: 1-MB, 8-way set associative, sectored cache, 64-byte line size
     7Dh    2nd-level cache: 2-MB, 8-way set associative, 64-byte line size
     7Fh    2nd-level cache: 512-KB, 2-way set associative, 64-byte line size
     82h    2nd-level cache: 256-KB, 8-way set associative, 32-byte line size
     83h    2nd-level cache: 512-KB, 8-way set associative, 32-byte line size
     84h    2nd-level cache: 1-MB, 8-way set associative, 32-byte line size
     85h    2nd-level cache: 2-MB, 8-way set associative, 32-byte line size
     86h    2nd-level cache: 512-KB, 4-way set associative, 64-byte line size
     87h    2nd-level cache: 1-MB, 8-way set associative, 64-byte line size
     B0h    Instruction TLB: 4-KB Pages, 4-way set associative, 128 entries
     B1h    Instruction TLB: 2-MB pages, 4-way, 8 entries or 4M pages, 4-way, 4 entries
     B2h    Instruction TLB: 4-KB pages, 4-way set associative, 64 entries
     B3h    Data TLB: 4-KB Pages, 4-way set associative, 128 entries
     B4h    Data TLB: 4-KB Pages, 4-way set associative, 256 entries
     CAh    Shared 2nd-level TLB: 4 KB pages, 4-way set associative, 512 entries
     D0h    512KB L3 Cache, 4-way set associative, 64-byte line size
     D1h    1-MB L3 Cache, 4-way set associative, 64-byte line size
     D2h    2-MB L3 Cache, 4-way set associative, 64-byte line size
     D6h    1-MB L3 Cache, 8-way set associative, 64-byte line size
     D7h    2-MB L3 Cache, 8-way set associative, 64-byte line size
     D8h    4-MB L3 Cache, 8-way set associative, 64-byte line size
     DCh    2-MB L3 Cache, 12-way set associative, 64-byte line size
     DDh    4-MB L3 Cache, 12-way set associative, 64-byte line size
     DEh    8-MB L3 Cache, 12-way set associative, 64-byte line size
     E2h    2-MB L3 Cache, 16-way set associative, 64-byte line size
     E3h    4-MB L3 Cache, 16-way set associative, 64-byte line size
     E4h    8-MB L3 Cache, 16-way set associative, 64-byte line size
     F0h    64-byte Prefetching
     F1h    128-byte Prefetching
  ```
* 举例来说，在我的机器上执行经过如下：

  <img src="images\Intel CPU的CPUID指令（二）-01.jpg"><br />

* EAX、EBX、ECX和EDX的bit 31均为0，说明其中的描述符均有效，EAX中的低8位（AL）为1，说明执行一次即可，下面是描述符含义：
  ```
    05h：Data TLB: 4-MB Pages, 4-way set associative, 32 entries
    B0h：Instruction TLB: 4-KB Pages, 4-way set associative, 128 entries
    B1h：Instruction TLB: 2-MB pages, 4-way, 8 entries or 4M pages, 4-way, 4 entries
   
    56h：L1 Data TLB: 4-MB pages, 4-way set associative, 16 entries
    57h：L1 Data TLB: 4-KB pages, 4-way set associative, 16 entries
    F0h：64-byte Prefetching

    2Ch：1st-level data cache: 32-KB, 8-way set associative, 64-byte line size
    B4h：Data TLB: 4-KB Pages, 4-way set associative, 256 entries
    30h：1st-level instruction cache: 32-KB, 8-way set associative, 64-byte line size
    7Dh：2nd-level cache: 2-MB, 8-way set associative, 64-byte line size
  ```

## 7、EAX=3：处理器序列号
  ```
    mov eax,3
    cpuid

    只有Pentium III提供该功能，486以后的CPU就不再提供该功能，据说是出于隐私的原因。查看你的处理器是否支持处理器序列号功能，可以执行EAX=1的CPUID指令，然后查看EDX的PSN功能(bit 18)，如果为1，说明你的处理器可以返回序列号，否则不支持序列号功能或者是序列号功能被关闭了。

    处理器序列号一共96位，最高32位就是处理器签名，通过执行EAX=1的CPUID指令获得，其余的64位在执行EAX=3的CPUID指令后，中间32位在EDX中，最低32位在ECX中。

    顺便提一句，AMD所有的CPU都没有提供过处理器序列号的功能。
  ```
 
## 后记
  ```
    CPUID指令的基本信息，其实后面还有好几个，不过我在我这里的机器（大概有7、8台吧），好像都不支持，大多数只支持EAX=0、1、2三个，所以后面的就不介绍了。
  ```

 