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
        if (conf->has_essid) {
            printf("\nESSID: %s\n", conf->essid);
        }
        if (strlen(conf->name) > 0) printf("Protocol: %s\n", conf->name);
        if (conf->has_nwid) {
            printf("NWID: %d\n", conf->nwid.value);
        }
        if (conf->has_freq) {
            if (conf->freq < 1000) {
                printf("Channel: %d\n", (int)conf->freq);
            } else {
                printf("Frequency: %.3f GHz\n", (conf->freq / (1e9)));
            }
        }
        if (result->has_ap_addr) {
            printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                    (uint8_t)result->ap_addr.sa_data[0], (uint8_t)result->ap_addr.sa_data[1], 
                    (uint8_t)result->ap_addr.sa_data[2], (uint8_t)result->ap_addr.sa_data[3], 
                    (uint8_t)result->ap_addr.sa_data[4], (uint8_t)result->ap_addr.sa_data[5]);
        }
        
        result = result->next;
    }

    free(ifname);
    iw_sockets_close(sock);
    return 0;
}