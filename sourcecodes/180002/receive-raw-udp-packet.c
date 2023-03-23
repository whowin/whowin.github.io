/*
 * File: receive-raw-udp-packet.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Receive raw udp packet using raw socket under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall receive-raw-udp-packet.c -o receive-raw-udp-packet
 * Usage: $ sudo ./receive-raw-udp-packet
 * 
 * Example source code for article 《Linux下如何在数据链路层接收原始数据包》
 * https://whowin.gitee.io/post/blog/network/0002-link-layer-programming/
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <malloc.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>           // to avoid warning at inet_ntoa

#include<linux/if_packet.h>
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#define LOG_FILE       "udp_packets.log"    // log file name

struct ethhdr *eth_hdr;
struct iphdr *ip_hdr;
struct udphdr *udp_hdr;

/*****************************************************************************
 * Function: unsigned int ethernet_header(unsigned char *buffer, int buflen)
 * Description: Extracting the Ethernet header
 *              struct ethhdr is defined in if_ether.h
 * 
 * Entry: buffer     data packet
 *        buf_len    length of data packet
 * Return: protocol of network layer or -1 when error
 *****************************************************************************/
int ethernet_header(unsigned char *buffer, int buf_len) {
    if (buf_len < sizeof(struct ethhdr)) {
        printf("Wrong data packet.\n");
        return -1;
    }

    eth_hdr = (struct ethhdr *)(buffer);

    return ntohs(eth_hdr->h_proto);
}
/*********************************************************************************
 * Function: void log_ethernet_header(FILE *log_file, struct ethhdr *eth_hdr)
 * Description: write ether header into log file
 * 
 * Entry:   log_file    log file object
 *          eth_hdr     pointer of ethernet header structure
 *********************************************************************************/
void log_ethernet_header(FILE *log_file, struct ethhdr *eth_hdr) {
    fprintf(log_file, "\nEthernet Header\n");
    fprintf(log_file, "\t|-Source MAC Address     : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", 
            eth_hdr->h_source[0], eth_hdr->h_source[1], eth_hdr->h_source[2], 
            eth_hdr->h_source[3], eth_hdr->h_source[4], eth_hdr->h_source[5]);
    fprintf(log_file, "\t|-Destination MAC Address: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", 
            eth_hdr->h_dest[0], eth_hdr->h_dest[1], eth_hdr->h_dest[2], 
            eth_hdr->h_dest[3], eth_hdr->h_dest[4], eth_hdr->h_dest[5]);
    fprintf(log_file, "\t|-Protocol               : 0X%04X\n", ntohs(eth_hdr->h_proto));
    // ETH_P_IP = 0x0800, ETH_P_LOOP = 0X0060
}
/********************************************************************************
 * Function: unsigned int ip_header(unsigned char *buffer, int buf_len)
 * Description: Extracting the IP header
 *              struct iphdr is defined in ip.h
 * 
 * Entry: buffer     data packet
 *        buf_len    length of data packet
 * return: protocol of transport layer or -1 when error
 ********************************************************************************/
int ip_header(unsigned char *buffer, int buf_len) {
    if (buf_len < sizeof(struct ethhdr) + 20) {
        printf("Wrong data packet.\n");
        return -1;
    }

    ip_hdr = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    int tot_len = ntohs(ip_hdr->tot_len);

    if (buf_len < sizeof(struct ethhdr) + tot_len) {
        printf("Wrong data packet.\n");
        return -1;
    }
    return (int)ip_hdr->protocol;
}
/********************************************************************************
 * Function: void log_ip_header(FILE *log_file, struct iphdr *ip_hdr)
 * Description: write ip header into log file
 * 
 * Entry:   log_file        log file's handler
 *          ip_hdr          the pointer of ip header structure
 ********************************************************************************/
