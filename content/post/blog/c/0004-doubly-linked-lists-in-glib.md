---
title: "双向链表及如何使用GLib的GList实现双向链表"
date: 2024-10-30T23:48:29+08:00
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
  - GList
  - 双向链表

draft: false
#references: 
# - [GLib – 2.0 手册](https://docs.gtk.org/glib/index.html)
# - [GLIB用户指南](https://blog.csdn.net/l197803/article/details/138244885)
# - [Wikipedia GLib](https://en.wikipedia.org/wiki/GLib)
# - [Manage C data using the GLib collections](https://developer.ibm.com/tutorials/l-glib/)
# - [Getting Started with GLib](https://remcokranenburg.com/2021/05/30/getting-started-with-glib.html)
# - [百度AI助手](https://chat.baidu.com/)
#       - 单向链表、glib单向链表、glib单向链表有哪些优缺点、如何使用glib单向链表、glib单向链表范例、glib单向链表相关函数


postid: 130004
---

双向链表是一种比单向链表更为灵活的数据结构，与单向链表相比可以有更多的应用场景，本文讨论双向链表的基本概念及实现方法，并着重介绍使用GLib的GList实现单向链表的方法及步骤，本文给出了多个实际范例源代码，旨在帮助学习基于GLib编程的读者较快地掌握GList的使用方法，本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；本文适合初学者阅读。


<!--more-->


## 1 双向链表及其实现
* 在文章[《单向链表以及如何使用GLib中的GSList实现单向链表》][article02]中，介绍了单向链表以及基于 GLib 实现单向链表的方法，建议阅读本文前先阅读这篇文章；
* 在文章[《使用GLib进行C语言编程的实例》][article01]中，简单介绍了 GLib，建议阅读本文前先阅读这篇文章；
* 双向链表(Doubly Linked List)是一种链式数据结构，每个节点包含三个主要部分：
    1. 数据部分：存储节点的数据
    2. 前向指针：指向链表中的下一个节点
    3. 后向指针：指向链表中的上一个节点
* 可以看出，和单向链表相比较，双向链表多了一个指向前一个节点的指针
* 双向链表的基本特性
    1. 双向性：与单向链表不同，双向链表允许从两个方向遍历，可以从头节点向尾节点遍历，也可以从尾节点向头节点遍历；
    2. 动态大小：双向链表的大小可以动态增长或缩小，不需要提前定义大小；
    3. 节点插入和删除：在双向链表中，插入和删除节点操作相对简单，因为每个节点都有指向前后节点的指针；
* 双向链表的节点结构：
    ```C
    struct Node {
        int data;               // 数据部分
        struct Node *next;      // 指向下一个节点的指针
        struct Node *prev;      // 指向前一个节点的指针
    };
    ```
* 双向链表的基本操作
    1. 插入节点：可以在链表的开头、结尾或任意位置插入节点；
    2. 删除节点：可以删除链表中的任意节点，操作相对简单，因每个节点都知道其前一个和后一个节点；
    3. 遍历链表：可以从头到尾遍历链表(正向遍历)或从尾到头遍历链表(反向遍历)；
* 与单向链表相比，双向链表有以下特点：
    1. 由于数据结构中增加了后向指针，使链表可以双向遍历，而单向链表仅能单向遍历；
    2. 通过后向指针可以直接访问前一个节点，与单向链表相比，可以简化节点删除操作的复杂度；
    3. 在插入节点时，比单向链表更快捷更灵活；
    4. 与单向链表相比，由于增加了后向指针，内存开销增加；
    5. 与单向链表相比，双向链表需要操作两个指针，其操作和维护的复杂度要高一些；

