---
title: "单向链表以及如何使用GLib中的GSList实现单向链表"
date: 2024-08-20T23:48:29+08:00
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
  - 单向链表

draft: false
#references: 
# - [GLib – 2.0 手册](https://docs.gtk.org/glib/index.html)
# - [GLIB用户指南](https://blog.csdn.net/l197803/article/details/138244885)
# - [Wikipedia GLib](https://en.wikipedia.org/wiki/GLib)
# - [Manage C data using the GLib collections](https://developer.ibm.com/tutorials/l-glib/)
# - [Getting Started with GLib](https://remcokranenburg.com/2021/05/30/getting-started-with-glib.html)
# - [百度AI助手](https://chat.baidu.com/)
#       - 单向链表、glib单向链表、glib单向链表有哪些优缺点、如何使用glib单向链表、glib单向链表范例、glib单向链表相关函数


postid: 130003
---

单向链表是一种基础的数据结构，也是一种简单而灵活的数据结构，本文讨论单向链表的基本概念及实现方法，并着重介绍使用GLib的GSList实现单向链表的方法及步骤，本文给出了多个实际范例源代码，旨在帮助学习基于GLib编程的读者较快地掌握GSList的使用方法，本文程序在 ubuntu 20.04 下编译测试完成，gcc 版本号 9.4.0；本文适合初学者阅读。


<!--more-->


## 1 单向链表及其实现
* 在文章[《使用GLib进行C语言编程的实例》][article01]中，简单介绍了 GLib，建议阅读本文前先阅读这篇文章；
* 单向链表是一种基础的数据结构，‌它由一系列节点组成，‌每个节点都包含数据部分和指向下一个节点的指针；
* 这种链表的特点是数据只能在一个方向上流动，‌即从头节点开始，‌通过每个节点的指针依次访问后续节点，‌直到链表的末尾；
* 在单向链表中，‌头节点是链表的起始点，‌它存储了链表的第一个数据元素以及指向下一个节点的指针；
* 随后的每个节点都存储了自己的数据和一个指向下一个节点的指针，‌这样形成了链表的结构；
* 链表的最后一个节点，‌也就是尾节点，‌它的指向下一个节点的指针指向 NULL，‌表示链表的结束。‌
* 单向链表的主要操作包括：‌
    1. 插入节点：‌可以在链表的头部、‌尾部或中间某个位置插入新的节点。‌
    2. 删除节点：‌可以删除链表中的任意节点，‌并通过调整指针来保持链表的完整性。‌
    3. 遍历链表：‌通过从头节点开始，‌依次访问每个节点，‌直到到达链表的末尾。‌
    4. 查找节点：‌根据特定的条件或值，‌在链表中查找节点。‌

* 单向链表的优势在于其动态的内存分配和高效的插入与删除操作，‌特别是在链表中间或头部插入和删除节点时，‌只需调整指针，‌无需移动其他数据；
* 然而，‌单向链表的访问效率较低，‌因为访问特定位置的元素需要从头节点开始遍历。‌
* 总的来说，‌单向链表是一种简单而灵活的数据结构，‌适用于需要频繁插入和删除操作，‌但访问操作相对较少的场景。‌
* 下面程序是一个简单的单向链表的 C 语言标准库实现，[sllist-c.c][src01](**点击文件名下载源程序**)
* 编译：`gcc -Wall -g sllist-c.c -o sllist-c`
* 运行：`./sllist-c`
* 该程序实现了单向链表的插入、删除、遍历和查找；
* 该程序首先建立一个单向链表，并在链表中加入 4 个节点，数据分别为：1、2、3、5，然后显示整个链表；
* 在第 2 个节点(数据为 3，索引号为 2)的后面插入节点，数据为 4，然后显示整个链表；
* 将第 2 个节点(数据为 3，索引号为 2)删除，然后显示整个链表；
* 最后释放整个链表；
* 运行截图：

    ![screenshot of sllist-c][img01]

