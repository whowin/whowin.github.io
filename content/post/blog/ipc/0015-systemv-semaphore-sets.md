---
title: "IPC之五：使用 System V 信号量集解决经典的'哲学家就餐问题'"
date: 2023-09-01T16:43:29+08:00
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
  - "system V"
draft: false
#references: 
# - [Linux Interprocess Communications](https://tldp.org/LDP/lpg/node7.html)
# - [Checking Which Processes Are Using Semaphores](https://www.baeldung.com/linux/check-processes-using-semaphores)
# - [Everything about Linux Semaphore](https://www.interserver.net/tips/kb/everything-about-linux-semaphore/)
# - [IPC之信号量](https://blog.csdn.net/weixin_44522306/article/details/89643615)
# - [IPC之信号量详解](https://blog.csdn.net/Qiuoooooo/article/details/60573433)
# - [semop() — Semaphore operations](https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-semop-semaphore-operations)
#       - 关于sem_op为负数时描述的比较好
# - `man semop` 中有一段关于 semadj 的说明
# - `man sysvipc`
# - [linux/ipc/sem.c](https://github.com/torvalds/linux/blob/master/ipc/sem.c)
#       - struct sem, struct sem_undo, struct sem_queue等
# - [哲学家就餐问题](https://baike.baidu.com/item/%E5%93%B2%E5%AD%A6%E5%AE%B6%E5%B0%B1%E9%A4%90%E9%97%AE%E9%A2%98/10929794)

postid: 190015
---

IPC 是 Linux 编程中一个重要的概念，IPC 有多种方式，本文主要介绍信号量集(Semaphore Sets)，尽管信号量集被认为是 IPC 的一种方式，但实际上通常把信号量集用于进程间同步或者资源访问互斥，信号量集和共享内存(Shared Memory)配合使用，可以实现完美的进程间通信；Linux 既支持 UNIX SYSTEM V 的信号量集，也支持 POSIX 的信号量集，本文仅针对 SYSTEM V 信号量集；本文给出了多个具体的实例，每个实例均附有完整的源代码；本文所有实例在 Ubuntu 20.04 上编译测试通过，gcc版本号为：9.4.0；本文的实例中涉及多线程编程和信号处理等，对 Linux 编程的初学者有一些难度。

<!--more-->

## 1 信号量集(Semaphore Sets)的基本概念
* 在传统的 UNIX SYSTEM V 中有三种 IPC 方法：消息队列(Message Queue)、信号量集(Semaphore Sets)和共享内存(Shared Memory)；
* 文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]中介绍了 SYSTEM V 下的消息队列，在阅读本文之前建议先阅读此文，因为 SYSTEM V 的各种 IPC 方法有很多相似之处，并且有一些基本概念是完全一致的；
* 如无特别说明，以下所述的信号量集均指 SYSTEM V 信号量集；

* 信号量集之所以被称为"**集**"，是因为在内核实现中是按照一个信号量集合(数组)进行管理的，而不是以单个信号量进行管理的，在文章的后面会介绍内核是如何管理信号量集的；
* 在实践中经常把信号量集称为**信号量**，是因为很多应用场景中，信号量集中只有一个信号量，不过，怎么称呼并不重要；
* 在多任务系统中，会有多个进程(线程)并行运行，这可能会出现多个进程同时操作同一个资源(比如：公共变量、共享内存、文件等)的情况，这种情况是有很大的潜在风险的：
    > 比如一个全局数组，A 进程在读出数组的第一个元素后，由于进程调度，A 进程暂停，B 进程开始写这个数组，B 进程写完后，由于进程调度，B 进程暂停，A 进程继续运行，继续读取数组的其它元素，A 进程读取了这个数组中的所有元素，但是，很显然，其读到的第一个元素的值，可能和其它元素的值并不是一组，这可能导致 A 进程通过数据做出的判断出现错误；
