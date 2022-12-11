---
title: "How to get ipv6 address in C language"
date: 2022-10-16T16:43:29+08:00
author: whowin
sidebar: false
authorbox: false
toc: true
pager: true
categories:
  - "C Language"
tags:
  - C语言
  - ipv6
draft: false
#references: 
# - [Get IPv6 addresses in linux using ioctl](https://stackoverflow.com/questions/20743709/get-ipv6-addresses-in-linux-using-ioctl)
# - [IPv6-related entries in /proc/net/](https://tldp.org/HOWTO/Linux+IPv6-HOWTO/ch11s04.html)
# - [scanf, fscanf, sscanf, scanf_s, fscanf_s, sscanf_s](https://en.cppreference.com/w/c/io/fscanf)
# - [C代码，用于在Linux中获取IP地址的接口名称](http://www.955yes.com/ask/127327909.html)
postid: 130003
---

It is impossible to get an ipv6 address by using the usual method of getting the IP address of ipv4. This article introduces three methods for getting the ipv6 address using the C language. Each method provides the complete source code. All examples in this article are tested under ubuntu 20.04, gcc version 9.4.0.
<!--more-->

## 1. How to get the IP address of ipv4
* Whether to get an IP address for ipv4 or an address for ipv6, the application needs to communicate with the kernel.
* ioctl is a common method for communicating with the kernel, and is also a common method for getting the IP address of ipv4. The following code demonstrates how to use ioctl to get the IP addresses of all interfaces on the computer:
    ```
    #include <stdio.h>
    #include <stdlib.h>

    #include <sys/ioctl.h>
    #include <linux/if.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    int main() {
        int i = 0;
        int sockfd;
        struct ifconf ifc;
        char buf[512] = {0};
        struct ifreq *ifr;

        ifc.ifc_len = 512;
        ifc.ifc_buf = buf;

        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("socket");
            return -1;
        }
        ioctl(sockfd, SIOCGIFCONF, &ifc);
        ifr = (struct ifreq*)buf;

        for (i = (ifc.ifc_len /sizeof(struct ifreq)); i > 0; i--) {
            printf("%s: %s\n",ifr->ifr_name, inet_ntoa(((struct sockaddr_in *)&(ifr->ifr_addr))->sin_addr));
            ifr++;
        }
    }
    ```
* But using ioctl can't get ipv6 address. Even if we create an AF_INET6 socket, the ioctl still only returns ipv4 information. We can try the following code.
    ```
    #include <stdio.h>
    #include <stdlib.h>

    #include <sys/ioctl.h>
    #include <linux/if.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    int main() {
        int i = 0;
        int sockfd;
        struct ifconf ifc;
        char buf[1024] = {0};
        struct ifreq *ifr;

        ifc.ifc_len = 1024;
        ifc.ifc_buf = buf;

        if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
            perror("socket");
            return -1;
        }
        ioctl(sockfd, SIOCGIFCONF, &ifc);
        ifr = (struct ifreq*)buf;

        struct sockaddr_in *sa;
        for (i = (ifc.ifc_len /sizeof(struct ifreq)); i > 0; i--) {
            sa = (struct sockaddr_in *)&(ifr->ifr_addr);
            if (sa->sin_family == AF_INET6) {
                printf("%s: AF_INET6\n", ifr->ifr_name);
            } else if (sa->sin_family == AF_INET){
                printf("%s: AF_INET\n", ifr->ifr_name);
            } else {
                printf("%s: %d.  It is an unknown address family.\n", ifr->ifr_name, sa->sin_family);
            }
            ifr++;
        }
    }
    ```
* The result of running this code on my computer looks like this:

    ![ioctl cannot get ipv6 address][img01]
    - **Figure 1: ioctl cannot get ipv6 address**
-----------------------

* We see that no matter what we do, it still returns only the ipv4 address. So we need some other way to get the ipv6 address. The following describes three methods to obtain ipv6 address using C language.

## 2. Get ipv6 address from file /proc/net/if_inet6
* Let's take a look at what's in the file /proc/net/if_inet6.

    ![Contents of file /proc/net/if_inet6][img02]
    - **Figure 2: Contents of file /proc/net/if_inet6**
----------------------

* In this file, each line contains data for one network interface. The data for each row is divided into 6 fields.

    |Field #|Field Name|Description|
    |:-----:|:------|:-----|
    |1|ipv6address|IPv6 address, each group is a 32-bit hexadecimal number. There are no colons between numbers.|
    |2|ifindex|Interface index number. Each device is different and is displayed in hexadecimal.|
    |3|prefixlen|The prefix length displayed in hexadecimal, similar to the subnet mask of ipv4.|
    |4|scopeid|scope id|
    |5|flags|Interface flags. These flags represent the features of the interface.|
    |6|devname|Device name.|

* The ipv6 addresses of all interfaces can be easily obtained from this file.
    ```
    #include <stdio.h>
    #include <linux/if.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    int main(void) {
        FILE *f;
        int scope, prefix;
        unsigned char _ipv6[16];
        char dname[IFNAMSIZ];
        char address[INET6_ADDRSTRLEN];

        f = fopen("/proc/net/if_inet6", "r");
        if (f == NULL) {
            return -1;
        }
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
    ```