## 2 GLib 中单向链表结构 GSList
* [GLib API version 2.0 手册][glib_api_v2.0] (**点击查看手册**)
* [GLib API 手册中 GSList 部分][glib_api_v2.0_gslist]  (**点击查看手册**)
* 在 GLib 中，‌单向链表是通过GSList结构体实现的。‌GSList是一个简单的单向链表结构，‌用于存储各种类型的数据；
* GSList 定义如下：
    ```C
    struct GSList {
        gpointer data;
        GSList *next;
    }
    ```
* data 为单向链表的数据指针，可以指向任何类型或结构的数据；
* next 为指向该单向链表下一个节点的指针；
* GLib 为单向链表结构 GSList 的操作提供了大量的函数，本文仅就其中的一部分函数进行介绍；

1. **添加、插入新节点**
    - `g_slist_append()` 在单向链表的最后添加一个新节点；
        ```C
        GSList *g_slist_append(GSList *list, gpointer data)
        ```
        + list - 指向单向链表的指针
        + data - 指向添加节点的数据
        + 返回指向单向链表的起始指针；
    - `g_slist_prepend()` 在单向链表的最前面添加一个新节点；
        ```C
        GSList *g_slist_prepend(GSList *list, gpointer data)
        ```
        + list - 指向单向链表的指针
        + data - 指向添加节点的数据
        + 返回指向单向链表的指针，在单向链表的开头添加一个节点，单向链表的指针是肯定会变化的；
        + 返回该单向链表的起始指针；
    - `g_slist_insert()` 在单向链表的中间插入一个新节点；
        ```C
        GSList *g_slist_insert(GSList *list, gpointer data, gint position)
        ```
        + list - 指向单向链表的指针
        + data - 指向添加节点的数据
        + position - 插入节点的位置，如果是负数或者超过了该单向链表的节点的数量，新节点将插到单向链表的最后；
        + 返回该单向链表的起始指针；
    - `g_slist_insert_before()` 在包含指定数据的节点之前插入一个新节点；
        ```C
        GSList *g_slist_insert_before(GSList *slist, GSList *sibling, gpointer data)
        ```
        + slist - 指向单向链表的指针
        + data - 指向添加节点的数据
        + sibling - 指向一个节点的指针，将在这个节点前插入新节点
        + 返回该单向链表的起始指针；
2. **删除节点**
    - `g_slist_remove_link()` 从单向链表中删除一个节点，但并不释放该节点占用的内存
        ```C
        GSList *g_slist_remove_link(GSList *list, GSList *link_)
        ```
        + list - 指向单向链表的指针；
        + link_ - 指向单向链表中一个节点的指针，该节点将被删除；
        + 返回该单向链表的起始指针；
        + 该函数并不释放被删除的节点内存，被删除的节点的 next 指针将指向 NULL，所以可以认为被删除的节点变成了一个只有一个节点的新的单向链表；
    - `g_slist_delete_link()` 从单向链表中删除一个节点，并释放该节点占用的内存；
        ```C
        GSList *g_slist_delete_link(GSList *list, GSList *link_)
        ```
        + list - 指向单向链表的指针；
        + link_ - 指向单向链表中一个节点的指针，该节点将被删除；
        + 返回该单向链表的起始指针；
        + 该函数与 `g_slist_remove_link()` 的唯一区别是该函数在删除节点后释放了被删除节点占用的内存；
    - `g_slist_remove()` 从单向链表中删除指定数据的一个节点，如果链表中有指定数据的节点有多个，将只删除第一个；
        ```C
        GSList *g_slist_remove(GSList *list, gconstpointer data)
        ```
        + list - 指向单向链表的指针
        + data - 指向要删除节点的数据
        + 返回该单向链表的起始指针；
    - `g_slist_remove_all()` 从单向链表中删除指定数据的所有节点；
        ```C
        GSList *g_slist_remove_all(GSList *list, gconstpointer data)
        ```
        + list - 指向单向链表的指针
        + data - 指向要删除节点的数据
        + 返回该单向链表的起始指针；