* 并发进程/线程有助于提高性能，但是，当它们对共享资源进行访问时，需要对共享资源的访问进行必要的保护，以免出现不一致和不正确的状态；
* 其它的资源，比如：数据库，共享内存、文件以及共享打印机等都有类似的问题，
* 我们把这种只能有限个进程(线程)同时访问的资源叫做 **临界资源**(Critical Resources)，把访问临界资源的代码称为 **临界区**(Critical Sections)；
* 信号量是内核级原语(Kernel Level Primitives)，是原子性(Atomic)的，不会产生竞争，这意味着对信号量的操作过程不会被中断；
* 信号量是一个不小于 0 的计数器，当计数器为 0 时，表示其对应的一个资源的使用率以达到 100%，不能再有新的进程访问；
* 对信号量有两个主要操作：
    - 一个叫 P 操作(获取信号量)，就是把信号量的值减 1(也可以是大于 1 的其它正整数)，如果此时信号量大于 0，则信号量值减 1，进程获得资源使用许可；如果此时信号量的值已经为 0，则进程等待在信号量上，直到信号量的值大于 0；
    - 另一个叫 V 操作(释放信号量)，就是把信号量的值加 1(也可以是大于 1 的其它正整数)；
    - P 操作的称谓来源于荷兰语的单词 **Proberen**，意思是"尝试"(to test)；
    - V 操作的称谓同样来源与荷兰语单词 **Verhogen**，意思是"增加"(to raise)；
* 信号量的一种作用正是用于保护临界资源，使并发进程(线程)对资源的访问实现互斥；
    > 以下描述一个简单的使用信号量实现访问临界区互斥的过程：把一个信号量的值初始化为 1，当程序要进入临界区时，对信号量执行 P 操作(信号量减 1)，信号量值变为 0，程序获得许可进入临界区；如果此时信号量为 0，则进入排队等待，等待其它进程执行 V 操作，使信号量大于 0，然后对信号量执行 P 操作，获得许可进入临界区；程序在退出临界区时，对信号量执行 V 操作(信号量加 1)，如果此时信号量为 0，加 1 后便会启动等待在该信号量上的其它进程(线程)，获得允许进入临界区的许可；

* 信号量的另一个主要用途是进程(线程)间的同步，进程间同步和互斥是不同的概念，互斥指的是多个进程不能同时访问临界资源，同步指的是多个进程要按照一个特定顺序访问临界资源；
    > 以下描述一个简单的进程同步过程：假定一组数据需要经过三个进程的依此处理后才能完成，这三个进程分别为：proc1、proc2 和 proc3，数据首先要经过 proc1 处理，然后 proc2 处理，然后 proc3 处理，循环往复，那么我们需要建立一个有三个信号量的信号量集：semset[3]，初始化时 semset[0]=1，semset[1]=semset[2]=0，proc1 对 semset[0] 执行 P 操作，proc2 对 semset[1] 执行 P 操作，proc3 对 semset[2] 执行 P 操作，初始状态时只有 proc1 可以获得许可(因为 semset[0]=1)，proc2 和 proc3 将等待在信号量上，proc1 完成任务后对 semset[1] 执行 V 操作，唤醒了 proc2，然后 proc1 再次对 semset[0] 执行 P 操作，等待在信号量上；proc2 完成任务后对 semset[2] 执行 V 操作，唤醒 proc3，然后再次对 semset[1] 执行 P 操作，等待在信号量上；proc3 完成任务后对 semset[0] 执行 V 操作，唤醒 proc1，然后对 semset[2] 执行 P 操作，等待在信号量上；......；这样便可以对数据的处理顺序的进行有效地同步；

* 所以，似乎信号量并不能完成进程间的通信；但信号量有一个重要的用法就是和 IPC 的共享内存共同使用完成进程间通信。

* 和 SYSTEM V 的其它 IPC 一样，信号量集对内核资源的使用也有一些限制，可以使用命令 `ipcs -s -l` 查看这些限制：

    ![Screenshot of ipcs -s -l][img01]

* 也可以在 proc 文件系统中找到这些限制值：

    ![Screenshot of sem limits in proc][img02]

    - 第一项为 SEMMSL：每一个信号量集中的最大信号量数；
    - 第二项为 SEMMNS：整个 Linux 系统中的信号量(不是信号量集)的最大数；
    - 第三项为 SEMOPM：执行 semop() 时，一次可以操作的信号量的最大数；
    - 第四项为 SEMMNI：整个 Linux 系统中的信号量集的最大数。

