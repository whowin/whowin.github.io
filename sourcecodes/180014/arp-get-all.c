/*
 * File: arp-get-all.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Get all ARP entries from proc file system under Linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall arp-get-all.c -o arp-get-all
 * Usage: $ sudo ./arp-get-all
 * 
 * Example source code for article 《如何用C语言操作arp cache》
 *
 */
#include <stdio.h>

#define xstr(s) str(s)
#define str(s) #s

#define ARP_CACHE_FILE      "/proc/net/arp"
#define ARP_STRING_LEN      1023
#define ARP_BUFFER_LEN      (ARP_STRING_LEN + 1)

/* Format for fscanf() to read the 1st, 4th, and 6th space-delimited fields */
#define ARP_LINE_FORMAT     "%" xstr(ARP_STRING_LEN) "s %*s %*s " \
                            "%" xstr(ARP_STRING_LEN) "s %*s " \
                            "%" xstr(ARP_STRING_LEN) "s"

int main() {
    FILE *arp_cache = fopen(ARP_CACHE_FILE, "r");
    if (!arp_cache) {
        perror("Arp Cache: Failed to open file \"" ARP_CACHE_FILE "\"");
        return 1;
    }

    // Ignore the first line, which contains the header
    char header[ARP_BUFFER_LEN];
    if (!fgets(header, sizeof(header), arp_cache)) {
        return 1;
    }

    char ip_addr[ARP_BUFFER_LEN], hw_addr[ARP_BUFFER_LEN], device[ARP_BUFFER_LEN];
    int count = 0;
    while (3 == fscanf(arp_cache, ARP_LINE_FORMAT, ip_addr, hw_addr, device)) {
        printf("%03d: Mac Address of [%s] on [%s] is \"%s\"\n",
                ++count, ip_addr, device, hw_addr);
    }
    fclose(arp_cache);
    return 0;
}