* 总的来说，‌双向链表比单向链表更加灵活，‌适用场景也要多一些。；
* 下面程序是一个简单的双向链表的 C 语言标准库实现，[dllist-c.c][src01](**点击文件名下载源程序**)
* 编译：`gcc -Wall -g dllist-c.c -o dllist-c`
* 运行：`./dllist-c`
* 该程序实现了双向链表的插入、删除以及正向遍历；
* 该程序首先建立一个双向链表，并在链表中加入 4 个节点，数据分别为：1、2、3、5，然后显示整个链表；
* 在第 3 个节点(数据为 3，索引号为 2)的后面插入节点，数据为 4，然后显示整个链表；
* 将第 3 个节点(数据为 3，索引号为 2)删除，然后显示整个链表；
* 最后释放整个链表；
* 运行截图：

    ![screenshot of dllist-c][img01]

## 2 GLib 中双向链表结构 GList
* [GLib API version 2.0 手册][glib_api_v2.0] (**点击查看手册**)
* [GLib API 手册中 GList 部分][glib_api_v2.0_glist]  (**点击查看手册**)
* 在 GLib 中，‌双向链表是通过 GList 结构体实现的，GList 是一个简单的双向链表结构，‌用于存储各种类型的数据；
* GSList 定义如下：
    ```C
    struct GList {
        gpointer data;
        GList *next;
        GList *prev;
    }
    ```
* data 为双向链表的数据指针，可以指向任何类型或结构的数据；
* next 为指向该双向链表当前节点的下一个节点的指针；
* prev 为指向该双向链表当前节点的前一个节点的指针；
* GLib 为双向链表结构 GList 的操作提供了大量的函数，本文仅就其中的一部分函数进行简单介绍；

1. **添加、插入新节点**
    - `g_list_append()` 在双向链表的最后添加一个新节点；
        ```C
        GList *g_slist_append(GList *list, gpointer data)
        ```
        + list - 指向双向链表的指针
        + data - 指向添加节点的数据
        + 返回指向双向链表的起始指针；
        + 说明：在双向链表的最后添加节点，必须要遍历整个链表才能找到链表的尾部，这种做法效率很低，通常的做法是使用 `g_list_prepend()` 在链表的起始位置添加节点，当所有节点添加完毕后，再使用 `g_list_reverse()` 将整个链表反转；
    - `g_list_prepend()` 在双向链表的最前面添加一个新节点；
        ```C
        GList *g_list_prepend(GList *list, gpointer data)
        ```
        + list - 指向双向链表的指针
        + data - 指向添加节点的数据
        + 返回指向双向链表的指针，在双向链表的开头添加一个节点，双向链表的指针是肯定会变化的；
    - `g_list_insert()` 在双向链表的中间插入一个新节点；
        ```C
        GList *g_list_insert(GList *list, gpointer data, gint position)
        ```
        + list - 指向双向链表的指针
        + data - 指向添加节点的数据
        + position - 插入节点的位置，如果是负数或者超过了该双向链表的节点的数量，新节点将插到双向链表的最后；
        + 返回该双向链表的起始指针；
    - `g_list_insert_before()` 在包含指定数据的节点之前插入一个新节点；
        ```C
        GList *g_list_insert_before(GList *list, GSList *sibling, gpointer data)
        ```
        + list - 指向双向链表的指针
        + data - 指向添加节点的数据
        + sibling - 指向一个节点的指针，将在这个节点前插入新节点
        + 返回该双向链表的起始指针；
