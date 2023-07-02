/*
 * File: magic-packet.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Send magic packet using raw socket under Linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall magic-packet.c -o magic-packet
 * Usage: $ sudo ./magic-packet
 * 
 * Example source code for article 《使用raw socket发送magic packet》
 * https://whowin.gitee.io/post/blog/network/0015-send-magic-packet-via-raw-socket/
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#define BUF_SIZE            256
#define MAGIC_PACKET_SIZE   102
#define MAGIC_PACKET_PROTO  0x842

#define USING_SENDTO        1
/********************************************************************
 * Function: int get_eth_index(int sock_raw, char *ifname)
 * Description: Get iterface index number from ifname using ioctl
 * 
 * retuen
 *          >= 0        iterface index
 *          < 0         error  
 ********************************************************************/
int get_eth_index(int sock_raw, char *ifname) {
    struct ifreq if_req;
    memset(&if_req, 0, sizeof(struct ifreq));
    strncpy(if_req.ifr_name, ifname, IFNAMSIZ - 1);

    if ((ioctl(sock_raw, SIOCGIFINDEX, &if_req)) < 0) {
        printf("error in SIOCGIFINDEX ioctl reading.\n");
        return -1;
    }
    printf("Interface Name: %s\tInterface Index=%d\n", ifname, if_req.ifr_ifindex);
    return if_req.ifr_ifindex;
}

/**************************************************************************
 * Function: int get_mac(int sock_raw, unsigned char *mac, char *ifname)
 * Description: Get local MAC from ifname using ioctl
 * 
 * retuen
 *          = 0         success. MAC has been stored into mac
 *          < 0         error 
 **************************************************************************/
int get_mac(int sock_raw, unsigned char *mac, char *ifname) {
    struct ifreq if_req;
    memset(&if_req, 0, sizeof(struct ifreq));
    strncpy(if_req.ifr_name, ifname, IFNAMSIZ - 1);

    if ((ioctl(sock_raw, SIOCGIFHWADDR, &if_req)) < 0) { 
        printf("error in SIOCGIFHWADDR ioctl reading.\n");
        return -1;
    }
    int i;
    for (i = 0; i < ETH_ALEN; ++i) mac[i] = (unsigned char)(if_req.ifr_hwaddr.sa_data[i]);

    printf("Source MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
    return 0;
}
/****************************************************************
 * Function: int is_valid_mac(char *mac)
 * Description: Check mac if valid or not
 * 
 * Return
 *          1       mac is valid
 *          0       mac is invalid
 ****************************************************************/
int is_valid_mac(char *mac){
    if (strlen(mac) != 17) return 0;
    int i;
    for (i = 0; i < 17; ++i) {
        if ((i + 1) % 3 == 0) {
            if (mac[i] != ':') return 0;
        } else {
            if ((mac[i] >= '0' && mac[i] <= '9') || 
                (mac[i] >= 'a' && mac[i] <= 'f') ||
                (mac[i] >= 'A' && mac[i] <= 'F')) {
                continue;
            } else return 0;
        }
    }
    return 1;
}

/******************************************************************
 * Main
 ******************************************************************/
int main(int argc, char **argv) {
    int sock_raw;                           // raw socket
    unsigned char src_mac[ETH_ALEN] = {0};  // source MAC address
    unsigned char dest_mac[ETH_ALEN] = {0}; // destination MAC address
    int if_index;                           // interface index number
    char ifname[IFNAMSIZ] = {0};            // interface name
    unsigned char *send_buf;                // buffer for packet
    
    int ret_value = EXIT_SUCCESS;           // return value

    // check parameters' number
    if (argc != 3) {
        printf("Usage: %s [ifname] [destination MAC]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check if MAC is valid
    if (!is_valid_mac(argv[2])) {
        printf("Invalid destination MAC address. %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    // Check if ifname length is too long
    if (strlen(argv[1]) > IFNAMSIZ) {
        printf("Invalid Interface Name. %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    // copy ifname
    strcpy(ifname, argv[1]);
    // convert MAC string to digits
    unsigned char *p = (unsigned char *)argv[2];
    sscanf(argv[2], "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
            &dest_mac[0], &dest_mac[1],&dest_mac[2], &dest_mac[3], &dest_mac[4],&dest_mac[5]);

    // Create a raw socket
    //sock_raw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    //sock_raw = socket(AF_PACKET, SOCK_RAW, 0);
    if (sock_raw == -1) {
        printf("error in socket.\n");
        exit(EXIT_FAILURE);
    }

    // allocate memory for sending
    send_buf = (unsigned char*)malloc(BUF_SIZE);
    memset(send_buf, 0, BUF_SIZE);
    // get interface index
    if_index = get_eth_index(sock_raw, ifname);         // get interface index number
    if (if_index < 0) {
        ret_value = EXIT_FAILURE;
        goto quit;
    }
    // get local MAC as source mac
    if (get_mac(sock_raw, src_mac, ifname) < 0) {           // get MAC address
        ret_value = EXIT_FAILURE;
        goto quit;
    }
    printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5]);

    // Fill ethernet header
    struct ethhdr *eth_hdr = (struct ethhdr *)(send_buf);
    int i;
    for (i = 0; i < ETH_ALEN; ++i) {
        eth_hdr->h_source[i] = src_mac[i];
        eth_hdr->h_dest[i] = 0xff;
    }
    eth_hdr->h_proto = htons(MAGIC_PACKET_PROTO);   // 0x842

    // Construct magic packet
    p = send_buf;
    p += sizeof(struct ethhdr);
    for (i = 0; i < ETH_ALEN; i++) *p++ = 0xFF;
    for (i = 0; i < 16; i++) memcpy((p + i * 6), dest_mac, ETH_ALEN);

    // print the packet
    size_t total_len = MAGIC_PACKET_SIZE + sizeof(struct ethhdr);
    p = send_buf;
    for (i = 1; i <= total_len; ++i) {
        printf(" %02x", *p++);
        if ((i % 16) == 0) printf("\n");
    }
    printf("\n");

    // Construct struct sockaddr_ll
    struct sockaddr_ll saddr_ll;
    memset(&saddr_ll, 0, sizeof(struct sockaddr_ll));
    saddr_ll.sll_family   = AF_PACKET;
    saddr_ll.sll_ifindex  = if_index;
    saddr_ll.sll_pkttype  = PACKET_BROADCAST;
    saddr_ll.sll_halen    = ETH_ALEN;
    for (i = 0; i < ETH_ALEN; ++i) saddr_ll.sll_addr[i] = 0xff;
    saddr_ll.sll_protocol = htons(MAGIC_PACKET_PROTO);  // 0x842

#if !USING_SENDTO
    if (bind(sock_raw, (const struct sockaddr *)&saddr_ll, sizeof(struct sockaddr_ll)) < 0) {
        perror("Bind()");
        ret_value = EXIT_FAILURE;
        goto quit;
    }
#endif

   // Send the packet
    int send_len = 0;       // how many bytes sent
#if USING_SENDTO
    if ((send_len = sendto(sock_raw, send_buf, total_len, 0, (const struct sockaddr *)&saddr_ll, sizeof(struct sockaddr_ll))) < 0) {
        perror("Sendto()");
    }
#else
    if ((send_len = send(sock_raw, send_buf, total_len, 0)) < 0) {
        perror("Send()");
    }
#endif

    printf("... done. send_len=%d\n", send_len);

quit:
    free(send_buf);
    close(sock_raw);
    return ret_value;
}
