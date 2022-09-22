---
title: "Linux文件属性，lsattr和chattr的使用"
date: 2022-08-29T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "Linux"
tags:
  - Linux
  - 文件属性
  - "file attribute"
  - lsattr
  - chattr
draft: true
#references: 
# - [Chattr Command in Linux (File Attributes)](https://linuxize.com/post/chattr-command-in-linux/)
# * [Extent (file systems)](https://en.wikipedia.org/wiki/Extent_(file_systems))
# * [chattr](https://en.wikipedia.org/wiki/Chattr)
postid: 100013
---


## 以下翻译自https://linuxize.com/post/chattr-command-in-linux/

> 在Linux中，文件属性是描述文件行为的元数据属性。例如，属性可以指示文件是否被压缩或指定文件是否可以删除。

> 一些属性(如不可变性[immutability])可以设置或清除，而其他属性(如加密)是只读的，只能查看。对某些属性的支持取决于所使用的文件系统。

> 本文介绍如何使用chattr命令在Linux文件系统上更改文件属性。

### chattr语法
* chattr命令采用以下语法格式
    ```
    chattr [OPTIONS] [OPERATOR][ATTRIBUTES] FILE...
    ```
* [OPERATOR]部分的值可以是下列符号之一:
    - \+ ：告诉chatr将指定的属性添加到现有的属性中。
    - \- ：告诉chatr从现有的属性中删除指定的属性。
    - \= ：告诉chatr将指定的属性设置为唯一的属性。
* 操作符后面跟着一个或多个[ATTRIBUTES]标志，这些标志是准备从文件属性中添加或删除的。下面是一些常见属性和相关标志的列表:
    - a(append) ： 当设置此属性时，文件只能以追加模式打开以进行写入。
    - A(atime) ： 当打开具有此属性集的文件时，其atime记录不会更改。Atime(访问时间)是文件被某些命令或应用程序访问/打开的最后一次时间。
    - e(extends) ： 这个属性表示文件使用区段来映射磁盘上的块。属性不能用chatr修改。
    - i(immutable) ： 此属性指示文件是不可变的，这意味着文件不能被删除或重命名。
* 要获得所有文件属性和标志的完整列表，请在终端中输入man chatattr。
* 默认情况下，当使用cp或rsync等命令复制文件时，文件属性不会被保留。

### chattr范例
* chattr的一个常见用途是将不可变标志设置为文件或目录，以防止用户删除或重命名文件。
* 可以使用lsattr命令查看文件属性
    ```
    lsattr todo.txt
    ```
* 下面的输出显示只设置了e标志
    ```
    output
    --------------e----- todo.txt
    ```
* 要使文件不可变，请在现有属性中添加i标记和+操作符
    ```
    sudo chattr +i todo.txt
    ```
* 使用chattr，您可以一次添加或删除多个属性。例如，要使文件不可变，并告诉内核不要跟踪最后一次访问时间，您可以使用：
    ```
    sudo chattr +iA todo.txt
    ```
* 最后一个可以使用的操作符是=操作符。例如，要将e属性设置为唯一属性，您可以使用：
    ```
    sudo chattr "=e" todo.txt
    ```
* 注意，操作符和标志用引号括起来，以避免shell对+字符的解释。

### 结论
* chattr是一个命令行工具，用于更改Linux文件系统上的文件属性。
* 如果你有任何问题或反馈，欢迎留下评论。


## 以下摘自 man chattr
* 文件全部属性
  - 可以被设置的属性
    |序号|参数|说明|
    |:--:|:--:|:--|
    |1|a|append only|
    |2|A|atime updates|
    |3|c|compressed|
    |4|C|no copy or write|
    |5|d|no dump|
    |6|D|synchronous directory updates|
    |7|e|extent format|
    |8|F|case-insensitive lookups|
    |9|i|immutable|
    |10|j|dat journalling|
    |11|P|project hierarchy|
    |12|s|secure deletion|
    |13|S|synchronous updates|
    |14|t|no tail-merging|
    |15|T|top of directory hierarchy|
    |16|u|undeletable|
  - 无法使用chattr设置，只能用lsattr显示的属性
    |序号|参数|说明|
    |:--:|:--:|:--|
    |1|E|encrypted|
    |2|I|indexed directory|
    |3|N|inline data|
    |4|V|verity|

