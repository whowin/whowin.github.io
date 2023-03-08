/*
 * File: tun-02.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Set up tun0 device and read data from tun0 under Linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall tun-02.c -o tun-02
 * Usage: $ sudo ./tun-02
 *
 * Example source code for article 《使用tun虚拟网络接口建立IP隧道的实例》
 * https://whowin.gitee.io/post/blog/network/0018-tun-example-for-setting-up-ip-tunnel/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/ip.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <arpa/inet.h> 

#define BUFSIZE         2048
#define TUN_DEV_NAME    "tun0"
#define TUN_DEV_FILE    "/dev/net/tun"

int tun_fd = 0;
unsigned long int tun_count = 0;

void sigint(int signum);

void print_iphdr(struct iphdr *iph) {
    uint8_t *p1, *p2;

    printf("===============================================\n");
    if (iph->version == 4) {
        printf("Version: %d\tInternet Header Length: %d bytes\n", iph->version, iph->ihl * 4);
        printf("TOS: %d\tTotal Length: %d bytes\n", iph->tos, ntohs(iph->tot_len));
        printf("ID: %d\tFragment Offset: %d\n", ntohs(iph->id), ntohs(iph->frag_off));
        printf("TTL: %d\tProtocol: %d\tChecksum: %d\n", iph->ttl, iph->protocol, ntohs(iph->check));
        p1 = (uint8_t *)&iph->saddr;
        p2 = (uint8_t *)&iph->daddr;
        printf("Source IP: %d.%d.%d.%d\tDestination IP: %d.%d.%d.%d\n",
                p1[0], p1[1], p1[2], p1[3], p2[0], p2[1], p2[2], p2[3]);
    } else {
        printf("The packet is not IPv4.\n");
    }
    printf("\n");

}
int main(int argc, char *argv[]) {
    uint16_t nread;
    char buffer[BUFSIZE];
    struct ifreq ifr;
    int err, ret_value = EXIT_SUCCESS;

    // Open device file /dev/net/tun
    if( (tun_fd = open(TUN_DEV_FILE , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        exit(EXIT_FAILURE);
    }
    // Register device name into Linux kernel
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strcpy(ifr.ifr_name, TUN_DEV_NAME);
    if( (err = ioctl(tun_fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        ret_value = EXIT_FAILURE;
        goto quit;
    }
    printf("Successfully connected to interface %s\n", TUN_DEV_NAME);
    signal(SIGINT, sigint);
    // set up IP address for tun0
    system("ifconfig tun0 10.0.0.1/24 up");
    // Process the packet received from tun0
    while (1) {
        if ((nread = read(tun_fd, buffer, BUFSIZE)) < 0) {
            perror("Read from tun");
            ret_value = EXIT_FAILURE;
            goto quit;
        } else if (nread == 0) {
            // Do nothing.
        } else {
            printf("TUN %lu: Read %d bytes from the tun interface\n", ++tun_count, nread);
            print_iphdr((struct iphdr *)buffer);
        }
    }

quit:
    if (tun_fd) close(tun_fd);
    return ret_value;
}

void sigint(int signum) {
    // Clean up ......
    if (tun_fd > 0) close(tun_fd);    
    printf("Terminating....\n");
    printf("Totally packets from tun: %ld packets\n", tun_count);
    exit(EXIT_SUCCESS);
}
