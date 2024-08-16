---
title: "IPC之八：使用 POSIX 共享内存进行进程间通信的实例"
date: 2023-09-27T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "IPC"
  - "Linux"
  - "C Language"
tags:
  - Linux
  - 进程间通信
  - IPC
  - POSIX
  - 共享内存
  - "Shared Memory"
draft: false
#references: 
# - `man shm_overview`
# - [Posix共享内存](https://zhuanlan.zhihu.com/p/639385701)
# - [POSIX Shared Memory](https://www.cs.unibo.it/~renzo/doc/C/corso2/node27.html#SECTION002740000000000000000)
# - [POSIX Shared Memory with C Programming](https://linuxhint.com/posix-shared-memory-c-programming/)
# - [POSIX Shared Memory in Linux](https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux)
# - [POSIX Shared Memory](https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/ShMem.html)
# - [Producer-Consumer Problem](http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/7-IPC2/sync-pro.html)
# - [POSIX shared-memory API](https://www.geeksforgeeks.org/posix-shared-memory-api/)

postid: 190018
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍共享内存(Shared Memory)，因为没有像管道、消息队列这样的中介介入，所以通常认为共享内存是迄今为止最快的 IPC 方式；Linux 既支持 UNIX SYSTEM V 的共享内存段，也支持 POSIX 的共享内存对象，本文针对 POSIX 共享内存对象，本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文的实例中涉及多进程编程和信号处理等，阅读本文还需要一些基本的内存管理知识，本文对 Linux 编程的初学者有一些难度。

<!--more-->

## 1 POSIX 共享内存对象
* 在文章[《IPC之七：使用 System V 共享内存段进行进程间通信的实例》][article07]中介绍 System V 的共享内存段，其概念相似，在阅读本文前可以先阅读这篇文章；
* POSIX 的共享内存与 System V 的共享内存段的使用步骤相似：
    - 创建并配置共享内存；
    - 将共享内存地址映射到进程的地址空间上；
    - 像使用普通内存一样使用共享内存；
    - 使用完毕后释放映射地址；
    - 删除共享内存；
* 通常把 System V 共享内存称为共享内存段(Shared Memory Segment)，把 POSIX 共享内存称为共享内存对象(Shared Memory Object)；
    - `man shm_overview` 可以在线查看 POSIX 共享内存的基本概念；
    - `man sysvipc` 可以在线查看 System V 共享内存的基本概念；
* 本文针对 POSIX 共享内存，如无特别说明，下面提到的"共享内存"均指 **POSIX 共享内存**；
* POSIX 的共享内存实际上和文件的内存映射的使用非常类似，Linux 可以把一个文件的全部或部分映射到内存上，应用程序通过访问内存便可以访问文件；
* POSIX 共享内存提供了一组用于操作共享内存的 API：
    - 打开/创建一个共享内存对象
        ```C
        int shm_open(const char *name, int oflag, mode_t mode);
        ```
    - 设置共享内存对象的大小
        ```C
        int ftruncate(int fd, off_t length);
        ```
    - 删除共享内存对象
        ```C
        int shm_unlink(const char *name);
        ```
* 还有一些用于共享内存的 API 并不是共享内存对象专有的：
    - 把共享内存对象映射到进程的地址空间；
        ```C
        void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
        ```
    - 释放映射的地址；
        ```C
        int munmap(void *addr, size_t length);
        ```
    - 关闭打开的共享内存对象；
        ```C
        int close(int fd);
        ```
    - 获取共享内存对象的相关信息；
        ```C
        int fstat(int fd, struct stat *statbuf);
        ```
    - 更改共享内存对象的所有人；
        ```C
        int fchown(int fd, uid_t owner, gid_t group);
        ```
    - 更改共享内存对象的权限；
        ```C
        int fchmod(int fd, mode_t mode);
        ```
* 本文将重点介绍：`shm_open()`、`ftruncate()`、`mmap()` 等几个；
* 通常情况下，共享内存是和信号量配合使用的，所以建议阅读本文之前阅读另一篇文章[《IPC之六：使用 POSIX 信号量解决经典的'生产者-消费者问题'》][article06]。

