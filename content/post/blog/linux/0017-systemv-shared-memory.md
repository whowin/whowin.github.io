---
title: "IPC之七：使用 System V 共享内存段进行进程间通信的实例"
date: 2023-09-12T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
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
# - [Linux Interprocess Communications](https://tldp.org/LDP/lpg/node7.html)

postid: 100017
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍共享内存(Shared Memory)，因为没有像管道、消息队列这样的中介介入，所以通常认为共享内存是迄今为止最快的 IPC 方式；Linux 既支持 UNIX SYSTEM V 的共享内存，也支持 POSIX 的共享内存，本文针对 System V 共享内存段，本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文的实例中涉及多进程编程和信号处理等，阅读本文还需要一些基本的内存管理知识，本文对 Linux 编程的初学者有一些难度。

<!--more-->

## 1 System V 共享内存基本概念
* 共享内存实际上是一个内存区域(段)的映射，一个进程创建一个共享内存段，然后被其它进程映射到自身的地址空间上，从而实现共享，各个进程实际读取的是同一个内存区域；
* 之所以说共享内存是迄今为止速度最快的 IPC 方式，是因为多个进程不管是使用 FIFO 还是消息队列传递数据，都需要经过内核的中转，而共享内存则不需要；
* 考虑如下过程：进程 A 从文件中读出数据，并将数据传递给进程 B，进程 B 再将文件内容写入到另一个文件；
* 当使用 FIFO 或者消息队列传递数据时，过程如下：
    - 进程 A 首先要将文件内容读出并存储到自身地址空间的内存中;
    - 进程 A 将数据发送到 FIFO 或者消息队列实际上是将自身地址空间的数据拷贝到内核空间的内存中；
    - 进程 B 从 FIFO 或者消息队列中接收数据实际上是从内核空间的内存中将数据拷贝到自身地址空间的内存中；
    - 进程 B 将自身地址空间内存中的数据写入文件。
* 当使用共享内存传递数据时，过程如下：
    - 进程 A 从文件中读出数据，并直接存储在共享内存中；
    - 进程 B 将共享内存中的数据写入到另一个文件中；
* 由此可见，共享内存在 IPC 上的优势；
* Linux 既支持 Sytem V 共享内存也支持 POSIX 共享内存，**本文针对 System V 共享内存**；
* 以下如无特别说明，共享内存均指 System V 共享内存；
* 使用共享内存有如下一些限制：
    - **SHMMNI**：整个系统中，共享内存段的最大数；
    - **SHMALL**：整个系统中，共享内存的总长度(字节数)；
    - **SHMMAX**：每个共享内存段的最大长度(字节数)；
    - **SHMMIN**：共享内存段的最小长度(字节数)；
* 可以使用命令行命令 `ipcs -m -l` 查看这些限制值；
    
    ![screenshot of 'ipcs -m -l'][img01]

* 也可以在 proc 文件系统中找到这些限制值

    ![screenshot of proc][img02]

* 还可以使用命令 `sysctl kernel.shm{mni,all,max}` 查看这些限制值；

    ![screenshot of sysctl][img03]

---------
## 2 创建/打开共享内存段
* 和 System V 消息队列和信号量集一样，共享内存段也是使用 key_t 和 ID 进行标识，请参考文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]中关于 key 和 ID 的介绍；
* key 一般使用 ftok() 生成；
* 在使用共享内存前，需要创建一个新的共享内存段，或者打开一个已经存在的共享内存段；
    ```C
    #include <sys/ipc.h>
    #include <sys/shm.h>

    int shmget(key_t key, size_t size, int shmflg);
    ```
* 执行成功，shmget() 返回一个与 key 关联的共享内存段的 ID(Identifier)，执行失败则返回 -1，errno 中为错误代码；
* **key**：参考文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]中关于 key 的介绍和生成方法；
* **size**：创建的共享内存段的大小，实际分配的内存大小会向上取整到 PAGE_SIZE 的整数倍；
    > PAGE_SIZE 是一个内存页的大小，在内核中定义，我们可以使用命令 getconf PAGESIZE 查看其具体值，通常为 4096；

