---
title: "使用glib进行C语言编程的实例(一)"
date: 2024-07-10T23:48:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "C Language"
tags:
  - C语言
  - glib

draft: faise
#references: 
# - [GLib – 2.0 手册](https://docs.gtk.org/glib/index.html)
# - [GLIB用户指南](https://blog.csdn.net/l197803/article/details/138244885)
# - [Wikipedia GLib](https://en.wikipedia.org/wiki/GLib)
# - [Manage C data using the GLib collections](https://developer.ibm.com/tutorials/l-glib/)
# - [Getting Started with GLib](https://remcokranenburg.com/2021/05/30/getting-started-with-glib.html)
# - [C Programming/GObject](https://en.wikibooks.org/wiki/C_Programming/GObject)
# - [ANSI C 做面向对象编程](https://www.cs.rit.edu/~ats/books/ooc.pdf)
# - [GObject Tutorial](https://docs.gtk.org/gobject/tutorial.html)
# - [GObject – 2.0](https://docs.gtk.org/gobject/index.html)

postid: 130002
---

本文将讨论使用GLib进行编程的基本步骤，GLib是一个跨平台的，用C语言编写的3个底层库(以前是5个)的集合，GLib提供了多种高级的数据结构，如内存块、双向和单向链表、哈希表等，GLib还实现了线程相关的函数、多线程编程以及相关的工具，例如原始变量访问、互斥锁、异步队列等，GLib主要由GNOME开发；本文是使用GLib编程的入门文章，旨在通过实例帮助希望学习GLib编程的读者较快地入门，本文将给出多个使用GLib库编程范例的源代码，本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；本文适合初学者阅读。


<!--more-->


## 1 前言
* GLib 与 glibc 不是一个东西，glibc 是 GNU 实现的一套标准 C 的库函数，而 GLib 是 GTK+ 的一套函数库，如果非要扯上点关系，GLib 依赖于 glibc，不过 Linux 下几乎所有的应用程序都是依赖于 glibc 的；
* GLib 最初是 GTK+ 项目(现名为 GTK)的一部分；在发布 GTK+ 版本 2 时，该项目的开发人员决定将非图形用户界面(GUI)的代码从 GTK+ 中分离出来，作为一个单独的库(GLib)发布，以使不需要使用 GUI 的开发人员可以使用这些功能，而无需依赖完整的 GUI 库，这就产生了 GLib 库；
* GLib 是一个跨平台库，使用 GLib 编写的应用程序无需进行重大修改即可移植到不同的操作系统上，所以 GLib 不仅可以用在 Linux 下，也可以在 Windows 下使用；
* GLib 仍然在不断地开发中，截止到 2024 年 7 月，GNOME 已经发布了 GLib 2.9版。
* GLib 包由五个库组成：
    - GObject
    - GLib
    - GModule
    - GThread
    - GIO
* 这 5 个库全部合并在一个库里，称为 GLib；目前在源代码中，还保留着三个目录：GLib、GObject 和 GIO，GModule、GThread 已经放在 GLib 中了，所以现在通常认为 GLib 是 3 个底层库的集合；
* C 语言有一些令程序员头疼的数据类型，比如指针、字符串(以nul为结束符)，GLib 拥有一系列自身的数据类型，较好地解决了这个问题；
* GLib 的设计很多都是面向对象的，所有可以使用面向对象的概念进行 C 语言编程。

## 2 如何将一个程序按 GLib 的方式改写
* 先使用标准 C 语言按照题目要求编写一个简单的程序，这个题目的原型出自 [Advent of Code - 2019][advent_of_code]
* 题目：宇宙飞船飞回地球需要多少燃料？飞船所需的燃料与飞船的质量有直接的关系，计算方式为：飞船质量 ÷ 3，结果向下取整，再减 2，若结果小于 0，则为 0；
  - 如果飞船质量为 12，除以 3 为 4，再减 2 则结果为 2；
  - 如果飞船质量为 14，除以 3 向下取整为 4，再减 2 其结果为 2；
  - 如果飞船质量为 1969，则所需燃料为 654；
