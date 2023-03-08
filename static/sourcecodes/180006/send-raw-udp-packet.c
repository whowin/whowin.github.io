/*
 * File: send-raw-udp-packet.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * how to send raw UDP packet with raw socket under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall send-raw-udp-packet.c -o send-raw-udp-packet
 * Usage: $ sudo ./send-raw-udp-packet
 * 
 * Example source code for article 《如何使用raw socket发送UDP报文》
 * https://whowin.gitee.io/post/blog/network/0006-send-udp-with-raw-socket/
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/udp.h>
#include <linux/if_packet.h>

#define GATEWAY         1

#define IF_NAME         "enp0s3"             // name of local eth interface
#define DEST_IP         "192.168.2.112"      // destination ip

#if GATEWAY
    #define DEST_MAC_0      0xdc
    #define DEST_MAC_1      0xfe
    #define DEST_MAC_2      0x18
    #define DEST_MAC_3      0x68
    #define DEST_MAC_4      0x73
    #define DEST_MAC_5      0x7e
#else
    #define DEST_MAC_0      0x84
    #define DEST_MAC_1      0x3a
    #define DEST_MAC_2      0x4b
    #define DEST_MAC_3      0x35
    #define DEST_MAC_4      0x40
    #define DEST_MAC_5      0xf4
#endif

int get_eth_index(int sock_raw) {
    struct ifreq if_req;
    memset(&if_req, 0, sizeof(struct ifreq));
    strncpy(if_req.ifr_name, IF_NAME, IFNAMSIZ - 1);

    if ((ioctl(sock_raw, SIOCGIFINDEX, &if_req)) < 0) {
        printf("error in SIOCGIFINDEX ioctl reading.\n");
        return -1;
    }
    printf("Interface Name: %s\tInterface Index=%d\n", IF_NAME, if_req.ifr_ifindex);
    return if_req.ifr_ifindex;
}

int get_mac(int sock_raw, unsigned char *mac) {
    struct ifreq if_req;
    memset(&if_req, 0, sizeof(struct ifreq));
    strncpy(if_req.ifr_name, IF_NAME, IFNAMSIZ - 1);

    if ((ioctl(sock_raw, SIOCGIFHWADDR, &if_req)) < 0) { 
        printf("error in SIOCGIFHWADDR ioctl reading.\n");
        return -1;
    }
    int i;
    for (i = 0; i < 6; ++i) mac[i] = (unsigned char)(if_req.ifr_hwaddr.sa_data[i]);

    printf("Mac = %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
    return 0;
}

int get_ip(int sock_raw, char *ip) {
    struct ifreq if_req;
    memset(&if_req, 0, sizeof(struct ifreq));
    strncpy(if_req.ifr_name, IF_NAME, IFNAMSIZ - 1);
    if (ioctl(sock_raw, SIOCGIFADDR, &if_req) < 0) {
        printf("error in SIOCGIFADDR.\n");
        return -1;
    }

    strcpy(ip, inet_ntoa((((struct sockaddr_in *)&(if_req.ifr_addr))->sin_addr)));
    printf("IP address: %s\n", ip);
    return 0;
}

void ethernet_header(unsigned char *send_buf, unsigned char *mac) {
    printf("Packaging ethernet header ...");
    
    struct ethhdr *eth_hdr = (struct ethhdr *)(send_buf);
    int i = 0;
    for (i = 0; i < 6; ++i) eth_hdr->h_source[i] = mac[i];

    eth_hdr->h_dest[0] =  DEST_MAC_0;
    eth_hdr->h_dest[1] =  DEST_MAC_1;
    eth_hdr->h_dest[2] =  DEST_MAC_2;
    eth_hdr->h_dest[3] =  DEST_MAC_3;
    eth_hdr->h_dest[4] =  DEST_MAC_4;
    eth_hdr->h_dest[5] =  DEST_MAC_5;

    eth_hdr->h_proto = htons(ETH_P_IP);     // 0x800

    printf("... done.\n");
}

void ip_header(unsigned char *send_buf, char *ip) {
    printf("Packaging IP header ...");
    struct iphdr *ip_hdr = (struct iphdr*)(send_buf + sizeof(struct ethhdr));
    ip_hdr->ihl      = 5;                   // Internet Header Length - 20 bytes
    ip_hdr->version  = 4;                   // ipv4
    ip_hdr->tos      = 0;                   // Type Of Service - fill 0
    ip_hdr->id       = htons(32501);        // unique value
    ip_hdr->ttl      = 64;                  // Time To Live
    ip_hdr->protocol = 17;                  // protocol number - 17 represents UDP protocol
    ip_hdr->saddr    = inet_addr(ip);       // source IP address
    ip_hdr->daddr    = inet_addr(DEST_IP);  // destination IP address
    printf("... done.\n");
}

void udp_header(unsigned char *send_buf) {
    printf("Packaging UDP header ...");
    struct udphdr *udp_hdr = (struct udphdr *)(send_buf + sizeof(struct iphdr) + sizeof(struct ethhdr));

    udp_hdr->source = htons(34561);     // source port
    //udp_hdr->dest   = htons(34562);     // destination port
    udp_hdr->dest   = htons(51562);     // destination port
    udp_hdr->check  = 0;

    printf("... done.\n");
}

void data(unsigned char *send_buf, int *total_len) {
    printf("Packaging data ...");
    send_buf[(*total_len)++] = 'h';
    send_buf[(*total_len)++] = 'e';
    send_buf[(*total_len)++] = 'l';
    send_buf[(*total_len)++] = 'l';
    send_buf[(*total_len)++] = 'o';
    printf("... done.\n");
}

void udp_header_len(unsigned char *send_buf, int total_len) {
    printf("Packaging UDP header ...");
    struct udphdr *udp_hdr = (struct udphdr *)(send_buf + sizeof(struct iphdr) + sizeof(struct ethhdr));
    udp_hdr->len = htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr)));
    printf("... done.\n");
}

unsigned short checksum(unsigned short *buff, int _16bitword) {
    unsigned long sum;
    for (sum = 0; _16bitword > 0; _16bitword--)
        sum += htons(*(buff)++);
    do {
        sum = ((sum >> 16) + (sum & 0xFFFF));
    } while(sum & 0xFFFF0000);

    return (~sum);
}

void ip_header_len_check(unsigned char *send_buf, int total_len){
    printf("Packaging IP header ...");
    struct iphdr *ip_hdr = (struct iphdr*)(send_buf + sizeof(struct ethhdr));
    ip_hdr->tot_len  = htons(total_len - sizeof(struct ethhdr));
    ip_hdr->check    = htons(checksum((unsigned short*)(send_buf + sizeof(struct ethhdr)), (sizeof(struct iphdr) / 2)));
    printf("... done.\n");
}

/******************************************************************
 * Main
 ******************************************************************/
