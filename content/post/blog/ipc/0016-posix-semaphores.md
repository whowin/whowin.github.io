---
title: "IPC之六：使用 POSIX 信号量解决经典的'生产者-消费者问题'"
date: 2023-09-06T16:43:29+08:00
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
  - 信号量
  - semaphore
  - POSIX
draft: false
#references: 
# - [Synchronizing Threads with POSIX Semaphores](http://www.csc.villanova.edu/~mdamian/threads/posixsem.html)
# - [How to use POSIX semaphores in C language](https://www.geeksforgeeks.org/use-posix-semaphores-c/)
# - [overview of POSIX semaphores](https://linux.die.net/man/7/sem_overview)
#       - man手册
# - [Synchronization primitives in the Linux kernel](https://0xax.gitbooks.io/linux-insides/content/SyncPrim/linux-sync-3.html)
# - [3.8. Semaphores](https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/IPCSems.html)
# - [生产者消费者问题](https://baike.baidu.com/item/%E7%94%9F%E4%BA%A7%E8%80%85%E6%B6%88%E8%B4%B9%E8%80%85%E9%97%AE%E9%A2%98/8412057)
# - [producer-consumer-problem-solution-in-c](https://github.com/codophobia/producer-consumer-problem-solution-in-c)
# - [Producer–consumer problem](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem)
# - [生产者消费者问题](https://zh.wikipedia.org/wiki/%E7%94%9F%E4%BA%A7%E8%80%85%E6%B6%88%E8%B4%B9%E8%80%85%E9%97%AE%E9%A2%98)

postid: 190016
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍信号量(Semaphores)，尽管信号量被认为是 IPC 的一种方式，但实际上通常把信号量用于进程间同步或者资源互斥，和共享内存(Shared Memory)配合使用，可以实现完美的进程间通信；Linux 既支持 UNIX SYSTEM V 的信号量集，也支持 POSIX 的信号量，本文针对 POSIX 信号量，本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文的实例中涉及多线程编程和信号处理等，对 Linux 编程的初学者有一些难度。

<!--more-->

## 1 POSIX 信号量概述
* 信号量与其他形式的 IPC 不同，它并不能完成一般意义上的的数据交换，也就是说，不能使用信号量将信息从一个进程传输到另一个进程；
* 信号量通常用于进程间同步或对共享资源的访问同步，也就是说，信号量可以控制可能发生冲突的共享访问的时机；
* POSIX 信号量与 SYSTEM V 信号量集不同，在内核中不是以数组的形式进行管理，所以也就不再称其为 **集**；
* 在阅读本文前可以参考文章[《IPC之五：使用 System V 信号量集解决经典的'哲学家就餐问题'》][article05]，关于信号量的概念都是一致的；
* 也可以参考文章[《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》][article04]，POSIX 标准下的很多定义都是类似的；
* 以下如无特别说明，信号量均指 POSIX 信号量；
* 使用 `man sem_overview` 可以查阅 POSIX 信号量的在线手册；
* POSIX 信号量分为**有名信号量**和**无名信号量**(也称匿名信号量)；
* 在 Linux 内核 2.6 之前，Linux 只支持 POSIX 的无名信号量，内核 2.6 以后开始全面支持 POSIX 信号量；
* 编译时需要加上 `-lpthread` 选项，以便链接到实时库 `libpthread`；
* POSIX 信号量提供了一组用于操作信号量的 API，这些调用均是以 `sem_` 开头：
    1. 打开/创建一个有名信号量
        ```C
        sem_t *sem_open(const char *name, int oflag);
        sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
        ```
    2. 获取信号量；信号量的值减 1，如果信号量当前值为 0，则阻塞；
        ```C
        int sem_wait(sem_t *sem);
        ```
    3. 释放信号量；也就是信号量的值加 1；
        ```C
        int sem_post(sem_t *sem);
        ```
    4. 关闭有名信号量
        ```C
        int sem_close(sem_t *sem);
        ```
    5. 删除一个有名信号量
        ```C
        int sem_unlink(const char *name);
        ```
    6. 信号量的值减 1，如果当前信号量的值为0，则返回一个错误；
        ```C
        int sem_trywait(sem_t *sem);
        ```
    7. 信号量的值减 1，并设置一个阻塞超时时间，阻塞超时则返回；
        ```C
        int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
        ```
    8. 创建并初始化一个无名信号量
        ```C
        int sem_init(sem_t *sem, int pshared, unsigned int value);
        ```
    9. 删除一个无名信号量
        ```C
        int sem_destroy(sem_t *sem);
        ```