* 这个问题的难点在于当我们计算出所需燃料后，实际上飞船的总质量已经改变，变成了飞船质量 + 燃料质量，需要为增加的燃料再补充适当的燃料，所以这实际上是一个递归计算；
* 按照标准 C 语言编写的源程序：[puzzle-2019.c][src01](**点击文件名下载源程序**)
    ```C
    #include <stdio.h>
    #include <stdlib.h>

    #include <math.h>

    // Calculate the fuel required
    int calculate_fuel(int weight) {
        int additional_weight = fmax(weight / 3 - 2, 0);

        printf("Weight: %d\tAdditional weight(Fuel required): %d\n", weight, additional_weight);
        if (additional_weight > 0) {
            additional_weight += calculate_fuel(additional_weight);
        }

        return additional_weight;
    }

    int main(int argc, char *argv[]) {
        if (argc < 2) {
            printf("USAGE: %s <mass>\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        int mass = atoi(argv[1]);
        if (mass <= 0) {
            printf("The mass can not be zero.\n");
            exit(EXIT_FAILURE);
        }

        int fuel_required = calculate_fuel(mass);
        printf("Total fuel reqired: %d\n", fuel_required);

        return EXIT_SUCCESS;
    }
    ```
* 编译：`gcc -Wall -g puzzle-2019.c -o puzzle-2019 -lm`
* 因为该程序使用了 fmax()，所以需要连接数学函数库，也就是 `-lm`；
* 运行：`./puzzle-2019 2024` (后面跟的参数为飞船质量)

    ![Screenshot of puzzle-2019][img01]

* 把这个程序使用基于 GLib 的方式改写，源程序为：[puzzle-2019-glib.c][src02](**点击文件名下载源程序**)
    ```C
    #include <glib.h>

    // Calculate the fuel required
    gint calculate_fuel(gint weight, GString *message) {
        gint additional_weight = MAX(weight / 3 - 2, 0);
        g_autoptr(GString) temp_str = g_string_new(NULL);

        g_string_printf(temp_str, "Weight: %d\tAdditional weight(Fuel required): %d\n", weight, additional_weight);
        g_string_append(message, temp_str->str);

        if (additional_weight > 0) {
            additional_weight += calculate_fuel(additional_weight, message);
        }

        return additional_weight;
    }

    gint main(gint argc, gchar *argv[]) {
        if (argc < 2) {
            g_print("USAGE: %s <mass>\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        gint mass = g_ascii_strtoll(argv[1], NULL, 10);         // Convert an ASCII string to a number
        if (mass <= 0) {
            g_print("The mass can not be zero.\n");
            exit(EXIT_FAILURE);
        }

        GString *message = g_string_new(NULL);                  // Create a string object
        gint fuel_required = calculate_fuel(mass, message);
        g_print("%sTotal fuel required: %d\n", message->str, fuel_required);
        g_string_free(message, TRUE);                           // Release the string object

        return EXIT_SUCCESS;
    }
    ```
* 将这个程序转换成基于 GLib 的程序首先要增加头文件 glib.h
    ```C
    #include <glib.h>
    ```
* 原程序中使用 `fmax()` 函数求两个数的最大值，这里可以使用 `MAX(a, b)` 来代替，这样，原来的头文件 `#include <math.h>` 可以去掉，编译时的 `-lm` 也就不需要了；
* 后面的修改主要是修改数据类型，GLib 定义了一系列的基本数据类型，一些是明确带有类型长度的，比如：gint8、gint16、guint32、gint64，分别为 8 位整数、16 位整数、32 位无符号整数和 64 位整数；
* 还有一些数据类型可以直接替换标准 C 中的类型，比如：gint 替换 int，guint 替换 `unsigned int` 等；
* C 语言的字符串是以一个 nul 结尾的字节数组，这其实是有隐患的，比如如果不小心将字符串结尾处的 nul 覆盖，将可能导致灾难性的后果，而且这个字符串在使用上也不方便，比如不知道字符串的长度，添加字符时可能超过定义的数组长度等，为此，GLib 定义了一个新的字符串类型 GString；
    - 这其实就是一个字符串对象，GString 是一个结构，其定义为：
        ```C
        struct _GString {
            gchar  *str;            // 以 nul 结尾的字符串
            gsize len;              // 字符串的长度
            gsize allocated_len;    // 为字符串 str 实际申请的内存长度
        };
        ```
    - GLib 提供了一系列初始化和操作这个字符串对象的方法
    - `g_string_new()` 用于初始化一个字符串对象，`g_string_append()` 用于在字符串后面添加一个新字符串，`g_string_insert()` 在一个字符串中插入一个新字符串，等等；
    - 在对字符串对象进行操作时，用于存放字符串 str 的内存空间会被重新分配，不会导致内存溢出的问题；
    - 在使用完字符串对象之后，要使用 `g_string_free()` 将字符串对象释放掉；
    - 这个程序中，在主程序中建立了字符串对象 message，在子程序 `calculate_fuel()` 中，使用 `g_string_append()` 向 message 中添加了内容，然后在主程序中显示了 message 的内容，最后使用 `g_string_free()` 释放了字符串对象 message；
    - 在 `calculate_fuel()` 中，建立了一个字符串对象 temp_str，用于生成一个临时字符串，并将其追加到字符串对象 message 的后边，该字符串对象的建立与 message 略有不同，并且也没有使用 `g_string_free()` 去释放，这一点下面介绍；