## 2 创建/获取信号量集 - semget()
* 在 SYSTEM V 三种 IPC 方法的 API 中，都是使用 `~get()` 来创建和获取的，消息队列使用 msgget()，共享内存使用 shmget()，信号量集中使用 semget()，其中的一些概念是一样的，这里就不在赘述，请务必参考文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]
* 与 SYSTEM V 的消息队列一样，信号量集的使用 ID 作为其唯一标识符；
* 使用 semget() 创建/获取信号量集
    ```C
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>

    int semget(key_t key, int nsems, int semflg);
    ```
* 可以使用 ftok() 生成 **key**，请参考文章[《IPC之三：使用 System V 消息队列进行进程间通信的实例》][article03]中的相关说明；
* **nsems** 表示创建的新信号量集中有多少个信号量，当仅仅是获取信号量的 ID(semflag=IPC_EXCL)，nsems 可以为 0，很多情况下，nsems 为 1，表示这个信号量集里只有一个信号量；
* **semflag** 的有效值有 IPC_CREAT 和 IPC_EXCL：
    - 当 **IPC_CREAT** 时，如果 key 对应的信号量集存在，则返回其信号量集的 ID，如果 key 对应的信号量集不存在，则建立与 key 关联的信号量集，并返回该新信号量集的 ID；
    - 当 **IPC_CREAT | IPC_EXCL** 时，如果 key 对应的信号量集存在，则报错返回 -1，`errno = EEXIST(File exists)`；如果 key 对应的信号量集不存在，则建立与 key 关联的信号量集，并返回该新信号量集的 ID；
    - 当 **IPC_EXEL** 时，如果 key 对应的信号量集存在，则返回信号量集的 ID(这点和 IPC_CREAT 一样)，如果 key 对应的信号量集不存在，则返回 -1，`errno = ENOENT(No such file or directory)`；
    - 另外，semflag 还可以加上所创建的消息队列的读写权限，要用八进制表示，比如：0666；
    - semflag 举例：`IPC_CREAT | IPC_EXEL | 0666`
* 当 `key = IPC_PRIVATE` 时，`semget()` 将创建一个新的信号量集并返回该信号量集的 ID；
    - 这样生成的信号量集只有 ID，没有 key(key 为 0)，所以其它进程并不能方便地使用这个信号量集，通常只能在子进程之间使用；
    - 实际上，IPC_PRIVATE 的值是 0，所以我们自己生成的 key 不能是 0，否则相当于将 key 设置为 IPC_PRIVATE；
* 下面这段代码创建一个有三个信号量的信号量集：
    ```C
    key_t sem_key = ftok("/tmp/", 1234);
    int semid = semget(sem_key, 3, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        perror("semget()");
        exit(EXIT_FAILURE);
    }
    ```
* 特别要注意的是 msgflag 参数中的读/写权限的设置，如果不显式标明，那么默认的读写权限将变成 0000，这样的一个信号量集是没有办法进行使用的；当然还可以使用 `semctl()` 修改信号量集的读写权限，但需要更高的权限(比如root)才能做到；
* 另外要注意的是，新创建的信号量集中的信号量的初始值均为 0，如果不希望其值为 0，可以使用 semctl() 去设置一下。