--------
## 2 共享内存对象和虚拟文件系统
* SYSTEM V 的共享内存段是用 key 或者 ID 来标识的，是没有名称的，POSIX 的共享内存对象是有名称的，使用一个唯一的名称来标识一个共享内存对象；
* 和 POSIX 的消息队列(参考文章[《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》][article04])一样，共享内存对象的名称是一个以 "/" 开头的字符串，而且名称中不允许再出现第二个 "/"，这个名称字符串以 NULL 结尾；
* POSIX 共享内存对象的名称最长为 `NAME_MAX`，宏定义 NAME_MAX 定义在头文件 `<linux/limits.h` 中，为 255；
* 任何一个知道共享内存对象名称且具有适当权限的进程都可以使用该共享内存对象；
* 和 POSIX 的消息队列一样，POSIX 的共享内存对象也是建在一个虚拟文件系统下的，是和 POSIX 的信号量共用一个虚拟文件系统，被挂载在 `/dev/shm/` 下，在 Ubuntu 下，默认是自动挂载的；
* 可以使用 `ls /dev/shm/` 命令列出已有的共享内存对象，也可以用 `rm /dev/shm/<shm_name>` 命令删除一个已有的共享内存对象；
* 尽管 POSIX 共享内存的在线手册(`man shm_open`)中明确共享内存对象的名称必须以 "/" 开头，但在具体编程实践中发现在调用 `shm_open()` 创建一个新的或打开一个已经存在的共享内存对象时，共享内存对象名称有没有 "/" 似乎并没有什么区别，即：下面两个调用没有区别；
    ```C
    shm_open("/shmname-testing", O_CREAT, 0666);

    shm_open("shmname-testing", O_CREAT, 0666);
    ```
* 但是，编程实践表明 POSIX 消息队列的名称是必须以 "/" 开头的；

* 如果在你的系统上没有这个虚拟文件系统，可以用下面的命令建立一个：
    ```bash
    # mkdir /dev/shm
    # mount -t shm none /dev/shm
    ```

---------
## 3 共享内存对象创建/打开和配置
* 在使用一个共享内存对象前必须要打开(或创建)该对象，使用 `shm_open()`
    ```C
    #include <sys/mman.h>
    #include <sys/stat.h>        /* For mode constants */
    #include <fcntl.h>           /* For O_* constants */

    int shm_open(const char *name, int oflag, mode_t mode);
    ```
* `shm open()` 创建一个新的或打开一个已存在的共享内存对象，调用成功则返回该对象的文件描述符，失败则返回 -1，errno 中为错误代码；
* 任何有适当权限的进程均可以通过 `shm_open()` 返回的文件描述符共享内存的同一块区域；
* **name** - 共享内存对象的名称，一个以 "/" 开头的字符串(实践表明可以也没有这个 "/")，是这个共享内存对象的唯一标识；
* **oflag** - 控制操作的一些标志，可以有以下选项，这些选项可以以 or 的方式进行组合：
    - **O_RDONLY** - 以只读方式打开一个已经存在的共享内存对象；以该方式打开的对象在使用 `mmap()` 进行映射时，可以使用只读方式(PORT_READ)映射；
    - **O_RDWR** - 以读/写方式打开一个共享内存对象，这是默认方式；
    - **O_CREAT** - 如果共享内存对象不存在，则建立一个新的共享内存对象，返回其文件描述符；如果共享内存对象存在，则返回该对象的文件描述符；
    - **O_EXCL** - 如果同时设置了 O_CREAT，则当打开的共享内存对象已经存在时，返回调用失败，errno=EEXIST
    - **O_TRUNC** - 如果共享内存对象已经存在，则将其长度截断为零字节；
* 关于 oflag 中的 O_EXCL：
    - 当 `O_CREAT | O_EXCL` 时：共享内存对象不存在则创建一个新的共享内存对象(与 O_CREAT 一样)，返回其文件描述符；如果共享内存对象存在则返回 -1，`errno=EEXIST(File exists)`
    - 当仅有 `O_EXCL` 时：如果共享内存对象已经存在，则返回其文件描述符(与 O_CREAT 一样)，如果共享内存对象不存在，则返回 -1，`errno=ENOENT(No such file or directory)`；
