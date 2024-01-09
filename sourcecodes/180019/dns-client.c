/*
 * File: dns-client.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * DNS resolver example
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall dns-client.c -o dns-client
 * Usage: $ ./dnstest www.baidu.com
 * 
 * Example source code for article 《用C语言实现的一个DNS客户端》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#define DNS_SERVER          "114.114.114.114"       // DNS server
#define DNS_PORT            53                      // DNS port

#define TYPE_HOST           0x01                    // type=host，RFC-1035 PAGE 12
#define TYPE_CNAME          0x05                    // type=cname

#define FLAGS_QR_MASK       0x8000                  // bit mask of QR
#define FLAGS_OPCODE_MASK   0x7800                  // bit mask of OPCODE
#define FLAGS_AA_MASK       0x0400
#define FLAGS_TC_MASK       0x0200
#define FLAGS_RD_MASK       0x0100                  // bit mask of RD
#define FLAGS_RA_MASK       0x0080                  // bit mask of RA
#define FLAGS_Z_MASK        0x0030

#define DNS_HDR_ID          0xf101
#define DNS_HDR_FLAGS       FLAGS_RD_MASK           // the value of flags when sending msg
#define CLASS_IN            0x01                    // QCLASS=IN(internet)

#define QTYPE_LEN           2
#define QCLASS_LEN          2
#define TTL_LEN             4
#define RDLENGTH_LEN        2

#define BUF_SIZE            512                     // buffer size for receiving
#define MAX_NAME_SIZE       256                     // max. lengen of a name

#define RECV_TIMEOUT        5                       // timeout when receiving data from dns server

struct dnshdr {
    uint16_t id;
    uint16_t flags;
    uint16_t qu_count;      // Number of questions
    uint16_t an_count;      // Number of answer rr
    uint16_t au_count;      // Number of authority rr
    uint16_t ad_count;      // Number of additional rr
};

void dump_hex(unsigned char *p, int len) {
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
/*********************************************************************************
 * Function: int parse_name(uint8_t *p_name, char *name, int len)
 * description: Parse a name in labels or compression scheme
 * 
 * entries: msg     point to the beginning of the message
 *          p_name  point to the beginning of the name
 *          name    store the name parsing from p_name
 *          len     max. length of name
 * return: octets occupied by p_name
 *********************************************************************************/
int parse_name(uint8_t *msg, uint8_t *p_name, char *name, int len) {
    int name_len = 0, ret = 0;
    char *pname = name;
    uint8_t *p = p_name;

    // calculate how many bytes that name occupied in RR
    if ((*p & 0xc0) != 0xc0) {
        while (*p != 0) {
            if ((*p & 0xc0) == 0xc0) {
                ret += 2;
                break;
            } else {
                ret += (*p + 1);
                p += (*p + 1);
            }
        }
    } else ret = 2;

    // parse the name
    p = p_name;
    while (*p != 0) {
        if ((*p & 0xc0) == 0xc0) {
            // Compression scheme
            int offset = (((int)(*p & 0x3f)) << 8) + (int)*(p + 1);
            p = (uint8_t *)(msg + offset);
        } else {
            // labels
            int n;
            if ((name_len + *p) < len) {
                n = *p;
            } else {
                n = len - name_len;
            }

            // Transform name into human readable domain name
            strncpy(pname, (char *)(p + 1), n);
            p += (*p + 1);
            pname += n;
            name_len += n;
            if (name_len < (len - 1)) {
                *pname = '.';
                pname++;
                name_len++;
            }
        }
    }
    if (*(pname - 1) == '.') {
        pname--;
        name_len--;
        *pname = 0;
    }
    return ret;
}
/***********************************************************************************
 * Function: int recv_response_from_dns(int sockfd)
 * Description: Receive response from DNS server and then parse the message
 * 
 * Entry:   sockfd      a socket using to receive response
 ***********************************************************************************/