void log_ip_header(FILE *log_file, struct iphdr *ip_hdr) {
    struct sockaddr_in source, dest;

    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = ip_hdr->saddr;     
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = ip_hdr->daddr;     

    fprintf(log_file, "\nIP Header\n");

    fprintf(log_file, "\t|-Version               : %d\n", (unsigned int)ip_hdr->version);
    fprintf(log_file, "\t|-Internet Header Length: %d DWORDS or %d Bytes\n", (unsigned int)ip_hdr->ihl, ((unsigned int)(ip_hdr->ihl)) * 4);
    fprintf(log_file, "\t|-Type Of Service       : %d\n", (unsigned int)ip_hdr->tos);
    fprintf(log_file, "\t|-Total Length          : %d  Bytes\n", ntohs(ip_hdr->tot_len));
    fprintf(log_file, "\t|-Identification        : %d\n", ntohs(ip_hdr->id));
    fprintf(log_file, "\t|-Time To Live          : %d\n", (unsigned int)ip_hdr->ttl);
    fprintf(log_file, "\t|-Protocol              : %d\n", (unsigned char)ip_hdr->protocol);
    fprintf(log_file, "\t|-Header Checksum       : %d\n", ntohs(ip_hdr->check));
    fprintf(log_file, "\t|-Source IP             : %s\n", inet_ntoa(source.sin_addr));
    fprintf(log_file, "\t|-Destination IP        : %s\n", inet_ntoa(dest.sin_addr));
}
/************************************************************************
 * Function: udp_header(FILE *log_file, struct iphdr *ip_hdr)
 * Description: Extracting the UDP header
 * 
 * Entry:   log_file    log file
 *          ip_hdr      pointer of IP header
 ************************************************************************/
void udp_header(FILE *log_file, struct iphdr *ip_hdr) {
    fprintf(log_file, "\nUDP Header\n");

    udp_hdr = (struct udphdr *)((unsigned char *)ip_hdr + (unsigned int)ip_hdr->ihl * 4);

    fprintf(log_file, "\t|-Source Port     : %d\n", ntohs(udp_hdr->source));
    fprintf(log_file, "\t|-Destination Port: %d\n", ntohs(udp_hdr->dest));
    fprintf(log_file, "\t|-UDP Length      : %d\n", ntohs(udp_hdr->len));
    fprintf(log_file, "\t|-UDP Checksum    : %d\n", ntohs(udp_hdr->check));
}
/**************************************************************************
 * Function: void udp_payload(FILE *log_file, struct udphdr *udp_hdr)
 * Description: Show data
 * 
 * Entry: buffer     data packet
 *        buf_len    length of data packet
 **************************************************************************/
void udp_payload(FILE *log_file, struct udphdr *udp_hdr) {
    int i = 0;
    unsigned char *data = (unsigned char *)udp_hdr + sizeof(struct udphdr);
    fprintf(log_file, "\nData\n");
    int data_len = ntohs(udp_hdr->len) - sizeof(struct udphdr);

    for (i = 0; i < data_len; i++) {
        if (i != 0 && i % 16 == 0) 
            fprintf(log_file, "\n");
        fprintf(log_file, " %.2X ", data[i]);
    }

    fprintf(log_file, "\n");
}
/*****************************************************
 * Main 
 *****************************************************/
int main() {
    FILE* log_file;                             // log file
    struct sockaddr saddr;

    int sock_raw, saddr_len, buf_len;
    int ret_value = 0;

    int done = 0;               // exit loop when done=1
    int udp = 0;                // udp packet count

    // open a raw socket
    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
    if (sock_raw < 0) {
        printf("Error in socket\n");
        return -1;
    }
    // Allocate a block of memory for the receive buffer
    unsigned char *buffer = (unsigned char *)malloc(65536); 
    if (buffer == NULL) {
        printf("Unable to allocate memory.\n");
        close(sock_raw);
        return -1;
    }
    memset(buffer, 0, 65536);

    // Create a log file for storing output
    log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        printf("Unable to open %s\n", LOG_FILE);
        free(buffer);
        close(sock_raw);
        return -1;
    }

    printf("starting .... %d\n", sock_raw);
    while (!done) {
        // Receive data packet
        saddr_len = sizeof saddr;
        buf_len = recvfrom(sock_raw, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);

        if (buf_len < 0) {
            printf("Error in reading recvfrom function\n");
            ret_value = -1;
            goto QUIT;
        }

        fflush(log_file);
        // Extracting the Ethernet header
        if (ethernet_header(buffer, buf_len) != ETH_P_IP) {
            // drop the packet if network layer protocol is not IP
            continue;
        }
        // Extracting the IP header
        if (ip_header(buffer, buf_len) != 17) {
            // drop packet if transport layer protocol is not UDP
            continue;
        }
        fprintf(log_file, "\n**** UDP packet %02d*********************************\n", udp + 1);
        // Write ethernet header into log file
        log_ethernet_header(log_file, eth_hdr);
        // Write IP header into log file
        log_ip_header(log_file, ip_hdr);
        // Extracting the UDP header and write into log file
        udp_header(log_file, ip_hdr);
        // write UDP payload into log file
        udp_payload(log_file, udp_hdr);
        // exit when the count of received udp packets is more than 10
        if (++udp >= 10) done = 1;
    }

QUIT:
    fclose(log_file);
    free(buffer);
    close(sock_raw);        // close raw socket 
    printf("DONE!!!!\n");
    return ret_value;
}
