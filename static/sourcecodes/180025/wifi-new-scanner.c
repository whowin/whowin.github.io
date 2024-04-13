/*
 * File: wifi-new-scanner.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * A program that uses ioctl to scan wifi signals
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall wifi-new-scanner.c -o wifi-new-scanner -lm
 * Usage: $ sudo ./wifi-new-scanner
 * 
 * Example source code for article 《使用ioctl扫描wifi信号获取信号属性的范例(二)》
 *
 */
#include <stdio.h>
#include <stdlib.h>             // for malloc
#include <string.h>
#include <unistd.h>             // to close the socket
#include <ctype.h>              // isspace()
#include <stdint.h>
#include <math.h>               // pow()
#include <ifaddrs.h>

#include <sys/socket.h>
#include <sys/ioctl.h>          // for the ioctl function
#include <sys/errno.h>          // for error

#include <linux/wireless.h>     // for the SIOCGIWNAME and the struct iwreq

//#define DEBUG                   // 打开注释会打印一些调试信息
#define MAC_ADDR_LEN            6                       // MAC地址的长度
#define PROC_NET_WIRELESS       "/proc/net/wireless"    // 无线网络接口文件

// AP 工作模式
const char *mode_str[] = {"AUTO", "ADHOC", "INFRA", "MASTER", "REPEAT", "SECOND", "MONITOR", "MESH"};
// 无线网络接口链表
struct wifs_chain {
    char wif_name[IFNAMSIZ + 1];    // 网络接口名称
    struct wifs_chain *next;        // 链表的后向指针
};
// event链表
struct events_chain {
    struct iw_event *event;         // event指针
    struct events_chain *next;      // 后向指针
};
// IE(Information Element)结构
struct ieee80211_ie {
    uint8_t eid;                    // Element ID
    uint8_t len;                    // length of ie_data field
    uint8_t ie_data[0];             // data of Information Element
};
// IE(Information Element)链表
struct ie_chain {
    struct ieee80211_ie *data;      // pointer of IE raw data
    struct ie_chain *next;          // backward pointer
};
// AP属性链表
struct aps_chain {
    int channel;                    // 占用信道
    float freq;                     // 工作频率
    char *essid;                    // essid
    uint8_t mac[MAC_ADDR_LEN];      // mac地址
    uint8_t quality;                // 信号质量
    uint8_t level;                  // 信号强度
    uint8_t noise;                  // 信号噪音
    uint32_t mode;                  // 操作模式
    int32_t bitrates[IW_MAX_BITRATES];      // 支持的速率
    uint16_t driver_str_len;        // 驱动提供的字符串长度
    char *driver_str;               // 驱动提供的字符串
    struct ie_chain *ie;            // IE链表
    struct aps_chain *next;         // 后向指针
};
// 事件数据为字符串(essid, driver_str etc.)时的数据格式
struct iw_string {
    uint16_t len;                   // 字符串的长度(不包括len、flags及对齐字符)
    uint16_t flags;
#ifdef __x86_64__
    char __attribute((aligned(8)))string;           // 字符串指针(64位机按8字节对齐)
#else
    char __attribute((aligned(4)))string;           // 字符串指针(32位机按4字节对齐)
#endif
};
// IE 事件的结构
struct iw_ie {
    uint16_t len;                   // 与struct iw_event一致
    uint16_t cmd;                   // 与struct iw_event一致
#ifdef __x86_64__
    uint16_t __attribute((aligned(8)))ie_len;           // IE数据总长度
    struct ieee80211_ie __attribute((aligned(8)))ie[0]; // IE数据
#else
    uint16_t __attribute((aligned(4)))ie_len;           // IE数据总长度
    struct ieee80211_ie __attribute((aligned(4)))ie[0]; // IE数据
#endif
};

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
/**************************************************************************
 * Function: void free_ie_chain(struct ie_chain *start)
 * Description: 释放IE链表
 **************************************************************************/
