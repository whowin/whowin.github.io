/*
 * File: dbus-hostname.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Example of using libdbus to realize the DNS client via calling system service
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g dbus-hostname.c -o dbus-hostname `pkg-config --libs --cflags dbus-1`
 * Usage: $ ./dbus-hostname
 *
 * Example source code for article 《IPC之十三：使用libdbus通过D-Bus请求系统调用实现域名解析的实例》
 * https://whowin.gitee.io/post/blog/linux/0023-dbus-resolve-hostname/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/socket.h>

#include <net/if.h>
#include <arpa/inet.h>

#include <dbus/dbus.h>

#define SERVICE_NAME        "org.freedesktop.resolve1"
#define OBJECT_PATH         "/org/freedesktop/resolve1"
#define INTERFACE_NAME      "org.freedesktop.resolve1.Manager"
#define METHOD_NAME         "ResolveHostname"

#define REPLY_TIMEOUT       5000


/********************************************************************************
 * Function: void print_dbus_error(DBusError *dbus_error_p, char *str)
 * Description: Print the error message when involking dbus functions
 ********************************************************************************/
void print_dbus_error(DBusError *dbus_error_p, char *str) {
    printf("%s: %s\n", str, dbus_error_p->message);
    dbus_error_free(dbus_error_p);
}

