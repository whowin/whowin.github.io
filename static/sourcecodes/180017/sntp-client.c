/*
 * File: sntp-client.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * SNTP client.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall sntp_client.c -o sntp_client -lm
 * Usage: $ ./sntp_client <time server ip/domain name>
 *
 * Time server reference:
 *      ntp.tencent.com
 *      ntp.aliyun.com
 *      time.edu.cn
 * 
 * Example source code for article 《使用SNTP协议从时间服务器同步时间》
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <netdb.h>
#include <math.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define VERSION         "==== SNTP Client v1.00 02/19/2023 ====\n"
#define SNTP_PORT       123
#define SNTP_EPOCH      86400U * (365U * 70U + 17U)      // the seconds from Jan 1, 1900 to Jan 1, 1970
#define MAX_IP_SIZE     16

#pragma pack(1)
struct li_vn_mode {
    unsigned int mode: 3;
    unsigned int vn: 3;
    unsigned int li: 2;
};
struct ntp_timestamp {
    uint32_t seconds;
    uint32_t fraction;
};
struct sntp_packet {
    struct li_vn_mode lvm;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    int16_t root_delay_int;
    uint16_t root_delay_fraction;
    int16_t root_dispersion_int;
    uint16_t root_dispersion_fraction;
    uint8_t ref_id[4];
    struct ntp_timestamp ref_time;
    struct ntp_timestamp ori_time;
    struct ntp_timestamp recv_time;
    struct ntp_timestamp tran_time;
};
#pragma pack()

/********************************************************************
 * Functin: bool is_vaild_ip(const char *ip)
 * Description: check if the ip string is valid
 ********************************************************************/
bool is_vaild_ip(const char *ip) {
    int dots    = 0;                                    // how many dots
    int setions = 0;                                    // the current section in digit(should be in 0-255)
    int strnum  = 0;                                    // total length of the IP string

    if (NULL == ip || *ip == '.') {                     // if IP string is NULL or the 1st char is dot, return false
        return  false;                                  // wrong format
    }

    while (*ip) {                                       // loop when string is not empty
        if (*ip == '.') {                               // check if the previous section is valid(0-255), if the current char is dot
            dots++;                                     // the number of dots plus 1
            if (setions >= 0 && setions <= 255) {       // check if the current section is valid
                setions = 0;                            // clear the previous section
            } else {  
                return  false;                          // wrong format. the value is not whithin 0 -255
            }
        } else if (*ip >= '0' && *ip <= '9') {          // if the current char is a digit
            setions = setions * 10 + (*ip - '0');       // 
        } else {
            return  false;                              // the current char is not a digit or dot. return false
        }
        ip++;                                           // point to net char
        if (++strnum > 15) {                            // The max. length of IP string must be less than 16
            return false;
        }
    }
    // check if the last section is valid
    if (setions >= 0 && setions <= 255) {                 
        if (dots == 3) {                                // avoid：“192.168.123”
            return true;                                // IP is valis
        }
    }
    return  false;
}

/****************************************************************************************************
 * Function: void convert_ntp_time_into_unix_time(struct ntp_timestamp *ntp_tm, struct timeval *unix_tm)
 * Decription: convert the NTP timestamp into UNIX timeval
 * 
 * Entry:   ntp_tm      pointer of struct ntp_timestamp
 *          unix_tm     pointer of struct timeval
 * Return:  none
 *          converted unix timeval is stored in unix_tm
 ****************************************************************************************************/
void convert_ntp_time_into_unix_time(struct ntp_timestamp *ntp_tm, struct timeval *unix_tm) {
    unix_tm->tv_sec = ntohl(ntp_tm->seconds) - SNTP_EPOCH;
    unix_tm->tv_usec = (uint32_t)( (double)ntohl(ntp_tm->fraction) * 1.0e6 / (double)(1LL << 32) );
}
/****************************************************************************************************
 * Function: void convert_unix_time_into_ntp_time(struct timeval *unix_tm, struct ntp_timestamp *ntp_tm)
 * Decription: convert UNIX timeval into the NTP timestamp
 * 
 * Entry:   ntp_tm      pointer of struct ntp_timestamp
 *          unix_tm     pointer of struct timeval
 * Return:  none
 *          converted ntp timestamp is stored in ntp_tm
 ****************************************************************************************************/