## 3 内核对信号量集的管理
* 信号量集是在内核中实现的，也是由内核进行管理的，内核提供了一组调用供应用程序对信号量集进行操作和控制；
* 先介绍内核对信号量集的管理还是先介绍 semop() 调用，其实是很让人纠结的，因为这两者交织在一起，很难分开，所以建议在阅读这一节时与下面介绍 semop() 调用的章节一起阅读，否则可能有些地方不好理解；
* 要了解内核对信号量集的管理，其实了解两个结构就可以了：
    - 第一个是定义在 `<linux/sem.h>` 中的完整的 `struct semid_ds`
        ```C
        struct semid_ds {
            struct ipc_perm	sem_perm;       /* permissions .. see ipc.h */
            __kernel_time_t	sem_otime;      /* last semop time */
            __kernel_time_t	sem_ctime;      /* create/last semctl() time */
            struct sem	*sem_base;          /* ptr to first semaphore in array */
            struct sem_queue *sem_pending;  /* pending operations to be processed */
            struct sem_queue **sem_pending_last;    /* last pending operation */
            struct sem_undo	*undo;          /* undo requests on this array */
            unsigned short	sem_nsems;      /* no. of semaphores in array */
        };
        ```
    - Linux 用一个结构数组来管理信号量集，这个数组的起始指针就是 `struct semid_ds` 中的 `sem_base` 字段，这个数组的元素数量就是 `sem_nsems` 字段，也就是使用 semget() 创建信号量集时的参数 nsems；
    - 第二个要了解的结构就是 `struct sem` 是 `struct semid_ds` 中的 `sem_base` 指向的结构数组，数组中的每一个元素表示一个信号量；
        ```C
        struct sem {
            unsigned short  semval;   /* semaphore value */
            unsigned short  semzcnt;  /* # waiting for zero */
            unsigned short  semncnt;  /* # waiting for increase */
            pid_t           sempid;   /* PID of process that last */
        }
        ```
    - **semval**：信号量的值，这个值一定为 0 或者正整数；
        > 任何一个进程在使用 semop() 试图将 semval 改变成小于 0 的数值时，都会产生阻塞，直至其它进程改变了 semval 的值，使当前进程在执行 semop() 操作后，semval 的值仍然大于或等于 0；

    - **semzcnt**：等待资源利用率达到100%的进程数；
        > semop() 通过设置 sem_op 的值来改变 semval 的值，当 sem_op 设置为 0 时，semop() 将等待在这个信号量上，直至该信号量的 semval 为 0，**semzcnt** 就是记录当前处于这种状态的进程数量；

    - **semncnt**：等待资源可用的进程数；
        > 当一个信号量的 semval 为 0 时，表示其关联的资源不可用，此时，想要获取该信号量的进程必须等待在这个信号量上，等待其它进程将信号量的值改变为大于 0 的值，**semncnt** 就是记录处于这种状态的进程数；

    - **sempid**：最后操作该信号量的进程号；

* 内核中还为每一个打开了信号量集的进程维护着一个变量 **semadj**，这个值的初始值为 0，当该进程执行 semop() 成功时，其对信号量的影响将记录在 semadj 中，这样当这个进程终止时，内核可以根据 semadj 消除该进程对某个信号量的影响；
    > 下面简单描述以下 semadj 的作用：某进程打开一个只有一个信号量的信号量集，此时：`semval = 1`，`semadj = 0`，该进程调用 semop() 执行 P 操作，使 `semval = 0`，内核将在semadj 做相反操作(+1)，使 `semadj = 1`，此后，该进程执行 V 操作，使 `semval = 1`，内核再次对 semadj 做相反操作(-1)，使 `semadj = 0`，然后，该进程再次执行 P 操作，使 `semval = 0`，内核对 semadj 做相反操作(+1)，使 `semadj = 1`，此时该进程意外终止，此时，该信号量的 `semval = 0`，将导致其它任何进程都无法获取该信号量，但是由于内核设置了 semadj 变量，在该进程终止时，执行 `semval += semadj` 操作，以消除该进程对这个信号量的影响，执行结果使 `semval = 1`，使其它进程可以继续使用该信号量。

    > 在调用 semop() 时在参数 sem_flg 上设置 `SEM_UNDO`，则其对信号量值的改变将被记录到 semadj 变量中。

* 另外在 `<sys/sem.h>` 中对 `struct semid_ds` 的定义仅是这个结构的一部分，但用户空间的应用程序已经足够了，应用程序并不需要了解内核如何管理信号量集；
    ```C
    struct semid_ds {
        struct ipc_perm sem_perm;
        time_t          sem_otime;   /* last operation time */
        time_t          sem_ctime;   /* last change time */
        unsigned long   sem_nsems;   /* count of sems in set */
    };
    ```