* **shmflag**：标志位，常用的其实就是 IPC_CREAT 和 IPC_EXCL，还有一些其它的不常用值，就不做介绍了；
    - 当 **IPC_CREAT** 时，如果 key 对应的共享内存段存在，则返回其 ID，如果 key 对应的共享内存段不存在，则建立一个新的与 key 关联的共享内存段，并返回其 ID；
    - 当 **IPC_CREAT | IPC_EXCL** 时，如果 key 对应的共享内存段存在，则报错返回 -1，`errno = EEXIST(File exists)`；如果 key 对应的共享内存段不存在，则建立一个新的与 key 关联的共享内存段，并返回其 ID；
    - 当 **IPC_EXEL** 时，如果 key 对应的共享内存段存在，则返回其 ID(这点和 IPC_CREAT 一样)，如果 key 对应的共享内存段不存在，则返回 -1，`errno = ENOENT(No such file or directory)`；
    - 另外，shmflag 还可以加上所创建的共享内存段的读写权限，要用八进制表示，比如：0666；
    - shmflag 举例：`IPC_CREAT | IPC_EXEL | 0666`
* 当 `key = IPC_PRIVATE` 时，`shmget()` 将创建一个新的共享内存段并返回其 ID；
    + 这样生成的共享内存段只有 ID，没有 key(key 为 0)，所以其它进程并不能方便地使用这个共享内存段，通常只能在子进程之间使用；
    + 实际上，IPC_PRIVATE 的值是 0，所以我们自己生成的 key 不能是 0，否则相当于将 key 设置为 IPC_PRIVATE；

--------
## 3 映射共享内存段地址到进程地址空间
* 已经获取了 ID 的共享内存段，需要将其地址映射到当前进程的地址空间上才能正常使用，这个过程使用 `shmat()` 实现；
    ```C
    #include <sys/types.h>
    #include <sys/shm.h>

    void *shmat(int shmid, const void *shmaddr, int shmflg);
    ```
* 调用成功，shmat() 返回共享内存段在当前进程的地址空间的映射地址，失败则返回 `(void *) -1`，errno 中为错误代码；
* **shmid**：使用 shmget() 获得的共享内存段的 ID;
* **shmaddr**：映射共享内存段的用户进程地址空间的地址；
    - 如果 `shmaddr` 为 NULL，系统会选择一个未使用的、页对齐的地址作为共享内存段的映射地址；
    - 如果 `shmaddr` 不为 NULL，并且 `shmflg` 中指定了 `SHM_RND`，系统会把共享内存段映射到 `shmaddr` 地址向下的最近的内存页的整数倍的地址上；
    - 其它 `shmaddr` 不为 NULL 的情况，`shmaddr` 必须是一个页对齐的地址；
    - 大多数情况下，**shmaddr** 填 NULL 即可，由系统为我们选择的映射地址通常是安全、可靠的；
* **shmflag**：这个标志的值除了前面提到的 SHM_RND 外，还可以有下列几个：
    - SHM_EXEC：如果进程对共享内存段有执行权限，允许执行共享内存段中的代码；
    - SHM_RDONLY：以只读方式映射共享内存段，不指定该标志，映射的共享内存段是有读写权限的，没有只写权限的共享内存段的概念；
    - SHM_REMAP：重新映射共享内存段到 `shmaddr` 指定的地址上，此时，`shmaddr` 不能为空，`shmaddr` 向上至共享内存段长度之间如果已有其它映射，将导致一个 EINVAL 错误；
    - 大多数情况下，我们并不需要设置 `shmflag`，将其设为 0 即可；
* 实际上 `shmat()` 最常用的方式就是：`void *ptr = shmat(shmid, NULL, 0);`；
* 共享内存在使用完后要释放这个映射地址，否则，这块地址是没有办法再次使用的，释放映射地址使用 `shmdt()`；
    ```C
    #include <sys/types.h>
    #include <sys/shm.h>

    int shmdt(const void *shmaddr);
    ```
* 调用成功返回 0，调用失败返回 -1，errno 为错误代码；
* **shmaddr** 为共享内存段的映射地址。

-----------
## 4 共享内存的控制操作
* 对共享内存的控制操作使用 `shmctl()`
    ```C
    struct ipc_perm {
        key_t          __key;       /* Key supplied to shmget(2) */
        uid_t          uid;         /* Effective UID of owner */
        gid_t          gid;         /* Effective GID of owner */
        uid_t          cuid;        /* Effective UID of creator */
        gid_t          cgid;        /* Effective GID of creator */
        unsigned short mode;        /* Permissions + SHM_DEST and SHM_LOCKED flags */
        unsigned short __seq;       /* Sequence number */
    };

    struct shmid_ds {
        struct ipc_perm shm_perm;    /* Ownership and permissions */
        size_t          shm_segsz;   /* Size of segment (bytes) */
        time_t          shm_atime;   /* Last attach time */
        time_t          shm_dtime;   /* Last detach time */
        time_t          shm_ctime;   /* Last change time */
        pid_t           shm_cpid;    /* PID of creator */
        pid_t           shm_lpid;    /* PID of last shmat(2)/shmdt(2) */
        shmatt_t        shm_nattch;  /* No. of current attaches */
        ...
    };

    #include <sys/ipc.h>
    #include <sys/shm.h>

    int shmctl(int shmid, int cmd, struct shmid_ds *buf);
    ```
