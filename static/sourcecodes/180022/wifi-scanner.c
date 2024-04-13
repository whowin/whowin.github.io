/*
 * File: wifi-scanner.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * A program that uses ioctl to scan wifi signals
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall wifi-scanner.c -o wifi-scanner -lm
 * Usage: $ sudo ./wifi-scanner
 * 
 * Example source code for article 《使用ioctl扫描wifi信号获取信号属性的范例(一)》
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>             // for malloc
#include <unistd.h>             // to close the socket
#include <math.h>               // pow()
#include <ifaddrs.h>

#include <sys/socket.h>
#include <sys/ioctl.h>          // for the ioctl function
#include <sys/errno.h>          // for error

#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/wireless.h>     // for the SIOCGIWNAME and the struct iwreq

//#define DEBUG                   // 打开注释会打印一些调试信息

#define MAC_ADDR_LEN        6   // MAC地址的长度

// 无线网络接口链表
struct wifs_chain {
    struct ifaddrs *wif;        // 无线网络接口指针
    struct wifs_chain *next;    // 后向指针
};
// event链表
struct events_chain {
    struct iw_event *event;     // event指针
    struct events_chain *next;  // 后向指针
};
// AP属性链表
struct aps_chain {
    int channel;                // 占用信道
    float freq;                 // 工作频率
    uint16_t essid_len;         // essid的长度
    char essid[IW_ESSID_MAX_SIZE];  // essid
    uint8_t mac[MAC_ADDR_LEN];  // mac地址
    struct aps_chain *next;     // 后向指针
};
// 事件数据为essid时的数据格式
struct iw_essid {
    uint16_t len;               // essid的长度
    uint16_t flags;
#ifdef __x86_64__
    char __attribute((aligned(8)))essid;                // essid指针
#else
    char __attribute((aligned(4)))essid;                // essid指针
#endif
};
/************************************************************************
 * Function: int ifs_count(struct ifaddrs *ifs_start_pointer)
 * Description: 从网络接口链表中计算本机网络接口(Interface)数量
 * 
 * Entry:   ifs_start_pointer   使用getifaddrs()获得的网络接口链表
 * Return:  网络接口数量
 ************************************************************************/
int ifs_count(struct ifaddrs *ifs_start_pointer) {
    int i = 0;
    struct ifaddrs *ifa = NULL;

    for (ifa = ifs_start_pointer; ifa != NULL; ifa = ifa->ifa_next){
        i++;
    }
    return i;
}
/************************************************************************
 * Function: int wifs_count(struct wifs_chain *wifs_start_pointer)
 * Description: 从无线网络接口链表中计算本机无线网络接口(Wireless Interface)数量
 * 
 * Entry:   wifs_start_pointer   无线网络接口链表
 * Return:  无线网络接口数量
 ************************************************************************/
int wifs_count(struct wifs_chain *wifs_start_pointer) {
    int i = 0;
    struct wifs_chain *curr_wif = NULL;

    for (curr_wif = wifs_start_pointer; curr_wif != NULL; curr_wif = curr_wif->next){
        i++;
    }
    return i;
}
/***************************************************************************
 * Function: void free_wifs_chain(struct wifs_chain *wifs)
 * Description: 释放无线网络接口链表
 ***************************************************************************/
void free_wifs_chain(struct wifs_chain *wifs) {
    struct wifs_chain *curr_wif;

    if (wifs != NULL) {
        curr_wif = wifs;
        while (curr_wif) {
            struct wifs_chain *next_wif = curr_wif->next;
            free(curr_wif);
            curr_wif = next_wif;
        }
    }
}
/***************************************************************************
 * Function: void free_events_chain(struct events_chain *events_list)
 * Description: 释放event链表
 ***************************************************************************/
void free_events_chain(struct events_chain *events_list) {
    struct events_chain *curr_event;

    if (events_list != NULL) {
        curr_event = events_list;
        while (curr_event) {
            struct events_chain *next_event = curr_event->next;
            free(curr_event);
            curr_event = next_event;
        }
    }
}
/***************************************************************************
 * Function: void free_aps_chain(struct aps_chain *aps)
 * Description: 释放AP属性链表
 ***************************************************************************/