## 2 有名信号量的名称和虚拟文件系统
* SYSTEM V 的信号量集是用 key 或者 ID 来标识的，是没有名称的，POSIX 的有名信号量是有名称的，使用一个唯一的信号量名称来标识一个信号量；
* 和 POSIX 的消息队列(参考文章[《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》][article04])一样，有名信号量的名称是一个以 "/" 开头的字符串，而且名称中不允许再出现第二个 "/"，这个名称字符串以 NULL 结尾；
* POSIX 有名信号量的名称最长为 `NAME_MAX - 4`，宏定义 NAME_MAX 定义在头文件 `<linux/limits.h` 中，为 255；
* 任何一个知道有名信号量名称且具有适当权限的进程都可以对该信号量进行操作；
* 和 POSIX 的消息队列一样，POSIX 的有名信号量也是建在一个虚拟文件系统下的，是和 POSIX 的共享内存共用一个虚拟文件系统，被挂载在 `/dev/shm/` 下，在 Ubuntu 下，默认是自动挂载的；
* 因为有名信号量和共享内存的虚拟文件系统被挂载在同一个目录下，为了以示区别，所有的有名信号量的名称都带有一个前缀 `sem.`，这就是为什么有名信号量的名称长度不能超过 `NAME_MAX - 4` 的原因；
* 可以使用 `ls /dev/shm/sem.*` 命令列出已有的有名信号量，也可以用 `rm /dev/shm/<sem_name>` 命令删除一个已有的有名信号量；
* 虚拟文件系统下的信号量文件是二进制存储的，无法使用 `cat` 查看，可以使用 `hexdump /dev/shm/<sem_name>` 查看信号量的值；
* 尽管 POSIX 信号量的在线手册(`man sem_overview`)中明确信号量的名称必须以 "/" 开头，但在具体编程实践中发现在调用 `sem_open` 创建一个新的或打开一个已经存在的信号量时，信号量名称有没有 "/" 似乎并没有什么区别，即：下面两个调用没有区别；
    ```C
    sem_open("/semname-testing", O_CREAT, 0666, 1);

    sem_open("semname-testing", O_CREAT, 0666, 1);
    ```
* 但是，编程实践表明 POSIX 消息队列的名称是必须以 "/" 开头的；

* 如果在你的系统上没有这个虚拟文件系统，可以用下面的命令建立一个：
    ```bash
    # mkdir /dev/shm
    # mount -t shm none /dev/shm
    ```

## 3 有名信号量的创建/打开、关闭和删除
* 在使用一个有名信号量之前需要打开(或创建)该信号量，使用 sem_open()
    ```C
    #include <fcntl.h>           /* For O_* constants */
    #include <sys/stat.h>        /* For mode constants */
    #include <semaphore.h>

    sem_t *sem_open(const char *name, int oflag);
    sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
    ```
* `sem_open()` 创建一个新的有名信号量，或者打开一个已经存在的有名信号量，调用成功则返回信号量的**地址**，调用失败返回 `SEM_FAILED`，且 errno 中为错误代码；
    > 这里和 POSIX 的消息队列有所不同，在创建/打开消息队列时，返回的是描述符(descriptor)，但是创建/打开有名信号量时，返回是信号量的地址(address)，二者是有区别的，消息队列由于是使用了描述符，有点类似于 socket 的描述符，是可以使用 select 来进行处理的；作为信号量而言，其本质就是一个变量，所以返回一个地址也在常理之中；

    > 另外，POSIX 的函数通常在发生错误时返回 -1，这个函数不同，发生错误时返回 `SEM_FAILED`，这个宏定义在头文件 `<semaphore.h` 中，其值为 0；