## 4 信号量集的操作 - semop()
* 前面说过，信号量就是一个计数器，那么对信号量的主要操作就是对它的增/减操作，这就是本节主要要介绍的内容；
* 使用 semop() 对信号量集进行操作：
    ```C
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>

    int semop(int semid, struct sembuf *sops, size_t nsops);
    int semtimedop(int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout);
    ```
* `semop()` 调用成功返回 0，调用失败返回 -1，errno 中为错误编码；
* **semid**：已经打开的信号量集 ID；
* 如果信号量集中有不止一个信号量，`semop()` 可以一次处理多个信号量，最多一次可以处理 **SEMOPM** 个信号量，这些信号量必须在一个信号量集中；
* **sops**：这是一个结构数组的起始指针，数组中的每个结构元素定义了对信号量集中某个信号量的操作，这个数组中的元素个数，由后面的参数 **nsops** 指定；
* **sops** 对应的结构如下：
    ```C
    struct sembuf
    {
        unsigned short int sem_num; /* semaphore number */
        short int sem_op;           /* semaphore operation */
        short int sem_flg;          /* operation flag */
    };
    ```
* **sem_num**：要操作的信号量在信号量集中的索引号(数组下标)；
* **sem_op**：要对信号量做的操作，根据其值有三种可能性：
    1. **sem_op 是正整数**：
        > 执行一个所谓的 V 操作；

        > 信号量值(semval)将被修改为：`semval + sem_op`；调用进程需要对信号量集有写权限；
        
        > 此外，如果为此操作指定了 SEM_UNDO，则系统会从此信号量的信号量调整 (semadj) 值中减去值 sem_op。

    2. **sem_op 为 0**：
        > 如果信号量值(semval)为 0，该调用将立即返回；调用进程需要有信号量集的读权限；
        
        > 如果信号量值(semval)不为 0，则调用进程进入阻塞，直至信号量值变为 0 时才返回成功；或者该信号量集被删除，该调用返回一个错误，errno=EIDRM；或者调用线程捕获到了一个信号(signal)进入信号处理程序，该调用返回一个错误，errno=EINTR；

        > 如果 sem_flag 中设置了 IPC_NOWAIT，且信号量值(semval)不为 0，则调用失败，errno=EAGAIN；
    
    3. **sem_op 为负数**：
        > 执行一个所谓的 P 操作；

        > 如果 `semval - |sem_op| >= 0`，则信号量值将被修改为：`semval - |sem_op|`；调用进程需要对信号量集有写权限；

        > 如果 `semval - |sem_op| < 0`，则调用进程/线程进入阻塞，直至 `semval >= |semop|`，调用返回成功；或者该信号量集被删除，该调用返回一个错误，errno=EIDRM；或者调用线程捕获到了一个信号(signal)进入信号处理程序，该调用返回一个错误，errno=EINTR；

        > 如果 sem_flag 中设置了 IPC_NOWAIT，且`semval - |sem_op| < 0`，则调用失败，errno=EAGAIN；

* 看上去 sem_op 有些复杂，但在具体编程实践中，多数情况下信号量集中只有一个信号量，所以 sem_num 始终为 0；sem_op 多数情况下仅为 1 或者 -1，这样看起来就不那么复杂了；

* **sem_flag**：操作标志，只有两种：IPC_NOWAIT 和 SEM_UNDO；
    > 设置 `IPC_NOWAIT` 后，当调用需要阻塞时会立即返回错误，并在 errno 中设置错误代码；

    > 设置 `SEM_UNDO` 后，调用中对 semval 的影响都将被记录在该进程的 `semadj` 中，请参考上一节中关于 semadj 的介绍，在一个进程终止时，内核会根据 semadj 的值修改 semval 的值，以消除一个进程对某个信号量的影响；

* 对 `semtimedop()` 就不做介绍了，这个函数用的不多，可以参考文章[《IPC之四：使用 POSIX 消息队列进行进程间通信的实例》][article04]中关于 `mq_timedsend()` 的介绍，其实主要是对 `struct timespec` 的介绍。

## 5 信号量集的控制操作 - semctl()
* 使用 semctl() 调用对信号量集进行控制操作
    ```C
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>

    int semctl(int semid, int semnum, int cmd, ...);
    ```
