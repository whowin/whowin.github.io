/*
 * File: iw-scanner.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * A program that uses ioctl to scan wifi signals
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall iw-scanner.c -o iw-scanner -liw
 * Usage: $ sudo ./iw-scanner
 * 
 * Example source code for article 《Linux下使用libiw进行无线信号扫描的实例》
 *
 */
#include <stdio.h>
#include <time.h>
#include <iwlib.h>

// Check if ifname is a wireless interface
// If so, ifname will be copied to args[0]
int iw_ifname(int skfd, char *ifname, char *args[], int count) {
    struct iwreq pwrq;
    memset(&pwrq, 0, sizeof(pwrq));
    strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(skfd, SIOCGIWNAME, &pwrq) != -1) {
        strncpy(args[0], ifname, IFNAMSIZ);
        printf("Found a wireless interface: %s\n", ifname);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    wireless_scan_head head;
    wireless_scan *result;
    iwrange range;
    wireless_config *conf;
    char *ifname;
    int sock;

    // Open socket to kernel
    sock = iw_sockets_open();

    // Get interface name of wireless
    ifname = malloc(IFNAMSIZ + 1);
    memset(ifname, 0, IFNAMSIZ + 1);
    iw_enum_devices(sock, &iw_ifname, &ifname, 1);      // Enumerate all interface

    if (strlen(ifname) == 0) {
        printf("There is no any wireless interface.\n");
        free(ifname);
        return -1;
    }

    // Get some metadata to use for scanning
    if (iw_get_range_info(sock, ifname, &range) < 0) {
        printf("Error during iw_get_range_info. Aborting.\n");
        free(ifname);
        return -1;
    }

    // Perform the scan
    if (iw_scan(sock, ifname, range.we_version_compiled, &head) < 0) {
        printf("Error during iw_scan. Aborting.\n");
        free(ifname);
        return -1;
    }

    // Traverse the results
    result = head.result;
    while (result != NULL) {
        conf = &result->b;
        // ESSID
        if (conf->has_essid) {
            printf("\nESSID: %s\n", conf->essid);
        }
        if (strlen(conf->name) > 0) printf("Protocol: %s\n", conf->name);
        if (conf->has_nwid) {
            printf("NWID: %d\n", conf->nwid.value);
        }
        // Frequency
        if (conf->has_freq) {
            if (conf->freq < 1000) {
                printf("Channel: %d\n", (int)conf->freq);
            } else {
                printf("Frequency: %.3f GHz\n", (conf->freq / (1e9)));
            }
        }
        // MAC address
        if (result->has_ap_addr) {
            printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                    (uint8_t)result->ap_addr.sa_data[0], (uint8_t)result->ap_addr.sa_data[1], 
                    (uint8_t)result->ap_addr.sa_data[2], (uint8_t)result->ap_addr.sa_data[3], 
                    (uint8_t)result->ap_addr.sa_data[4], (uint8_t)result->ap_addr.sa_data[5]);
        }
        // Max. bitrate
        if (result->has_maxbitrate) {
            printf("Max. bitrate: %d Mb/s\n", result->maxbitrate.value/(int)(1e6));
        }
        //
        if (result->has_stats) {
            printf("Signal quality: %d/70\n", result->stats.qual.qual);
            printf("Signal level: %d dBm\n", (int8_t)result->stats.qual.level);
        }
        
        result = result->next;
    }

    free(ifname);
    iw_sockets_close(sock);
    return 0;
}