int recv_response_from_dns(int sockfd){
    // allocate memory for receiving buffer
    uint8_t *buf = malloc(BUF_SIZE);
    memset(buf, 0, BUF_SIZE);
    // DNS server's address will be stores in addr when recvfrom()
    struct sockaddr_in addr;
    unsigned int addr_len = sizeof(struct sockaddr_in);
    // Receive response from DNS server
    int n = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
    if (n < 0) {
        perror("recvfrom()");
        return -1;
    } else if (n == 0) {
        printf("Nothing received.\n");
        return -1;
    } else if (n <= sizeof(struct dnshdr)) {
        printf("The message may be trunked.\n");
        return -1;
    }

    //dump_hex(buf, n);

    struct dnshdr *dns_hdr = (struct dnshdr *)buf;      // point to DNS header
    uint16_t flags = ntohs(dns_hdr->flags);             // get flags field from response
    if ((flags & FLAGS_QR_MASK) == 0) {                 // QR=0 request. QR=1 response
        // It is not a response message
        printf("It is not a reponse message.\n");
        goto quit;
    }
    if ((flags & FLAGS_OPCODE_MASK) > 0) {              // Opcode=0 standard response
        printf("It is not a standard reponse message.\n");
        goto quit;
    }
    uint16_t ancount = ntohs(dns_hdr->an_count);        // how many answers

    uint8_t *p_question = buf + sizeof(struct dnshdr);  // point to question part
    uint8_t *p = p_question;
    while (*p > 0) {
        p += *p;
        p++;
    }
    p++;
    uint8_t *p_answer = p + QTYPE_LEN + QCLASS_LEN;     // point to answer part

    char *owner_name;           // owner name
    char *cname;                // canonical name
    int name_len = 0;           // octets occupied by name
    uint16_t ans_type;          // type in rr of answer
    //uint32_t ans_ttl;           // ttl in rr of answer
    uint16_t rdlength;          // rdlength of rr
    uint8_t *p_rdata;           // point to rdata of rr

    printf("Number of answers: %d\n", ancount);
    owner_name = malloc(MAX_NAME_SIZE);
    int i;
    for (i = 0; i < ancount; ++i) {
        printf("\nAnswer No.%d\n", i + 1);

        memset(owner_name, 0, MAX_NAME_SIZE);
        name_len = parse_name(buf, p_answer, owner_name, MAX_NAME_SIZE);
        ans_type = ntohs(*(int16_t *)(p_answer + name_len));
        //ans_ttl = ntohl(*(int32_t *)(p_answer + name_len + QTYPE_LEN + QCLASS_LEN));
        rdlength = ntohs(*(int16_t *)(p_answer + name_len + QTYPE_LEN + QCLASS_LEN + TTL_LEN));

        p_rdata = p_answer + name_len + QTYPE_LEN + QCLASS_LEN + TTL_LEN + RDLENGTH_LEN;
        if (ans_type == TYPE_HOST) {
            // host ip in rdata. ip points to rdata.
            printf("The owner name: %s\n", owner_name);
            printf("ip: %d.%d.%d.%d\n", p_rdata[0], p_rdata[1], p_rdata[2], p_rdata[3]);
        } else if (ans_type == TYPE_CNAME) {
            // canonical name in rdata
            cname = malloc(MAX_NAME_SIZE);
            memset(cname, 0, MAX_NAME_SIZE);
            parse_name(buf, p_rdata, cname, MAX_NAME_SIZE);
            printf("The alias name: %s\n", owner_name);
            printf("The canonical name: %s\n", cname);
            free(cname);
        }
        // point to next answer
        p_answer = p_answer + name_len + QTYPE_LEN + QCLASS_LEN + TTL_LEN + RDLENGTH_LEN + rdlength;
    }
    free(owner_name);

quit:
    free(buf);
    return 0;
}
/**************************************************************************
 * Function: int send_request_to_dns(int sockfd, const char *domain_name)
 * Description: Send DNS request to DNS server
 * 
 * Entry:   sockfd          socket for sending msg
 *          domain_name     domain name to query
 **************************************************************************/
int send_request_to_dns(int sockfd, const char *domain_name) {
    // how many octets will be occupied in request msg by name to query
    int dns_name_len = strlen(domain_name) + 2;
    // total length of request message
    int dns_request_len = sizeof(struct dnshdr) + dns_name_len + QTYPE_LEN + QCLASS_LEN;
    // allocate memory for request message
    unsigned char *dns_request = malloc(dns_request_len);
    memset(dns_request, 0, dns_request_len);

    struct dnshdr *dns_header = (struct dnshdr *)dns_request;       // point to DNS header
    uint8_t *dns_name = dns_request + sizeof(struct dnshdr);        // point to name
    uint16_t *qtype = (uint16_t *)(dns_name + dns_name_len);        // point to qtype
    uint16_t *qclass = (uint16_t *)(dns_name + dns_name_len + 2);   // point to qclass

    // fill domain name to query in labels format
    char *p = (char *)dns_name;
    strcpy(p + 1, domain_name);
    char *pdot;
    while ((pdot = index(p + 1, '.')) != NULL) {
        *pdot = 0;
        *p = strlen(p + 1);
        p = pdot;
    }
    *p = strlen(p + 1);

    // Fill rest fields of the request
    *qtype = htons(TYPE_HOST);
    *qclass = htons(CLASS_IN);
    // Fill the DNS header for request message
    dns_header->id = htons(DNS_HDR_ID);
    dns_header->flags = htons(DNS_HDR_FLAGS);
    dns_header->qu_count = htons(1);        // 1 question
    dns_header->an_count = 0;
    dns_header->au_count = 0;
    dns_header->ad_count = 0;

    // The address of DNS server
    struct sockaddr_in dns_addr;
    memset(&dns_addr, 0, sizeof(struct sockaddr_in));
    dns_addr.sin_family      = AF_INET;
    dns_addr.sin_port        = htons(DNS_PORT);
    dns_addr.sin_addr.s_addr = inet_addr(DNS_SERVER);

    // send request to DNS server
    int ret = sendto(sockfd, dns_request, dns_request_len, 0, (struct sockaddr *)&dns_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        perror("sendto()");
    }

    free(dns_request);
    return ret;
}

/**************************************************************************************
 * main
 **************************************************************************************/
int main(int argc , char *argv[]) {
    int sockfd;

    if (argc != 2) {
        printf("Usage: %s <domain name>\n", argv[0]);
        exit(-1);
    }
    // create socket for DNS server
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(-1);
    }
    // Set timeout for receiving data from DNS server
    struct timeval timeout;
    timeout.tv_sec  = RECV_TIMEOUT;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt() with SO_SOCKET:SO_RCVTIMEO");
        return EXIT_FAILURE;
    }


    printf("%s\n", argv[1]);

    if (send_request_to_dns(sockfd, argv[1]) > 0) {
        recv_response_from_dns(sockfd);
    }

    close(sockfd);
    return 0;
}