* 这个调用有三个或者四个参数，是否有第四个参数，取决于第三个参数 cmd 的值，当有第四个参数时，第四个参数为 `union semun arg`，其类型为 `union semun`，见如下定义：
    ```C
    union semun {
        int              val;    /* Value for SETVAL */
        struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
        unsigned short  *array;  /* Array for GETALL, SETALL */
        struct seminfo  *__buf;  /* Buffer for IPC_INFO(Linux-specific) */
    };
    ```
    - 其中的 `struct semid_ds` 前面有过介绍，定义在 `<sys/sem.h>` 中：
        ```C
        struct semid_ds {
            struct ipc_perm sem_perm;  /* Ownership and permissions */
            time_t          sem_otime; /* Last semop time */
            time_t          sem_ctime; /* Last change time */
            unsigned long   sem_nsems; /* No. of semaphores in set */
        };
        ```
    - `struct semid_ds` 中的 `struct ipc_perm` 定义如下：
        ```C
        struct ipc_perm {
            key_t          __key; /* Key supplied to semget(2) */
            uid_t          uid;   /* Effective UID of owner */
            gid_t          gid;   /* Effective GID of owner */
            uid_t          cuid;  /* Effective UID of creator */
            gid_t          cgid;  /* Effective GID of creator */
            unsigned short mode;  /* Permissions */
            unsigned short __seq; /* Sequence number */
        };
        ```
* 这个调用在失败时返回 -1，在成功时的返回值与 cmd 有关；

* **semid**：已经打开的信号量集 ID；
* **semnum**：要操作的信号量在信号量集中的索引号(数组下标)，对于只有一个信号量的信号量集，这个值为 0；
* **cmd**：执行的命令，这个参数决定着对信号量做什么操作，也决定着这个调用最后的不定长参数应该是什么参数；
* semctl() 可以执行的操作很多，我们仅就常用的 cmd 值做出介绍：
    - **IPC_STAT**：获取信号量集当前的状态
        > 该调用的第四个参数为 `arg.buf`，是一个指向 `struct semid_ds` 的指针，调用成功，内核中 `struct semid_ds` 的数据将复制到 `arg.buf` 中；实际上该调用主要用于获取信号量集的读写权限以及该信号量集中信号量的数量等；

        > 此命令中，参数 semnum 无效，设置为 0 即可。

        > 调用成功时，返回 0。

    - **IPC_SET**：设置信号量集的状态，这个命令和 `IPC_STAT` 是一对；
        > 该调用的第四个参数为 `arg.buf`，是一个指向 `struct semid_ds` 的指针，通常在调用前，应该先使用 `IPC_STAT` 命令调用 semctl() 获取信号量集的状态，然后修改其中需要设置的值，然后再次用 `IPC_SET` 命令调用 semctl() 设置信号量集的新状态；已知的可以设置的状态有：`sem_perm.uid`、`sem_perm.gid` 和 `sem_perm.mode`(后 9 位)；在实际编程实践中，这个命令仅用于设置信号量集的读写权限，也就是 `sem_perm.mode`(后 9 位)；

        > 此命令中，参数 semnum 无效，设置为 0 即可；

        > 调用成功时，返回 0。

    - **IPC_RMID**：删除一个信号量集；
        > 此命令无需第四个参数，参数 semnum 没有意义，为 0 即可，该调用成功时返回 0；

    - **GETALL**：获取信号量集中所有信号量的值；
        > 第四个参数为 arg.array，其数组的大小为该信号量集中信号量的数量，此命令中，参数 semnum 没有意义，为 0 即可，该调用成功时返回 0；

    - **SETALL**：设置信号量集中所有信号量的值，该命令与 GETALL 是一对；
        > 第四个参数为 arg.array，其数组的大小为该信号量集中信号量的数量，其数组中各元素的值将设置为对应的信号量的值；此命令中，参数 semnum 没有意义，为 0 即可，该调用成功时返回 0；

        > 该命令可以用于初始化一个信号量集；
    
    - **GETVAL**：获取信号量集中某个信号量的值；
        > 该命令不需要第四个参数，调用成功时，返回该信号量的值；

    - **SETVAL**：设置信号量集中指定信号量的值；
        > 第四个参数为 arg.val，该值将设置为指定信号量的值，调用成功时返回 0；

        > 该命令可以用来初始化信号量集中的一个信号量；

    - 还有一些用于 semctl() 的命令，请自行翻阅在线手册 `man semctl`

