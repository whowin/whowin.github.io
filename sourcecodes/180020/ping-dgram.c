/*
 * File: ping-dgram.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * A simple ping program using SOCK_DGRAM under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall ping-dgram.c -o ping-dgram -lm
 * Usage: $ ./ping-dgram baidu.com
 * 
 * Example source code for article 《使用SOCK_DGRAM类型的socket实现的ping程序》
 * https://whowin.gitee.io/post/blog/network/0020-implement-ping-program-with-sock-dgram/
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <math.h>

#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>

#define ICMP_DATA_SIZE      (64 - sizeof(struct icmphdr))      // data size of icmp packet
#define RECV_BUF_SIZE       1024    // size of receive buffer
#define RECV_TIMEOUT        5       // timeout when receiving ICMP reply
#define IPADDR_SIZE         16      // max IP length in numbers-and-dots
#define PING_INTERVAL       1       // ping interval in seconds
#define MIN_TIME_ORIGINAL   10000   // The original value of min. trip time
#define DEST_PORT           0       // Destination port number

int ping_loop = true;               // true - loop for ping, false - quit from loop

// structure of ICMP packet
struct icmp_packet {
    struct icmphdr icmp_hdr;        // icmp header
    uint8_t data[ICMP_DATA_SIZE];   // data of icmp packet
};
// ICMP statistics for time
struct pingtimes {
    float trip_time;            // current trip time 
    float min_time;             // minimium trip time
    float max_time;             // maximium trip time
    float sum_time;             // sum of all trip times
    float qsum_time;            // quadratic sum of all trip time
};

struct pingtimes ping_times;    // variables about time
pid_t pid;                      // process ID
int nsend = 0, nreceived = 0;   // number of packets sent/received
int last_seq = -1;              // The last sequence No. of packet received
int ttl_val = 64;               // TTL(Time To Live) value

void sigint(int signum);        // int handle for catching ctrl+c

// it is just for debuging
void dumpHex(unsigned char *p, int len) {
    int i;

    for (i = 0; i < len; ++i) {
        if (i % 8 == 0) {
            printf("\n");
        }
        printf("0x%02x  ", *p);
        ++p;
    }
    printf("\n");

}

// Check the IP address is valid or not
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
            setions = setions * 10 + (*ip - '0');
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

// Calculat the internet checksum
uint16_t checksum(uint16_t *addr, int len) {
    register long sum = 0;
    uint16_t *w = addr;
    uint16_t check_sum = 0;
    int nleft = len;
 
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    // Add left-over byte, if any
    if (nleft == 1) {
        check_sum = *(unsigned char *)w;
        sum += check_sum;
    }
    // Add carries
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    check_sum = ~(uint16_t)sum;     // one's complement
    return check_sum;
}

// Pack an ICMP packet
int pack(int pack_no, char *send_buf, int datalen) {
    int pack_size;
    struct icmp_packet *ping_p;
    struct timeval *tval;
    
    ping_p = (struct icmp_packet *)send_buf;
    ping_p->icmp_hdr.type = ICMP_ECHO;              // ICMP_ECHO packet
    ping_p->icmp_hdr.code = 0;                      // code=0
    ping_p->icmp_hdr.checksum = 0;                  // checksum will be calculated later
    ping_p->icmp_hdr.un.echo.sequence = pack_no;    // serial no
    ping_p->icmp_hdr.un.echo.id = pid;              // process id
 
    pack_size = sizeof(struct icmphdr) + datalen;   // the total length of icmp packet
    tval = (struct timeval *)ping_p->data;
    gettimeofday(tval, NULL);                       // fill a sending timestamp into data
    char *p = (char *)ping_p->data + sizeof(struct timeval);
    int i = pack_size - sizeof(struct timeval);
    memset(p, '0', i);                              // fill '0' into rest place of send_buf

    // checksum
    ping_p->icmp_hdr.checksum = checksum((uint16_t *)ping_p, pack_size); 
    return pack_size;
}

// Send an ICMP packet with socket
int send_packet(int sockfd, struct sockaddr_in *dest_addr) {
    size_t buf_size = sizeof(struct icmphdr) + ICMP_DATA_SIZE;
    char *send_buf = malloc(buf_size);                          // sending buffer
    memset(send_buf, 0, buf_size);
    int packet_size = pack(nsend, send_buf, ICMP_DATA_SIZE);    // set ICMP header and data
    int ret = 0;

    ret = sendto(sockfd, send_buf, packet_size, 0,
               (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        perror("sendto()");
        return -1;
    }
    ++nsend;
    free(send_buf);
    return 0;
}

// Unpack an ICMP packet received
int unpack(char *buf, int len, pid_t pid, struct in_addr *dest) {
    struct icmp_packet *ping_packet;            // point to ICMP packet
    int icmp_len;
 
    struct timeval *send_time_p;                // Send time
    struct timeval recv_time;
    gettimeofday(&recv_time, NULL);             // Receive time

    ping_packet = (struct icmp_packet *)buf;    // Point to ICMP header
    icmp_len = len;                             // The length of ICMP packet
    if (icmp_len < sizeof(struct icmphdr)) {    // invalid if less than the length of ICMP header
        printf("ICMP packet\'s length is less than ICMP header length.\n");
        return -1;
    }
    // Only process ICMP ECHO REPLY packet
    if (ping_packet->icmp_hdr.type != ICMP_ECHOREPLY) {
        return -2;
    }
    if (ping_packet->icmp_hdr.un.echo.id == pid) return -3;
    if (ping_packet->icmp_hdr.un.echo.sequence <= last_seq) {
        printf("Duplicate packet\n");
        return -4;
    }
    last_seq = ping_packet->icmp_hdr.un.echo.sequence;
    // calculate the checksum of packet
    if (checksum((uint16_t *)buf, len) != 0) {
        printf("Lost. Wrong checksum\n");
        return -5;
    }

    send_time_p = (struct timeval *)ping_packet->data;  // point to sending time
    float interval = (recv_time.tv_sec - send_time_p->tv_sec) * 1000.00 + ((recv_time.tv_usec - send_time_p->tv_usec) * 1.00) / 1000;

    ping_times.sum_time += interval;
    ping_times.qsum_time += (interval * interval);
    if (interval < ping_times.min_time) ping_times.min_time = interval;
    if (interval > ping_times.max_time) ping_times.max_time = interval;

    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.2f\n", 
            len, inet_ntoa(*dest), (unsigned int)ping_packet->icmp_hdr.un.echo.sequence, 
            ttl_val, interval);
    return (unsigned int)ping_packet->icmp_hdr.un.echo.sequence;
}

// Receive ICMP packets
int recv_packet(int sockfd) {
    int n;
    socklen_t from_len;
    int ret = -1;
    struct sockaddr_in from;
    char recv_buf[RECV_BUF_SIZE];       // receive buffer

    from_len = sizeof(from);
    while (nreceived <= nsend) {
        // Receive a packet
        n = recvfrom(sockfd, recv_buf, RECV_BUF_SIZE, 0, (struct sockaddr *)&from, &from_len);
        if (n < 0) {
            if (errno == EWOULDBLOCK) {
                // timeout
                printf("Lost. Timeout\n");
            } else {
                perror("recvfrom()");
            }
            break;
        } else if (n > 0) {
            ret = unpack(recv_buf, n, pid, &from.sin_addr);
            if (ret >= 0) nreceived++;
            if (ret == nsend - 1) {
                break;
            }
        }
    }
    return ret;
}

// Do DNS lookup
int dns_lookup(char *hostname, struct sockaddr_in *dest_addr) {
    struct hostent *host;
    struct in_addr ipaddr;
    int i;

    if ((host = gethostbyname(hostname)) == NULL) {
        // fail to DNS lookup
        herror("gethostbyname()");
        return -1;
    }
    // DNS lookup is okay.
    printf("Host name: %s\n", host->h_name);
    printf("Address type: %s\n", (host->h_addrtype == AF_INET) ? "IPV4" : "IPV6");
    if (host->h_addrtype != AF_INET) {
        printf("It is not IPv4 address.\n");
        return -1;
    }

    // ipv4 address
    for (i = 0; host->h_aliases[i] != NULL; ++i) {
        printf("Aliases #%d: %s\n", i + 1, host->h_aliases[i]);
    }
    for (i = 0; host->h_addr_list[i] != NULL; ++i) {
        ipaddr.s_addr = *(in_addr_t *)host->h_addr_list[i];
        if (i == 0) dest_addr->sin_addr.s_addr = ipaddr.s_addr;
        printf("IP address #%d: %s\n", i + 1, inet_ntoa(ipaddr));
    }
    return 0;
}

int main(int argc, char **argv) {
    int sockfd;
 
    struct sockaddr_in d_addr;      // destination address
    struct protoent *protocol;      // protocol

    if (argc != 2) {
        printf("Usage: %s <domain name or IP>\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&d_addr, 0, sizeof(struct sockaddr_in));
    memset(&ping_times, 0, sizeof(struct pingtimes));
    ping_times.min_time = MIN_TIME_ORIGINAL;
    // Step 1: Convert host name to IP address
    //==========================================
    // check if the input from cli is an IP address or a host name
    if (inet_addr(argv[1]) == INADDR_NONE) {
        // host name. 
        if (dns_lookup(argv[1], &d_addr) < 0)
            return EXIT_FAILURE;
    } else {
        // input is ip address
        if (is_vaild_ip(argv[1])) {
            d_addr.sin_addr.s_addr = inet_addr(argv[1]);
        } else {
            printf("%s is neither a valid IP nor a valid hostname.\n", argv[1]);
            return EXIT_FAILURE;
        }
        
    }
    printf("The IP address: %s\n", inet_ntoa(d_addr.sin_addr));
    d_addr.sin_family = AF_INET;
    d_addr.sin_port = htons(DEST_PORT);

    // Step 2: Get ICMP protocol ID
    //================================
    if ((protocol = getprotobyname("icmp")) == NULL) {
        printf("No ICMP protocol.\n");
        return EXIT_FAILURE;
    }
    // Step 3: Create a socket for ICMP protocol
    //============================================
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, protocol->p_proto)) < 0) {
        perror("socket()");
        return EXIT_FAILURE;
    }
    // Step 4: Set timeout for socket when receiving ICMP reply
    //==========================================================
    struct timeval timeout;
    timeout.tv_sec  = RECV_TIMEOUT;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt() with SO_SOCKET:SO_RCVTIMEO");
        return EXIT_FAILURE;
    }
    // Step 5: Set TTL for IP layer
    //==============================
    if (setsockopt(sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) < 0) {
        perror("setsockopt() with SOL_IP:IP_TTL");
        return EXIT_FAILURE;
    }
    // Step 6: Catch INT handler for ctrl+c
    //=======================================
    signal(SIGINT, sigint);             // catching interrupt

    // Step 7: Record the start time
    //===============================
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    // Step 8: ping loop
    //====================
    pid = getpid();
    putchar('\n');
    while (ping_loop) {
        sleep(PING_INTERVAL);
        send_packet(sockfd, &d_addr);   // send ICMP ECHO packet
        recv_packet(sockfd);            // receive ICMP ECHO REPLY packet
    }
    // Step 9: Calculate the statistics of ping
    //==========================================
    gettimeofday(&end_time, NULL);
    uint32_t total_time = (end_time.tv_sec * 1000 + end_time.tv_usec / 1000) - 
                          (start_time.tv_sec * 1000 + start_time.tv_usec / 1000);
    printf("\n--- %s ping statistics ---\n", argv[1]);
    printf("%d packets transmitted, %d received, %d packet loss, time %d ms\n", 
            nsend, nreceived, (nsend - nreceived), total_time);
    float avg = 0.0, mdev = 0.0;
    if (nreceived) {
        avg = (ping_times.sum_time * 1.00) / nreceived;
        mdev = sqrt(((ping_times.qsum_time * 1.00) / nreceived) - (avg * avg));
    }
    if (ping_times.min_time == MIN_TIME_ORIGINAL) ping_times.min_time = 0;
    printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", 
            ping_times.min_time, avg, ping_times.max_time, mdev);

    // Step 10: Clean up....
    //========================
    close(sockfd);
    return EXIT_SUCCESS;
}

// Signal handler for catching ctrl+c
void sigint(int signum) {
    ping_loop = false;
}