void free_aps_chain(struct aps_chain *aps) {
    struct aps_chain *curr_ap;

    if (aps != NULL) {
        curr_ap = aps;
        while (curr_ap) {
            struct aps_chain *next_ap = curr_ap->next;
            free(curr_ap);
            curr_ap = next_ap;
        }
    }
}
/************************************************************************
 * Function: struct aps_chain *get_last_ap(struct aps_chain *aps)
 * Description: 获取ap链表中的最后一项
 * 
 * Entry:   aps         AP链表的首指针
 * Return:  成功        AP链表最后一项的指针
 *          失败        NULL
 ************************************************************************/
struct aps_chain *get_last_ap(struct aps_chain *aps) {
    if (aps == NULL) return NULL;

    struct aps_chain *curr_ap = aps;
    while (curr_ap->next != NULL) {
        curr_ap = curr_ap->next;
    }

    return curr_ap;
}
/******************************************************************************************
 * Function: int extract_ap_attributes(struct iw_event *event, struct aps_chain *last_ap)
 * Description: 从一个event提取出AP属性，本例只处理AP的MAC地址、ESSID、信道和工作频率四个属性
 *              event只有三种：SIOCGIWAP、SIOCGIWESSID
 * 
 * Entry:   event       当前要处理event的指针
 *          last_ap     ap链表的最后一项的指针
 * return:  0           成功从event中提取到ap属性，并存储在ap链表中
 *          -1          失败
 ******************************************************************************************/
int extract_ap_attributes(struct iw_event *event, struct aps_chain **aps_p) {
    int ret = -1;

    if (event->len < IW_EV_LCP_PK_LEN) {    // IW_EV_LCP_PK_LEN定义在wireless.h中，表示event数据流中结构头的长度
        return ret;
    }
    
    struct aps_chain *last_ap = get_last_ap(*aps_p);       // 找到AP链表的最后一项
    if (event->cmd == SIOCGIWAP) {     // 8B15 = SIOCGIWAP - get access point MAC addresses
        // MAC地址是一个AP返回的events中的第一个，在这里应该在链表的最后增加一项
        // 获取AP的MAC地址
        ret = 0;
        if (last_ap == NULL) {
            last_ap = (struct aps_chain *)malloc(sizeof(struct aps_chain));
            if (last_ap == NULL) {
                ret = -1;
            } else {
                *aps_p = last_ap;
            }
        } else {
            last_ap->next = (struct aps_chain *)malloc(sizeof(struct aps_chain));
            if (last_ap->next == NULL) {
                ret = -1;
            } else {
                last_ap = last_ap->next;
            }
        }
        if (ret == 0) {
            memset(last_ap, 0, sizeof(struct aps_chain));
            memcpy(last_ap->mac, event->u.ap_addr.sa_data, MAC_ADDR_LEN);
        }
    } else if (event->cmd == SIOCGIWESSID) {    // 8B1B = SIOCGIWESSID - get ESSID
        // 获取AP的ESSID
        if (last_ap != NULL && last_ap->mac != NULL) {
            struct iw_essid *ap_essid = (struct iw_essid *)&(event->u.data);
            last_ap->essid_len = ap_essid->len;
            strncpy(last_ap->essid, &ap_essid->essid, ap_essid->len);
            ret = 0;
        }
    } else if (event->cmd == SIOCGIWFREQ) {     // 8B05 = SIOCGIWFREQ - get channel/frequency (Hz)
        if (last_ap != NULL && last_ap->mac != NULL) {
            struct iw_freq *ap_freq = (struct iw_freq *)&(event->u.freq);
            double freq = (double)ap_freq->m * pow(10, ap_freq->e);
            if (freq > 1000) {
                // ap的工作频率
                last_ap->freq = (float)freq / (1e9);
            } else {
                // AP的channel
                last_ap->channel = freq;
            }
            ret = 0;
        }
    }
    return ret;
}
/******************************************************************************************
 * Function: struct events_chain *extract_events(struct iwreq *wreq, char *buffer)
 * Description: 从扫描返回结果中提取需要的event，形成一个链表，供下一步使用
 * 
 * Entry:   wreq            指向struct iwreq的指针，里面有返回结果的信息
 *          buffer
 * Return:  指针            成功时，返回指向struct events_chain的指针
 *          NULL            失败
 *****************************************************************************************/
struct events_chain *extract_events(struct iwreq *wreq) {
    if (wreq->u.data.length <= 0) {                  // 扫描结果的数据长度
        return NULL;
    }

    int data_len = wreq->u.data.length;     // 扫描结果中尚未处理的数据长度
    uint32_t event_no = 1;                  // event编号
    char *current = wreq->u.data.pointer;
    struct iw_event *p = (struct iw_event *)current;     // p指向当前event
    