* 源程序：[sem-ctl.c][src01](**点击文件名下载源程序**)演示了以上列举的各种用于 semctl() 的命令的简单用法；
* 编译：`gcc -Wall sem-ctl.c -o sem-ctl`
* 运行：`./sem-ctl`
* 运行截图：

    ![Screenshot fot running sem-ctl][img03]

------
## 6 实例
* 在下面这个实例中，我们使用信号量集解决一个经典的问题 - 哲学家就餐问题(The Dining Philosophers problem)；
* 如果希望更多地了解这个问题，可以百度一下，这里仅做简要说明：
    - 5 位哲学家围坐在一个圆桌周围，餐桌中央摆着一盘意大利面，每个哲学家的左边都有一把叉子；
    - 哲学家们只做两件事：思考或者吃饭，而且同时只能做一件事，即：吃饭时不思考，思考时不吃饭；
    - 哲学家吃饭时，需要用两把叉子，只能用临近的两把叉子；
* 哲学家就餐问题的要点：
    - 哲学家思考的时候，无需任何资源；
    - 哲学家吃饭时，需要拿起临近的两把叉子，只有在他的旁边的两个人没有吃饭时，他才能够得到这两把叉子；
    - 当相邻的两位哲学家都要吃饭时，他们将在他们之间的叉子上产生竞争；
* 解决方案：
    - 每位哲学家有三种状态：思考、饥饿、吃饭
    - 哲学家们的初始状态均为：思考；
    - 状态转换过程：思考 ---> 饥饿 ---> 吃饭 ---> 思考；
    - 主要竞争资源：叉子
    - 假设条件：同时只允许一位哲学家进行状态转换，所以改变状态的代码为临界区，设置一个信号量 `sem[0]` 保证临界区的互斥；
    - 从思考状态变为饥饿状态：
        > 只需要获得进入临界区的许可，对信号量 `sem[0]` 执行 P 操作；

    - 哲学家 x 要从饥饿状态变为吃饭状态：
        > 如果他左右两边的哲学家都不在吃饭状态，则可以顺利进入吃饭状态，然后对信号量 `sem[0]` 执行 V 操作，以允许其它哲学家改变状态；

        > 如果他左右两边至少有一位哲学家在吃饭状态，则无法进入吃饭状态，只能等待在饥饿状态，为每一位哲学家设置一个信号量 `sem[x](x=1-5)`，无法从饥饿状态进入吃饭状态的哲学家在饥饿状态下需要等待左右两边的哲学家都进入思考状态，对 `sem[x]` 执行 P 操作以进行等待，在对 `sem[x]` 执行 P 操作前要首先对 `sem[0]` 进行 V 操作，以允许其它哲学家进入临界区改变状态；

        > 在饥饿状态等待的哲学家需要等待被他的邻居(他左右两边的哲学家)唤醒，被唤醒后仍要首先获得临界区的许可，即对 sem[0] 做 P 操作，然后才能尝试进入吃饭状态，如果仍然无法进入吃饭状态，则要继续等待在饥饿状态，也就是重复上面的两个步骤，直至进入吃饭状态；

    - 哲学家要从吃饭状态变成思考状态：
        > 首先对信号量 `sem[0]` 执行 P 操作，以获得进入临界区的许可，然后进入思考状态，如果其左边或右边的哲学家在饥饿状态，则要对相应的信号量 `sem[LEFT]` 或 `sem[RIGHT]` 执行 V 操作，以唤醒正在等待的哲学家，然后对信号量 `sem[0]` 执行 V 操作，以允许其它哲学家进入临界区改变状态；

* 下面示意图描述了这个问题的基本场景：

    ![diagram of the dinning philosphers problem][img04]
    
* 下面示意图描述了哲学家状态转换与信号量集的关系

    ![state transition diagram][img05]