* GLib 提供了内存自动管理功能
    - GLib 有一个类型 g_autoptr，它可以自动释放所引用对象的内存：系统跟踪对该对象的所有引用，如果删除最后一个引用或者函数退出，则释放内存；
    - 程序中在 `calculate_fuel()` 中，在建立字符串对象 temp_str 时，使用了 `g_autoptr()`，使 temp_str 对象成为一个可以自动释放内存的对象，当函数 `calculate_fuel()` 退出时，temp_str 所占用的内存被自动释放；

* 程序中所有字符串对象的操作都不是必需的，仅用于演示 GLib 的一些特性；
* 程序中还使用 `g_ascii_strtoll()` 取代了 `atoi()` 函数将一个字符串变量转换成数字，`g_ascii_strtoll()` 与 `strtol()` 基本相同；
* 程序中还使用 GLib 的 `g_print()` 替换了 `printf()`，这两个函数基本是相同的功能；
* 编译(下面有关于编译的一些说明)：
    ```
    gcc -Wall -g puzzle-2019-glib.c -o puzzle-2019-glib `pkg-config --cflags --libs glib-2.0`
    ```
* 运行：`./puzzle-2019-glib 2024`

## 3 安装和编译基于 GLib 的程序
* 在 Ubuntu 下检查是否安装了 GLib：
    ```bash
    dpkg -l | grep libglib2.0-dev
    ```
* 如果没有安装，可按照下面方法安装：
    ```bash
    sudo apt update
    sudo apt install libglib2.0-dev
    ```
* 还可以编一个小程序显示一下 GLib 的版本号
    ```C
    #include <glib.h>
    
    int main(int argc, char *argv[]) {
        g_print("Glib version: %d.%d.%d\n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
        return 0;
    }
    ```
* 把这段程序命名为 glib-ver.c，用下面的方式进行编译：
    ```bash
    gcc glib-ver.c -o glib-ver `pkg-config --cflags --libs glib-2.0`
    ```
* GLib 支持 `pkg-config`，亦即可以使用 `pkg-config` 导出其编译环境，可以在命令行下单独运行 `pkg-config --cflags --libs glib-2.0` 看看可以得到什么结果；
    ```bash
    pkg-config --cflags --libs glib-2.0
    ```
* 这个命令又可以分为两个部分，一部分是编译(compile)所需的环境，另一部分是连接(link)所需的环境，可以在命令行分别运行下列两个命令看看会得到什么结果；
    ```bash
    pkg-config --cflags glib-2.0
    pkg-config --libs glib-2.0
    ```
* Glib 对 pkg-config 的支持使得编译基于 Glib 的应用程序变得比较简单，在编译时只要加上下面的命令即可； 
    ```bash
    `pkg-config --cflags --libs glib-2.0`
    ```

