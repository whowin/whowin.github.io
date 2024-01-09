/*
 * File: get-ipv6-proc.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Get ipv6 address from file /proc/net/if_inet6 under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall get-ipv6-proc.c -o get-ipv6-proc
 * Usage: $ ./get-ipv6-proc
 * 
 * Example source code for article 《C语言如何获取ipv6地址》
 *
 */
#include <stdio.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void) {
    FILE *f;
    int scope, prefix;
    char dname[IFNAMSIZ];
    char address[INET6_ADDRSTRLEN];

    f = fopen("/proc/net/if_inet6", "r");
    if (f == NULL) {
        return -1;
    }
    unsigned char _ipv6[16];
    while (19 == fscanf(f,
                        " %2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %*x %x %x %*x %s",
                        &_ipv6[0], &_ipv6[1], &_ipv6[2], &_ipv6[3], &_ipv6[4], &_ipv6[5], &_ipv6[6], &_ipv6[7],
                        &_ipv6[8], &_ipv6[9], &_ipv6[10], &_ipv6[11], &_ipv6[12], &_ipv6[13], &_ipv6[14], &_ipv6[15],
                        &prefix, &scope, dname)) {
        if (inet_ntop(AF_INET6, _ipv6, address, sizeof(address)) == NULL) {
            continue;
        }
        printf("%s: %s\n", dname, address);
    }
    fclose(f);

    return 0;
}