* **name** - 信号量的名称，一个以 "/" 开头的字符串(实践表明可以也没有这个 "/")，是这个信号量的唯一标识；
* **oflag** - 控制操作的一些标志，可以有以下选项，这些选项可以以 or 的方式进行组合：
    - **O_CREAT** - 如果信号量不存在，则建立一个新的有名信号量，返回其地址；如果信号量存在，则返回该信号量的地址；
    - **O_EXCL** - 如果同时设置了 O_CREAT，则当打开的信号量已经存在时，返回调用失败，errno=EEXIST
* 如果在 oflag 中设置了 O_CREAT，则必须增加另外两个参数：`mode` 和 `value`
* 关于 oflag 中的 O_EXCL：
    - 当 `O_CREAT | O_EXCL` 时：信号量不存在则创建一个新的信号量(与 O_CREAT 一样)，返回其地址；如果信号量存在则返回 `SEM_FAILED`，`errno=EEXIST(File exists)`
    - 当仅有 `O_EXCL` 时：如果信号量已经存在，则返回其地址(与 O_CREAT 一样)，如果信号量不存在，则返回  `SEM_FAILED`，`errno=ENOENT(No such file or directory)`
* **mode** - 新创建的信号量的读/写权限，与文件的读/写权限的表达方式一致，使用八进制数，比如：0660、0666，实际的读/写权限还要受到 umask 的影响，最终的结果是 `mode & ~umask`；
    > 可以使用 shell 命令 `umask` 查看当前的 `umask`
* **value** - 指定该信号量的初始值；
* 如果设置了 `O_CREAT`，但是信号量已经存在，则 `mode` 和 `value` 无效；

* 有名信号量使用完毕后，需要关闭该信号量；
    ```C
    #include <semaphore.h>

    int sem_close(sem_t *sem);
    ```
    - **sem** 就是使用 sem_open() 打开信号量时返回的信号量地址；
    - 调用成功时返回 0，失败时返回 -1，只有一种失败的可能就是 sem 不是一个合法的信号量地址；
    - 进程在终止时，已经打开的有名信号量会自动关闭。
* 源程序：[sem-create.c][src01](**点击文件名下载源程序**)是一个简单的命令行工具，用于创建一个新的有名信号量；
* 编译：`gcc -Wall sem-create.c -o sem-create -lpthread`
* 运行：`./sem-create -cx -v 1 /test_sem_name 0666`
* 在使用该程序时需要在命令行输入一些参数：
    - **-c** 表示使用 O_CREATE 标志，**-x** 表示使用 O_EXCL 标志；
    - **-v** 表示该信号量的初始值，如果没有设置，初始值为 0；
    - 这几个选项中，**-c** 和 **-x** 至少要有一个，也可以两个都有，写成 **-cx** 或者 **-c -x** 都可以；
    - 在这几个选项后面是信号量的名称，注意，信号量的名称必须以 `"/"` 开头，而且后面的字符串中不能再有 `"/"`；
    - 在名称后面可以跟一个消息队列的读写权限，也可以没有，如果要设置消息队列的权限，这里要使用八进制，比如 `0666`、`0644` 等；
    - 如果没有设置权限，默认权限为 `0660`；
* 下面是这个程序的运行截屏：

    ![screenshot of sem-create command][img01]

* 请注意，尽管我们设置了信号量的读/写权限是 `0666`，但最后显示的信号量的权限却是 `0664`，这是因为 umask 为 `0002`。