## 4 基于 GLib 的 GString 的一些基本用法
* 前面已经对 GString 做了简单的介绍，其基本使用方法非常简单，下面程序演示了如何在 GString 中添加、删除、插入以及截断字符串，与标准 C 库中的字符串不同，GString 会自动管理内存，在对 GString 操作时不必关心内存的重新分配问题；

    ```C
    #include <glib.h>

    int main(int argc, char *argv[]) {
        // 创建一个新的 GString
        GString *gstring = g_string_new("");

        // 追加字符串到 GString
        g_string_append(gstring, "Hello, ");
        g_string_append(gstring, "GString!");

        // 打印 GString 的内容
        g_print("Initial GString content: %s\n", gstring->str);

        // 在 GString 的指定位置插入字符串
        g_string_insert(gstring, 7, "GLib ");

        // 再次打印 GString 的内容
        g_print("GString content after insert: %s\n", gstring->str);

        // 替换 GString 中的子串
        g_string_erase(gstring, 7, 5);          // 删除 "GLib "
        g_string_insert(gstring, 7, "World ");  // 在指定位置插入“World ”

        // 再次打印 GString 的内容
        g_print("GString content after replace: %s\n", gstring->str);

        // 截断 GString 到指定长度
        g_string_truncate(gstring, 12);

        // 打印截断后的 GString 内容
        g_print("GString content after truncate: %s\n", gstring->str);

        // 释放 GString 及其内容
        g_string_free(gstring, TRUE);

        return 0;
    }
    ```
* GString 在使用完后必需使用 g_string_free() 进行释放，g_string_free() 的定义如下：
    ```C
    gchar *g_string_free(GString *string, gboolean free_segment)
    ```
    - GString 的数据结构在前面已经介绍过了；
    - 当 free_segment 为 TRUE 时，这个函数不仅会释放 GString 结构本身，也会释放掉 GString 中的 str 占用的内存，否则该函数仅释放 GString 结构本身，而不释放其中的字符串 str；
* 下面的这段程序演示了在释放了 GString 后如何继续使用原 GString 中以 nul 结尾的字符串；
    ```C
    #include <glib.h>

    int main(int argc, char *argv[]) {
        // 创建一个新的 GString
        GString *gstring = g_string_new("");

        // 追加字符串到 GString
        g_string_append(gstring, "Hello World!");
        g_print("Initial GString content: %s\n", gstring->str);

        // 取出 GString 中字符串的指针
        gchar *p = gstring->str;
        // 以 FALSE 的方式释放 GString
        g_string_free(gstring, FALSE);

        // 释放 GString 后再次显示原 GString 中的字符串
        g_print("String after releasing GString: %s\n", p);
        return EXIT_SUCCESS;
    }
    ```

## 5 结束语
* 此篇文章仅仅是 GLib 的介绍文章，远没有涉及 GLib 的重要部分，GLib 能做的远不止像本文介绍的那样去替代 C 标准库中的函数；
* GLib 的 GObject 可以让程序员使用 C 进行面向对象的编程，听起来有点天方夜谭的感觉；
* C 语言中最令人头疼的无疑是指针，内存指针的操作失误会使 C 语言的程序崩溃，最近波及全球的微软蓝屏事件据初步报道就和 C 语言的内存指针相关，GLib 提供了一些宏来辅助指针的操作，同时，GLib 还提供了一系列内存管理的函数和宏，使得内存管理和指针应用更加安全；
* GLib 提供了一套丰富的类型系统，能够有效地减少编程错误，提高代码的可读性和可维护性，GLib 还提供了多种丰富的数据结构，如链表(单向链表、双向链表)、哈希表、动态数组等，这些数据结构能够高效地存储和管理数据，提升程序的性能；
* GLib 还提供了许多实用的功能支持，如事件循环、线程操作、动态链接库的操作、出错处理和日志等，这些功能使得基于 GLib 开发的应用程序能够更加方便地处理并发事件、管理资源、处理错误等，提高了程序的健壮性和稳定性；
* GLib 良好的可移植性也是广受赞誉的，基于 GLib 编写的应用程序可以轻松地在 Linux、Unix 以及 Windows 下运行，如果你要编写跨平台的应用程序，可以选择基于 GLib 编程。
* 在以后得文章中奖尽可能地介绍更多的基于 GLib 编程的方法和范例。






-------------
**email: hengch@163.com**



[advent_of_code]:https://adventofcode.com/2019/day/1

[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130002/puzzle-2019.c
[src02]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130002/puzzle-2019-glib.c

[img01]:/images/130002/screenshot-of-puzzle2019.png