* 源程序：[sem-philo.c][src02](**点击文件名下载源程序**)演示了如何使用信号量集解决经典的‘**哲学家就餐问题**’；
* 编译：`gcc -Wall sem-philo.c -o sem-philo -lpthread`
* 运行：`./sem-philo`
* 运行动图：

    ![GIF of running sem-philo][img06]


## 7 信号量集的命令行命令
* `ipcs -s -l` - 显示信号量集的限制值；
* `ipcs -s` - 显示现有信号量集的 key、ID 等部分属性；
* `ipcs -s -i <ID>` - 显示指定 ID 的信号量集的属性(比 `ipcs -s` 显示的属性要多些)；

* `ipcrm -S <key>` - 删除指定 key 的信号量集；
* `ipcrm -s <ID>` - 删除指定 ID 的信号量集；
* `ipcrm --all=sem` - 删除所有的信号量集

* `ipcmk -S <num>` - 创建一个新的信号量集，其中包含 `<num>` 个信号量，其读写权限为默认的 0644；
* `ipcmk -S <num> -p <perm>` - 创建一个新的信号量集，其中包含 `<num>` 个信号量，其读写权限为指定的 `<perm>`。


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

[img_sponsor_qrcode]:/images/qrcode/sponsor-qrcode.png


[article01]: /post/blog/ipc/0010-ipc-example-of-anonymous-pipe/
[article02]: /post/blog/ipc/0011-ipc-examples-of-fifo/
[article03]: /post/blog/ipc/0013-systemv-message-queue/
[article04]: /post/blog/ipc/0014-posix-message-queue/
[article05]: /post/blog/ipc/0015-systemv-semaphore-sets/
[article06]: /post/blog/ipc/0016-posix-semaphores/
[article07]: /post/blog/ipc/0017-systemv-shared-memory/
[article08]: /post/blog/ipc/0018-posix-shared-memory/
[article09]: /post/blog/ipc/0019-ipc-with-unix-domain-socket/
[article10]: /post/blog/ipc/0020-ipc-using-files/
[article11]: /post/blog/ipc/0021-ipc-using-dbus/
[article12]: /post/blog/ipc/0022-dbus-asyn-process-signal/
[article13]: /post/blog/ipc/0023-dbus-resolve-hostname/
[article14]: /post/blog/ipc/0024-select-recv-message/
[article15]: /post/blog/ipc/0025-resolve-arbitrary-dns-record/

<!-- for CSDN
[article01]: https://blog.csdn.net/whowin/article/details/132171311
[article02]: https://blog.csdn.net/whowin/article/details/132171930
[article03]: https://blog.csdn.net/whowin/article/details/132172172
[article04]: https://blog.csdn.net/whowin/article/details/134869490
[article05]: https://blog.csdn.net/whowin/article/details/134869636
-->

[src01]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190015/sem-ctl.c
[src02]: https://gitee.com/whowin/whowin/blob/blog/sourcecodes/190015/sem-philo.c


[img01]: /images/190015/screenshot-of-sem-limits.png
[img02]: /images/190015/screenshot-of-sem-limits-proc.png
[img03]: /images/190015/screenshot-of-sem-ctl.png
[img04]: /images/190015/diag-of-philosophers.png
[img05]: /images/190015/state-transition-diagram.png
[img06]: /images/190015/dinning-philosopher-problem.gif

<!-- CSDN
[img01]: https://img-blog.csdnimg.cn/img_convert/1d9be7c19722b01c607c651c5e068b2b.png
[img02]: https://img-blog.csdnimg.cn/img_convert/dc6e3c5e26cb791ede73c8fb8190fe32.png
[img03]: https://img-blog.csdnimg.cn/img_convert/09fd56f5bcfa739456c24c98384db57b.png
[img04]: https://img-blog.csdnimg.cn/img_convert/cc9740424e76d1c720de276b8f7ce0e6.png
[img05]: https://img-blog.csdnimg.cn/img_convert/961de7761fae80d324f1bbe8a728053a.png
[img06]: https://img-blog.csdnimg.cn/img_convert/0a3cf5f6e8061d069f0c49e7c96e8408.gif
-->
