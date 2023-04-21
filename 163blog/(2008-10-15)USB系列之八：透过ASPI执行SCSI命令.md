# USB系列之八：透过ASPI执行SCSI命令  
**2008-10-15 15:15:52**

* 在《USB系列之七》里我们介绍了ASPI的规范，并对一系列ASPI的命令做了测试，其中的02号命令是执行SCSI命令，我们专门在这篇文章中介绍，在《USB系列七》中，我们已经了解了调用ASPI的方法，主要是要填一个SRB（SCSI Request Block）的表，在以前的《USB系列之三：从你的U盘里读出更多的内容》文章中我们通过DOSUSB已经实现了许多SCSI命令，这些命令包括：
  - SCSI INQUIRY Command
  - SCSI READ CAPACITY (10) Command
  - SCSI REQUEST SENSE Command
  - SCSI TEST UNIT READY Command
  - SCSI READ (10) Command
* 在《USB系列之四：向U盘上写数据》一文中，我们又实现了SCSI WRITE(10) Command，所以我们已经实现过6个SCSI命令了，先让我们回顾一下以前的SCSI命令我们是如何实现的。
* 按照《USB Mass Storage Class -- Bulk Only Transport》（这个文档在USB系列之三中有过介绍并提供了下载）的说明，我们首先要填写一个CBW(Command Block Wrapper)表，在这个表中中的最后有一个不定长的部分叫做CBWCB，这部分的长度及字段含义视发出的SCSI命令不同而不同，实际上，这个部分就是描述SCSI命令的，在SCSI的规范中，这个部分叫做CDB(Command Descriptor Block)，它和CBWCB完全是一个东西，在本文中，我们沿用SCSI规范中的叫法，把这个表叫做CDB。
* 了解SCSI命令，需要了解两个规范SPC-3和SBC-2，这两个文档在USB系列之三中都有介绍并提供了下载，鉴于以前我们已经实现过许多SCSI命令，在本文中我们仅就最重要的三个SCSI命令进行实现，它们是：
  - SCSI TEST UNIT READY Command
  - SCSI READ (10) Command
  - SCSI WRITE (10) Command
  > 相信读者可以自行实现其它的命令。
* 为了清晰地表现读、写扇区的操作，我们用两个源程序来分别测试读、写扇区，第一个程序叫aspiread，专门测试读扇区，通过这个程序，我们有把握使用其中的SCSI READ(10)命令把指定的扇区内容读出；第二个程序叫aspiwrit，专门测试写扇区操作，因为写扇区本身是看不见什么的，要靠读扇区命令把写入的内容读出来才能验证，这也就是我们为什么单独测试读扇区命令的原因。
* 这两个程序的下载地址如下：
  - aspiread下载：http://blog.whowin.net/source/aspiread.zip
  - aspiwrit下载：http://blog.whowin.net/source/aspiwrit.zip
