/*
 * File: get-gateway-netlink.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Get gateway IP address using netlink under linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall get-gateway-netlink.c -o get-gateway-netlink
 * Usage: $ sudo ./get-gateway-netlink
 * 
 * Example source code for article 《linux下使用netlink获取gateway的IP地址》
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <arpa/inet.h>

//#define BUF_SIZE        8192
#define IP_SIZE         16

int debug = 0;
struct route_info {
    struct in_addr dst_addr;            // destination IP address
    struct in_addr src_addr;            // source IP address
    struct in_addr gateway;             // gateway IP address
    char ifname[IF_NAMESIZE];           // network interface name
    uint32_t rt_table_id;               // route table id
    uint32_t rt_priority;               // priority of route
};

void print_bytes(unsigned char *p, int len) {
    if (!debug) return;

    int i;

    if (len > 0) printf("\t");
    for (i = 0; i < len; ++i) 
        printf("%02x ", (unsigned char)*(p + i));
    printf("\n");

}

void print_rt_attr(struct rtattr *rt_attr) {
    if (!debug) return;
    printf("\t===== Route Attribute ======\n");
    printf("\t rta_len = %d\n", rt_attr->rta_len);
    printf("\trta_type = %d(1-DST,4-OIF,5-GATEWAY,6-PRIORITY,7-PREFSRC,15-TABLE)\n", rt_attr->rta_type);
    printf("\t============================\n");
}

void print_rt_msg(struct rtmsg *rt_msg) {
    if (!debug) return;
    printf("\t==== Routing Message =========\n");
    printf("\t  rtm_family: %d(1-AF_LOCAL,2-AF_INET,10-AF_INET6\n", rt_msg->rtm_family);
    printf("\t rtm_dst_len: %d\n", rt_msg->rtm_dst_len);
    printf("\t rtm_src_len: %d\n", rt_msg->rtm_src_len);
    printf("\t     rtm_tos: 0x%02x\n", rt_msg->rtm_tos);
    // 0-UNSPEC(unknown), 253-DEFAULT(the default table), 254-MAIN(the main table), 255-LOCAL(the local table)
    printf("\t   rtm_table: %d(0-UNKNOWN,253-DEFAULT,254-MAIN,255-LOCAL)\n", rt_msg->rtm_table);
    // 0-UNSPEC(unknown), 1-REDIRECT(not used), 2-KERNEL(by the kernel), 3-BOOT(during boot), 4-STATIC(by the administrator)
    printf("\trtm_protocol: %d(0-unknown,1-not used,2-kernel,3-boot,4-administrtor)\n", rt_msg->rtm_protocol);
    // 0-UNIVERSE(global route), 200-SITE(interior route in the local autonomous system)
    // 253-LINK(route on this link), 254-HOST(route on the local host), 255-NOWHERE(destination doesn't exist)
    printf("\t   rtm_scope: %d(0-universe,200-site,253-link,254-host,255-host)\n", rt_msg->rtm_scope);
    // 0-UNSPEC(unknown), 1-UNICAST(a gateway or direct route), 2-LOCAL(a local interface route)
    // 3-BROADCAST(a local broadcast route), ......
    printf("\t    rtm_type: %d(0-UNSPEC,1-GATEWAY OR DIRECT,2-LOCAL,3-BROADCAST)\n", rt_msg->rtm_type);
    printf("\t   rtm_flags: 0x%02x\n", rt_msg->rtm_flags);
    printf("\t==============================\n");
}

void print_nl_msg_hdr(struct nlmsghdr *nl_msg_hdr) {
    if (!debug) return;
    printf("\t== Netlink Message Header ==\n");
    printf("\t  nlmsg_len = %d\n", nl_msg_hdr->nlmsg_len);
    // 1-Nothing,2-Error,3-NLMSG_DONE,24-RTM_NEWROUTE,26-RTM_GETROUTE
    printf("\t nlmsg_type = %d(2-Error,3-NLMSG_DONE,24-RTM_NEWROUTE)\n", nl_msg_hdr->nlmsg_type);
    // 0x02-NLM_F_MULTI,
    printf("\tnlmsg_flags = 0X%04X(0X02-NLM_F_MULTI)\n", nl_msg_hdr->nlmsg_flags);
    printf("\t  nlmsg_seq = %d\n", nl_msg_hdr->nlmsg_seq);
    printf("\t  nlmsg_pid = %d\n", nl_msg_hdr->nlmsg_pid);
    printf("\t============================\n");
}

/****************************************************************************
 * Function: read_nl_sock(int nl_sock, char **buf_ptr, int seq_num, int pid)
 * Description: Read data from netlink socket
 * 
 * Entry:   nl_sock     netlink socket
 *          buf_ptr     receive buffer
 *          seq_num     message sequence number
 *          pid         process id
 * return:  >0          length of received message
 *          <0          error
 ****************************************************************************/