* **mode** - 新创建的共享内存对象的读/写权限，与文件的读/写权限的表达方式一致，使用八进制数，比如：0660、0666，实际的读/写权限还要受到 umask 的影响，最终的结果是 `mode & ~umask`；
    > 可以使用 shell 命令 `umask` 查看当前的 `umask`

* 新创建的共享内存对象需要使用 `ftruncate()` 配置其空间大小
    ```C
    #include <unistd.h>
    #include <sys/types.h>

    int ftruncate(int fd, off_t length);
    ```
* `ftruncate()` 将 fd 指定的共享内存对象的长度设置为 length 指定的长度；
* **fd** - 使用 `shm_open()` 返回的共享内存对象的文件描述符；
* **length** - 共享内存的长度，以字节为单位；
* 实际上 `ftruncate()` 也可以改变一个已有的共享内存对象的长度；
    - 当原有共享内存的长度大于 length 时：`ftruncate()` 将截短其长度，被截断部分的数据将丢失；
    - 当原有共享内存的长度小于 length 时：`ftruncate()` 将其长度加大，多出的部分将填充 NULL；
* 调用 `ftruncate()` 时，共享内存必须以读/写方式打开；
* `ftruncate()` 调用成功返回 0，失败返回 -1，errno 为错误代码；

------
## 4 将共享内存对象映射到进程地址空间
* 已经打开的共享内存对象必须将其地址映射到进程的地址空间后才可以使用，使用 `mmap()`
    ```C
    #include <sys/mman.h>

    void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    ```
* mmap() 在调用进程的地址空间中为一个已经打开的共享内存对象创建一个映射；
* **addr**：在进程地址空间的映射地址；
    - 当该参数为 NULL 时，系统将自动在进程地址空间上选择一个合适的映射地址，这种方式可以满足大多数的应用需求，是最常用、最便捷的映射方式；
    - 当该参数不为 NULL 时，请自行查看在线手册的说明：`man mmap`
* **fd**：已经打开的共享内存对象的文件描述符；
* **length** 和 **offset**：从 fd 指定的共享内存中，将偏移 offset，长度为 length 的内存块映射到进程地址空间上；
* **port**：映射所需的内存保护(不能与共享内存对象的打开模式冲突)，可以是以下标志位的组合：
    - PROT_EXEC：当前映射可执行；
    - PROT_READ：当前映射可读；
    - PROT_WRITE：当前映射可写；
    - PROT_NONE：当前映射不可存取；
* **flags**：该参数决定对共享内存中数据的修改是否对映射同一区域的其他进程可见，常用值只有两个：
    - MAP_SHARED：当前映射是进程间共享的，其含义为：当修改了映射内存中的内容时，映射了相同共享内存对象的其它进程中可以看到这些修改；
    - MAP_PRIVATE：当前映射是调用进程私有的，其含义为：对映射内存内容的修改仅当前进程可见，不会被其它映射了相同共享内存对象的进程看到；
    - 还有一些很不常用的，没有列出，可以在在线手册中查询：`man mmap`
* mmap() 并不是共享内存专有的一个系统调用，更多的时候这个调用是用来将一个文件映射到内存中，shm_open() 打开一个共享内存对象时返回的实际就是一个文件描述符，所以可以使用这个调用将一个共享内存对象映射到进程地址空间上；
* 下面代码段可以创建/打开一个共享内存对象并将其映射到进程地址空间上；
    ```C
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>

    ......

    int shmfd = shm_open("posix-shm-name", O_CREAT | O_RDWR, 0666);
    ftruncate(shmfd, 256);
    char *addr = mmap(NULL, 256, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    ......
    ```
* 当我们使用 `shm_open()` 建立一个新的共享内存对象，并使用 `ftruncate()` 设置其长度后，可以只映射共享内存的一部分到进程地址空间，在进程中当然也只能使用映射过来的这块共享内存；
    - mmap() 调用中的 offset 参数帮助我们实现共享内存的部分映射，要注意的是 offset 必须是内存页长度的整数倍；
    - 可以使用命令 `getconf PAGESIZE` 获取当前系统的内存页长度；
    - 在程序中可以使用 `getpagesize()` 函数获取系统的内存页长度；
