# BPB(BIOS Parameter Block)  私人日志
**2008-09-26 16:18:16**

  ```
    该结构如下：

    偏移  大小      描述
    ----------------------------------------------------------------
     00h  word      每扇区字节数
     02h  byte      每簇扇区数
     03h  word      扇区起始处保留的扇区数
     05h  byte      FAT数
     06h  word      根目录中的最大项数
     08h  word      扇区总数，如为0，则偏移15h处的dword为扇区总数
     0ah  byte      介质ID字节
     0bh  word      每FAT的扇区数
     0dh  word      每磁道的扇区数
     0fh  word      磁头数
     11h  dword     隐藏的扇区数
     15h  dword     扇区总数
     19h  6bytes    未知
     1fh  word      磁道柱面数
     21h  byte      设备类型
     22h  word      设备属性
  ```
 