* 有名信号量的删除
    ```C
    #include <semaphore.h>

    int sem_unlink(const char *name);
    ```
    - **name** 为有名信号量的名称，使用该调用的进程需要有该信号量的写权限；
    - 该调用成功时返回 0，失败时返回 -1，errno 中为错误代码。

----
## 4 信号量的操作
* 对信号量的操作只有两种：
    - 获取信号量，也就是对信号量值减 1，也称为 P 操作；
    - 释放信号量，也就是对信号量值加 1，也称为 V 操作；
* 获取信号量 - sem_wait()
    ```C
    #include <semaphore.h>

    int sem_wait(sem_t *sem);
    int sem_trywait(sem_t *sem);
    int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
    ```
* **sem** 为使用 `sem_open()` 返回的信号量的地址；
* `sem_wait()` 将信号量值减 1，如果当前信号量值为 0，则产生阻塞，直到该信号量值大于 0 时再执行操作并返回；
* `sem_trywait()` 与 `sem_wait()` 一样将信号量值减 1，如果当前信号量值为 0，不会阻塞，而是立即返回 -1，`errno=EAGAIN`，否则返回 0；
* `sem_timedwait()` 在 `sem_wait()` 的基础上增加了一个超时时间 `abs_timeout`，当产生阻塞时，到达超时时间则返回 -1，`errno=ETIMEDOUT`，如果不会产生阻塞，则信号量值减 1 然后返回 0；
    - 特别要注意的时，`abs_timeout` 并不是一个时长，而是一个绝对时间，其值表示从 `1970-01-01 00:00:00+0000 (UTC)` 开始，到函数返回的秒(tv_sec)和纳秒(tv_nsec)的数值；
    - 请参考文章[《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》][article04]中关于 `mq_timedsend()` 和 `mq_timedreceive()` 的说明；

* 释放信号量 - sem_post()
    ```C
    #include <semaphore.h>

    int sem_post(sem_t *sem);
    ```
* **sem** 为使用 `sem_open()` 返回的信号量的地址；
* 对信号量值加 1 是无需更多条件的，该调用只有两种可能会失败，一是 sem 不是一个合法的信号量地址，二是信号量值已经达到其允许的最大值；
* 该调用成功时返回 0，失败时返回 -1，errno中为错误代码；

## 5 无名信号量
* POSIX 无名信号量没有名称，而且要求必须存在于共享内存中，这是其与有名信号量的一个重要区别；
* 对于多线程的程序，因为一个进程下的多个线程共享相同的内存空间，所以可以使用一个全局变量作为无名信号量；
* 但是对于多进程的程序，就必须将信号量放在共享内存中，可以使用 shmget() 创建 SYSTEM V 共享内存段，也可以使用 shm_open() 创建 POSIX 共享内存对象；
* 显然，无名信号量对共享内存的要求给多进程程序使用无名信号量带来了一些麻烦，所以，在实际编程实践中，无名信号量多用于多线程程序；
* 特别要注意的是，父子进程间有不同的地址空间，在父进程中使用全局变量建立的无名信号量虽然会被子进程继承，但是它们并不是同一个信号量，所以父子进程间也是不能简单地使用全局变量作为无名信号量的；
* 在使用一个无名信号量之前要首先初始化：
    ```C
    #include <semaphore.h>

    int sem_init(sem_t *sem, int pshared, unsigned int value);
    ```
    - **sem**：无名信号量的地址；
    - **pshared**：指示这个信号量是在线程之间共享，还是在进程之间共享；
        > pshared = 0，表示信号量在线程之间共享，该无名信号量应该位于对所有线程可见的地址上(例如，全局变量)；

        > pshared != 0，表示信号量在进程之间共享，该无名信号量应该位于共享内存中，任何可以访问该共享内存区域的进程都可以使用 sem_post() 和 sem_wait() 等方法对信号量进行操作；

    - **value**：无名信号量的初始值；
    - 调用成功返回 0，失败返回 -1，errno 为错误编码；
    - 特别要注意的是，初始化一个已经初始化的无名信号量可能会发生不可预知的后果。

