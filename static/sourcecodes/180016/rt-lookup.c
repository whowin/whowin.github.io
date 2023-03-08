/*
 * File: rt-lookup.c
 * Author: Songqing Hua
 *
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Route lookup simulator under Linux
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall rt-lookup.c -o rt-lookup
 * Usage: $ sudo ./rt-lookup
 * 
 * Example source code for article 《简单的路由表查找程序》
 * https://whowin.gitee.io/post/blog/network/0016-longest-prefix-match/
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define M               15
#define N               150
#define LINE_SIZE       200
#define IFNAME_SIZE     16

#define ROUTING_FILE    "routing.txt"

struct node {
    char ifname[IFNAME_SIZE];
    uint32_t net;
    uint32_t gateway;
    uint32_t mask;
    struct node *next;
};

struct node *head = NULL;

/******************************************************
 * Function: bool is_valid_ip(const char *ip)
 * Description: Check if ip is valid
 * 
 * Return   false   ip is invalid
 *          true    ip is valid
 *****************************************************/
bool is_valid_ip(const char *ip) {
    int dots    = 0;
    int setions = 0;
    int strnum  = 0;

    if (NULL == ip || *ip == '.') {
        return false;
    }

    while (*ip) {
        if (*ip == '.') {
            dots++;
            if (setions >= 0 && setions <= 255) {
                setions = 0;
            } else {
                return false;
            }
        } else if (*ip >= '0' && *ip <= '9') {
            setions = setions * 10 + (*ip - '0');
        } else {
            return false;
        }
        ip++;
        if (++strnum > 15) {
            return false;
        }
    }

    if (setions >= 0 && setions <= 255) {                 
        if (dots == 3) {
            return true;
        }
    }
    return  false;
}

/******************************************************************
 * Function: void print_route(struct node *route)
 * Description: Print a route
 * 
 * Return   none
 ******************************************************************/
void print_route(struct node *route) {
    printf("ifname: %s\tnet: %d.%d.%d.%d\tgataway: %d.%d.%d.%d\tmask: %d.%d.%d.%d\n",
            route->ifname, 
            (uint8_t)route->net, (uint8_t)*((uint8_t *)&route->net + 1), 
            (uint8_t)*((uint8_t *)&route->net + 2), (uint8_t)*((uint8_t *)&route->net + 3),
            (uint8_t)route->gateway, (uint8_t)*((uint8_t *)&route->gateway + 1), 
            (uint8_t)*((uint8_t *)&route->gateway + 2), (uint8_t)*((uint8_t *)&route->gateway + 3),
            (uint8_t)route->mask, (uint8_t)*((uint8_t *)&route->mask + 1), 
            (uint8_t)*((uint8_t *)&route->mask + 2), (uint8_t)*((uint8_t *)&route->mask + 3));
}

/******************************************************************
 * Function: int read_routing()
 * Description: Read the routing table from file 'routing.txt'
 * 
 * Format of the routing table file: 1st row is header, then data. 
 *      Each line represents one routing entry
 *      [ifname], [destination ip], [gateway ip], [netmask]
 * 
 * Return   >0      how many routes being read
 *          =0      no route being read
 ******************************************************************/