* 参考文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]，其调用方法非常接近；
* `shmctl()` 在 **shmid** 所指定的共享内存段上执行一个由 cmd 指定的操作，包括：获取/设置共享内存段的属性，删除共享内存段等；
* Linux 下允许的 cmd 值为：
    - **IPC_STAT**：获取共享内存段的属性，此时，buf 指向一个 `struct shmid_ds`，共享内存段的属性将从内核空间拷贝到 buf 中；
    - **IPC_SET**：设置共享内存段的属性，实践中主要用来设置共享内存段的读写权限，已知的可以设置的字段有：`shm_perm.uid`、`shm_perm.gid` 以及 `shm_perm.mode`(低9位)；
        > 具体编程实践中，往往是先使用 **IPC_STAT** 命令获取共享内存段的属性，然后修改需要设置的值，再使用 **IPC_SET** 命令设置属性；

    - **IPC_RMID**：删除一个共享内存段，实际上，执行完这个命令后，共享内存段不一定立即被销毁，只有当所有映射了共享内存段地址的进程全部释放了映射地址后，这个共享内存段才会被真正销毁掉；
        > 如果已经执行了 IPC_RMID 命令，但共享内存段还没有被销毁，这时使用 IPC_STAT 获取属性时，会看到 shm_perm.mode 字段上被写入了 SHM_DEST 标记；

        > 必须要保证调用 IPC_RMID 后共享内存最终会被销毁，否则其残留将一直驻留在内存或交换区中。

    - **SHM_INFO**：获取有关共享内存所消耗的系统资源的信息，这些信息将返回到一个 `struct shm_info` 中，此时，buf 要指向一个 `struct shm_info`；
        ```C
        struct shm_info {
            int           used_ids;         /* # of currently existing segments */
            unsigned long shm_tot;          /* Total number of shared memory pages */
            unsigned long shm_rss;          /* # of resident shared memory pages */
            unsigned long shm_swp;          /* # of swapped shared memory pages */
            unsigned long swap_attempts;    /* Unused since Linux 2.4 */
            unsigned long swap_successes;   /* Unused since Linux 2.4 */
        };
        ```
        > 获得的信息包括：当前已存在的共享内存段的数量、共享内存页的总数量、驻留在物理内存的共享内存页的数量以及在交换区的共享内存页的数量；对一般的应用程序而言，这些值基本没有什么用处。

    - **SHM_STAT** 和 **SHM_STAT_ANY** 基本用不上，这里就不介绍了；
    - **SHM_LOCK**：程序可以使用这个命令"锁定"共享内存段，使其不会被放在交换区(让这个共享内存段一直驻留在物理内存中)，被"锁定"的共享内存段，当使用 **IPC_STAT** 获取其属性时，会在 `shm_perm.mode` 字段上看到 **SHM_LOCKED** 标志；

* **源程序**：[shm-ctl.c][src01](**点击文件名下载源程序**)演示了如何使用 shmctl() 对共享内存段进行操作；
* 编译：`gcc -Wall shm-ctl.c -o shm-ctl`
* 运行：`./shm-ctl`
    1. 该程序首先建立了一个共享内存段，并返回其 ID；
    2. 使用 `IPC_STAT` 获取了共享内存段的属性，并显示了其中的读/写权限，可以看到和建立共享内存段时设定的权限一样；
    3. 将共享内存段的读/写权限的最低 3 位清 0，然后使用 `IPC_SET` 重新设置其读/写权限；
    4. 使用 `IPC_STAT` 获取了共享内存段的属性，并显示了其中的读/写权限，可以看到和修改过的权限一样；
    5. 显示属性中的 **LOCKED** 标志，可以看到没有 LOCKED 标志；
    6. 使用 `SHM_INFO` 读取共享内存段的信息，并显示出来；
    7. 使用 `SHM_LOCK` '锁定'共享内存段；
    8. 使用 `IPC_STAT` 获取了共享内存段的属性，并显示 LOCKED 标志，可以看到 LOCKED 标志已经被设置；
    9. 使用 `SHM_UNLOCK` '解锁'共享内存段；
    10. 销毁共享内存段。
* 运行截图：

    ![Screenshot of running shm-ctl][img04]