* 无名信号量的删除：
    ```C
    #include <semaphore.h>

    int sem_destroy(sem_t *sem);
    ```
    - `sem_destroy()` 会销毁一个由 sem 指向的无名信号量；
    - 在无名信号量所占用的共享内存被释放之前，一定要使用 `sem_destroy()` 销毁该信号量，否则有可能发生资源泄露；
    - 只有使用 sem_init() 进行过初始化的无名信号量才能被 `sem_destroy()` 销毁；
    - 如果仍然有其它进程在使用一个无名信号量，销毁该信号量将产生不可预知的后果；
    - 使用一个已经销毁的无名信号量可能产生不可预知的结果；

* 源程序：[official-exam.c][src02](**点击文件名下载源程序**)演示了初始化一个无名信号量和使用 sem_timedwait() 操作无名信号量的方法；
* 这个程序是官方的例子，可以在 `man sem_timedwai` 中找到，本文未做任何删改；
* 编译：`gcc -Wall official-exam.c -o official-exam -lpthread`
* 运行：`./official-exam 5 8`
* 需要对该程序做一个简要的说明：
    - 该程序首先初始化了无名信号量，信号量的初始值为 0；
    - 然后截获了信号 `SIGALRM`，这样在后面使用 alarm() 定时器后，就可以在信号处理程序中处理这个定时器产生的 `SIGALRM` 信号；
    - 在 `SIGALRM` 信号处理程序中，执行了 `sem_post()` 将无名信号量加 1；
    - 该程序在运行时需要传递两个参数，
        + 第一个 argv[1] 为定时器的时长，在这个时长后将产生 `SIGALRM` 信号；
        + 第二个 argv[2] 为 `sem_timedwait()` 的超时时长，超过这个时长后，`sem_timedwait()` 将因为超时而返回一个错误；
    - 当 `argv[1] > argv[2]` 时，在产生 `SIGALRM` 信号之前，`sem_timedwait()` 就超时，程序会收到 `sem_timedwait()` 返回的错误；
    - 当 `argv[1] < argv[2]` 时，先产生 `SIGALRM` 信号，由于信号处理程序中将信号量加 1，所以 `sem_timedwait()` 会成功返回，不会超时，当然也不会返回错误；
    - 当 `argv[1] == argv[2]` 时，`SIGALRM` 信号和 `sem_timedwait()` 超时同时发生，尽管信号处理程序中成功地执行了 `sem_post()`，但 `sem_timedwait()` 已经超时返回，不会获得这个信号量；
    - 所以，在运行这个程序时，`./official-exam 5 8`、`./official-exam 8 5` 和 `./official-exam 5 5` 会使三种不同的运行结果；

* 下面是这个程序的运行截屏：

    ![screenshot of official-exam][img02]

-----------
## 6 实例
* 在下面这个实例中，我们使用 POSIX 信号量解决一个经典的问题 - 生产者-消费者问题(Producer–consumer problem)；
* 如果希望更多地了解这个问题，可以百度一下，这里仅做简要说明：
    > 有一个固定长度为 BUF_SIZE 的共享内存块，生产者(Producer)和消费者(Consumer)均可以存取这块共享内存，生产者将生产出的数据放入共享内存，消费者从共享内存中取出数据进行消费；

* 生产者-消费者问题的要点：
    - 生产者和消费者同时只能有一位对共享内存进行操作，否则会产生竞争，所以共享内存是个临界资源；
    - 当共享内存的占用达到 BUF_SIZE 时，也就是共享内存已经占满时，生产者不能再向共享内存中放入数据；
    - 当共享内存中没有数据时，也就是共享内存块为空时，消费者不能再从共享内存中取出数据