2. **删除节点**
    - `g_list_remove_link()` 从双向链表中删除一个节点，但并不释放该节点占用的内存
        ```C
        GList *g_list_remove_link(GList *list, GList *llink_)
        ```
        + list - 指向双向链表的指针；
        + llink_ - 指向双向链表中一个节点的指针，该节点将被删除；
        + 返回该双向链表的起始指针；
        + 该函数并不释放被删除的节点内存，被删除的节点的 next 和 prev 指针将指向 NULL，所以可以认为被删除的节点变成了一个只有一个节点的新的双向链表；
    - `g_list_delete_link()` 从双向链表中删除一个节点，并释放该节点占用的内存；
        ```C
        GList *g_list_delete_link(GList *list, GList *link_)
        ```
        + list - 指向双向链表的指针；
        + link_ - 指向双向链表中一个节点的指针，该节点将被删除；
        + 返回该双向链表的起始指针；
        + 该函数与 `g_list_remove_link()` 的唯一区别是该函数在删除节点后释放了被删除节点占用的内存；
    - `g_list_remove()` 从双向链表中删除指定数据的一个节点，如果链表中有指定数据的节点有多个，将只删除第一个；
        ```C
        GList *g_list_remove(GList *list, gconstpointer data)
        ```
        + list - 指向双向链表的指针
        + data - 指向要删除节点的数据
        + 返回该双向链表的起始指针；
    - `g_list_remove_all()` 从双向链表中删除指定数据的所有节点；
        ```C
        GList *g_list_remove_all(GList *list, gconstpointer data)
        ```
        + list - 指向双向链表的指针
        + data - 指向要删除节点的数据
        + 返回该双向链表的起始指针；
3. **遍历链表**
    - `g_list_foreach()` 遍历双向链表，每个节点都会调用一个指定函数；
        ```C
        void g_list_foreach(GList *list, GFunc func, gpointer user_data)
        ```
        + list - 指向双向链表的指针
        + func - 一个指向函数的指针，遍历到双向链表的每个节点时，都会调用这个函数；
        + GFunc 的定义如下：
        ```C
        void (* GFunc) (gpointer data, gpointer user_data)
        ```
        + GFunc 的定义表明，传递给 func 的参数有两个，一个是 data - 指向当前节点的节点数据指针，另一个就是指向自定义参数 user_data 的指针
        + user_data - 指针指向调用 func 时传递的用户参数；
4. **查找节点**
    - `g_list_find()` 查找链表中包含给定数据的节点；
        ```C
        GList *g_list_find(GList *list, gconstpointer data)
        ```
        + list - 指向双向链表的指针
        + data - 指向要查找节点的数据
        + 返回在双向链表中找到的节点的指针，如果没有找到相应节点，返回 NULL;
    - `g_list_index()` 获取包含给定数据的节点的位置(从 0 开始)；
        ```C
        gint g_list_index(GList *list, gconstpointer data)
        ```
        + list - 指向双向链表的指针；
        + data - 指向要查找节点的数据；
        + 返回数据为 data 的节点在双向链表中的位置(从 0 开始)，如果没找到相应节点，则返回 -1；
    - `g_list_position()` 获取给定节点在链表中的位置(从 0 开始)；
        ```C
        gint g_list_position(GList *list, GList *llink)
        ```
        + list - 指向双向链表的指针；
        + llink - 指向双向链表中的一个节点的指针；
        + 返回 llink 指向的节点在双向链表中的位置(从 0 开始)，如果没找到相应节点，则返回 -1；

5. **释放链表**
    - `g_list_free()` 释放链表使用的所有内存，该函数不会释放节点中动态分配的内存；
        ```C
        void g_list_free(GList *list)
        ```
        + list - 指向双向链表的指针；
        + 该函数仅释放 GList 占用的内存，并不释放双向链表中各个节点动态申请的内存，如果链表中有动态申请内存，考虑使用 `g_list_free_full()` 或手动释放内存；
    - `g_list_free_full()` 释放链表使用的所有内存，并对每个节点的数据调用指定的销毁函数
        ```C
        void g_list_free_full(GList *list, GDestroyNotify free_func)
        ```
        + list - 指向双向链表的指针；
        + free_func - 销毁函数，对双向链表中的每个节点数据将调用该函数，可用于释放节点中动态分配的内存；
        + GDestroyNotify 的定义如下：
        ```C
        void (* GDestroyNotify) (gpointer data)
        ```
        + 所以在调用 free_func 时会将指向节点数据的指针传递给该函数；