int read_nl_sock(int nl_sock, char **buf_ptr, int seq_num, int pid) {
    char *p;
    int read_len = 0;       // how many bytes to read from the netlink socket
    int msg_len  = 0;       // how many bytes have been read from the netlink socket
    int buf_size = 0;       // the size of the receive buffer
    //int count    = 0;       // how many times have read from the netlink socket

    if (debug) printf("5-1. Check how many bytes can be read from the netlink socket.\n");
    if ((buf_size = recv(nl_sock, NULL, 0, MSG_PEEK|MSG_TRUNC)) < 0) {
        perror("Fail to receive from socket");
        return -1;
    }
    // No data can be read
    if (buf_size == 0) {
        printf("EOF of socket.\n");
        return 0;
    }

    if (debug) printf("5-2. Reallocate memory for receiving the response from the kernel.\n");
    // caculate the size of the receive buffer. plus sizeof(struct nlmsghdr) & sizeof(struct rtmsg)
    buf_size += NLMSG_SPACE(sizeof(struct rtmsg));
    if (debug) printf("\tNew buffer size: %d.\n", buf_size);
    // Reallocate memory for receive buffer
    p = realloc(*buf_ptr, buf_size);
    if (p == NULL) {
        perror("Realloc(): ");
        return -1;
    }
    *buf_ptr = p;
    memset(p, 0, buf_size);

    if (debug) printf("5-3. Receive response from kernel.\n");

    do {
        // Recieve response from the kernel
        read_len = recv(nl_sock, p, buf_size - msg_len, MSG_DONTWAIT);
        if (read_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (debug) printf("\tBreak due to EAGAIN or EWOULDBLOCK.\n");
                break;
            } else {
                printf("errno=%d\n", errno);
                perror("SOCK READ: ");
                return -1;
            }
        } else if (read_len == 0) {
            if (debug) printf("\tBreak due to reading nothing.\n");
            break;
        }

        p += read_len;
        msg_len += read_len;
    } while (msg_len < buf_size && read_len > 0);

    return msg_len;
}

/*************************************************************************************
 * Function: void parse_message(struct nlmsghdr *nl_hdr, struct route_info *rt_info)
 * Description: parse the route info returned
 * 
 * Entry:   nl_hdr      pointer of struct nlmsghdr
 *          rt_info     pointer of struct route_info
 * return:  none
 *          routes info will fill into rtinfo
 *************************************************************************************/
void parse_message(struct nlmsghdr *nl_hdr, struct route_info *rt_info) {
    struct rtmsg *rt_msg;
    struct rtattr *rt_attr;
    int rt_len;

    if (debug) printf("6-1. Check if the route msg is IPv4 route table.\n");
    rt_msg = (struct rtmsg *)NLMSG_DATA(nl_hdr);
    print_rt_msg(rt_msg);

    // If the route is not for AF_INET or does not belong to main routing table then return.
    if ((rt_msg->rtm_family != AF_INET) || (rt_msg->rtm_table != RT_TABLE_MAIN)) {
        if (debug) printf("\tIt is not the IPv4 route table. Just return.\n");
        return;
    }

    // get the rtattr field
    if (debug) printf("\tIt is the IPv4 route table.\n");
    rt_attr = (struct rtattr *)RTM_RTA(rt_msg);
    rt_len = RTM_PAYLOAD(nl_hdr);       // return length of all rtattr

    if (debug) printf("6-2. Parse the route attribute.\n");
    unsigned char *p;
    for (; RTA_OK(rt_attr, rt_len); rt_attr = RTA_NEXT(rt_attr, rt_len)) {
        print_rt_attr(rt_attr);
        switch (rt_attr->rta_type) {
            case RTA_OIF:
                // rta_data is index of network interface. converter it to ifterface name here.
                if_indextoname(*(int *)RTA_DATA(rt_attr), rt_info->ifname);
                if (debug) printf("\tInterface name: %s\n", rt_info->ifname);
                break;

            case RTA_GATEWAY:
                // rta_date is gateway ip in 32bits(struct in_addr).
                memcpy(&rt_info->gateway, RTA_DATA(rt_attr), sizeof(rt_info->gateway));
                p = (unsigned char *)&rt_info->gateway;
                if (debug) printf("\tGateway IP: %d.%d.%d.%d\n", *p, *(p + 1), *(p + 2), *(p + 3));
                break;

            case RTA_PREFSRC:
                // Preferred source IP address in 32bits(struct in_addr)
                memcpy(&rt_info->src_addr, RTA_DATA(rt_attr), sizeof(rt_info->src_addr));
                p = (unsigned char *)&rt_info->src_addr;
                if (debug) printf("\tSource IP: %d.%d.%d.%d\n", *p, *(p + 1), *(p + 2), *(p + 3));
                break;

            case RTA_DST:
                // Destination IP address in 32 bits(struct in_addr)
                memcpy(&rt_info->dst_addr, RTA_DATA(rt_attr), sizeof(rt_info->dst_addr));
                p = (unsigned char *)&rt_info->dst_addr;
                if (debug) printf("\tDestination IP: %d.%d.%d.%d\n", *p, *(p + 1), *(p + 2), *(p + 3));
                break;

            case RTA_TABLE:
                // Routing table ID. 
                memcpy(&rt_info->rt_table_id, RTA_DATA(rt_attr), sizeof(rt_info->rt_table_id));
                if (debug) printf("\tRouting Table ID: 0x%08x\n", rt_info->rt_table_id);
                break;

            case RTA_PRIORITY:
                // Priority of route.
                memcpy(&rt_info->rt_priority, RTA_DATA(rt_attr), sizeof(rt_info->rt_priority));
                if (debug) printf("\tPriority of route: 0x%08x\n", rt_info->rt_priority);
                break;

            default:
                printf("\t***It is an unknown route attribute***\n");
                printf("\trta_len = %d, rta_type = 0x%04x****\n\t", rt_attr->rta_len, rt_attr->rta_type);
                print_bytes((unsigned char *)(&rt_attr + sizeof(rt_attr)), (rt_attr->rta_len - sizeof(struct rtattr)));
                break;
        }
    }

    return;
}