void convert_unix_time_into_ntp_time(struct timeval *unix_tm, struct ntp_timestamp *ntp_tm) {
    ntp_tm->seconds = htonl(unix_tm->tv_sec + SNTP_EPOCH);
    ntp_tm->fraction = htonl( (uint32_t)((double)(unix_tm->tv_usec+1) * (double)(1LL << 32) * 1.0e-6) );
}
/********************************************************************************
 * Function: void print_sntp_packet(struct sntp_packet *p)
 * Description: print sntp packet
 ********************************************************************************/
void print_sntp_packet(struct sntp_packet *p) {
    printf("\n=========== SNTP packet ================\n");
    
    printf("li: %d\tvn: %d\tmode: %d\n", p->lvm.li, p->lvm.vn, p->lvm.mode);
    printf("stratum: %d\tpoll interval: %d seconds\tprecision: %f microseconds\n", 
            p->stratum, (int32_t)1 << p->poll, pow(2, p->precision) * 1000000);
    printf("root delay: %d.%u seconds\troot dispersion: %d.%u seconds\n", 
            ntohs(p->root_delay_int), ntohs(p->root_delay_fraction),
            ntohs(p->root_dispersion_int), ntohs(p->root_dispersion_fraction));
    if (p->stratum == 2) {
        printf("reference identifier: %u.%u.%u.%u\n", p->ref_id[0], p->ref_id[1], p->ref_id[2], p->ref_id[3]);
    } else {
        printf("reference identifier: %02x %02x %02x %02x\n", p->ref_id[0], p->ref_id[1], p->ref_id[2], p->ref_id[3]);
    }

    struct timeval tv;
    convert_ntp_time_into_unix_time(&p->ori_time, &tv);
    printf("Reference Timestamp(%ld seconds and %ld microseconds)\n", tv.tv_sec, tv.tv_usec);
    printf("\n");
    convert_ntp_time_into_unix_time(&p->ori_time, &tv);
    printf("Originate Timestamp(T1)\t\t(%ld seconds and %ld microseconds)\n", tv.tv_sec, tv.tv_usec);
    convert_ntp_time_into_unix_time(&p->recv_time, &tv);
    printf("Receive Timestamp(T2)\t\t(%ld seconds and %ld microseconds)\n", tv.tv_sec, tv.tv_usec);
    convert_ntp_time_into_unix_time(&p->tran_time, &tv);
    printf("Transmit Timestamp(T3)\t\t(%ld seconds and %ld microseconds)\n", tv.tv_sec, tv.tv_usec);
}