* %2hhx in fscanf is a rare usage. hhx means that the pointer behind &_ipv6[x] points to an unsigned char *. 2 represents the length read from the file.
* For the usage of hh and h in fscanf, you can check the online manual. Learn more with the command ''man fscanf'';
* The ipv6 address has a total of 128 bits. 16 bits as a group, a total of 8 groups. But here why not read 4 characters (16 bits) from the file at a time, 8 times, and 2 characters at a time and read 16 times?
    - Let's look at the parameters of inet_ntop. Use the command 'man inet_ntop' to take a look at the online manual for inet_ntop.
        ```
        const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
        ```
    - When the first parameter af = AF_INET6, there is a detailed description for the second parameter:
        ```
        AF_INET6
            src  points  to  a struct in6_addr (in network byte order) which is converted to a representation of
            this address in the most appropriate IPv6 network address format for this address.  The  buffer  dst
            must be at least INET6_ADDRSTRLEN bytes long.
        ```
    - Obviously, the second parameter is required to point to a struct in6_addr. This structure is defined in netinet/in.h:
        ```
        /* IPv6 address */
        struct in6_addr
        {
            union
            {
                uint8_t  __u6_addr8[16];
                uint16_t __u6_addr16[8];
                uint32_t __u6_addr32[4];
            } __in6_u;
        #define s6_addr         __in6_u.__u6_addr8
        #ifdef __USE_MISC
        # define s6_addr16      __in6_u.__u6_addr16
        # define s6_addr32      __in6_u.__u6_addr32
        #endif
        };
        ```
    - This structure generally uses uint8_t __u6_addr8[16], which is an array of 16 unsigned char. Arrays of 8 unsigned short int or 4 unsigned int are only used in "promiscuous mode".
    - So, in fact, the structure of struct in6_addr is as follows:
        ```
        struct in6_addr {
            unsigned char __u6_addr8[16];
        }
        ```
    - That's why when reading the file only 2 characters are read at a time. To ensure that the read content conforms to the definition of struct in6_addr.
* What's the difference between reading 4 characters (16 bits) from a file at a time, 8 times, and reading 2 characters at a time, 16 times? Let's take f8e9 in hexadecimal as an example.
    - When f8e9 is stored in the file in ASCII, it occupies 4 bytes. After converting it into a number according to hexadecimal, there are 16 bits in total, occupying 2 bytes.
    - When we read 2 characters from the file each time and convert them into numbers, the arrangement of the numbers in memory is like this.
        ```
        unsigned char _ipv6[16];
        fscanf(f, "%2hhx2hhx", &_ipv6[0], &_ipv6[1]);
        unsigned char *p = _ipv6
        
        f8  e9
        -+  -+
         |   |
         |   +------- p + 1
         +----------- p
        ```
    - When we read 4 characters from a file and convert them to numbers, the arrangement of the numbers in memory is like this.
        ```
        unsigned int _ipv6[8]
        fscanf(f, "%4x", &_ipv6[0])
        unsigned char *p = (unsigned char *)_ipv6
        e9  f8
        -+  -+
         |   |
         |   +------- p + 1
         +----------- p
        ```
    - This is because the storage mode of the X86 series CPU is the little endian mode, that is, the high-order byte is stored at the high address. f8e9 This number, f8 is the high-order byte, and e9 is the low-order byte. So when we read f8e9 as an integer, e9 will be stored at a low address, and f8 will be stored at a high address, which is inconsistent with the definition of struct in6_addr.
    - So if we read 4 characters at a time, 8 times, we can't use inet_ntop() to convert the ipv6 address to the string we need. Of course, you can convert it yourself, but it is a bit troublesome, refer to the code below.
        ```
        unsigned short int _ipv6[8];
        int zero_flag = 0;
        while (11 == fscanf(f,
                            " %4hx%4hx%4hx%4hx%4hx%4hx%4hx%4hx %*x %x %x %*x %s",
                            &_ipv6[0], &_ipv6[1], &_ipv6[2], &_ipv6[3], &_ipv6[4], &_ipv6[5], &_ipv6[6], &_ipv6[7],
                            &prefix, &scope, dname)) {
            printf("%s: ", dname);
            for (int i = 0; i < 8; ++i) {
                if (_ipv6[i] != 0) {
                    if (i) putc(':', stdout); 
                    printf("%x", _ipv6[i]);
                    zero_flag = 0;
                } else {
                    if (!zero_flag) putc(':', stdout);
                    zero_flag = 1;
                }
            }
            putc('\n', stdout);
        }
        ```
    - Compared with the above code, this code is a bit cumbersome.    

