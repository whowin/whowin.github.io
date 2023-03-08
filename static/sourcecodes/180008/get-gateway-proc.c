/*
 * File: get-gateway-proc.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Get gateway IP address from proc file system under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall get-gateway-proc.c -o get-gateway-proc
 * Usage: $ sudo ./get-gateway-proc
 * 
 * Example source code for article 《从proc文件系统中获取gateway的IP地址》
 * https://whowin.gitee.io/post/blog/network/0008-get-gateway-ip-from-proc-filesys/
 *
 */
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <linux/route.h>

#define ROUTING_TABEL   "/proc/net/route"
#define BUF_SIZE        128

int get_gateway(char *gw) {
    FILE *fp;
    char line[BUF_SIZE], *ifname, *dest_ip, *gw_ip;
    int ret = 1;

    // open file /proc/net/route
    if ((fp = fopen(ROUTING_TABEL, "r")) == NULL) {
        printf("Can't open file - %s.\n", ROUTING_TABEL);
        return ret;
    }

    // skip the header
    if (fgets(line, BUF_SIZE, fp) == NULL) {
        printf("Failed to read the header of routing table.\n");
        return ret;
    }

    // read the file line by line until the gateway is found or the end of file
    while (fgets(line, BUF_SIZE, fp)) {
        ifname  = strtok(line , "\t");      // interface name
        dest_ip = strtok(NULL , "\t");      // destination IP
        gw_ip   = strtok(NULL , "\t");      // gateway IP

        if (ifname != NULL && dest_ip != NULL) {
            if (strcmp(dest_ip, "00000000") == 0) {
                printf("Interface name: %s\n", ifname);
                if (gw_ip) {
                    struct in_addr addr;
                    sscanf(gw_ip, "%x", &addr.s_addr);      // Converter string to 32bits integer
                    strcpy(gw, inet_ntoa(addr));            // Converter 32bits IP to string
                    ret = 0;
                }
                break;
            }
        }
    }

    fclose(fp);
    return ret;
}

int main() {
    char gw[16];

    if (get_gateway(gw)) printf("Failed to get gateway IP.\n");
    else printf("Gateway IP: %s\n", gw);

    return 0;    
}