int get_gateway_ip(char *gateway_ip, int size) {
    int found_gateway_ip = 0;

    struct nlmsghdr *nl_msg_hdr;
    struct rtmsg *rt_msg;
    struct route_info *rt_info;
    char *msg_buf;

    int msg_buf_len;
    int nl_sock;
    int nlmsg_count = 0;
    int msg_seq = 0;

    if (debug) printf("1. Create a netlink socket.\n");
    if ((nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        perror("Socket Creation: ");
        return(-1);
    }

    if (debug) printf("2. Allocate memory for sending the nelink request.\n");
    msg_buf_len = NLMSG_SPACE(sizeof(struct rtmsg));    // sizeof(struct nlmsghdr) + sizeof(struct rtmsg)
    msg_buf = malloc(msg_buf_len);
    memset(msg_buf, 0, msg_buf_len);

    if (debug) printf("3. Fill netlink msg header and route msg structure for sending the netlink request.\n");
    // point the netlink msg header(nl_msg_hdr) and the route msg(rt_msg) structure pointers into the buffer
    nl_msg_hdr = (struct nlmsghdr *)msg_buf;
    rt_msg = (struct rtmsg *)NLMSG_DATA(nl_msg_hdr);

    // Fill in the nlmsg header
    nl_msg_hdr->nlmsg_len   = msg_buf_len;                      // Length of message.
    nl_msg_hdr->nlmsg_type  = RTM_GETROUTE;                     // Get the routes from kernel routing table .

    nl_msg_hdr->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;       // The message is a request for dump.
    msg_seq = time(NULL);
    nl_msg_hdr->nlmsg_seq   = msg_seq;                          // Sequence of the message packet.
    nl_msg_hdr->nlmsg_pid   = getpid();                         // PID of process sending the request.
    rt_msg->rtm_family  = AF_INET;

    if (debug) printf("4. Send the netlink request.\n");
    // Send the netlink request
    if (send(nl_sock, nl_msg_hdr, nl_msg_hdr->nlmsg_len, 0) < 0) {
        fprintf(stderr, "Write To Socket Failed...\n");
        free(msg_buf);
        return -1;
    }

    if (debug) printf("5. Receive the response from the kernel.\n");
    // Receive the response from the kernel
    if ((msg_buf_len = read_nl_sock(nl_sock, &msg_buf, msg_seq, getpid())) < 0) {
        fprintf(stderr, "Read From Socket Failed...\n");
        free(msg_buf);
        return -1;
    }

    if (debug) printf("\tData length read from socket: %d\n", msg_buf_len);

    if (debug) printf("6. Parse the message received from the kernel.\n");
    // msg_buf has been changed in function read_nl_sock(), so we have to set it again.
    nl_msg_hdr = (struct nlmsghdr *)msg_buf;
    // allocate memory for storing the item of routing table
    rt_info = (struct route_info *)malloc(sizeof(struct route_info));

    for (; NLMSG_OK(nl_msg_hdr, msg_buf_len); nl_msg_hdr = NLMSG_NEXT(nl_msg_hdr, msg_buf_len)) {
        if (debug) printf("\tNetlink msg No.: %d\n", ++nlmsg_count);
        memset(rt_info, 0, sizeof(struct route_info));
        print_nl_msg_hdr(nl_msg_hdr);
        if (nl_msg_hdr->nlmsg_type == NLMSG_DONE) {
            printf("\tEnd of Netlink Routing Message.\n");
            break;
        }
        parse_message(nl_msg_hdr, rt_info);             // parse a netlink message

        // Check if default gateway
        if (strstr((char *)inet_ntoa(rt_info->dst_addr), "0.0.0.0")) {
            // copy it over
            inet_ntop(AF_INET, &rt_info->gateway, gateway_ip, size);
            found_gateway_ip = 1;
            break;
        }
    }

    if (debug) printf("7. Clean up and Return.\n");
    free(msg_buf);
    free(rt_info);
    close(nl_sock);

    return found_gateway_ip;
}

int main(int argc, char **argv) {
    char gateway_ip[IP_SIZE] = {0};     // Store gateway IP

    if (argc > 1) debug = 1;
    if (get_gateway_ip(gateway_ip, IP_SIZE)) {
        printf("\n==== Gateway IP: %s ====\n", gateway_ip);
    } else {
        printf("Failed to get gateway IP.\n");
    }

    return 0;
}
