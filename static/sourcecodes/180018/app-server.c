/*
 * File: app-server.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Set up tun device and wait for connecting by client to set up an IP tunnel under Linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall app-server.c -o app-server
 * Usage: $ sudo ./app-server
 *
 * Example source code for article 《使用tun虚拟网络接口建立IP隧道的实例》
 * https://whowin.gitee.io/post/blog/network/0018-tun-example-for-setting-up-ip-tunnel/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/ip.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <arpa/inet.h> 

#define BUFSIZE         2048
//#define SERVER_IP       "192.168.2.114"
#define PORT            5678
#define TUN_DEV_NAME    "tun0"
#define TUN_DEV_FILE    "/dev/net/tun"
#define TUN_IP          "10.0.0.1"

int tun_fd = 0, sock_fd = 0, net_fd = 0;
unsigned long int tun_count = 0, nic_count = 0;

void sigint(int signum);

/******************************************************************
 * Function: void print_iphdr(struct iphdr *iph)
 * Description: Print IP header
 ******************************************************************/
void print_iphdr(struct iphdr *iph) {
    uint8_t *p1, *p2;

    if (iph->version == 4) {
        printf("===============================================\n");
        printf("Version: %d\tInternet Header Length: %d bytes\n", iph->version, iph->ihl * 4);
        printf("TOS: %d\tTotal Length: %d bytes\n", iph->tos, ntohs(iph->tot_len));
        printf("ID: %d\tFragment Offset: %d\n", ntohs(iph->id), ntohs(iph->frag_off));
        printf("TTL: %d\tProtocol: %d\tChecksum: %d\n", iph->ttl, iph->protocol, ntohs(iph->check));
        p1 = (uint8_t *)&iph->saddr;
        p2 = (uint8_t *)&iph->daddr;
        printf("Source IP: %d.%d.%d.%d\tDestination IP: %d.%d.%d.%d\n",
                p1[0], p1[1], p1[2], p1[3], p2[0], p2[1], p2[2], p2[3]);
        printf("\n");
    }

}

int main(int argc, char *argv[]) {
    uint16_t nread, nwrite;
    char buffer[BUFSIZE];
    //struct sockaddr_in remote;

    struct ifreq ifr;
    int err, ret_value = EXIT_SUCCESS;

    int maxfd = 0;
    char cmd[128];

    struct sockaddr_in server_addr;

    // Step 01: set up tun_fd and register tun0 into linux kernel
    //============================================================
    // Open device file /dev/net/tun
    if( (tun_fd = open(TUN_DEV_FILE , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        exit(EXIT_FAILURE);
    }
    // Register device name into Linux kernel
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strcpy(ifr.ifr_name, TUN_DEV_NAME);
    if( (err = ioctl(tun_fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        ret_value = EXIT_FAILURE;
        goto quit;
    }
    printf("Successfully connected to interface %s\n", TUN_DEV_NAME);

    // Step 02: set up sock_fd and listen on a port
    //==============================================
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    bzero(&server_addr, sizeof(server_addr));
    // assign IP, PORT
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr))) != 0) {
        perror("bind()");
        close(sock_fd);
        exit(EXIT_FAILURE);
    } else printf("Successfully bind an address on sock_fd.\n");

    if ((listen(sock_fd, 5)) != 0) {
        perror("listen()");
        ret_value = EXIT_FAILURE;
        goto quit;
    } else printf("Server listening..\n\n");
    
    // Step 03: Accept the connection request from the client
    //========================================================
    // Accept the data packet from client and verification
    //len = sizeof(client_addr);
    //connfd = accept(sockfd, (struct sockaddr *)&client_addr, &len);
    net_fd = accept(sock_fd, NULL, NULL);
    if (net_fd < 0) {
        perror("accept()");
        ret_value = EXIT_FAILURE;
        goto quit;
    } else printf("Accept the connection request from client.\n");

    // Step 04: set up ctrl+c handler and allocate an IP for tun0
    //============================================================
    signal(SIGINT, sigint);
    // set up IP address for tun0
    sprintf(cmd, "ifconfig %s %s/24 up", TUN_DEV_NAME, TUN_IP);
    system(cmd);
    //daemon(0, 0);

    // Process the packet received from tun0
    maxfd = (tun_fd > net_fd) ? tun_fd : net_fd;
    while(1) {
        int ret;
        fd_set rd_set;

        FD_ZERO(&rd_set);
        FD_SET(net_fd, &rd_set);
        FD_SET(tun_fd, &rd_set);

        ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
        if (ret < 0 && errno == EINTR){
            continue;
        }

        if (ret < 0) {
            perror("select()");
            ret_value = EXIT_FAILURE;
            goto quit;
        }

        // Step 05: Process data received from tun
        //=========================================
        if (FD_ISSET(tun_fd, &rd_set)) {
            // data from tun: just read it and write it to the network
            if ((nread = read(tun_fd, buffer, BUFSIZE)) < 0) {
                perror("Read from tun");
                ret_value = EXIT_FAILURE;
                goto quit;
            } else if (nread == 0) {
                // Do nothing.
            } else {
                struct iphdr *p = (struct iphdr *)buffer;
                if (p->version == 4 && p->protocol == IPPROTO_UDP) {
                    // write the data to sock_fd
                    printf("Received data from tun.\n");
                    tun_count++;
                    /*
                    nwrite = write(net_fd, buffer, nread);
                    if (nwrite < 0) {
                        perror("write");
                        ret_value = EXIT_FAILURE;
                        goto quit;
                    }
                    */
                }
            }
        }

        // Step 06: Process data received from NIC
        //=========================================
        if (FD_ISSET(net_fd, &rd_set)) {
            // data from physical nic, read it and send to tun
            // read packet
            if ((nread = read(net_fd, buffer, BUFSIZE)) < 0) {
                perror("Read from nic");
                ret_value = EXIT_FAILURE;
                goto quit;
            } else if (nread == 0) {
                // socket has been closed
                printf("The socket has been closed by client end.\n");
                ret_value = EXIT_FAILURE;
                goto quit;
            }
            // now buffer[] contains a full packet or frame
            buffer[nread] = 0;
            nic_count++;
            printf("Server received message from client.\n");
            //if (nread >= sizeof(struct iphdr)) print_iphdr((struct iphdr *)buffer);
            nwrite = write(tun_fd, buffer, nread);
            if (nwrite < 0) {
                perror("write to tun");
                ret_value = EXIT_FAILURE;
                goto quit;
            }
        }
    }

quit:
    if (sock_fd > 0) close(sock_fd);
    if (net_fd > 0) close(net_fd);
    if (tun_fd > 0) close(tun_fd);
    return(ret_value);
}

/*********************************************
 * Function: void sigint(int signum)
 * Desxription: ctrl+c handler
 *********************************************/
void sigint(int signum) {
    // Clean up ......
    if (tun_fd > 0) close(tun_fd);
    if (sock_fd > 0) close(sock_fd);
    if (net_fd > 0) close(net_fd);
    printf("Terminating....\n");
    printf("Totally packets from tun: %ld packets\n", tun_count);
    printf("Totally packets from server: %ld packets\n", nic_count);
    exit(EXIT_SUCCESS);
}