3. **遍历链表**
    - `g_slist_foreach()` 遍历单向链表，每个节点都会调用一个指定函数；
        ```C
        void g_slist_foreach(GSList *list, GFunc func, gpointer user_data)
        ```
        + list - 指向单向链表的指针
        + func - 一个指向函数的指针，遍历到单向链表的每个节点时，都会调用这个函数；
        + GFunc 的定义如下：
        ```C
        void (* GFunc) (gpointer data, gpointer user_data)
        ```
        + GFunc 的定义表明，传递给 func 的参数有两个，一个是 data - 指向当前节点的节点数据指针，另一个就是指向自定义参数 user_data 的指针
        + user_data - 指针指向调用 func 时传递的用户参数；
4. **查找节点**
    - `g_slist_find()` 查找链表中包含给定数据的节点；
        ```C
        GSList *g_slist_find(GSList *list, gconstpointer data)
        ```
        + list - 指向单向链表的指针
        + data - 指向要查找节点的数据
        + 返回在单向链表中找到的节点的指针，如果没有找到相应节点，返回 NULL;
    - `g_slist_index()` 获取包含给定数据的节点的位置(从 0 开始)；
        ```C
        gint g_slist_index(GSList *list, gconstpointer data)
        ```
        + list - 指向单向链表的指针；
        + data - 指向要查找节点的数据；
        + 返回数据为 data 的节点在单向链表中的位置(从 0 开始)，如果没找到相应节点，则返回 -1；
    - `g_slist_position()` 获取给定节点在链表中的位置(从 0 开始)；
        ```C
        gint g_slist_position(GSList *list, GSList *llink)
        ```
        + list - 指向单向链表的指针；
        + llink - 指向单向链表中的一个节点的指针；
        + 返回 llink 指向的节点在单向链表中的位置(从 0 开始)，如果没找到相应节点，则返回 -1；
        + 
5. **释放链表**
    - `g_slist_free()` 释放链表使用的所有内存，该函数不会释放节点中动态分配的内存；
        ```C
        void g_slist_free(GSList *list)
        ```
        + list - 指向单向链表的指针；
        + 该函数仅释放 GSList 占用的内存，并不释放单向链表中各个节点动态申请的内存，如果链表中有动态申请内存，考虑使用 `g_slist_free_full()` 或手动释放内存；
    - `g_slist_free_full()` 释放链表使用的所有内存，并对每个节点的数据调用指定的销毁函数
        ```C
        void g_slist_free_full(GSList *list, GDestroyNotify free_func)
        ```
        + list - 指向单向链表的指针；
        + free_func - 销毁函数，对单向链表中的每个节点数据将调用该函数，可用于释放节点中动态分配的内存；
        + GDestroyNotify 的定义如下：
        ```C
        void (* GDestroyNotify) (gpointer data)
        ```
        + 所以在调用 free_func 时会将指向节点数据的指针传递给该函数；
6. **其它**
    - `g_slist_length()` 获取单向链表的长度；
        ```C
        guint g_slist_length(GSList *list)
        ```
        + list - 指向单向链表的指针；
        + 返回单向链表中节点的数量。
    - `g_slist_last()` 获取单向链表的最后一个节点；
        ```C
        GSList *g_slist_last(GSList *list)
        ```
        + list - 指向单向链表的指针；
        + 返回单向链表的最后一个节点的指针，如果单向链表没有节点，则返回 NULL；
    - `g_slist_concat()`  连接两个单向链表；
        ```C
        GSList *g_slist_concat(GSList *list1, GSList *list2)
        ```
        + list1 - 指向第 1 个单向链表的指针；
        + list2 - 指向准备连接到第 1 个单向链表后面的单向链表的指针；
        + 返回连接好的单向链表的指针，

