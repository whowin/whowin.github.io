/******************************************************************************
 * File Name: wakeOnLan.c
 * Description: 向局域网中的计算机发出远程唤醒的指令
 *              该程序将运行在迅雷一代赚钱宝上(已刷openwrt)，用于唤醒主机
 *
 * Compile: source /bin/a5.sh
 *          arm-openwrt-linux-gcc -Wall wakeOnLan.c -o wakeOnLan
 *
 * Usage: wakeOnLan [boradcast IP] [MAC]
 * Example: sudo ./wakeOnLan 192.168.2.255 00:e0:2b:68:00:03
 *   
 * Date: 2022-07-26
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>   ////inet_ntop
#include <netinet/in.h>  //inet_addr

#define  BIND_PORT         7                   // 端口号
#define  MSG_LEN           102                 // magic packet的长度
#define  USAGE             "wakeOnLan [broadcast IP] [MAC]\n" \
                            "Example: sudo ./wakeOnLan 192.168.1.255 00:e0:2b:69:00:03\n"

/*****************************************************
* Function: int is_IP(char *IP)
* Description: 检查IP是否为一个合法的IPv4
* Return: 1    合法的IPv4
*         0    非法的IPv4
*****************************************************/
int is_IP(char *IP) {
    int a, b, c, d;
    char e;

    if (4 == sscanf(IP, "%d.%d.%d.%d%c", &a, &b, &c, &d, &e)) {
        if (a >= 0 && a < 256 &&
            b >= 0 && b < 256 &&
            c >= 0 && c < 256 &&
            d >= 0 && d < 256) {
            return 1;
        }
    }
    return 0;
}

/*************************************************************************
* Function: int is_MAC(char *mac_str, char *mac)
* Description: 检查MAC是否为合法的MAC格式，如果合法，将其转换成6组数字放到mac中
* 
* Return: 1    合法的MAC，char *mac中为转换后的mac地址
*         0    非法的MAC
*************************************************************************/
int is_MAC(char *mac_str, char *mac) {
    int temp[6];
    char e;
    int i;

    if (6 == sscanf(mac_str, "%x:%x:%x:%x:%x:%x%c", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &e)) {
        for (i = 0; i < 6; ++i) {
            if (temp[i] < 0 || temp[i] > 255) break;
        }
        if (i == 6) {
            for (i = 0; i < 6; ++i) {
                mac[i] = temp[i];
            }
            return 1;
        }
    }
    return 0;
}

// ===================主程序================================================
int main(int argc, char *argv[]) {
    struct sockaddr_in sin;
    char *broadcast_ip;
    char wol_msg[MSG_LEN + 2];                           // magic packet
    char mac[6];
    int socket_fd;
    int i, j;
    int on = 1;
    
    // 检查参数数量
    if (argc < 3) {
        // 参数数量不对
        printf("Incorrect input parameters.\n");
        printf(USAGE);
        return -1;
    }

    // 检查第一个参数是否为一个IP地址
    if (! is_IP(argv[1])) {
        printf("%s is an invalid IPv4.\n", argv[1]);
        printf(USAGE);
        return -1;
    }

    // 检查第二个参数是否为一个MAC地址
    if (! is_MAC(argv[2], mac)) {
        printf("%s is an invalid MAC address.\n", argv[2]);
        printf(USAGE);
        return -1;
    }
    printf("MAC is %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]);

    // 建立socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        printf("Can not set up socket. Program exits(%d).\n", socket_fd);
        return -1;
    }

    // 为IP地址分配内存
    broadcast_ip = calloc(strlen(argv[1]) + 1, sizeof(char));
    if (! broadcast_ip) {
        printf("Can not allocate memory. Program exits\n");
        return -1;
    }
    // 允许在 socket_fd 上发送广播消息
    setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    
    memset((void *)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(broadcast_ip);       // 广播IP
    sin.sin_port = htons(BIND_PORT);                     // 端口号

    // 6个 ff
    for (i = 0; i < 6; i++) {
        wol_msg[i] = 0xFF;
    }
    // 16遍 mac 地址
    for (j = 0; j < 16; j++) {
        for (i = 0; i < 6; i++) {
            wol_msg[6 + j * 6 + i] = mac[i];
        }
    }
    // 发送 magic packet
    sendto(socket_fd, &wol_msg, MSG_LEN, 0, (struct sockaddr *)&sin, sizeof(sin));
    close(socket_fd);
    // 释放前面分配的内存空间
    free(broadcast_ip);

    printf("Magic Packet has been sended.\n");

    return 0;
}