## 3. Use getifaddrs() to get ipv6 address
* Detailed information on getifaddrs() can be found in the online manual 'man getifaddrs'.
* getifaddrs() will create a linked list of local network interfaces defined in struct ifaddrs.
* There are many articles about the structure of ifaddrs. This article only briefly introduces the content that is closely related to this article. The following is the definition of struct ifaddrs.
    ```
    struct ifaddrs {
        struct ifaddrs  *ifa_next;    /* Next item in list */
        char            *ifa_name;    /* Name of interface */
        unsigned int     ifa_flags;   /* Flags from SIOCGIFFLAGS */
        struct sockaddr *ifa_addr;    /* Address of interface */
        struct sockaddr *ifa_netmask; /* Netmask of interface */
        union {
            struct sockaddr *ifu_broadaddr;
                            /* Broadcast address of interface */
            struct sockaddr *ifu_dstaddr;
                            /* Point-to-point destination address */
        } ifa_ifu;
    #define              ifa_broadaddr ifa_ifu.ifu_broadaddr
    #define              ifa_dstaddr   ifa_ifu.ifu_dstaddr
        void            *ifa_data;    /* Address-specific data */
    };
    ```
* ifa_next is the back pointer of the linked list, pointing to the next item in the linked list. When the current item is the last item, the pointer is NULL.
* ifa_addr is the main item used in this article, which is struct sockaddr. Take a look at the definition of struct sockaddr:
    ```
    struct sockaddr {
        sa_family_t sa_family;
        char        sa_data[14];
    }
    ```
* Actually, when ifa_addr->sa_family is AF_INET, ifa_addr points to struct sockaddr_in. When ifa_addr->sa_family is AF_INET6, ifa_addr points to a struct sockaddr_in6.
* The two structures of sockaddr_in and sockaddr_in6 can also find a lot of introduction articles, so I won't talk about them here. There are also structures nested within this structure, and you have to straighten your thinking to avoid confusion.
* Below is the source code to use getifaddrs() to get ipv6 address. As you can see, the lines that print the ipv6 address are the same as the example above.
    ```
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
                // print ipv6 address
                sa = (struct sockaddr_in6 *)ifa->ifa_addr;
                if (inet_ntop(AF_INET6, (void *)&sa->sin6_addr, addr, INET6_ADDRSTRLEN) == NULL)
                    continue;
                printf("%s: %s\n", ifa->ifa_name, addr);
            }
        }

        freeifaddrs(ifap);
        return 0;
    }
    ```
* It should be noted that after using getifaddrs(), you must remember to use freeifaddrs() to release the memory occupied by the linked list.
* In this example, we use inet_ntop() to convert the sin6_addr structure to an ipv6 address in string form, you can also use getnameinfo() to do this conversion.
* Details of getnameinfo() can be found in the online manual 'man getnameinfo'.
* Below is the source code that uses getifaddrs() to get the ipv6 address and use getnameinfo() to convert the ipv6 address to a string.
    ```
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <stdio.h>
    #include <stdlib.h>

    #include <netdb.h>

    int main () {
        struct ifaddrs *ifap, *ifa;
        char addr[INET6_ADDRSTRLEN];

        if (getifaddrs(&ifap) == -1) {
            perror("getifaddrs");
            exit(1);
        }

        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
                // print ipv6 address
                if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr, sizeof(addr), NULL, 0, NI_NUMERICHOST))
                    continue;
                printf("%s: %s\n", ifa->ifa_name, addr);
            }
        }

        freeifaddrs(ifap);
        return 0;
    }
    ```
* Compared with the previous program, this program adds an include file netdb.h. There are some related definitions of getnameinfo() here.
* When using getnameinfo(), make sure that ifa->ifa_addr points to a struct sockaddr_in6, and the following constant NI_NUMERICHOST indicates that the returned host address is a numeric string.
* Slightly different from the above example, the ipv6 address obtained with getnameinfo() will use '%' at the end to connect the name of a network interface, as shown in the following figure:

    ![Use getnameinfo() to get the ipv6 address][img03]
    - **Figure 3: Use getnameinfo() to get the ipv6 address**
------------------

## 4. get ipv6 address using netlink
* netlink socket is another method for user space to communicate with kernel space. This article does not discuss the programming of netlink, but gives the source code to get ipv6 address using netlink.
* Compared with the above two methods, using netlink to get ipv6 address is slightly more complicated. This method is rare in actual programming, so this article will not discuss it more.
* Below is the source code to get ipv6 address using netlink.
    ```
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
    ```
## 5. epilogue
* This article provides three methods for getting ipv6 addresses, all of which provide complete source codes.
* This article does not discuss the three methods in detail to avoid the article lengthy.
* To get the ipv6 address, the first two methods are more common and simple.
* Generally, there are four ways for user programs to communicate with the kernel
    1. System call
    2. Virtual file system (/proc, /sys, etc.)
    3. ioctl
    4. netlink
* The source codes in this article use the three methods 2, 3, and 4 above. There is no way to simply use a system call to get an ipv6 address.






[img01]:/images/130003/ipv6_ioctl.png
[img02]:/images/130003/file_if_inet6.png
[img03]:/images/130003/ipv6_getnameinfo.png