* 解决方案：
    - 建立一个信号量 `sem_mutex`，初始值为 1，用于访问临界资源共享内存的互斥；
    - 建立一个信号量 `sem_full`，初始值为 BUF_SIZE：
        > 生产者放入共享内存一次数据，`sem_full - 1`，消费者取出一次数据 `sem_full + 1`，为 0 时表示共享内存已满；

    - 建立一个信号量 `sem_empty`，初始值为 0：
        > 生产者放入共享内存一次数据，`sem_full + 1`，消费者取出一次数据 `sem_full - 1`，为 0 时表示共享内存已空；

    - 生产者在 `sem_full` 为 0 时停止生产，消费者在 `sem_empty` 为 0 时停止消费。

* 源程序：[producer-consumer.c][src03](**点击文件名下载源程序**)演示了如何使用信号量解决经典的‘**生产者-消费者问题**’；
* 编译：`gcc -Wall producer-consumer.c -o producer-consumer -lpthread`
* 运行：`./producer-consumer 5 8`
* 需要对该程序做一个简要的说明：
    - 该程序从命令行上运行时可以带两个参数，分别表示生产者的生产速度和消费者的消费速度，如果不带参数，将使用默认值；
    - 两个参数必须为 `1-19` 之间的数字，数值越大表示速度越快；
    - 当生产速度大于消费速度时，会导致生产过剩，从而使缓冲区满；
    - 当生产速度小于消费速度时，会导致生产不足，从而使缓冲区空；
    - 该程序并不是只有一个生产者和一个消费者，而是考虑到了多个生产者和消费者的情况，默认为 5 个生产者和 5 个消费者，通过调整程序中的宏定义可以改变生产者和消费者的数量；
* 运行动图：

    ![GIF of running producer-consumer][img03]

    - 运行了两次，第一次生产速度小于消费速度，所以会出现提示：The buffer is empty.
    - 第二次生产速度大于消费速度，所以会出现提示：The buffer is full.

-------
## 7 POSIX 信号量与 SYSTEM V 信号量集的主要区别
* POSIX 信号量一次创建和初始化一个信号量；System V 信号量集一次可以创建一组信号量，每个信号量可以独立初始化；
* POSIX 信号量只能递增或递减 1；System V 信号量集可以将信号量值一次增加或减少一个任意无符号整数；
* POSIX 信号量提供 sem_trywait() 操作，如果该信号量值当前为 0，该调用不会阻塞而是返回一个错误，使程序可以对这个即将发生的阻塞做出恰当的处理；System V 信号量集没有提供类似功能；
* System V 信号量集由 key_t 值标识；POSIX 信号量可以通过一个名称(以“/”开头)来标识，也可以不命名。

## 8 有关进程间通信(IPC)的的其它文章：
* [IPC之一：使用匿名管道进行父子进程间通信的例子][article01]
* [IPC之二：使用命名管道(FIFO)进行进程间通信的例子][article02]
* [IPC之三：使用 System V 消息队列进行进程间通信的实例][article03]
* [IPC之四：使用 POSIX 消息队列进行进程间通信的实例][article04]
* [IPC之五：使用 System V 信号量集解决经典的‘哲学家就餐问题‘][article05]


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
-->

[src01]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190016/sem-create.c
[src02]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190016/official-exam.c
[src03]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190016/producer-consumer.c


[img01]: https://whowin.gitee.io/images/190016/screenshot-of-sem-create.png
[img02]: https://whowin.gitee.io/images/190016/screenshot-of-official-exam.png
[img03]: https://whowin.gitee.io/images/190016/producer-consumer.gif

<!-- CSDN
[img01]: https://img-blog.csdnimg.cn/img_convert/a6bb46bf5ea2c3e0b5449cfc12ec4e7a.png
[img02]: https://img-blog.csdnimg.cn/img_convert/50d9471b0a8f4fb7c1b5a41abae98a41.png
[img03]: https://img-blog.csdnimg.cn/img_convert/6d3e55e7340156b54ba33a1626b57e60.gif
-->