6. **其它**
    - `g_list_length()` 获取双向链表的长度；
        ```C
        guint g_list_length(GList *list)
        ```
        + list - 指向双向链表的指针；
        + 返回双向链表中节点的数量。
    - `g_list_last()` 获取双向链表的最后一个节点；
        ```C
        GList *g_list_last(GList *list)
        ```
        + list - 指向单向链表的指针；
        + 返回双向链表的最后一个节点的指针，如果双向链表没有节点，则返回 NULL；
    - `g_list_concat()`  连接两个双向链表；
        ```C
        GList *g_list_concat(GList *list1, GList *list2)
        ```
        + list1 - 指向第 1 个双向链表的指针；
        + list2 - 指向准备连接到第 1 个双向链表后面的双向链表的指针；
        + 返回连接好的双向链表的指针，
    - `g_list_reverse()` 反转整个双向链表
        ```C
        GList *g_list_reverse(GList *list)
        ```
        + list - 指向双向链表的指针；
        + 返回该双向链表的起始指针；

## 3 如何使用 GList 实现双向链表
* 文章的一开始有一个使用标准 C 语言函数库的双向链表的实例，使用 GLib 的 GList 操作双向链表要容易得多；
* 下面程序是使用 C 语言，基于 GLib 实现的双向链表，[dllist-glib.c][src02](**点击文件名下载源程序**)
* 该程序实现的功能与文章开头的程序 [dllist-c.c][src01] 完全一样，但程序看上去要简洁很多，我们不妨把源程序列在这里
* 该程序与文章[《单向链表以及如何使用GLib中的GSList实现单向链表》][article02]中使用 GLib 实现单向链表的程序非常相似
    ```C
    #include <stdio.h>
    #include <glib.h>

    void print_node(gpointer data, gpointer user_data) {
        printf("%d -> ", GPOINTER_TO_INT(data));
    }
    void print_list(GList *list) {
        g_list_foreach(list, &print_node, NULL);
        printf("NULL\n");
    }

    int main() {
        GList *list = NULL;

        printf("Append 4 nodes, the data are 1, 2, 3, 5.\n");
        list = g_list_append(list, GINT_TO_POINTER(1));
        list = g_list_append(list, GINT_TO_POINTER(2));
        list = g_list_append(list, GINT_TO_POINTER(3));
        list = g_list_append(list, GINT_TO_POINTER(5));
        print_list(list);

        printf("Insert a new node after node with the data 3.\n");
        list = g_list_insert(list, GINT_TO_POINTER(4), 3);
        print_list(list);

        printf("Remove node with the data 3.\n");
        list = g_list_remove(list, GINT_TO_POINTER(3));
        print_list(list);

        // Free the list
        g_list_free(list);

        return 0;
    }
    ```
* 该程序中涉及到的两个宏：`GINT_TO_POINTER(value)` 和 `GPOINTER_TO_INT(p)`，在文章[《单向链表以及如何使用GLib中的GSList实现单向链表》][article02]中有比较详细的介绍；
* 编译：
    ```bash
    gcc -Wall -g dllist-glib.c -o dllist-glib `pkg-config --cflags --libs glib-2.0`
    ```

* 其中，`pkg-config --cflags --libs glib-2.0` 的含义在文章[《使用GLib进行C语言编程的实例》][article01]中做过介绍；
* 运行：`./dllist-glib`
* 该程序实现了双向链表的插入、删除、遍历；
* `print_list()` 中使用 `g_list_foreach()` 对链表进行遍历，对链表中的每个节点数据，将调用函数 `print_node()`；
* 运行截图：

    ![screenshot of dllist-glib][img02]

## 4 双向链表的应用场景
* 双向链表是一种数据结构，它的每个节点包含对前一个节点和后一个节点的引用；这种结构在许多应用场景中非常有用，以下是一些常见的应用场景：

1. 浏览器历史记录：
    
    > 双向链表可以用来实现浏览器的“后退”和“前进”按钮，用户可以在历史记录中前后移动当前指针；