int main(int argc, char **argv) {
    int ret_value;                      // return value

    fd_set readfds;

    struct sntp_packet sntp_request;    // sntp packet for sending
    struct sntp_packet sntp_reply;      // sntp packet for receiving

    int sock;
    struct sockaddr_in to_addr;
    socklen_t addr_len;

    struct timeval tv;                  // unix timestamp for normal usgae
    struct timeval tv1, tv2, tv3, tv4;  // T1, T2, T3, T3 in timeval format

    char ip[MAX_IP_SIZE] = {0};

    printf(VERSION);
    if (argc > 1) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "-V") == 0) {
            exit(EXIT_SUCCESS);
        }
    }

    // if invalid ip format, resolve the argument as a domain name
    if (!is_vaild_ip(argv[1])) {
        struct hostent *hptr;
        char *p = argv[1];
        if ((hptr = gethostbyname(p)) == NULL) {    // Resolve a domain name
            perror("gethostbyname");
            printf("Invalid IP address or domain name. %s\n", argv[1]);
            exit(EXIT_FAILURE);
        } else {
            if (hptr->h_addrtype == AF_INET) {      // exit if it is not IPv4
                unsigned char *p = (unsigned char *)hptr->h_addr_list[0];
                sprintf(ip, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
            } else {
                printf("It is not IPv4 address type(%d).\n", hptr->h_addrtype);
                exit(EXIT_FAILURE);
            }
            
        }
    } else strncpy(ip, argv[1], MAX_IP_SIZE - 1);

    printf("The time server's IP: %s\n", ip);
    if (!is_vaild_ip(ip)) {
        printf("Invalid IP address or domain name. %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    // Creat a socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("\nsocket creation Failed!\n");
        exit(EXIT_FAILURE);
    }

    // Set the server address and port
    to_addr.sin_family      = AF_INET;
    to_addr.sin_port        = htons(SNTP_PORT);
    to_addr.sin_addr.s_addr = inet_addr(ip);       // Time server's IP
    bzero(&(to_addr.sin_zero), 8);
    addr_len = sizeof(struct sockaddr);

    // Fill the NTP packet for request
    bzero(&sntp_request, sizeof(struct sntp_packet));
    sntp_request.lvm.li = 0;
    sntp_request.lvm.vn = 4;
    sntp_request.lvm.mode = 3;

    gettimeofday(&tv1, NULL);
    convert_unix_time_into_ntp_time(&tv1, &sntp_request.tran_time);     // Originate timestamp(T1)
    printf("Size of sntp_packet is %ld\n", sizeof(struct sntp_packet));

    ret_value = sendto(sock, &sntp_request, sizeof(struct sntp_packet), 0,
                       (struct sockaddr *)&to_addr, addr_len);
    if (ret_value < 0) {
        perror("sendto()");
        ret_value = EXIT_FAILURE;
        goto quit;
    }

    // Receive reply from time server
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    select(sock + 1, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(sock, &readfds)) {
        ret_value = recvfrom(sock,
                             (char *)&sntp_reply,
                             sizeof(struct sntp_packet),
                             0,
                             (struct sockaddr *)&to_addr,
                             &addr_len);
    } else {
        printf("\nDid not Get information from time server in 10 seconds.\n");
        ret_value = EXIT_FAILURE;
        goto quit;
    }

    if (ret_value < 0 ) {
        perror("recvfrom()");
        ret_value = EXIT_FAILURE;
        goto quit;
    }
    if (ret_value == 0) {
        printf("Received zero byte from server.\n");
        ret_value = EXIT_FAILURE;
        goto quit;
    }

    gettimeofday(&tv4, NULL);       // Destination timestamp(T4)

    // Show received data
    /*
    int i, j;
    unsigned char *p1 = (unsigned char *)&sntp_request;
    unsigned char *p2 = (unsigned char *)&sntp_reply;
    printf("\n\tSent...\t\t\t\t\tReceiving...");
    for (j = 0; j < 12; j++) {
        printf("\n");
        for (i = 0; i < 4; i++) {
            printf("\t%02x", p1[i + j * 4]);
        }
        printf("\t");
        for (i = 0; i < 4; i++) {
            printf("\t%02x", p2[i + j * 4]);
        }
    }
    */

    convert_ntp_time_into_unix_time(&sntp_reply.ori_time, &tv2);        // Receive timestamp(T2)
    convert_ntp_time_into_unix_time(&sntp_reply.tran_time, &tv3);       // Transmit timestamp(T3)

    print_sntp_packet(&sntp_reply);         // print ntp packet received from time server
    printf("Destination Timestamp(T4)\t(%ld seconds and %ld microseconds)\n", tv4.tv_sec, tv4.tv_usec);

    // calculate the difference
    double diff, t2_t1, t3_t4;
    t2_t1 = ((double)tv2.tv_sec + (double)tv2.tv_usec / 1000000) - 
            ((double)tv1.tv_sec + (double)tv1.tv_usec / 1000000);
    t3_t4 = ((double)tv3.tv_sec + (double)tv3.tv_usec / 1000000) - 
            ((double)tv4.tv_sec + (double)tv4.tv_usec / 1000000);

    diff = (t2_t1 + t3_t4) / 2;
    printf("\nThe difference is %lf seconds.\n", diff);

    struct timeval tv_diff;
    tv_diff.tv_sec = (int)diff;
    tv_diff.tv_usec = (int)((diff - (double)tv_diff.tv_sec) * 1000000);
    printf("The different in timeval format is %ld seconds & %ld microseconds.\n", tv_diff.tv_sec, tv_diff.tv_usec);

    ret_value = 0;

quit:
    close(sock);
    return ret_value;
}