int main() {
    int sock_raw;                   // raw socket
    char ip[16] = {0};              // local IP address
    unsigned char mac[6] = {0};     // local MAc address
    int if_index;                   // interface index number
    unsigned char *send_buf;        // buffer for packet
    int total_len = 0;              // total length of packet
    int send_len = 0;               // how many bytes sent

    int ret_value = 0;              // return value

    sock_raw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sock_raw == -1) {
        printf("error in socket.\n");
        return -1;
    }

    send_buf = (unsigned char*)malloc(64);
    memset(send_buf, 0, 64);

    if_index = get_eth_index(sock_raw);         // get interface index number
    if (if_index < 0) {
        ret_value = -1;
        goto quit;
    }
    if (get_mac(sock_raw, mac) < 0) {           // get MAC address
        ret_value = -1;
        goto quit;
    }
    if (get_ip(sock_raw, ip) < 0) {             // get IP address
        ret_value = -1;
        goto quit;
    }
    ethernet_header(send_buf, mac);             // construct ethernet header
    total_len += sizeof(struct ethhdr);
    ip_header(send_buf, ip);                    // construct ip header
    total_len += sizeof(struct iphdr);
    udp_header(send_buf);                       // construct udp header
    total_len += sizeof(struct udphdr);
    data(send_buf, &total_len);                 // fill data
    udp_header_len(send_buf, total_len);        // fill len field in udp header
    ip_header_len_check(send_buf, total_len);   // fill len and check fields in ip header

    struct sockaddr_ll saddr_ll;
    memset(&saddr_ll, 0, sizeof(struct sockaddr_ll));
    saddr_ll.sll_ifindex  = if_index;
    saddr_ll.sll_halen    = ETH_ALEN;
    saddr_ll.sll_addr[0]  = DEST_MAC_0;
    saddr_ll.sll_addr[1]  = DEST_MAC_1;
    saddr_ll.sll_addr[2]  = DEST_MAC_2;
    saddr_ll.sll_addr[3]  = DEST_MAC_3;
    saddr_ll.sll_addr[4]  = DEST_MAC_4;
    saddr_ll.sll_addr[5]  = DEST_MAC_5;

    printf("Sending ...");
    while (send_len == 0) {
        send_len = sendto(sock_raw, send_buf, 64, 0, (const struct sockaddr *)&saddr_ll, sizeof(struct sockaddr_ll));
        if (send_len < 0) {
            printf("error in sending....sendlen=%d....errno=%d\n", send_len, errno);
            ret_value = -1;
            goto quit;
        }
    }
    printf("... done.\n");

quit:
    free(send_buf);
    close(sock_raw);
    return ret_value;
}