2. 音乐播放器：

    > 在音乐播放器中，双向链表可以用于管理播放列表，允许用户在歌曲之间前后切换；

3. 文本编辑器：

    > 在实现撤销和重做功能时，双向链表可用于存储编辑历史，方便在不同操作间切换；

4. LRU缓存：

    > 在实现最近最少使用(LRU)缓存时，双向链表可以高效地维护访问顺序，以便快速找到和删除最少使用的项；

5. 操作系统中的进程调度：

    > 在某些调度算法中，双向链表可用于管理就绪队列，使得进程可以方便地添加和移除；

6. 图形界面中的组件布局：

    > 在某些图形用户界面(GUI)框架中，双向链表用于管理组件的顺序和关系，使得组件之间的插入和删除变得灵活；

7. 实现栈和队列：

    > 双向链表可以作为基础结构来实现栈和队列，提供灵活的插入和删除操作。

## 5 基于 GLib 的 GList 模拟终端命令的历史记录
* 当我们在 Linux 终端上输入命令时，终端应用程序会记录你输入的命令并形成历史记录，可以使用 `history` 命令来查看这个历史记录；
* 在终端上也可以使用上、下箭头键来翻看曾经输入过的前一个或者后一个历史命令，这个命令历史记录给使用终端带来了一定的便利；
* 本实例模拟了终端输入命令并使用双向链表生成命令的历史记录，按上下箭头键可以查看上一条或下一条命令；
* 源程序 [cmd-history.c][src02](**点击文件名下载源程序**) 基于 GLib 的 GList 模拟了终端历史记录；
* 该程序首先建立了一个双向链表队列，然后模拟输入命令，链表中的每个节点存储一条命令，命令输入完成后显示最后一条命令，然后按上下箭头键可以从链表中取出上一条命令或者下一条命令并显示在屏幕上；
* 很显然，使用单向链表实现命令历史记录是不方便的，但使用双向链表就很方便；
* 编译：
    ```bash
    gcc -Wall -g cmd-history.c -o cmd-history `pkg-config --cflags --libs glib-2.0`
    ```

* 其中，`pkg-config --cflags --libs glib-2.0` 的含义在文章[《使用GLib进行C语言编程的实例》][article01]中做过介绍；
* 运行：`./cmd-history`
* 运行截图：

    ![screenshot of cmd-history][img03]

* 该程序涉及到终端的操作，使用了结构 `struct termios`、函数 `tcgetattr()` 和 `tcsetattr()`，这些并不在 C 标准库 libc 中，需要启用 GNU 扩展库，所以在程序的开始有 `#define _GNU_SOURCE`
* 有关终端操作的相关数据结构、宏定义以及相关函数，并不在本文的讨论之内，请自行参考其它资料；
* 该程序中还涉及到了使用 ESC 转义符对终端屏幕进行清屏操作，有关 ESC 转义符的含义，请参考另一篇文章[《ANSI的ESC转义序列》][article03]
* 该程序中还涉及到了从键盘缓冲区读取上、下箭头键的方法，上箭头键返回的编码为 `ESC [ A`，下箭头键返回的编码为 `ESC [ B`，这里说明一下有助于读者更快地读懂程序。



-------------
**email: hengch@163.com**


![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[glib_api_v2.0]:https://docs.gtk.org/glib/index.html
[glib_api_v2.0_glist]:https://docs.gtk.org/glib/struct.List.html

[article01]:https://blog.csdn.net/whowin/article/details/142472383
[article02]:https://blog.csdn.net/whowin/article/details/142472406
[article03]:https://blog.csdn.net/whowin/article/details/128767730

[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130004/dllist-c.c
[src02]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130004/dllist-glib.c
[src03]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130004/cmd-history.c

[img01]:/images/130004/screenshot-of-dllist-c.png
[img02]:/images/130004/screenshot-of-dllist-glib.png
[img03]:/images/130004/screenshot-of-cmd-history.png