## 3 如何使用 GSList 实现单向链表
* 文章的一开始有一个使用标准 C 语言函数库的单向链表的实例，使用 GLib 的 GSList 操作单向链表要容易得多；
* 下面程序是使用 C 语言，基于 GLib 实现的单向链表，[sllist-glib.c][src02](**点击文件名下载源程序**)
* 该程序实现的功能与文章开头的程序 [sllist-c.c][src01] 完全一样，但程序看上去要简洁很多，我们不妨把源程序列在这里
    ```C
    #include <stdio.h>
    #include <glib.h>

    void print_node(gpointer data, gpointer user_data) {
        printf("%d -> ", GPOINTER_TO_INT(data));
    }

    void print_list(GSList *list) {
        g_slist_foreach(list, &print_node, NULL);
        printf("NULL\n");
    }

    int main() {
        GSList *list = NULL;

        printf("Append 4 nodes, the data are 1, 2, 3, 5.\n");
        list = g_slist_append(list, GINT_TO_POINTER(1));
        list = g_slist_append(list, GINT_TO_POINTER(2));
        list = g_slist_append(list, GINT_TO_POINTER(3));
        list = g_slist_append(list, GINT_TO_POINTER(5));
        print_list(list);

        printf("Insert a new node after node with the data 3.\n");
        list = g_slist_insert(list, GINT_TO_POINTER(4), 3);
        print_list(list);

        printf("Remove node with the data 3.\n");
        list = g_slist_remove(list, GINT_TO_POINTER(3));
        print_list(list);

        // Free the list
        g_slist_free(list);

        return 0;
    }
    ```
* 编译：
    ```bash
    gcc -Wall -g sllist-glib.c -o sllist-glib `pkg-config --cflags --libs glib-2.0`
    ```

* 其中，`pkg-config --cflags --libs glib-2.0` 的含义在文章[《使用GLib进行C语言编程的实例》][article01]中做过介绍；
* 运行：`./sllist-glib`
* 该程序实现了单向链表的插入、删除、遍历和查找；
* `print_list()` 中使用 `g_slist_foreach()` 对链表进行遍历，对链表中的每个节点数据，将调用函数 `print_node()`；
* 运行截图：

    ![screenshot of sllist-glib][img02]

## 4 单向链表的应用场景
* 单向链表是一种基础的数据结构，具有节点之间按顺序相连的特性，在特定场景下非常有用，以下是一些典型的应用场景：
    1. **动态数据集**

        > 当数据量不确定且频繁增删时，单向链表比数组更适用，它可以方便地在任意位置插入或删除节点，而不需要像数组那样移动大量元素；

    2. **队列和栈的实现**
        > 单向链表常用于实现队列(FIFO)和栈(LIFO)，因为它支持高效的插入和删除操作，尤其在头部或尾部进行操作时性能更好；

    3. **浏览历史记录或撤销操作**
        > 在一些应用程序中，如浏览器的历史记录，单向链表可以用来保存用户的浏览路径或操作步骤，方便逐步返回或撤销；

    4. **分配器管理内存块**
        > 操作系统的内存管理器中，单向链表经常被用于管理空闲的内存块(free lists)，通过链表可以快速地找到可用的内存块；

* 由于单向链表的简单结构，它在上述场景下既灵活又高效，特别是当增删操作频繁时。