void free_ie_chain(struct ie_chain *start) {
    struct ie_chain *next;

    while (start) {
        next = start->next;
        free(start);
        start = next;
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
            if (curr_ap->driver_str) free(curr_ap->driver_str);
            if (curr_ap->essid) free(curr_ap->essid);
            if (curr_ap->ie) free_ie_chain(curr_ap->ie);
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
/***********************************************************************
 * Function: int get_bitrates_index(struct aps_chain *curr_ap)
 * Description: 获取速率数组中，第一个空位置的索引号
 ***********************************************************************/
int get_bitrates_index(struct aps_chain *curr_ap) {
    int i = 0;
    for (i = 0; i < IW_MAX_BITRATES; ++i) {
        if (curr_ap->bitrates[i] == 0) return i;
    }
    return 0;
}
/*********************************************************************
 * Function: struct ie_chain *get_last_ie(struct ie_chain *start)
 * Description: 获取IE链表中最后一个IE的指针
 *********************************************************************/
struct ie_chain *get_last_ie(struct ie_chain *start) {
    if (!start) return NULL;

    struct ie_chain *p = start;
    while (p) p = p->next;

    return p;
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
        // 在ap链表的最后添加一项
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
        // 初始化链表中新添加的项
        if (ret == 0) {
            memset(last_ap, 0, sizeof(struct aps_chain));       // 初始化 struct aps_chain
            last_ap->mode = -1;
            memcpy(last_ap->mac, (uint8_t *)event->u.ap_addr.sa_data, MAC_ADDR_LEN);    // MAC address
        }
    } else if (event->cmd == SIOCGIWESSID) {    // 8B1B = SIOCGIWESSID - get ESSID
        // 获取AP的ESSID
        if (last_ap != NULL && last_ap->mac != NULL) {
            struct iw_string *ap_essid = (struct iw_string *)&(event->u.data);

            if (ap_essid->len > 0) {
                last_ap->essid = malloc(ap_essid->len + 1);     // 为ESSID申请内存
                memset(last_ap->essid, 0, ap_essid->len + 1);
                memcpy(last_ap->essid, (char *)&ap_essid->string, ap_essid->len);
            }
            ret = 0;
        }
    } else if (event->cmd == SIOCGIWFREQ) {     // 8B05 = SIOCGIWFREQ - get channel/frequency (Hz)
        // 获取工作频率和工作信道
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
    } else if (event->cmd == IWEVQUAL) {        // 8C01 = IWEVQUAL - Quality part of statistics (scan)
        // 获取信号质量
        if (last_ap != NULL && last_ap->mac != NULL) {
            struct iw_quality *ap_quality = (struct iw_quality *)&(event->u.qual);
            if ((ap_quality->updated & IW_QUAL_QUAL_UPDATED) &&             // qual字段已经更新
                ((ap_quality->updated & IW_QUAL_QUAL_INVALID) == 0)) {      // qual字段内容有效
                last_ap->quality = ap_quality->qual;                        // 信号质量
            }
            if ((ap_quality->updated & IW_QUAL_LEVEL_UPDATED) &&            // level字段已经更新
                ((ap_quality->updated & IW_QUAL_LEVEL_INVALID) == 0)) {     // level字段内容有效
                last_ap->level = ap_quality->level;                         // 信号强度
            }
            if ((ap_quality->updated & IW_QUAL_NOISE_UPDATED) &&            // noise字段已经更新
                ((ap_quality->updated & IW_QUAL_NOISE_INVALID) == 0)) {     // noise字段内容有效
                last_ap->noise = ap_quality->noise;                         // 信号噪音
            } else if (ap_quality->updated & IW_QUAL_NOISE_INVALID) {
                //printf("Does not support **noise**.\n");
            }
            ret = 0;
        }
    } else if (event->cmd == SIOCGIWMODE) {     // 8B07 = SIOCGIWMODE - get operation mode
        // 获取工作模式
        if (last_ap != NULL && last_ap->mac != NULL) {
            last_ap->mode = event->u.mode;
            ret = 0;
        }
    } else if (event->cmd == SIOCGIWRATE) {     // 8B21 = SIOCGIWRATE - get default bit rates
        // 获取支持的传输速率
        if (last_ap != NULL && last_ap->mac != NULL) {
            int len = event->len - IW_EV_LCP_LEN;               // 数据长度
            struct iw_param *p = &event->u.bitrate;
            int rates_count = len / sizeof(struct iw_param);    // 包含的传输速率的数量
            int i = get_bitrates_index(last_ap);                // 得到速率数组中第1个空闲位置，当支持的速率较多时，
                                                                // 会收到多个SIOCGIWRATE事件，要避免后面收到的覆盖前面收到的数据
            int j = 0;
            while (j < rates_count) {
                if (i < IW_MAX_BITRATES) {                      // 数组的长度
                    last_ap->bitrates[i] = p[j].value;          // 传输速率
                    ++i, ++j;
                } else break;
            }

            ret = 0;
        }
    } else if (event->cmd == IWEVCUSTOM) {      // 8C02 = IWEVCUSTOM - Driver specific ascii string
        // 获取驱动程序提供的字符串
        if (last_ap != NULL && last_ap->mac != NULL) {
            struct iw_string *ap_driver_str = (struct iw_string *)&(event->u.data);
            int last_len = last_ap->driver_str_len;     // 以前字符串的长度
            int curr_len = ap_driver_str->len;          // 当前字符串的长度
            int len = last_len + curr_len + 1;          // 合并后字符串的长度(增加了一个\t)

            char *p = malloc(len);                      // 为合并两个字符串申请内存
            memset(p, 0, len);                          // 初始化内存
            memcpy(p, last_ap->driver_str, last_len);   // 将以前字符串搬过来
            memcpy((p + last_len), &ap_driver_str->string, curr_len);   // 将新字符串添加到以前字符串的后面
            *(p + last_len + curr_len) = '\t';          // 最后添加一个 tab
            free(last_ap->driver_str);                  // 释放以前字符串内存
            last_ap->driver_str = p;                    // 结构中的指针指向新的内存
            last_ap->driver_str_len = len;              // 字符串的长度

            ret = 0;
        }
    } else if (event->cmd == IWEVGENIE) {       // 8C05 = IWEVGENIE - Generic IE (WPA, RSN, WMM, ..)
        // 解析IE(Information Elements)
        if (last_ap != NULL && last_ap->mac != NULL) {
            struct iw_ie *ie_event = (struct iw_ie *)event;         // 使用专门为IE事件设置的结构，和比较方便
            int ie_tlen = ie_event->ie_len;                         // 所有IE的总长度
            struct ieee80211_ie *p = ie_event->ie;                  // 指向第1个IE

            struct ie_chain *last_ie = get_last_ie(last_ap->ie);    // 获取IE链表的最后一项
            if (last_ie == NULL) {
                // 表示IE链表为空，链表中没有项目
                last_ap->ie = malloc(sizeof(struct ie_chain));      // 为链表中添加的新项分配内存
                last_ie = last_ap->ie;                              // IE链表新添加项的指针
            } else {
                last_ie->next = malloc(sizeof(struct ie_chain));    // 为链表中添加的新项分配内存
                last_ie = last_ie->next;                            // IE链表新添加项的指针
            }
            last_ie->next = NULL;
            last_ie->data = p;                                      // 第1个IE的指针


            int ie_len = p->len;                                    // 第1个IE的长度
            while (ie_len < ie_tlen) {                              // 判断是否有下一个IE
                p = (struct ieee80211_ie *)((char *)p + (p->len + 2));  // 下一个IE的指针
                last_ie->next = malloc(sizeof(struct ie_chain));    // 为下一个IE分配内存
                last_ie = last_ie->next;                            // 指向新添加项
                last_ie->next = NULL;
                last_ie->data = p;
                ie_len += (p->len + 2);                             // 已加入到链表中的IE的总长度
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
            p->cmd == SIOCGIWFREQ ||    // get channel/frequency (Hz)       - 8B05
            p->cmd == IWEVQUAL ||       // get quality part of statistics   - 8c01
            p->cmd == SIOCGIWMODE ||    // get operation mode of the signal - 8b07
            p->cmd == SIOCGIWRATE ||    // get bit rates                    - 8B21
            p->cmd == IWEVCUSTOM ||     // Driver specific ascii string     - 8C02
            p->cmd == IWEVGENIE)        // Generic IE (WPA, RSN, WMM, ..)   - 8C05
        {
            // 仅在链表中存放我们需要解析的事件，其它事件数据则丢弃
#ifdef DEBUG
            if (p->cmd == IWEVGENIE) {
                printf("\nEvent No. %d\n", event_no);
                printf("Event length: %d\t\tEvent Command: %04x\n", p->len, p->cmd);
                for (int i = 0; i < p->len; ++i) {
                    if (i % 16 == 0) printf("\n");
                    printf("%02X ", (uint8_t)*((char *)p + i));
                }
                puts("");
            }
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
        } else {
#ifdef DEBUG
            printf("\nEvent No. %d\n", event_no);
            printf("Event length: %d\t\tEvent Command: %04x\n", p->len, p->cmd);
            for (int i = 0; i < p->len; ++i) {
                if (i % 16 == 0) printf("\n");
                printf("%02X ", (uint8_t)*((char *)p + i));
            }
            puts("");
#endif
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
            sleep(1);
            goto GET_AGAIN;
        }
        if (errno == E2BIG) {   // 7 - Argument list too long
            // buffer不够大
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
#if 0
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
#endif
/*********************************************************************
 * Function: int get_wifname(char *name, int nsize, char *buf)
 * Description: 从buf中获取无线网络接口名称并存到name中
 * 
 * Entry:   name    存放无线网络接口名称的缓冲区
 *          nsize   name缓冲区的长度
 *          buf     原始数据指针
 * Return:  0       success
 *          -1      failure
 *********************************************************************/
int get_wifname(char *name, int nsize, char *buf) {
    char *end;

    // Skip leading spaces
    while (isspace(*buf)) buf++;

    end = strstr(buf, ": ");

    // Not found
    if ((end == NULL) || (((end - buf) + 1) > nsize)) {
        return -1;
    }

    // Copy
    memcpy(name, buf, (end - buf));
    name[end - buf] = '\0';

    return 0;
}
/***********************************************************************************
 * Function: struct wifs_chain *get_wireless_interface()
 * Description: 从/proc/net/wireless文件中获得无线网络接口
 * 
 * Entry:   NONE
 * Return:  返回无线网络接口链表的首指针
 *          如果没有无线网络接口则返回NULL
 ***********************************************************************************/
struct wifs_chain *get_wireless_interface() {
    FILE *fh;
    char buff[1024];

    struct wifs_chain *wifs_start_pointer = NULL;   // 无线网络接口链表的首指针
    struct wifs_chain *curr_wif = NULL;             // 当前无线网络接口指针

    fh = fopen(PROC_NET_WIRELESS, "r");             // 打开文件/proc/net/wireless
    if (fh != NULL) {
        // 从文件中读取数据

        // 跳过前两行(表格头)
        fgets(buff, sizeof(buff), fh);
        fgets(buff, sizeof(buff), fh);

        // 一次读一行，每行一个设备
        while (fgets(buff, sizeof(buff), fh)) {
            char name[IFNAMSIZ + 1];

            // 跳过空行，有时fgets()会返回只有换行符的空行
            if ((buff[0] == '\0') || (buff[1] == '\0'))
                continue;

            // 解析出接口名称
            if (get_wifname(name, sizeof(name), buff) == 0) {
                printf("The wireless interface name is %s\n", name);
                if (curr_wif == NULL) {         // 无线网络接口链表为空链表
                    curr_wif = (struct wifs_chain *)malloc(sizeof(struct wifs_chain));          // 为链表中的新项分配内存
                    if (curr_wif == NULL) {
                        break;
                    }
                    wifs_start_pointer = curr_wif;
                } else {
                    curr_wif->next = (struct wifs_chain *)malloc(sizeof(struct wifs_chain));    // 为链表中的新项分配内存
                    curr_wif = curr_wif->next;
                    if (curr_wif == NULL) {
                        break;
                    }
                }
                curr_wif->next = NULL;
                strcpy(curr_wif->wif_name, name);       // 将无线网络接口名称放到链表中
            } else {
                printf("Can't parse the file %s\n", PROC_NET_WIRELESS);
            }
        }

        fclose(fh);
    } else {
        printf("Can't open file %s\n", PROC_NET_WIRELESS);
    }

    if (curr_wif == NULL) {
        // 没有无线网络接口
        free_wifs_chain(wifs_start_pointer);
        wifs_start_pointer = NULL;
    }

    return wifs_start_pointer;
}
/*********************************************************************
 * main
 *********************************************************************/
int main() {
    int sockfd = 0;                             // 用于ioctl
    char *buffer = NULL;                        // 用于存储扫描AP返回的结果
    struct iwreq wreq;                          // ioctl的第三个参数，用于设置ioctl参数
    struct events_chain *events = NULL;         // event链表

    // Step 01: 获取无线网络接口
    //=========================
    struct wifs_chain *wifs = NULL;                     // 初始化无线网络接口链表
    struct wifs_chain *wifs_start_pointer = NULL;       // 初始化无线网络接口链表首指针     
    wifs_start_pointer = get_wireless_interface();      // 获取无线网络接口
    if (wifs_start_pointer == NULL) {
        printf("\nThere is no any wireless interface. Terminated.\n");
        return -1;
    }
    // 至此，wifi_ifs_start_point指向的链表中存储着所有的无线接口(通常只有一个)
    printf("There are %d wireless interfaces. ", wifs_count(wifs_start_pointer));
    // Step 02: 建立 socket
    //======================
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket(main)");
        free_wifs_chain(wifs_start_pointer);
        return -1;
    }

    printf("\n---- Scan wifi signals ----\n");
    // 遍历无线接口链表
    for (wifs = wifs_start_pointer; wifs != NULL; wifs = wifs->next) {
        printf("Wireless interface name: %s\n", wifs->wif_name);

        // 对无线接口进行扫描后，会返回很多AP，每个AP的每个属性(mac, essid, frequency等)都会以一个event返回，
        // 所以每个AP会返回多个event，首先获取所有的event，放在链表events中(get_all_ap_events())，
        // 然后从events中提取出所有的AP属性(extract_ap_attributes)

        // Step 03: 通过当前无线接口扫描所有的AP
        //====================================
        if (scan_all_ap(sockfd, wifs->wif_name, &wreq) == -1) {
            printf("Error occurs when starting to scan APs.\n");
            break;
        }
#ifdef DEBUG
        printf("Successfully started to scan wireless interface.\n");
#endif
        sleep(1);

        // Step 04: 获取扫描返回的event stream
        //====================================
        buffer = get_scanning_result(sockfd, &wreq);
        if (buffer == NULL) {
            printf("Error occurs when getting scanning result.\n");
            break;
        }
#ifdef DEBUG
        printf("Successfully got the scanning result.\n");
#endif

        // Step 05: 分析event stream，将我们所需的event存在链表中
        //====================================================
        events = extract_events(&wreq);
        if (events == NULL) {
            printf("Error occurs when extracting events.\n");
            break;
        }
#ifdef DEBUG
        printf("Successfully extracted the events.\n");
#endif

        // Step 06: 从event链表中提取AP的所有属性
        //=======================================
        struct events_chain *curr_event = NULL;      // event链表
        // 为AP链表的第一项申请内存并初始化
        struct aps_chain *aps = NULL;
        for (curr_event = events; curr_event != NULL; curr_event = curr_event->next) {
            extract_ap_attributes(curr_event->event, &aps);  // 从event中提取wifi信号属性
        }

        // Step 07: 显示wifi信号属性
        //============================
        if (aps->mac != NULL) {
            printf("\n---- WIFI signal list ----");
            struct aps_chain *curr_ap = aps;
            while (curr_ap != NULL) {
                printf("\nESSID: %s\n", (curr_ap->essid?curr_ap->essid:"(Hidden ESSID)"));
                printf("Channel: %d\n", curr_ap->channel);
                printf("Frequency: %.3f GHz\n", curr_ap->freq);
                printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                        curr_ap->mac[0], curr_ap->mac[1], curr_ap->mac[2], 
                        curr_ap->mac[3], curr_ap->mac[4], curr_ap->mac[5]);
                if (curr_ap->quality != 0) printf("Signal quality: %d/70", curr_ap->quality);
                if (curr_ap->level != 0)printf("\t\tSignal level: %d dBm", (int8_t)curr_ap->level);
                if (curr_ap->noise != 0)printf("\t\tSignal noise: %d dBm", (int8_t)curr_ap->noise);
                puts("");
                if (curr_ap->mode >= 0 && curr_ap->mode <= IW_MODE_MESH) {
                    printf("Operation mode: %s\n", mode_str[curr_ap->mode]);
                } else {
                    printf("Operation mode: %d\n", curr_ap->mode);
                }

                printf("Bit rates: ");
                int i = 0;
                while (i < IW_MAX_BITRATES && curr_ap->bitrates[i] > 0) {
                    printf("%c %d Mb/s", (i == 0)?' ':';', curr_ap->bitrates[i]/1000000);
                    ++i;
                }
                puts("");
                // 驱动程序提供的字符串                
                if (curr_ap->driver_str_len > 0) {
                    for (int i = 0; i < curr_ap->driver_str_len; ++i) {
                        putchar(curr_ap->driver_str[i]);
                    }
                    puts("");
                }
                // Information Elements
                printf("IEs: \n");
                struct ie_chain *p = curr_ap->ie;
                while (p) {
                    uint8_t len = p->data->len;
                    uint8_t eid = p->data->eid;

                    if (eid == 0x00 && len > 0) {
                        // ssid
                        printf("SSID: ");
                        char *ssid = (char *)p->data->ie_data;
                        for (i = 0; i < len; ++i) {
                            putchar(ssid[i]);
                        }
                        puts("");
                    } else if (eid == 0x01) {
                        // bit rates
                        printf("Bit rates: ");
                        uint8_t *rates = (uint8_t *)p->data->ie_data;
                        for (i = 0; i < len; ++i) {
                            if (i > 0) printf("; ");
                            printf("%d Mb/s", (rates[i] & 0x7f) / 2);
                        }
                        puts("");
                    } else {
#ifdef DEBUG
                        uint8_t *data = (uint8_t *)p->data;
                        for (i = 0; i < len + 2; ++i) printf("%02x ", data[i]);
#endif
                    }
                    
                    p = p->next;
                }
                puts("");

                curr_ap = curr_ap->next;
            }
        }
        // Step 08: 释放内存
        //===================
        free_events_chain(events);      // 释放event链表
        events = NULL;
        free_aps_chain(aps);            // 释放ap链表
        aps = NULL;
        if (buffer != NULL) {
            free(buffer);
            buffer = NULL;
        }
    }

    free_wifs_chain(wifs_start_pointer);    // 释放无线网络接口链表
    if (buffer) free(buffer);
    if (sockfd > 0) close(sockfd);

    printf("\nEnd of scanning\n");
    return 0;
}