    struct events_chain *events = NULL;
    struct events_chain *curr_event = NULL;

    while (data_len > 0) {
        if (p->cmd == SIOCGIWAP ||      // get access point MAC addresses   - 8B15
            p->cmd == SIOCGIWESSID ||   // get ESSID                        - 8B1B
            p->cmd == SIOCGIWFREQ) {    // get channel/frequency (Hz)       - 8B05
#ifdef DEBUG
            printf("\nEvent No. %d\n", event_no);
            printf("Event length: %d\t\tEvent Command: %04x\n", p->len, p->cmd);
            for (int i = 0; i < p->len; ++i) {
                if (i % 16 == 0) printf("\n");
                printf("%02X ", (uint8_t)*((char *)p + i));
            }
            puts("");
#endif
            if (curr_event == NULL) {
                // event链表的第一项
                curr_event = (struct events_chain *)malloc(sizeof(struct events_chain));
                events = curr_event;           // 链表首指针
                curr_event->event = p;
                curr_event->next = NULL;
            } else {
                // event链表中已有其它元素
                curr_event->next = (struct events_chain *)malloc(sizeof(struct events_chain));
                curr_event = curr_event->next;
                curr_event->event = p;
                curr_event->next = NULL;
            }
        }
        data_len -= p->len;
        current += p->len;
        p = (struct iw_event *)current;
        event_no++;
    }
    printf("The data length of scanning result: %d, Total %d events.\n", 
            wreq->u.data.length, event_no);

    return events;
}
/******************************************************************************************
 * Function: int get_scanning_result(int sockfd, struct iwreq *wreq)
 * Description: 获取扫描结果，结果是一个event stream，将其存储在*p指向的内存中
 * 
 * Entry:   sockfd          用于ioctl的socket
 *          wreq            指向struct iwreq的指针，用于设置ioctl参数
 * return:  指针            成功时，返回存储返回结果的内存指针
 *          NULL            失败        
 ******************************************************************************************/
char *get_scanning_result(int sockfd, struct iwreq *wreq) {
    char *buffer = NULL;                            // 用于存储wifi扫描返回的stream
    uint32_t buflen = IW_SCAN_MAX_DATA;             // 申请的buffer的内存大小
    int counter = 0;    // 调用ioctl时出现errno=E2BIG的次数，每出现一次，buffer的大小增加IW_SCAN_MAX_DATA

    REALLOC_MEM:
    // 为buffer分配内存
    // wifi扫描返回的stream有时会非常大，如果分配的内存不够，ioctl会返回错误，如果error表明buffer不够，
    // 则需增加buffer的尺寸，并再次调用ioctl获取扫描结果
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
    buflen = IW_SCAN_MAX_DATA * (counter + 1);
    buffer = (char *)malloc(buflen);
    if (buffer == NULL) {
        printf("Can't allocate enough memory for scanning result.\n");
        return NULL;
    }

    GET_AGAIN:
    // 在调用ioctl获取扫描结果前，需要初始化wreq，件下面三个字段
    wreq->u.data.pointer = buffer;   // buffer指针，返回结果将存储在指针指向的内存
    wreq->u.data.length = buflen;    // buffer的长度
    wreq->u.data.flags = 0;          // falgs=0，调用成功后falgs=1
    if (ioctl(sockfd, SIOCGIWSCAN, wreq) == -1) {      // 获取wifi扫描结果
#ifdef DEBUG
        perror("ioctl(SIOCGIWSCAN)");
        printf("errno=%d\n", errno);
#endif
        if (errno == EAGAIN) {  // 11 - Resource temporarily unavailable
            // 扫描还没有完成
            sleep(2);
            goto GET_AGAIN;
        }
        if (errno == E2BIG) {   // 7 - Argument list too long
            // buffer不够大
            printf("Data length: %d\n", wreq->u.data.length);
            counter++;
            goto REALLOC_MEM;
        }
        // ioctl调用失败
        if (buffer) {
            free(buffer);
            buffer = NULL;
        }
        return NULL;
    }
    // IOCTL成功
    return buffer;
}
/***************************************************************************
 * Function: int scan_all_ap(int sockfd, char *ifname, struct iwreq *wreq)
 * Description: 向当前无线接口(ifname)发出扫描指令
 * 
 * Entry:   sockfd          用于ioctl的socket
 *          ifname          当前无线接口的名称
 *          wreq            指向struct iwreq的指针，用于设置ioctl参数
 * Return:  0               成功
 *          -1              失败
 ***************************************************************************/
