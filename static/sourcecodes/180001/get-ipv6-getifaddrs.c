/*
 * File: get-ipv6-getifaddrs.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Get ipv6 address using getifaddrs() under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall get-ipv6-getifaddrs.c -o get-ipv6-getifaddrs
 * Usage: $ ./get-ipv6-getifaddrs
 * 
 * Example source code for article 《C语言如何获取ipv6地址》
 * https://whowin.gitee.io/post/blog/network/0001-how-to-get-ipv6-in-c/
 *
 */
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>

int main () {
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in6 *sa;
    char addr[INET6_ADDRSTRLEN];

    if (getifaddrs(&ifap) == -1) {
        perror("getifaddrs");
        exit(1);
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
            // 打印ipv6地址
            sa = (struct sockaddr_in6 *)ifa->ifa_addr;
            if (inet_ntop(AF_INET6, (void *)&sa->sin6_addr, addr, INET6_ADDRSTRLEN) == NULL)
                continue;
            printf("%s: %s\n", ifa->ifa_name, addr);
        }
    }

    freeifaddrs(ifap);
    return 0;
}
