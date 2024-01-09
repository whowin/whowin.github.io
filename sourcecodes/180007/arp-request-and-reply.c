/*
 * File: arp-request-and-reply.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Receive an arp request and send a reply under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall arp-request-and-reply.c -o arp-request-and-reply
 * Usage: $ sudo ./arp-request-and-reply
 * 
 * Example source code for article 《接收arp请求并发送回应的实例》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/if_ether.h>
#include <linux/if_arp.h>

#include <arpa/inet.h>

#define DEVICE              "enp0s3"    // Please modify according to the actual device

int sock_raw = 0;               // Socket descriptor
void *buffer = NULL;            // Buffer for receiving and sending packet
long total_packets = 0;         // how many packets were received
long total_arp_packets = 0;     // how many arp packets were received
long total_arp_req_packets = 0; // how many arp request packets were received
long answered_packets = 0;      // how many arp reply packets were sent

void sigint(int signum);

struct __attribute__((packed)) arp_packet {
    struct arphdr arp_hdr;              // arp header
    unsigned char arp_sha[ETH_ALEN];    // Sender Hardware Address
    unsigned char arp_spa[4];           // Sender Protocol Address
    unsigned char arp_dha[ETH_ALEN];    // Destination Hardware Address
    unsigned char arp_dpa[4];           // Destination Protocol Address
};

int main(void) {
    int buf_size = sizeof(struct ethhdr) + sizeof(struct arp_packet);
    buffer = (void *)malloc(buf_size);                  // Buffer for Ethernet Frame
    unsigned char *eth_header = buffer;                 // Pointer to Ethenet Header
    struct ethhdr *eh = (struct ethhdr *)eth_header;    // Another pointer to ethernet header
    unsigned char *arp_header = buffer + sizeof(struct ethhdr); // it is arp header after ethernet header
    struct arp_packet *ah = (struct arp_packet *)arp_header;    // pointer to ARP header

    struct ifreq ifr;               // use for getting MAC and IP via ioctl
    unsigned char src_mac[ETH_ALEN];// local MAC address
    uint32_t src_ip;                // local IP address

    struct sockaddr_ll saddr_ll;    // structure for sending packet via raw socket
    int ifindex = 0;                // Ethernet Interface index
    int length;                     // length of received packet
    int sent;                       // length of sent packet

    printf("Server started, entering initialiation phase...\n");

    // Step 1: create raw socket
    //===========================
    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw == -1) {
        perror("socket():");
        exit(EXIT_FAILURE);
    }
    printf("Successfully created socket: %i\n", sock_raw);

    // Step 2: retrieve ethernet interface index
    //===========================================
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, DEVICE, IFNAMSIZ - 1);
    if (ioctl(sock_raw, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        close(sock_raw);
        exit(EXIT_FAILURE);
    }
    ifindex = ifr.ifr_ifindex;
    printf("Successfully got interface index: %i\n", ifindex);

    // Step 3: retrieve corresponding MAC
    //====================================
    if (ioctl(sock_raw, SIOCGIFHWADDR, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        close(sock_raw);
        exit(EXIT_FAILURE);
    }
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    printf("Successfully got local MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);

    // Step 4: retrieve corresponding IP
    //===================================
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, DEVICE, IFNAMSIZ - 1);
    if (ioctl(sock_raw, SIOCGIFADDR, &ifr) < 0) {
        printf("SIOCGIFADDR.\n");
        close(sock_raw);
        exit(EXIT_FAILURE);
    }
    memcpy((void *)&src_ip, &(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr), sizeof(uint32_t));
    printf("successfully got local IP address: %s\n", inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));

    // step 5: prepare sockaddr_ll for sending packet
    //================================================
    memset((void *)&saddr_ll, 0, sizeof(struct sockaddr_ll));
    saddr_ll.sll_family   = PF_PACKET;
    saddr_ll.sll_protocol = htons(ETH_P_IP);
    saddr_ll.sll_ifindex  = ifindex;
    saddr_ll.sll_hatype   = ARPHRD_ETHER;         // hardware address type: ethernet
    saddr_ll.sll_pkttype  = PACKET_OTHERHOST;     // packet type: To someone else
    saddr_ll.sll_halen    = ETH_ALEN;

    // Step 5: establish signal handler
    //==================================
    signal(SIGINT, sigint);
    printf("Successfully established signal handler for SIGINT\n\n");

    while (sock_raw > 0) {
        // Step 6: Wait for incoming packet...
        //======================================
        memset(buffer, 0, buf_size);
        length = recvfrom(sock_raw, buffer, buf_size, 0, NULL, NULL);
        if (length == -1) {
            perror("recvfrom():");
            close(sock_raw);
            exit(EXIT_FAILURE);
        }
        total_packets++;        // how many packets were received
        // Step 7: ignore the packet if not arp protocol
        //=============================================== 
        if (htons(eh->h_proto) != ETH_P_ARP)           // 0x806
            continue;
        total_arp_packets++;        // how many arp packets were received

        // Step 8: ignore the packet if not arp request(may reply)
        //=========================================================
        if (htons(ah->arp_hdr.ar_op) != ARPOP_REQUEST) {    // 0x0001
            /*
            printf("Received an arp reply. Source IP:%d.%d.%d.%d\tDest IP: %d.%d.%d.%d\n",
                    ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3],
                    ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);
            */
            continue;
        }
        total_arp_req_packets++;    // how many arp request packets were received

        // Step 9: ignore the packet if the destination is not this machine
        //==================================================================
        if (src_ip != *((uint32_t *)ah->arp_dpa)) {
            printf("Received an arp request. Source IP:%d.%d.%d.%d\tDest IP: %d.%d.%d.%d\n",
                    ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3],
                    ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);
            continue;
        } 
        // receive an arp request packet and the destination IP is this machine
        printf("\nReceived an arp request to this machine.\n");

        // Step 10: display ethernet header
        //==================================
        printf("================== ETHERNET HEADER ========================\n");
        printf("ETHER DST MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                eh->h_dest[0], eh->h_dest[1], eh->h_dest[2],
                eh->h_dest[3], eh->h_dest[4], eh->h_dest[5]);
        printf("ETHER SRC MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                eh->h_source[0], eh->h_source[1], eh->h_source[2],
                eh->h_source[3], eh->h_source[4], eh->h_source[5]);

        // Step 11: display arp packet 
        //=============================
        printf("================== ARP PACKET ========================\n");
        printf("------------------ ARP Header ------------------------\n");
        printf("  H/D TYPE: 0x%04x\t  PROTO TYPE: 0x%04x\n", ntohs(ah->arp_hdr.ar_hrd), ntohs(ah->arp_hdr.ar_pro));
        printf("H/D length: %d\t\tPROTO length: %d\n", (unsigned short)ah->arp_hdr.ar_hln, (unsigned short)ah->arp_hdr.ar_pln);
        printf(" OPERATION: %x\n", ntohs(ah->arp_hdr.ar_op));
        printf("------------------ ARP Payload -----------------------\n");
        printf("SENDER MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                ah->arp_sha[0], ah->arp_sha[1], ah->arp_sha[2],
                ah->arp_sha[3], ah->arp_sha[4], ah->arp_sha[5]);
        printf(" SENDER IP address: %d.%d.%d.%d\n",
                ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3]);
        printf("TARGET MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                ah->arp_dha[0], ah->arp_dha[1], ah->arp_dha[2],
                ah->arp_dha[3], ah->arp_dha[4], ah->arp_dha[5]);
        printf(" TARGET IP address: %d.%d.%d.%d\n",
                ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);

        // Step 12: construct ethernet header for sending arp reply
        //==========================================================
        // Destination MAC: should be received sender's MAC
        memcpy((void *)eth_header, (const void *)(eth_header + ETH_ALEN), ETH_ALEN);
        // Source MAC: should be local MAC
        memcpy((void *)(eth_header + ETH_ALEN), (const void *)src_mac, ETH_ALEN);
        eh->h_proto = ETH_P_ARP;        // protocol in network layer. defined in if_ether.h

        // Step 13: construct arp packet for sending arp reply
        //=====================================================
        ah->arp_hdr.ar_op = ARPOP_REPLY;            // 0x0002;

        memcpy(ah->arp_dha, ah->arp_sha, ETH_ALEN); // copy received source MAC to destination MAC
        memcpy(ah->arp_dpa, ah->arp_spa, 4);        // copy received source IP to destination IP
        memcpy(ah->arp_sha, src_mac, ETH_ALEN);     // change the sender MAC to local MAC
        memcpy(ah->arp_spa, (unsigned char *)&src_ip, 4);   // copy local IP to source IP

        // Step 14: change sll_addr to destination MAC
        //=============================================
        memcpy(saddr_ll.sll_addr, eh->h_dest, ETH_ALEN);    // copy h_dest field of ethernet header to sll_addr

        // Step 15: diaplay ethernet header for sending arp reply
        //========================================================
        printf("\n=========== ETHERNET HEARDER FOR SENDING PACKET =========\n");
        printf("ETHER DST MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                eh->h_dest[0], eh->h_dest[1], eh->h_dest[2],
                eh->h_dest[3], eh->h_dest[4], eh->h_dest[5]);
        printf("ETHER SRC MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                eh->h_source[0], eh->h_source[1], eh->h_source[2],
                eh->h_source[3], eh->h_source[4], eh->h_source[5]);

        // Step 16: display the content of arp reply
        //===========================================
        printf("================== ARP REPLY PACKET ========================\n");
        printf("------------------ ARP header ------------------------------\n");
        printf("  H/D TYPE: 0x%04x\t  PROTO TYPE: 0x%04x\n", ntohs(ah->arp_hdr.ar_hrd), ntohs(ah->arp_hdr.ar_pro));
        printf("H/D length: %d\t\tPROTO length: %d\n", (unsigned short)ah->arp_hdr.ar_hln, (unsigned short)ah->arp_hdr.ar_pln);
        printf(" OPERATION: %x\n", ah->arp_hdr.ar_op);
        printf("------------------ ARP Payload -----------------------------\n");
        printf("SENDER MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                ah->arp_sha[0], ah->arp_sha[1], ah->arp_sha[2],
                ah->arp_sha[3], ah->arp_sha[4], ah->arp_sha[5]);
        printf(" SENDER IP address: %d.%d.%d.%d\n",
                ah->arp_spa[0], ah->arp_spa[1], ah->arp_spa[2], ah->arp_spa[3]);
        printf("TARGET MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                ah->arp_dha[0], ah->arp_dha[1], ah->arp_dha[2],
                ah->arp_dha[3], ah->arp_dha[4], ah->arp_dha[5]);
        printf(" TARGET IP address: %d.%d.%d.%d\n\n",
                ah->arp_dpa[0], ah->arp_dpa[1], ah->arp_dpa[2], ah->arp_dpa[3]);

        // Step 17: Send the arp replay
        //==============================
        sent = sendto(sock_raw, buffer, buf_size, 0, (struct sockaddr *)&saddr_ll, sizeof(struct sockaddr_ll));
        if (sent == -1) {
            perror("sendto():");
            close(sock_raw);
            exit(EXIT_FAILURE);
        }
        answered_packets++;     // How many arp reply packets were sent
    }
}

void sigint(int signum) {
    // Clean up ......
    if (sock_raw > 0) close(sock_raw);
    if (buffer) free(buffer);

    printf("Server terminating....\n");

    printf("Totally received: %ld packets\n", total_packets);
    printf("Totally received: %ld ARP packets\n", total_arp_packets);
    printf("Totally received: %ld ARP request packets\n", total_arp_req_packets);
    printf("Answered %ld packets\n", answered_packets);
    exit(EXIT_SUCCESS);
}