* 我们需要对这两个程序分别做一下说明。
  - 先说aspiread程序，这个程序在我的环境下执行后的结果如下：
    ```
    ASPI Reading test program v1.00.

    Open SCSIMRG success!    ASPI entry : 0786:6a5c

    Test unit ready......
    Command Status: 01    Host Adapter Status: 00
    Target Status: 00
    Press any key when ready......

    Reading.........
    Command Status: 01    Host Adapter Status: 00
    Target Status: 00    Residual Byte: 0000
    Data in buffer: fa 33 c0 8e d0 bc 00 7c 16 07 bb 78 06 8d 87 8a fb ba 80 01 b9 1d f5 16 cd 13 72 e4 53 cb af 29 b3 04 80 3c 80 74 0e 80 3c 00 75 1c 83 c6 10 fe cb 75 ef cd 18 8b 14 8b 4c 02 8b ee 83 c6 10 fe cb 74 1a 80 3c 00 74 f4 be 8b 06 ac 3c 00 74 0b 56 bb 07 00 b4 0e cd 10 5e eb f0 eb fe bf 05 00 bb 00 7c b8 01 02 57 cd 13 5f 73 0c 33 c0 cd 13 4f 75 ed be a3 06 eb d3 be c2 06 bf fe 7d 81 3d 55 aa 75 c7 8b f5 ea 00 7c 00 00 49 6e 76 61 6c 69 64 20 70 61 72 74 69 74 69 6f 6e 20 74 61 62 6c 65 00 45 72 72 6f 72 20 6c 6f 61 64 69 6e 67 20 6f 70 65 72 61 74 69 6e 67 20 73 79 73 74 65 6d 00 4d 69 73 73 69 6e 67 20 6f 70 65 72 61 74 69 6e 67 20 73 79 73 74 65 6d 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 01 01 00 01 01 20 f4 20 00 00 00 20 3d 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 55 aa
    ```
  - 这个程序我们先发出SCSI命令Test Unit Ready，用以观察设备是否处于Ready状态，然后我们读取LBA = 0的扇区，因为我们可以保证这个扇区是有内容的，肯定不会是全0，如果希望读取其它扇区，把相关语句该一下就可以了，在执行SCSI READ(10)命令时，有下面几点需要注意一下：
    > 1、我们在USB系列之七中的测试已经表明，在我的环境下，只有Host Adapter Number = 0和Target ID = 0是合法的，所以，程序中对这两项均没有填写，因为在初始化SRB后，本身所有项就都是0，如果在执行USB系列之七中的程序发现这个数据有变，请自行在程序中填写。

    > 2、注意一定要填写SCSI Request Flags字段，在程序中我们填为0ch，即bit2和bit3为1，其它为0，从ASPI的规范中可以看到，bit2为1表明剩余字符报告使能，bit3为1表明数据传输方向是从SCSI Target到Host，就是从U盘到主机的意思，这一位是一定要设的，否则无法成功完成读扇区的动作；bit2这一位如果不设，那么在完成读扇区的动作时，Data Allocation Length这个字段仍然是512（和我们调用ASPI前设置的一样），如果设置，这个字段在调用后一般会为0，表明我们所要求的512个字节都读出来了，没有剩余字符。

    > 3、执行完读操作后，Command Status = 01，表明动作已经完成，如果不是01，则buffer中的内容无效，可以从返回的Command Status以及Host Status和Target Status返回的状态上分析原因。

  - 下面说说aspiwrit这个程序
  - 这个程序首先要初始化一下buffer，把内容变成00--0ffh的两次循环，然后把buffer中的数据写到LBA = 50h的扇区中（注意：这里可能会破坏已有的文件数据，所以请使用一个空的U盘，或者没有存放有用数据的U盘），写完成后我们把buffer清为全0，然后把LBA = 50h的扇区读出，并显示读出的内容。
  - 以下是aspiwrit的执行结果：
    ```
    ASPI Reading test program v1.00.

    Open SCSIMRG success!    ASPI entry : 0786:6a5c

    Test unit ready......
    Command Status: 01    Host Adapter Status: 00
    Target Status: 00
    Press any key when ready......

    Writing.........
    Command Status: 01    Host Adapter Status: 00
    Data in buffer: 00    Residual Byte: 0000
    Press any key when ready......

    Reading.........
    Command Status: 01    Host Adapter Status: 00
    Target Status: 00    Residual Byte: 0000
    Data in buffer: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f 80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f 90 91 92 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 ca cb cc cd ce cf d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd de df e0 e1 e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f 80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f 90 91 92 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 ca cb cc cd ce cf d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd de df e0 e1 e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff
    ```
  - 在前面说明aspiread时已经说明的要点在这里就不再重复了。
    > 1、在执行SCSI WRITE(10)命令时，SRB中SCSI Request Flags字段在程序中我们填为14h，即bit2和bit4为1，其它为0，bit3和bit4组成二进制的10，从ASPI的规范中可以看出表明数据传输方向是从Host到SCSI Target，就是从主机到U盘的意思。
    > 2、在USB系列之三中，我们介绍过SCSI的一个重要规范SPC-3，并提供了下载，在这份规范中，有一个SCSI命令叫做REQUEST SENSE，当执行命令出错时，可以使用这个命令获得Device的Sense Data，进而判断错误情况，当然我们也可以透过ASPI执行这个SCSI命令，但似乎ASPI已经帮我们想到了这一点，这就是ASPI For DOS这个规范中SRB表中的Sense Allocation Area这个字段的意义，当执行SCSI命令出错时，你会在这里得到Sense Data，得到的数据长度小于等于Sense Allocation Length字段设置的值。

* 好了，掌握这些知识后，我们已经具备了使用ASPI编写一个U盘的设备驱动程序的能力，我们会在下一篇文章中涉及这个问题。