------
## 5 实例
* 这个实例是一个服务端，若干个客户端使用共享内存段的例子
* 服务端源程序：[shm-server.c][src02](**点击文件名下载源程序**)
* 客户端源程序：[shm-client.c][src03](**点击文件名下载源程序**)
* 包含文件：[shm-public.h][src04](**点击文件名下载源程序**)
* 基本过程：
    > 客户端程序 fork 出若干个(本例中为 5 个，由宏 MAX_PROCESSES 控制子进程数量)子进程，每个子进程运行相同的程序，子进程会随机从字符串数组中选择一个字符串，将其依次放到共享内存中，当共享内存满时则停止放入；

    > 服务端程序从共享内存中依次取出客户端放入的字符串(先进先出原则)，并将其打印到屏幕上，当共享内存空时则停止取出字符串；

* 服务端程序说明：
    1. 建立共享内存段，并将其映射到服务端进程的地址空间，共享内存是由一个结构体(`struct shared_memory`)组成；
        - 结构体中有一个可以存储若干(本例中为 10，由宏 `MAX_BUFFERS` 决定)个字符串的缓冲区(`buf[MAX_BUFFERS][MAX_STR_LEN]`)；
        - 结构体中有一个字符串索引号(`string_index`)，用于向缓冲区(`buf`)中放入字符串，初始值为 0，放入一个字符串后 +1，达到 `MAX_BUFFERS` 时归 0；
        - 结构体中有一个打印索引号(`print_index`)，用于从缓冲区(`buf`)中取出字符串并打印到屏幕上，初始值为 0，取出一个字符串后 +1，达到 `MAX_BUFFERS` 时归 0；
    2. 建立信号量集，里面包含三个信号量：
        - SEM_MUTEX：这是一个互斥信号量，用于访问临界区(对共享内存进行存取的代码)的互斥，初始值为 1；
        - SEM_BUF_FULL：这是一个计数信号量，其初始值为 `MAX_BUFFERS`，客户端程序放入一个字符串后 `SEM_BUF_FULL-1`，服务端程序取出一个字符串后 `SEM_BUF_FULL+1`，为 0 时，表示共享内存已满，无法再向共享内存中放入字符串；
        - SEM_BUF_EMPTY：这是一个计数信号量，其初始值为 0，服务端程序取出一个字符串后 `SEM_BUF_EMPTY-1`，客户端程序放入一个字符串后 `SEM_BUF_EMPTY+1`，为 0 时，表示共享内存空，无法再从共享内存中取出字符串；
    3. 对计数信号量 SEM_BUF_EMPTY 做 P 操作，即：`SEM_BUF_EMPTY-1`，如果获取到信号量(`SEM_BUF_EMPTY>0`)，表示共享内存中有没有处理的字符串；
    4. 使用打印索引号(`print_index`)从共享内存中读出一个字符串(`buf[print_index]`)并显示在屏幕上，打印索引号(`print_index`) +1；
    5. 对计数信号量 SEM_BUF_FULL 做 V 操作，即：`SEM_BUF_FULL+1`；
    6. 循环 step 3
    > 由于只有一个服务端程序运行，服务端程序仅对打印索引号(`print_index`)进行了写操作，而客户端程序无需对打印索引号(`print_index`)进行操作，所以这里没有请求互斥信号量(`SEM_MUTEX`)，对本例而言没有风险，如果有多个服务端，应在 `step 4` 前请求互斥信号量(SEM_MUTEX)，在 `step 5` 后释放互斥信号量(SEM_MUTEX)；如果增加了互斥信号量的请求和释放，则服务端程序在从共享内存中取出字符串的过程中，客户端程序是无法将新字符串放入共享内存的；本例中的做法可以使服务端程序取出字符串时，客户端程序同时可以将新字符串放入共享内存，运行效率略高一些；因此，合理地运用互斥信号量可以有效地提高整体的运行效率。

* 客户端程序说明：
    1. 获取服务端建立的共享内存段的 ID，并映射到客户端进程的地址空间；
    2. 获取服务端建立的信号量集的 ID；
    3. 对计数信号量 SEM_BUF_FULL 进行 P 操作，即： `SEM_BUF_FULL-1`，如果获取到信号量(`SEM_BUF_FULL>0`)，表示共享内存中有空闲空间放入新字符串，并且当前进程已经抢占了一个位置；
    4. 对互斥信号量 SEM_MUTEX 进行 P 操作，获取进入临界区的许可，因为下面要改变字符串索引号(`string_index`)，有多个客户端进程都要做这个操作，必须保证同时只有一个进程在做这个操作；
    5. 向字符串索引号(`string_index`)所在的共享内存位置(`buf[string_index]`)放入随机字符串；
    6. 字符串索引号(`string_index`) +1；
    7. 对互斥信号量 SEM_MUTEX 执行 V 操作，释放互斥信号量 SEM_MUTEX；
    8. 对计数信号量 SEM_BUF_EMPTY 执行 V 操作，即：`SEM_BUF_EMPTY+1`；
    9. 循环至 `step 3`。

