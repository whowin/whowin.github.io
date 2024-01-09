/*
 * File: get-ipv6-netlink.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Get ipv6 address using netlink under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall get-ipv6-netlink.c -o get-ipv6-netlink
 * Usage: $ ./get-ipv6-netlink
 * 
 * Example source code for article 《C语言如何获取ipv6地址》
 *
 */
#include <asm/types.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char ** argv) {
    char buf1[16384], buf2[16384];

    struct {
        struct nlmsghdr nlhdr;
        struct ifaddrmsg addrmsg;
    } msg1;

    struct {
        struct nlmsghdr nlhdr;
        struct ifinfomsg infomsg;
    } msg2;

    struct nlmsghdr *retmsg1;
    struct nlmsghdr *retmsg2;

    int len1, len2;

    struct rtattr *retrta1, *retrta2;
    int attlen1, attlen2;

    char pradd[128], prname[128];

    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    memset(&msg1, 0, sizeof(msg1));
    msg1.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    msg1.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    msg1.nlhdr.nlmsg_type = RTM_GETADDR;
    msg1.addrmsg.ifa_family = AF_INET6;

    memset(&msg2, 0, sizeof(msg2));
    msg2.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    msg2.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    msg2.nlhdr.nlmsg_type = RTM_GETLINK;
    msg2.infomsg.ifi_family = AF_UNSPEC;

    send(sock, &msg1, msg1.nlhdr.nlmsg_len, 0);
    len1 = recv(sock, buf1, sizeof(buf1), 0);
    retmsg1 = (struct nlmsghdr *)buf1;

    while NLMSG_OK(retmsg1, len1) {
        struct ifaddrmsg *retaddr;
        retaddr = (struct ifaddrmsg *)NLMSG_DATA(retmsg1);
        int iface_idx = retaddr->ifa_index;

        retrta1 = (struct rtattr *)IFA_RTA(retaddr);
        attlen1 = IFA_PAYLOAD(retmsg1);

        while RTA_OK(retrta1, attlen1) {
            if (retrta1->rta_type == IFA_ADDRESS) {
                inet_ntop(AF_INET6, RTA_DATA(retrta1), pradd, sizeof(pradd));

                len2 = recv(sock, buf2, sizeof(buf2), 0);
                send(sock, &msg2, msg2.nlhdr.nlmsg_len, 0);
                len2 = recv(sock, buf2, sizeof(buf2), 0);
                retmsg2 = (struct nlmsghdr *)buf2;
                while NLMSG_OK(retmsg2, len2) {
                    struct ifinfomsg *retinfo;
                    retinfo = NLMSG_DATA(retmsg2);
                    memset(prname, 0, sizeof(prname));
                    if (retinfo->ifi_index == iface_idx) {
                        retrta2 = IFLA_RTA(retinfo);
                        attlen2 = IFLA_PAYLOAD(retmsg2);

                        while RTA_OK(retrta2, attlen2) {
                            if (retrta2->rta_type == IFLA_IFNAME) {
                                strcpy(prname, RTA_DATA(retrta2));
                                break;
                            }
                            retrta2 = RTA_NEXT(retrta2, attlen2);
                        }
                        break;
                    }
                    retmsg2 = NLMSG_NEXT(retmsg2, len2);       
                }
                printf("%s: %s\n", prname, pradd);
            }
            retrta1 = RTA_NEXT(retrta1, attlen1);
        }
        retmsg1 = NLMSG_NEXT(retmsg1, len1);       
    }
    return 0;
}
