/*
 * File: arp-get.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Get an ARP entry from ARP cache under Linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall arp-get.c -o arp-get
 * Usage: $ sudo ./arp-get
 * 
 * Example source code for article 《如何用C语言操作arp cache》
 * https://whowin.gitee.io/post/blog/network/0014-handling-arp-cache/
 *
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <net/if.h>
#include <net/if_arp.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define TRUE        1
#define FALSE       0

// Check if the IP address is valid
int valid_ip(char *p) {
    int i, j = strlen(p);
    char *p1;
    int n = 0;

    if (j > 15 || j < 7) return FALSE;

    p1 = p;
    for (i = 0; i < j; ++i) {
        if ((p[i] < '0' || p[i] > '9') && p[i] != '.') return FALSE;
        if (p[i] == '.') {
            ++n;
            if (atoi(p1) > 255 || *p1 == '.') return FALSE;
            p1 = p + i + 1;
        }
    }
    if (n != 3 || p1 == p + j) return FALSE;

    return TRUE;
    
}

int main(int argc, char *argv[]) {
    // Check arguments
    if (argc != 3) {
        printf("Usage: %s [interface name] [dst IP]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Interface name
    char ifname[IF_NAMESIZE] = {'\0'};
    strncpy(ifname, argv[1], IF_NAMESIZE - 1);

    // IP address
    char ipaddr[INET_ADDRSTRLEN] = {'\0'};
    strncpy(ipaddr, argv[2], INET_ADDRSTRLEN);

    // Check if the IP address is valid
    if (!valid_ip(ipaddr)) {
        printf("Invalid IP address: %s\n", ipaddr);
        exit(EXIT_FAILURE);
    }

    int sockfd;
    unsigned char *mac;
    struct arpreq arp_req;
    struct sockaddr_in *sin;

    // initialize struct arpreq
    memset(&arp_req, 0, sizeof(arp_req));

    sin = (struct sockaddr_in *)&(arp_req.arp_pa);
    sin->sin_family = AF_INET;
    inet_pton(AF_INET, ipaddr, &(sin->sin_addr));
    strncpy(arp_req.arp_dev, ifname, IF_NAMESIZE - 1);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed...");
        exit(EXIT_FAILURE);
    }

    // getting arp entry via ioctl
    if (ioctl(sockfd, SIOCGARP, &arp_req) < 0) {
        perror("Getting arp entry failed...");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // ioctl successfully.
    if (arp_req.arp_flags & ATF_COM) {
        // the entry in arp cache is okay.
        mac = (unsigned char *)arp_req.arp_ha.sa_data;
        printf("Interface name: %s\tIP: %s\n", ifname, ipaddr);
        printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        // the entry in arp cache is incomplete.
        printf("The entry in arp cache is incomplete. IP: %s\tInterface name: %s\n", ipaddr, ifname);
        printf("Sometimes just wait for a moment, will get complete MAC address.\n");
    }

    close(sockfd);
    return 0;
}

