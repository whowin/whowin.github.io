/*
 * File: udp-checksum.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * how to calculat checksum of udp header under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall udp-checksum.c -o udp-checksum
 * Usage: $ ./udp-checksum
 * 
 * Example source code for article 《如何计算UDP头的checksum》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <arpa/inet.h>
#include <linux/udp.h>

#define SOURCE_IP       "192.168.2.114"
#define DESTINATION_IP  "192.168.2.112"
#define SOURCE_PORT     56789
#define DESTNATION_PORT 8000
#define UDP_DATA        "hello"

// udp pseudo header structure
struct pseudohdr {
    uint32_t source_ip;
    uint32_t destination_ip;
    uint8_t zero;
    uint8_t protocol;
    uint16_t udp_len;
};
// udp packet structure for calculating checksum
struct udpcheckhdr {
    struct pseudohdr pseudo_hdr;
    struct udphdr udp_hdr;
    unsigned char data[16];
};
// initial pseudo header
void init_pseudohdr(struct udpcheckhdr *p) {
    p->pseudo_hdr.source_ip = inet_addr(SOURCE_IP);
    p->pseudo_hdr.destination_ip = inet_addr(DESTINATION_IP);
    p->pseudo_hdr.zero = 0;
    p->pseudo_hdr.protocol = IPPROTO_UDP;
    p->pseudo_hdr.udp_len = sizeof(struct udphdr) + strlen(UDP_DATA);
}
// initial udp header
void init_udphdr(struct udpcheckhdr *p) {
    p->udp_hdr.source = htons(SOURCE_PORT);
    p->udp_hdr.dest = htons(DESTNATION_PORT);
    p->udp_hdr.len = sizeof(struct udphdr) + strlen(UDP_DATA);
    p->udp_hdr.check = 0;
}
// initial udp data
void init_udpdata(struct udpcheckhdr *p) {
    strcpy((char *)p->data, UDP_DATA);
}

uint16_t checksum1(uint16_t *p, int count) {
    register long sum = 0;
    unsigned short checksum;

    uint16_t temp;
    uint16_t i = 0;
    while (count > 1) {
        //  This is the inner loop
        temp = *p++;
        printf("Step %02d: 0X%08lX + 0X%04X\n", ++i, sum, temp);
        sum += temp;
        count -= 2;
    }

    //  Add left-over byte, if any
    if (count > 0) {
        temp = *(unsigned char *)p;
        printf("Step %02d: 0X%08lX + 0X%04X\n", ++i, sum, temp);
        sum += temp;
    }

    printf("Result before folding: 0X%08lX\n", sum);
    //  Fold 32-bit sum to 16 bits
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    printf("Result after folding: 0X%08lX\n", sum);

    checksum = ~(uint16_t)sum;
    printf("\nChecksum = 0x%04X\n", checksum);

    return checksum;
}

uint16_t checksum2(uint16_t *p, int count) {
    register long sum = 0;
    uint16_t checksum;

    uint16_t temp;
    uint16_t i = 0;

    while (count > 1) {
        //  This is the inner loop
        temp = *p++;
        printf("Step %02d: 0X%08lX + 0X%04X(0X%04X)\n", ++i, sum, (uint16_t)~temp, temp);
        sum += (uint16_t)~temp;
        count -= 2;
    }

    // Add left-over byte, if any
    if (count > 0) {
        temp = *(unsigned char *)p;
        printf("Step %02d: 0X%08lX + 0X%04X(0X%04X)\n", ++i, sum, (uint16_t)~temp, temp);
        sum += (uint16_t)~temp;
    }

    printf("Result before folding: 0X%08lX\n", sum);
    /*  Fold 32-bit sum to 16 bits */
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    printf("Result after folding: 0X%08lX\n", sum);

    checksum = (uint16_t)sum;
    printf("\nChecksum = 0x%04X\n", checksum);
    return checksum;
}

int main(int argc, char **argv) {
    struct udpcheckhdr udp_packet;

    init_pseudohdr(&udp_packet);
    init_udphdr(&udp_packet);
    init_udpdata(&udp_packet);

    unsigned short *p = (unsigned short *)&udp_packet;
    int count = sizeof(struct pseudohdr) + udp_packet.udp_hdr.len;

    printf("\nThe one's complement code of 16-bit true code sum\n");
    checksum1(p, count);
    printf("\nThe one's complement sum\n");
    checksum2(p, count);

    return 0;
}