int scan_all_ap(int sockfd, char *ifname, struct iwreq *wreq) {
    memset((char *)wreq, 0, sizeof(struct iwreq));  // 初始化wreq

    // 启动扫描之前要先初始化wreq，如下四个字段
    strncpy(wreq->ifr_name, ifname, IFNAMSIZ);
    wreq->u.data.pointer = NULL;
    wreq->u.data.flags = 0;
    wreq->u.data.length = 0;
    if (ioctl(sockfd, SIOCSIWSCAN, wreq) == -1) {      // 启动wifi扫描
        perror("ioctl(SIOCSIWSCAN)");
        return -1;
    }
    return 0;
}
/***********************************************************************
 * Function: int is_wireless(char *if_name)
 * Description: 当前网络接口是否为无线网络接口
 * 
 * Entry:   if_name     接口名称
 * Return:  1           是无线网络接口
 *          0           不是无线网络接口
 *          <0          发生错误
 ***********************************************************************/
int is_wireless(char *if_name) {
    int sock;
    struct iwreq wreq;
    int ret = 0;

    memset(&wreq, 0, sizeof(wreq));
    strncpy(wreq.ifr_name, if_name, IFNAMSIZ);      // 接口名称
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Can't create socket for this domain!");
        return -1;
    }

    if (ioctl(sock, SIOCGIWNAME, &wreq) == 0) {
        printf("\nThe [%s] is a wireless interface. The protocol is %s\n", if_name, wreq.u.name);
        ret = 1;
    }

    close(sock);
    return ret;
}
/****************************************************************************************************
 * Function: struct wifs_chain *get_wireless_interface(struct ifaddrs *ifs_start_pointer)
 * Description: 从网络接口链表中找出无线网络接口
 * 
 * Entry:   ifs_start_pointer   指向网络接口链表的起始位置
 * Return:  返回无线网络接口链表的首指针
 *          如果没有无线网络接口则返回NULL
 ****************************************************************************************************/
struct wifs_chain *get_wireless_interface(struct ifaddrs *ifs_start_pointer) {
    struct wifs_chain *wifs_start_pointer = NULL;   // 无线网络接口链表的首指针
    struct wifs_chain *curr_wif = NULL;             // 当前无线网络接口指针

    struct ifaddrs *ifa;        // 当前网络接口指针

    for (ifa = ifs_start_pointer; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != NULL) {                // 接口的地址 struct sockaddr
            // 过滤掉除ipv4以外的其它interface
            if (ifa->ifa_addr->sa_family == AF_INET) {
                // ipv4 - 本例只处理IPv4的无线网络接口
                struct sockaddr_in *sock_addr = (struct sockaddr_in *)ifa->ifa_addr;
                char *ip = inet_ntoa(sock_addr->sin_addr);
                if (is_wireless(ifa->ifa_name)) {   // interface name
                    // 当前接口是无线网卡
                    printf("IP Address: %s\n\n", ip);

                    if (curr_wif == NULL) {
                        curr_wif = (struct wifs_chain *)malloc(sizeof(struct wifs_chain));
                        if (curr_wif == NULL) {
                            break;
                        }
                        wifs_start_pointer = curr_wif;
                    } else {
                        curr_wif->next = (struct wifs_chain *)malloc(sizeof(struct wifs_chain));
                        curr_wif = curr_wif->next;
                        if (curr_wif == NULL) {
                            break;
                        }
                    }
                    curr_wif->next = NULL;
                    curr_wif->wif = ifa;
                }
            }
        }
    }
    if (curr_wif == NULL) {
        free_wifs_chain(wifs_start_pointer);
        wifs_start_pointer = NULL;
    }

    return wifs_start_pointer;
}
/*********************************************************************
 * main
 *********************************************************************/