* 共享内存在映射成功后，可以使用 `close()` 将其文件描述符关闭，并不会影响共享内存的使用；
* 映射的共享内存在使用完毕后需要使用 `munmap()` 取消其映射，使用 `close()` 关闭共享内存对象并不能取消映射；
    ```C
    #include <sys/mman.h>

    int munmap(void *addr, size_t length);
    ```
* **munmap()** 可以取消在指定地址范围内的映射，尽管关闭文件描述符不能取消映射，但实际上，当进程终止时，进程地址空间内的映射会自动取消，但是显式地取消映射是编程的一个良好习惯；
* **addr**：必须是一个页对齐的地址，也就是必须是 PAGESIZE 的整数倍，实际编程中填入 mmap() 返回的地址即可；
* **length**：munmap() 会取消在 addr -- (addr + length) 范围内的映射地址，对 length 没有要求，但是要保证在这个范围内有需要取消映射的地址；
* munmap() 调用成功时返回 0，失败时返回 -1，errno 中为错误代码，注意：当地址范围内没有可以取消的映射时，munmap() 不会返回 -1。

* **源程序**：[map-offset.c][src01](**点击文件名下载源程序**)演示了如何映射共享内存的一部分；
    - 该程序使用 `getpagesize()` 获取内存页的 PAGESIZE；
    - 建立一个长度达到两个 PAGESIZE 的共享内存对象；
    - 建立一个子进程，并在子进程中将共享内存中偏移为 PAGESIZE 的 256 个字节映射到进程地址空间；
    - 在子进程中将一个字符串写入共享内存；
    - 在父进程中将全部共享内存映射到进程地址空间；
    - 父进程等待子进程退出后，从共享内存偏移为 PAGESIZE 处显示子进程放入的字符串；
    - 为避免冲突，子进程以只写方式映射共享内存，父进程以只读方式映射共享内存
* 编译：`gcc -Wall map-offset.c -o map-offset -pthread -lrt`
* 运行：`./map-offset`
* 运行截图：

    ![Screenshot for running map-offset][img01]

---------
## 6 共享内存对象的关闭和删除
* 前面已经介绍过如何使用 close() 关闭共享内存对象的文件描述符，当一个共享内存对象完成映射后，关闭其文件描述符并不会影响共享内存的使用；
* 使用 `shm_unlink()` 可以删除一个共享内存对象；
    ```C
    #include <sys/mman.h>

    int shm_unlink(const char *name);
    ```
* `shm_unlink()` 执行成功时返回 0，失败时返回 -1，errno 中为错误编码；
* `shm_unlink()` 实际上最终是调用 `unlink()` 从虚拟文件系统 `/dev/shm/` 下把文件 `name` 删除；
* `shm_unlink()` 会立即从虚拟文件系统中删除共享内存对象的名字，但是共享内存并不会立即被销毁，已经映射了该共享内存的进程可以继续使用该共享内存，直至该共享内存的所有映射全部取消，并关闭所有打开的、与该共享内存关联的文件描述符后，共享内存才会被销毁；
* `shm_unlink()` 执行成功后，再使用 shm_open() 打开这个共享内存对象时将出现错误，但如果指定了 O_CREAT，则会执行成功，但这是一个新建的共享内存对象，与刚删除的那个不一样；
* 一个进程退出时，Linux 会回收该进程的资源，包括打开的文件描述符和映射，所以理论上说，程序中不用显式地取消映射和关闭文件描述符，当进程退出时会自动完成这些事，但显式地取消映射和关闭文件描述符是程序员良好编程习惯的具体体现；