## 5 基于 GLib 的 GSList 实现的 FIFO 队列
* FIFO(First Input First Output)队列，也就是先进先出队列，是一种简单的机制，操作一个 FIFO 队列需要队列的头指针和尾指针；
* 当向 FIFO 队列中加入数据时，数据添加到队列的尾指针处，当从队列中取出数据时，要从队列的头指针处取；
* FIFO 队列的重要参数是队列的最大长度，当队列中数据的数量达到队列的最大长度时，则不能再向队列中添加数据；
* FIFO 队列的两个重要判断就是判断队列为空(队列中没有数据)或者队列已满(数据数量达到最大长度)；
* 源程序 [queue-glib.c][src02](**点击文件名下载源程序**) 基于 GLib 的 GSList 实现了一个简单的 FIFO 队列；
* 该程序实现了 FIFO 队列的两个基本操作：入队操作和出队操作，基于 GLib 使得程序相当的简单；
    ```C
    #include <stdio.h>
    #include <glib.h>

    #define QUEUE_MAX_LEN           10
    GSList *queue_head, *queue_tail;        // head and tail pointers of the queue
    guint32 queue_max_len;                  // Max. length of the queue

    void queue_init(int maxn) {
        queue_head = queue_tail = NULL;
        queue_max_len = maxn;
    }

    gboolean queue_put(gpointer data) {
        guint queue_len = g_slist_length(queue_head);           // length of the queue
        if (queue_len >= queue_max_len) {
            // the queue is full
            return FALSE;
        } else {
            queue_head = g_slist_append(queue_head, data);      // append a node with data to the queue
            queue_tail = g_slist_last(queue_head);              // get the pointer of last node
        }
        return TRUE;
    }

    gpointer queue_get() {
        guint queue_len = g_slist_length(queue_head);           // length of the queue
        if (queue_len == 0) {
            // the queue is empty
            return NULL;
        }
        gpointer queue_data = queue_tail->data;                 // data pointer of the last node
        queue_head = g_slist_delete_link(queue_head, queue_tail);   // delete the last node
        if (queue_head == NULL) {
            queue_tail = queue_head;                            // the queue is empty
        } else {
            queue_tail = g_slist_last(queue_head);              // get the pointer of last node
        }
        return queue_data;
    }

    int main(int argc, char **argv) {
        guint64 len;
        if (argc >= 2) {
            len = g_ascii_strtoll(argv[1], NULL, 10);           // Convert string to int
            if (len <= 0 || len > (QUEUE_MAX_LEN * 10)) {
                len = QUEUE_MAX_LEN;
            }
        } else {
            len = QUEUE_MAX_LEN;
        }

        printf("Max. length of the queue is %ld.\n", len);
        queue_init(len);            // Initialize the queue

        guint16 i;
        // append some data to the queue
        for (i = 0; i < (queue_max_len << 1); ++i) {
            if (queue_put(GINT_TO_POINTER(i + 1))) {
                printf("Put data %d into the queue.\n", i + 1);
            } else {
                printf("The queue is full.\n");
                break;
            }
        }
        // get some data from the queue
        for (i = 0; i < (queue_max_len << 1); ++i) {
            gpointer queue_data = queue_get();
            if (queue_data != NULL) {
                printf("Get data %d from the queue.\n", GPOINTER_TO_INT(queue_data));
            } else {
                printf("The queue is empty.\n");
                break;
            }
        }

        return 0;
    }
    ```
* 可以看出，用 GLib 实现的 FIFO 队列非常简洁；
* 编译：
    ```bash
    gcc -Wall -g queue-glib.c -o queue-glib `pkg-config --cflags --libs glib-2.0`
    ```

* 其中，`pkg-config --cflags --libs glib-2.0` 的含义在文章[《使用GLib进行C语言编程的实例》][article01]中做过介绍；
* 运行：`./queue-glib 8`
* 运行截图：

    ![screenshot of queue-glib][img03]

* 该程序并不完整，如果实际运用，至少要加一个互斥锁，以保证 FIFO 队列的线程安全；
* 使用 GLib 的 GSList 实现的 FIFO 队列，其中的数据并不需要是相同的数据类型，因为队列中存储的数据的指针，这一点在某些应用场景下会带来一些方便，但也会增加开销，而且在数据使用完成后有可能需要释放额外申请的内存空间。




-------------
**email: hengch@163.com**


![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[glib_api_v2.0]:https://docs.gtk.org/glib/index.html
[glib_api_v2.0_gslist]:https://docs.gtk.org/glib/struct.SList.html

[article01]:https://blog.csdn.net/whowin/article/details/142472383


[src01]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130003/sllist-c.c
[src02]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130003/sllist-glib.c
[src03]:https://gitee.com/whowin/whowin/blob/blog/sourcecodes/130003/queue-glib.c

[img01]:/images/130003/screenshot-of-sllist-c.png
[img02]:/images/130003/screenshot-of-sllist-glib.png
[img03]:/images/130003/screenshot-of-queue-glib.png

<!--CSDN -->
[img01]:https://i-blog.csdnimg.cn/direct/40a88ebab8f145d39bcacac6fd3af029.png
[img02]:https://i-blog.csdnimg.cn/direct/8dd305d65a9c4084a3d463fc6558878b.png
[img03]:https://i-blog.csdnimg.cn/direct/6fd31fc6c6e44881aef014c360e95f95.png