* 服务端和客户端程序均只能用 `CTRL + C` 退出，程序中截获了 `CTRL + C` 的信号；
* 运行时要打开两个终端窗口，一个终端运行 `.\shm-server`，另一个终端运行 `./shm-client`；
* 一定要先运行 `shm-server`，再运行 `shm-client`，因为共享内存段和信号量集都是在 `shm-server` 中建立的，单独运行 `shm-client` 无法运行成功；

* 运行动图：

    ![GIF for running shm-server and shm-client][img05]

-----------
## 6 操作共享内存段的命令行命令
* `ipcs -m -l` - 显示共享内存段的限制值；
* `ipcs -m` - 显示现有共享内存端的 key、ID 等部分属性；
* `ipcs -m -i <ID>` - 显示指定 ID 的共享内存段的属性(比 `ipcs -s` 显示的属性要多些)；

* `ipcrm -M <key>` - 删除指定 key 的共享内存段；
* `ipcrm -m <ID>` - 删除指定 ID 的共享内存段；
* `ipcrm --all=shm` - 删除所有的共享内存段；

* `ipcmk -M <shm size>` - 创建一个新的大小为 `<shm size>` 共享内存段，其读写权限为默认的 0644；
* `ipcmk -M <shm size> -p <perm>` - 创建一个新的大小为 `<shm size>` 共享内存段，其读写权限为指定的 `<perm>`。


## **欢迎订阅 [『进程间通信专栏』](https://blog.csdn.net/whowin/category_12404164.html)**


-------------
**欢迎访问我的博客：https://whowin.cn**

**email: hengch@163.com**

![donation][img_sponsor_qrcode]

[img_sponsor_qrcode]:https://whowin.gitee.io/images/qrcode/sponsor-qrcode.png


[article01]: https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
[article02]: https://whowin.gitee.io/post/blog/linux/0011-ipc-examples-of-fifo/
[article03]: https://whowin.gitee.io/post/blog/linux/0013-systemv-message-queue/
[article04]: https://whowin.gitee.io/post/blog/linux/0014-posix-message-queue/
[article05]: https://whowin.gitee.io/post/blog/linux/0015-systemv-semaphore-sets/
[article06]: https://whowin.gitee.io/post/blog/linux/0016-posix-semaphores/
[article07]: https://whowin.gitee.io/post/blog/linux/0017-systemv-shared-memory/
[article08]: https://whowin.gitee.io/post/blog/linux/0018-posix-shared-memory/
[article09]: https://whowin.gitee.io/post/blog/linux/0019-ipc-with-unix-domain-socket/
[article10]: https://whowin.gitee.io/post/blog/linux/0020-ipc-using-files/
[article11]: https://whowin.gitee.io/post/blog/linux/0021-ipc-using-dbus/
[article12]: https://whowin.gitee.io/post/blog/linux/0022-dbus-asyn-process-signal/

<!-- for CSDN
[article01]: https://blog.csdn.net/whowin/article/details/132171311
[article02]: https://blog.csdn.net/whowin/article/details/132171930
[article03]: https://blog.csdn.net/whowin/article/details/132172172
[article04]: https://blog.csdn.net/whowin/article/details/134869490
[article05]: https://blog.csdn.net/whowin/article/details/134869636
[article06]: https://blog.csdn.net/whowin/article/details/134939609
-->

[src01]: https://whowin.gitee.io/sourcecodes/100017/shm-ctl.c
[src02]: https://whowin.gitee.io/sourcecodes/100017/shm-server.c
[src03]: https://whowin.gitee.io/sourcecodes/100017/shm-client.c
[src04]: https://whowin.gitee.io/sourcecodes/100017/shm-public.h

[img01]: https://whowin.gitee.io/images/100017/screen-of-ipcs-m-l.png
[img02]: https://whowin.gitee.io/images/100017/screenshot-max-proc.png
[img03]: https://whowin.gitee.io/images/100017/screenshot-of-sysctl.png
[img04]: https://whowin.gitee.io/images/100017/screenshot-of-shm-ctl.png
[img05]: https://whowin.gitee.io/images/100017/shm-server-client.gif