int read_routing() {
    FILE *fp;
    char line[LINE_SIZE], *ifname, *net, *gateway, *mask, *p, *comma;
    int i = 0, flag = 0, j;
    struct node *new_node = NULL, *curr_node = NULL;

    if ((fp = fopen(ROUTING_FILE, "r")) == NULL) {
        printf("Can't open routing file. %s\n", ROUTING_FILE);
        return -1;
    }
    // Read data from the file line by line and each line is stored in array separately.
    while (fgets(line, sizeof(line), fp)) {
        if (i++ == 0) continue;             // Skip the header of routing table
        flag = 0;
        ifname = NULL;
        net = NULL;
        gateway = NULL;
        mask = NULL;

        p = line;
        for (flag = 0; flag < 4; ++flag) {
            while(isspace((int)*p) && *p != 0) ++p;
            if (*p == 0) goto error;
            if (flag < 3) comma = index(p, ',');
            if (comma) {
                switch (flag) {
                case 0:
                    ifname = p;
                    break;

                case 1:
                    net = p;
                    break;

                case 2:
                    gateway = p;
                    break;

                case 3:
                    mask = p;
                    j = strlen(mask) - 1;
                    while (*(p + j) == '\n') *(p + j--) = 0;
                    break;

                default:
                    break;
                }
            } else {
                if (flag < 3) {
                    goto error;
                }
            }
            *comma = 0;
            j = strlen(p);
            while (isspace((int)*(p + --j))) *(p + j) = 0;
            p = ++comma;
        }
        if (ifname == NULL || net == NULL || gateway == NULL || mask == NULL) {
            goto error;
        }

        new_node = malloc(sizeof(struct node));
        memset(new_node, 0, sizeof(struct node));
        if (curr_node == NULL) {
            head = new_node;
        } else {
            curr_node->next = new_node;
        }
        curr_node = new_node;

        new_node->next = NULL;
        strncpy(new_node->ifname, ifname, IFNAME_SIZE - 1);
        new_node->net = inet_addr(net);
        new_node->gateway = inet_addr(gateway);
        new_node->mask = inet_addr(mask);
    }

    // display all entries of routing table
    curr_node = head;
    printf("Routing table before sorting.\n");
    while (curr_node != NULL) {
        print_route(curr_node);
        curr_node = curr_node->next;
    }

    // sort by net
    do {
        j = 0;
        curr_node = head;
        struct node *prev = NULL, *next = NULL;
        while (curr_node->next != NULL) {
            next = curr_node->next;
            if ((ntohl(curr_node->net) < ntohl(next->net)) ||
                ((ntohl(curr_node->net) == ntohl(next->net)) && 
                 (ntohl(curr_node->mask) < ntohl(next->mask)))) {
                struct node *temp;
                temp = next->next;
                next->next = curr_node;
                curr_node->next = temp;
                j++;
                if (prev == NULL) {
                    head = next;
                    prev = next;
                } else {
                    prev->next = next;
                    prev = next;
                }
            } else {
                prev = curr_node;
                curr_node = next;
            }
        }
    } while (j > 0);
    // display all entries of routing table again
    printf("\n");
    printf("Routing table after sorting.\n");
    curr_node = head;
    while (curr_node != NULL) {
        print_route(curr_node);
        curr_node = curr_node->next;
    }

    if (fp) fclose(fp);
    return i;

error:
    printf("Wrong format in file %s.\n", ROUTING_FILE);
    if (fp) fclose(fp);
    return 0;
}

void free_node() {
    struct node *next = NULL;

    while(head != NULL) {
        next = head->next;
        free(head);
        head = next;
    }
}

/**********************************************************************
 * Function: int route_lookup(char *ip, struct node *result)
 * Description: Look up the route in routing table
 * 
 * Return:  1       Found the route
 *          0       Not found 
 **********************************************************************/
int route_lookup(char *ip, struct node *result) {
    struct node *route = NULL;
    uint32_t val;
    int is_found = 0;

    route = head;
    while (route != NULL) {
        // Perform bitwise AND operation
        val = inet_addr(ip) & route->mask;
        if (val == route->net) {
            is_found = 1;
            *result = *route;
            break;
        }
        route = route->next;
    }

    return is_found;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s [IP address]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!is_valid_ip(argv[1])) {
        printf("Wrong IP format.\n");
        exit(EXIT_FAILURE);
    }
    // Read routing table from file routing.txt
    if (read_routing() <= 0) {
        printf("Can't read routing table file.\n");
        exit(EXIT_FAILURE);
    }

    // route lookup
    struct node result;
    if (!route_lookup(argv[1], &result)) {
        printf("\nNo result\n");
    } else {
        printf("\nResult(%s):\n", argv[1]);
        print_route(&result);
    }

    free_node();
    return EXIT_SUCCESS;
}