int main(int argc, char **argv) {
    DBusError dbus_error;
    DBusConnection *conn;

    dbus_int32_t ifindex = 0;                   // means any interface
    char *hostname;                             // domain name
    dbus_int32_t afamily = AF_UNSPEC;           // Address family. means AF_INET or AF_INET6.
    dbus_uint64_t flags = 0;                    // no use. see specification

    if (argc < 2) {
        printf("Usage: %s <domain name> [address family]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    hostname = argv[1];

    if (argc > 2) {
        if (strcmp(argv[2], "AF_INET") == 0) afamily = AF_INET;
        else if (strcmp(argv[2], "AF_INET6") == 0) afamily = AF_INET6;
    }
    // Step 01: Get bus connection
    //=============================
    dbus_error_init(&dbus_error);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "dbus_bus_get()");
        exit(EXIT_FAILURE);
    }
    // Step 02: Construct a DBusMessage for sending request to system service
    //======================================================================== 
    DBusMessage *request = dbus_message_new_method_call(SERVICE_NAME, 
                                                        OBJECT_PATH, 
                                                        INTERFACE_NAME, 
                                                        METHOD_NAME);
    if (request == NULL) {
        printf("Can't construct a new message.\n");
        exit(EXIT_FAILURE);
    }

    // Step 03: Append a string to the message
    //==========================================
    /*
    Will call the 'ResolveHostname' mothod from the service - 'org.freedesktop.resolve1', path - /org/freedesktop/resolve1, 
    and interface - 'org.freedesktop.resolve1.Manager'.
    As per the official specification(https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.resolve1.html),
    ResolveHostname(in  i ifindex, in  s name, in  i family, in  t flags,
                    out a(iiay) addresses, out s canonical, out t flags);
    */
    if (!dbus_message_append_args(request, 
                                  DBUS_TYPE_INT32,
                                  &ifindex,
                                  DBUS_TYPE_STRING, 
                                  &hostname, 
                                  DBUS_TYPE_INT32,
                                  &afamily,
                                  DBUS_TYPE_UINT64,
                                  &flags,
                                  DBUS_TYPE_INVALID)) {
        printf("Can't append the string to the message.\n");
        exit(EXIT_FAILURE);
    }
    // Step 04: Send the request and waiting for reply
    //==================================================
    DBusMessage *reply;
    reply = dbus_connection_send_with_reply_and_block(conn, 
                                                      request, 
                                                      REPLY_TIMEOUT,
                                                      &dbus_error);
    dbus_message_unref(request);
    if (dbus_error_is_set(&dbus_error)) {
        print_dbus_error(&dbus_error, "dbus_connection_send_with_reply_and_block()");
        dbus_error_free(&dbus_error);
        exit(EXIT_FAILURE);
    }
    // Step 05: get the result from the reply
    //========================================= 
    int msg_type = dbus_message_get_type(reply);            // message type must be a reply
    if (msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
        printf("Wrong message type: %d\n", msg_type);
        exit(EXIT_FAILURE);
    }

    DBusMessageIter iter;
    if (!dbus_message_iter_init(reply, &iter)) {            // Create a iterator for parsing the args
        printf("Can't initial an iterator.\n");
        exit(EXIT_FAILURE);
    }

    // As per the official spcification,
    dbus_int32_t ifindex_out;       // interface index parsed from the reply
    char ifname[IF_NAMESIZE];       // interface name
    dbus_int32_t afamily_out;       // address family parsed from the reply
    char afamily_name[16];          // name of address family
    uint8_t ip[16];                 // IP address parsed from the reply
    char *hostname_out;             // canonical hostname parsed from the reply
    dbus_uint64_t flags_out;        // flags parsed from the reply

    //ResolveHostname(..., out a(iiay) addresses, out s canonical, out t flags);
    do {
        int current_type = dbus_message_iter_get_arg_type(&iter);
        if (current_type == DBUS_TYPE_STRING) {
            // get the canonical hostname
            dbus_message_iter_get_basic(&iter, &hostname_out);
            printf("The canonical hostname is %s\n", hostname_out);
            goto NEXT_ITEM;
        }
        if (current_type == DBUS_TYPE_UINT64) {
            // get the flags
            dbus_message_iter_get_basic(&iter, &flags_out);
            printf("The flags is 0x%016lx\n", flags_out);
            goto NEXT_ITEM;
        }

        if (current_type != DBUS_TYPE_ARRAY) {
            printf("Expects '%s', but it's '%c'.\n", DBUS_TYPE_ARRAY_AS_STRING, (u_int8_t)current_type);
            exit(EXIT_FAILURE);
        }

        // Address array
        // a(iiay) - address, interface index, address family and IP address
        // Construct a sub iterator for the array
        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&iter, &array_iter);
        do {
            int array_type = dbus_message_iter_get_arg_type(&array_iter);
            // The data type must be a structure 
            if (array_type != DBUS_TYPE_STRUCT) {
                printf("Expects '%s', but it's '%c'.\n", DBUS_TYPE_STRUCT_AS_STRING, (u_int8_t)array_type);
                exit(EXIT_FAILURE);
            }
            // Construct a sub iterator for the struct
            DBusMessageIter struct_iter;
            dbus_message_iter_recurse(&array_iter, &struct_iter);
            // The data type of the first element must be an INT32. It is the Interface Index.
            int struct_type = dbus_message_iter_get_arg_type(&struct_iter);
            if (struct_type != DBUS_TYPE_INT32) {
                printf("Expects '%s' for ifname, but it's '%c'.\n", DBUS_TYPE_INT32_AS_STRING, (u_int8_t)struct_type);
                exit(EXIT_FAILURE);
            }
            dbus_message_iter_get_basic(&struct_iter, &ifindex_out);
            dbus_message_iter_next(&struct_iter);

            // The data type of the second element must be an INT32. It is the address family.
            struct_type = dbus_message_iter_get_arg_type(&struct_iter);
            if (struct_type != DBUS_TYPE_INT32) {
                printf("Expects '%s' for address family, but it's '%c'.\n", DBUS_TYPE_INT32_AS_STRING, (u_int8_t)struct_type);
                exit(EXIT_FAILURE);
            }
            dbus_message_iter_get_basic(&struct_iter, &afamily_out);
            dbus_message_iter_next(&struct_iter);
            
            if_indextoname(ifindex_out, ifname);
            if (afamily_out == AF_INET) strcpy(afamily_name, "AF_INET");
            else if (afamily_out == AF_INET6) strcpy(afamily_name, "AF_INET6");
            else strcpy(afamily_name, "UNKNOWN");
            printf("Interface name: %s, address family: %s\n", ifname, afamily_name);

            // The data type of the third element must be an ARRAY. It is the IP address(IPV4 or IPV6, deponding on address family).
            struct_type = dbus_message_iter_get_arg_type(&struct_iter);
            if (struct_type != DBUS_TYPE_ARRAY) {
                printf("Expects '%s' for address family, but it's '%c'.\n", DBUS_TYPE_ARRAY_AS_STRING, (u_int8_t)struct_type);
                exit(EXIT_FAILURE);
            }
            DBusMessageIter ip_iter;
            dbus_message_iter_recurse(&struct_iter, &ip_iter);
            int j = 0;
            do {
                int ip_type = dbus_message_iter_get_arg_type(&ip_iter);
                if (ip_type != DBUS_TYPE_BYTE) {
                    printf("Expects '%s', but it's '%c'.\n", DBUS_TYPE_BYTE_AS_STRING, (u_int8_t)ip_type);
                    exit(EXIT_FAILURE);
                }
                dbus_message_iter_get_basic(&ip_iter, &ip[j]);
                ++j;
            } while (dbus_message_iter_next(&ip_iter));

            if (afamily_out == AF_INET6) {
                char ipv6_addr[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, (void *)ip, ipv6_addr, INET6_ADDRSTRLEN);
                printf("The IPv6 address is %s\n", ipv6_addr);
            } else if (afamily_out == AF_INET) {
                printf("The IP address is %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
            }
        } while (dbus_message_iter_next(&array_iter));
NEXT_ITEM:
        ;
    } while (dbus_message_iter_next(&iter));

    dbus_message_unref(reply);

    return 0;
}