* **源程序**：[shm-unlink.c][src02](**点击文件名下载源程序**)演示了在使用 `shm_unlink()` 删除了共享内存对象后，已经存在的映射仍然可以使用的情形；
    - 该程序在主进程中建立了一个共享内存对象和一个信号量；
    - 主进程建立了一个子进程，子进程中打开共享内存对象并进行了映射，映射完成后通过信号量通知主进程；
    - 主进程等在信号量上，一旦子进程完成映射便使用 ls 显示虚拟文件系统 `/dev/shm/` 目录，确认建立的共享内存对象存在；
    - 子进程在完成映射后便进入一个有限次数的循环，每次将一个字符串放入映射的共享内存中，再从共享内存中读出该字符串并显示出来；
    - 主进程使用 `shm_unlink()` 删除共享内存对象；
    - 主进程再次使用 ls 显示虚拟文件系统 `/dev/shm/` 目录，确认共享内存对象名称已被删除；
    - 此时仍然可以看到子进程显示到屏幕上的信息，说明子进程映射的共享内存仍然可以正常工作；
    - 子进程退出，主进程检测到子进程退出后退出自身。
* 编译：`gcc -Wall shm-unlink.c -o shm-unlink -pthread -lrt`
* 运行：`./shm-unlink`
* 运行截图：

    ![Screenshot of running shm-unlink][img02]

----------
## 7 实例
* 共享内存很多情况下都是和信号量配合使用的，因为共享内存的写操作是互斥的，否则会产生混乱；
* **源程序**：[pizzeria.c][src03](**点击文件名下载源程序**)模拟了一个披萨餐厅里厨师制作披萨和服务员端给顾客的过程；
* 模拟的情景描述如下：
    > 披萨餐厅中有若干位(本例中为 2 位)厨师，每位厨师面前都有一个披萨架子，厨师制作的披萨放到自己的披萨架上，每个架子可放的披萨数量有限(本例中是 5 个)，厨师只能将披萨放到自己的架子上，当架子放满后就不能再制作披萨，直到服务员拿走披萨；餐厅中有若干个服务员(本例中为 3 位)，她们负责将厨师制作好的披萨拿给顾客，每次只能拿一个披萨；
* 简要说明：
    - 程序为每个披萨架子在共享内存中建立一个整数变量，记录架子上的披萨数量；
    - 厨师制作出一个披萨，共享内存中的相应计数 + 1，当达到架子可放披萨的最大数量时，厨师停止制作披萨，等待服务员从架子上取走披萨；
    - 服务员按照一定的规则从披萨架上取走披萨，每取走一个披萨，共享内存中的相应计数 - 1，当达到 0 时，表示架子上没有披萨，可以从其它披萨架上取；
    - 共享内存中的披萨数量的计数属于临界区，改变计数值的代码属于临界代码，需要互斥，为此设置了一个信号量 sem_mutex，初始值为 1；
    - 建立一个计数信号量 sem_fill，初始值为 0，厨师每次放到架子上一个披萨，该信号量 +1，服务员每次从架子上取走一个披萨，该信号量 -1，该信号量表示目前所有架子上没有取走的披萨数量，该信号量为 0 时表示所有架子上都没有披萨；
* 编译：`gcc -Wall -g pizzeria.c -o pizzeria -pthread -lrt`
* 运行：`./pizzeria`
* 运行动图：

    ![GIF of running pizzeria][img03]

----------
## 8 System V 共享内存与 POSIX 共享内存的主要区别
* 以下内容译自 ChatGPT；

1. API 和接口：

    * System V: System V 共享内存使用一组函数，如：`shmget()`、`shmat()`、`shmdt()` 和 `shmctl()` 来创建、映射、取消映射和控制共享内存段；
    * POSIX: POSIX 共享内存使用一组不同的函数，包括：`shm_open()`、`shm_unlink()`、`mmap()` 和 `munmap()`，来创建、删除、映射和取消映射共享内存对象；

2. 标识和访问：

    * System V: System V 共享内存段由一个整数标识符 ID 来标识，该标识符通常使用 `shmget()` 系统调用获得，进程使用 ID 标识符把共享内存段映射到进程地址空间；
    * POSIX: POSIX 共享内存对象由字符串名称标识，类似于文件名，进程使用共享内存对象的名称调用 `shm_open()` 打开共享内存对象，获得其文件描述符，然后像把文件映射到内存一样将共享内存映射到进程地址空间；