int main() {

    // Step 01: 获取所有的网络接口，结果在interfaces_start_pointer指向的链表中
    //===================================================================
    struct ifaddrs *ifs_start_pointer = NULL;           // 网络接口链表
    if (getifaddrs(&ifs_start_pointer) == -1) {
        perror("can't get local address\n");
        return -1;
    }

    // Step 02: 从网络接口中找到无线网卡
    //=================================
    struct wifs_chain *wifs = NULL;
    struct wifs_chain *wifs_start_pointer = NULL;       // 无线网络接口链表     
    wifs_start_pointer = get_wireless_interface(ifs_start_pointer);

    printf("There are %d interfaces. ", ifs_count(ifs_start_pointer));
    printf("%d of them are wireless.\n", wifs_count(wifs_start_pointer));
    // 至此，wifi_ifs_start_point指向的链表中存储着所有的无线接口(通常只有一个)
    if (wifs_start_pointer == NULL) {
        printf("\nThere is no any wireless interface. Terminated.\n");
        freeifaddrs(ifs_start_pointer);
        return -1;
    }

    // Step 03: 建立一个socket
    //========================
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);   // 建立一个socket用于ioctl
    if (sockfd == -1) {
        perror("socket(main)");
        free_wifs_chain(wifs_start_pointer);
        freeifaddrs(ifs_start_pointer);
        return -1;
    }

    char *buffer = NULL;                        // 用于存储扫描AP返回的结果
    struct iwreq wreq;                          // ioctl的第三个参数，用于设置ioctl参数或接收ioctl返回结果
    struct events_chain *events = NULL;         // event链表

    printf("\n---- Scan wifi signals ----\n");
    // 遍历无线接口链表
    for (wifs = wifs_start_pointer; wifs != NULL; wifs = wifs->next) {
        printf("Wireless interface name: %s\n", wifs->wif->ifa_name);

        // 对无线接口进行扫描后，会返回很多AP，每个AP的每个属性(mac, essid, frequency等)都会以一个event返回，
        // 所以每个AP会返回多个event，这里只处理了三种event: SIOCGIWAP, SIOCGIWFREQ, SIOCGIWESSID
        // 首先获取所有的event，放在链表events中(get_all_ap_events())，
        // 然后从events中提取出所有的AP属性(extract_ap_attributes)

        // Step 04: 通过当前无线接口扫描所有的AP
        //====================================
        if (scan_all_ap(sockfd, wifs->wif->ifa_name, &wreq) == -1) {
            printf("Error occurs when starting to scan APs.\n");
            break;
        }
#ifdef DEBUG
        printf("Successfully started to scan wireless interface.\n");
#endif
        sleep(5);

        // Step 05: 获取扫描返回的event stream
        //====================================
        buffer = get_scanning_result(sockfd, &wreq);
        if (buffer == NULL) {
            printf("Error occurs when getting scanning result.\n");
            break;
        }
#ifdef DEBUG
        printf("Successfully got the scanning result.\n");
#endif

        // Step 06: 分析event stream，将我们所需的event存在链表中
        //====================================================
        events = extract_events(&wreq);
        if (events == NULL) {
            printf("Error occurs when extracting events.\n");
            break;
        }
#ifdef DEBUG
        printf("Successfully extracted the events.\n");
#endif

        // Step 07: 从event链表中提取AP的所有属性
        //=======================================
        struct events_chain *curr_event = NULL;      // event链表
        // 为AP链表的第一项申请内存并初始化
        struct aps_chain *aps = NULL;
        for (curr_event = events; curr_event != NULL; curr_event = curr_event->next) {
            extract_ap_attributes(curr_event->event, &aps);  // 从event中提取wifi信号属性
        }

        // Step 08: 显示wifi信号属性
        //============================
        if (aps->mac != NULL) {
            printf("\n---- WIFI signal list ----");
            struct aps_chain *curr_ap = aps;
            while (curr_ap != NULL) {
                //printf("\nESSID: %s\n", curr_ap->essid);
                printf("\nESSID: ");
                for (int i = 0; i < curr_ap->essid_len; ++i) {
                    putchar(curr_ap->essid[i]);
                }
                if (curr_ap->essid_len == 0) printf("\t(Hidden ESSID)");
                printf("\nChannel: %d\n", curr_ap->channel);
                printf("Frequency: %.3f GHz\n", curr_ap->freq);
                printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                        curr_ap->mac[0], curr_ap->mac[1], curr_ap->mac[2], 
                        curr_ap->mac[3], curr_ap->mac[4], curr_ap->mac[5]);
                curr_ap = curr_ap->next;
            }
        }
        // Step 09: 释放内存
        //===================
        free_events_chain(events);     // 释放event链表
        events = NULL;
        free_aps_chain(aps);                // 释放ap链表
        aps = NULL;
        if (buffer != NULL) {
            free(buffer);
            buffer = NULL;
        }
    }

    free_wifs_chain(wifs_start_pointer);
    if (buffer) free(buffer);
    if (sockfd > 0) close(sockfd);
    if (ifs_start_pointer) freeifaddrs(ifs_start_pointer);

    printf("\nEnd of scanning\n");
    return 0;
}