3. 共享内存的持久性：

    * System V: System V 共享内存段是持久化的，必须使用 `shmctl()` 显式地删除(使用 IPC_RMID 命令)，否则它会一直存在于系统中(意即占用一块内存)；
    * POSIX: 默认情况下，POSIX 共享内存对象是临时性的，当所有进程取消映射并关闭该对象的文件描述符时，系统将自动删除该共享内存(意即不再占用内存)，只有在至少一个进程使用 `shm_open()` 打开共享内存对象时，才占用内存；

4. 权限与安全性:

    * System V: System V 共享内存使用类似于文件权限的权限模型，分别为所有者、组和其他用户指定读、写和执行权限；
    * POSIX: POSIX 共享内存也使用文件权限模型，允许为所有者、组和其他用户指定读、写和执行权限；

5. 可移植性:

    * System V：System V 共享内存不是 POSIX 标准的一部分，只能用于基于 System V 的类 unix 操作系统；
    * POSIX：POSIX 共享内存是标准化的，可以跨不同的、符合 POSIX 标准的类unix操作系统移植。


## **欢迎订阅 [『进程间通信专栏』](https://blog.csdn.net/whowin/category_12404164.html)**



-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[article01]: https://whowin.gitee.io/post/blog/ipc/0010-ipc-example-of-anonymous-pipe/
[article02]: https://whowin.gitee.io/post/blog/ipc/0011-ipc-examples-of-fifo/
[article03]: https://whowin.gitee.io/post/blog/ipc/0013-systemv-message-queue/
[article04]: https://whowin.gitee.io/post/blog/ipc/0014-posix-message-queue/
[article05]: https://whowin.gitee.io/post/blog/ipc/0015-systemv-semaphore-sets/
[article06]: https://whowin.gitee.io/post/blog/ipc/0016-posix-semaphores/
[article07]: https://whowin.gitee.io/post/blog/ipc/0017-systemv-shared-memory/
[article08]: https://whowin.gitee.io/post/blog/ipc/0018-posix-shared-memory/
[article09]: https://whowin.gitee.io/post/blog/ipc/0019-ipc-with-unix-domain-socket/
[article10]: https://whowin.gitee.io/post/blog/ipc/0020-ipc-using-files/
[article11]: https://whowin.gitee.io/post/blog/ipc/0021-ipc-using-dbus/
[article12]: https://whowin.gitee.io/post/blog/ipc/0022-dbus-asyn-process-signal/
[article13]: https://whowin.gitee.io/post/blog/ipc/0023-dbus-resolve-hostname/
[article14]: https://whowin.gitee.io/post/blog/ipc/0024-select-recv-message/
[article15]: https://whowin.gitee.io/post/blog/ipc/0025-resolve-arbitrary-dns-record/

<!-- for CSDN
[article01]: https://blog.csdn.net/whowin/article/details/132171311
[article02]: https://blog.csdn.net/whowin/article/details/132171930
[article03]: https://blog.csdn.net/whowin/article/details/132172172
[article04]: https://blog.csdn.net/whowin/article/details/134869490
[article05]: https://blog.csdn.net/whowin/article/details/134869636
[article06]: https://blog.csdn.net/whowin/article/details/134939609
[article07]: https://blog.csdn.net/whowin/article/details/135015196
[article08]: https://blog.csdn.net/whowin/article/details/135074991
-->

[src01]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190018/map-offset.c
[src02]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190018/shm-unlink.c
[src03]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190018/pizzeria.c


[img01]: https://whowin.gitee.io/images/190018/screenshot-of-map-offset.png
[img02]: https://whowin.gitee.io/images/190018/screenshot-of-shm-unlink.png
[img03]: https://whowin.gitee.io/images/190018/pizzeria-running.gif

<!-- CSDN
[img01]: https://img-blog.csdnimg.cn/img_convert/8fb23733955eafd0e2b42a4d0e0487ab.png
[img02]: https://img-blog.csdnimg.cn/img_convert/fbb35cc00f9e4405c1d41377a97c0adc.png
[img03]: https://img-blog.csdnimg.cn/img_convert/dab17d8e85a6c6dcbd41ea23919dba08.gif
-->
