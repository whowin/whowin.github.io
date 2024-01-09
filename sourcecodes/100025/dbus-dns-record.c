/*
 * File: dbus-dns-recrd.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Example of using libdbus to realize the DNS client via calling system service
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g dbus-dns-record.c -o dbus-dns-record `pkg-config --libs --cflags dbus-1`
 * Usage: $ ./dbus-dns-record
 *
 * Example source code for article 《IPC之十五：使用libdbus通过D-Bus请求系统调用实现任意DNS记录解析的实例》
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
#define METHOD_NAME         "ResolveRecord"

#define DNS_CLASS_IN        1U              // class=IN(internet)
#define DNS_CLASS_ANY       255U            // class=ANY

#define DNS_TYPE_A          1U              // type=A，RFC-1035 PAGE 12
#define DNS_TYPE_CNAME      5U              // type=CNAME
#define DNS_TYPE_MX         15U             // type=MX

#define TYPE_LEN            2               // bytes of TYPE field
#define CLASS_LEN           2               // bytes of CLASS field
#define TTL_LEN             4               // bytes of TTL field
#define RDLENGTH_LEN        2               // bytes of rdlength field

#define RDATA_BUF_SIZE      512             // buffer size for receiving rdata
#define MAX_NAME_SIZE       256             // max. lengen of a name

#define REPLY_TIMEOUT       5000            // millisecond, timeout when getting response from DNS service

/********************************************************************************
 * Function: void print_dbus_error(DBusError *dbus_error_p, char *str)
 * Description: Print the error message when involking dbus functions
 ********************************************************************************/
void print_dbus_error(DBusError *dbus_error_p, char *str) {
    printf("%s: %s\n", str, dbus_error_p->message);
    dbus_error_free(dbus_error_p);
}
/**************************************************************************
 * Function: int parse_name(uint8_t *p, char *hostname)
 * Description: Parsing the name from the response of DNS service
 **************************************************************************/
int parse_name(uint8_t *p, char *name) {
    int name_len = 0;
    int i = 0;
    int j = 0;
    while (p[name_len] != 0) {
        int label_len = (int)p[name_len];
        if (j > 0) {
            name[j] = '.'; 
            j++;
        }
        for (i = 0; i < label_len; ++i) {
            name[j + i] = p[name_len + i + 1];
        }
        j += label_len;
        name_len += (label_len + 1);
    }
    ++name_len;
    name[j] = 0;

    return name_len;
}
/**********************************************************************
 * Function: int parse_ip(uint8_t *p, char *ip)
 * Description: Parsing IP address from rdata
 **********************************************************************/
int parse_ip(uint8_t *p, char *ip) {
    int ip_len = ntohs(*(int16_t *)p);
    if (ip_len != 4) return -1;
    sprintf(ip, "%d.%d.%d.%d", p[2], p[3], p[4], p[5]);
    return 0;
}
/*********************************************************************
 * Function: int parse_mx(uint8_t *p, char *mx)
 * Description: Parsing the domain name of mail exchange from rdata
 *********************************************************************/
int parse_mx(uint8_t *p, char *mx) {
    // skip the rdlength and references fields. Refer to RFC-1035.
    return parse_name((p + 4), mx);
}
/*********************************************************************
 * Function: parse_cname(uint8_t *p, char *cname)
 * Description: Parsing the domain name for an alias from rdata
 *********************************************************************/
int parse_cname(uint8_t *p, char *cname) {
    // skip the rdlength field. Refer to RFC-1035.
    return parse_name((p + 2), cname);
}


int main(int argc, char **argv) {
    DBusError dbus_error;
    DBusConnection *conn;

    dbus_int32_t ifindex = 0;                   // means any interface
    char *domain_name;                          // domain name
    dbus_uint16_t qclass = DNS_CLASS_IN;        // class
    dbus_uint16_t qtype = DNS_TYPE_A;           // type
    dbus_uint64_t flags = 0;                    // set 0. refer to D-Bus specification

    if (argc < 2) {
        printf("Usage: %s <domain name> [HOST/CNAME/MX]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    domain_name = argv[1];

    if (argc > 2) {
        if (strcmp(argv[2], "A") == 0) qtype = DNS_TYPE_A;
        else if (strcmp(argv[2], "CNAME") == 0) qtype = DNS_TYPE_CNAME;
        else if (strcmp(argv[2], "MX") == 0) qtype = DNS_TYPE_MX;
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

    // Step 03: Append arguments to the message
    //==========================================
    /*
    Will call the 'ResolveRecord' mothod from the service - 'org.freedesktop.resolve1', path - /org/freedesktop/resolve1, 
    and interface - 'org.freedesktop.resolve1.Manager'.
    As per the official specification(https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.resolve1.html),
    ResolveRecord(in  i ifindex, in  s name, in  q class, in  q type, in  t flags,
                  out a(iqqay) records, out t flags);
    */
    if (!dbus_message_append_args(request, 
                                  DBUS_TYPE_INT32,
                                  &ifindex,
                                  DBUS_TYPE_STRING, 
                                  &domain_name, 
                                  DBUS_TYPE_UINT16,
                                  &qclass,
                                  DBUS_TYPE_UINT16,
                                  &qtype,
                                  DBUS_TYPE_UINT64,
                                  &flags,
                                  DBUS_TYPE_INVALID)) {
        printf("Can't append the argument to the message.\n");
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
    // Step 05: getting the message type of the reply
    //================================================= 
    int msg_type = dbus_message_get_type(reply);            // message type must be a reply
    if (msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
        printf("Wrong message type: %d\n", msg_type);
        exit(EXIT_FAILURE);
    }
    // Step 06: parsing the data received from systemd-resolved
    //==========================================================
    DBusMessageIter iter;
    if (!dbus_message_iter_init(reply, &iter)) {            // Create a iterator for parsing the args
        printf("Can't initial an iterator.\n");
        exit(EXIT_FAILURE);
    }

    /******************************************************************************************* 
     * As per the official spcification, the reply should be a(iqqay)
     * "iqqay" means interface index, class, type, and rdata. Refer to the structure below
     * struct {
     *          int32_t     ifindex,
     *          uint16_t    class,
     *          uint16_t    type,
     *          uint8_t     rrdata[]
     * } records[];
     * about rrdata, refer to RFC-1035
     *                                 1  1  1  1  1  1
     *   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
     * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     * |                                               |
     * /                                               /
     * /                      NAME                     /
     * |                                               |
     * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     * |                      TYPE                     |
     * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     * |                     CLASS                     |
     * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     * |                      TTL                      |
     * |                                               |
     * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     * |                   RDLENGTH                    |
     * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
     * /                     RDATA                     /
     * /                                               /
     * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     * The structure and content of rrata depends on TYPE.
     * The RDATA will be an IP address when type is A.
     * The RDATA will be a hostname when type is CNAME.
     * The RDATA will be a domain name of a mail exchange when type is MX.
     *******************************************************************************************/
    dbus_int32_t ifindex_out;           // interface index parsed from the reply
    char ifname[IF_NAMESIZE];           // interface name converted from ifindex
    dbus_uint16_t class_out;            // class - always IN(internet)
    dbus_uint16_t type_out;             // type - this article only supports A(1), CNAME(5) and MX(15)
    char hostname[MAX_NAME_SIZE];       // hostname
    uint8_t rrdata[RDATA_BUF_SIZE];     // buffer for rdata
    int rrdata_len = 0;                 // length of rdata
    char ip[16] = {0};                  // IP address parsed from the reply
    dbus_uint64_t flags_out;            // flags parsed from the reply

    // ResolveRecord(..., out a(iqqay) records, out t flags);
    do {
        // Step 06-1: parsing the "flags"
        //--------------------------------
        // Processing "flags" first is just to make the program clearer.
        int current_type = dbus_message_iter_get_arg_type(&iter);
        if (current_type == DBUS_TYPE_UINT64) {
            // get the flags
            dbus_message_iter_get_basic(&iter, &flags_out);
            //printf("The flags is 0x%016lx\n", flags_out);
            goto NEXT_ITEM;
        }
        // Step 06-2: parsing an array - A(iqqay)
        //-----------------------------------------
        if (current_type != DBUS_TYPE_ARRAY) {
            printf("Expects '%s', but it's '%c'.\n", DBUS_TYPE_ARRAY_AS_STRING, (u_int8_t)current_type);
            exit(EXIT_FAILURE);
        }

        // Construct a sub iterator for the array
        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&iter, &array_iter);
        do {
            // Step 06-2-1: parsing a structure under the array - A ( iqqay)
            //---------------------------------------------------------------
            int array_type = dbus_message_iter_get_arg_type(&array_iter);
            // The data type must be a structure 
            if (array_type != DBUS_TYPE_STRUCT) {
                printf("Expects '%s', but it's '%c'.\n", DBUS_TYPE_STRUCT_AS_STRING, (u_int8_t)array_type);
                exit(EXIT_FAILURE);
            }
            // Construct a sub iterator for the struct
            DBusMessageIter struct_iter;
            dbus_message_iter_recurse(&array_iter, &struct_iter);
            // Step 06-2-2: parsing the interface index - A(Iqqay)
            //-----------------------------------------------------
            // The data type must be an INT32. It is the Interface Index.
            int struct_type = dbus_message_iter_get_arg_type(&struct_iter);
            if (struct_type != DBUS_TYPE_INT32) {
                printf("Expects '%s' for ifname, but it's '%c'.\n", DBUS_TYPE_INT32_AS_STRING, (u_int8_t)struct_type);
                exit(EXIT_FAILURE);
            }
            dbus_message_iter_get_basic(&struct_iter, &ifindex_out);
            dbus_message_iter_next(&struct_iter);
            // Step 06-2-3: parsing the class - A(IQqay)
            //-------------------------------------------
            // The data type must be an UINT16. It is the class.
            struct_type = dbus_message_iter_get_arg_type(&struct_iter);
            if (struct_type != DBUS_TYPE_UINT16) {
                printf("Expects '%s' for class, but it's '%c'.\n", DBUS_TYPE_UINT16_AS_STRING, (u_int8_t)struct_type);
                exit(EXIT_FAILURE);
            }
            dbus_message_iter_get_basic(&struct_iter, &class_out);
            dbus_message_iter_next(&struct_iter);
            // Step 06-2-4: parsing the type - A(IQQay)
            //------------------------------------------
            // The data type must be an UINT16. It is the type.
            struct_type = dbus_message_iter_get_arg_type(&struct_iter);
            if (struct_type != DBUS_TYPE_UINT16) {
                printf("Expects '%s' for type, but it's '%c'.\n", DBUS_TYPE_UINT16_AS_STRING, (u_int8_t)struct_type);
                exit(EXIT_FAILURE);
            }
            dbus_message_iter_get_basic(&struct_iter, &type_out);
            dbus_message_iter_next(&struct_iter);
            // Step 06-2-5: converting ifindex to interface name and print some infos
            //------------------------------------------------------------------------
            if_indextoname(ifindex_out, ifname);
            printf("Interface name: %s, class: %u, type: %u\n", ifname, class_out, type_out);
            // Step 06-2-6: parsing another array under the structure - A(IQQAy)
            //-------------------------------------------------------------------
            // The data type of the forth element must be an ARRAY. It is the RR data.
            struct_type = dbus_message_iter_get_arg_type(&struct_iter);
            if (struct_type != DBUS_TYPE_ARRAY) {
                printf("Expects '%s' for RR data, but it's '%c'.\n", DBUS_TYPE_ARRAY_AS_STRING, (u_int8_t)struct_type);
                exit(EXIT_FAILURE);
            }
            DBusMessageIter rr_iter;
            dbus_message_iter_recurse(&struct_iter, &rr_iter);
            // Step 06-2-7: moving the whole rrdata to buffer
            //------------------------------------------------
            int j = 0;
            do {
                int rr_type = dbus_message_iter_get_arg_type(&rr_iter);
                if (rr_type != DBUS_TYPE_BYTE) {
                    printf("Expects '%s', but it's '%c'.\n", DBUS_TYPE_BYTE_AS_STRING, (u_int8_t)rr_type);
                    exit(EXIT_FAILURE);
                }
                dbus_message_iter_get_basic(&rr_iter, &rrdata[j]);
                ++j;
            } while (dbus_message_iter_next(&rr_iter));
            rrdata_len = j;
            j = 0;
            // Step 06-2-8: parsing all fields from rrdata
            //----------------------------------------------
            uint8_t *p = rrdata;
            // in rrdata: hostname, type, class, TTL, rdlength, rdata
            // == hostname ==
            int name_len = parse_name(p, hostname);
            printf("Domain name: %s\n", hostname);
            //printf("name_len: %d\n", name_len);
            p += name_len;
            type_out = ntohs(*(int16_t *)p);
            p += TYPE_LEN;
            class_out = ntohs(*(int16_t *)p);
            // Skip the TTL
            p += (CLASS_LEN + TTL_LEN);
            // now p points to rdlength
            if (type_out == DNS_TYPE_A) {
                parse_ip(p, ip);
                printf("IP: %s\n", ip);
            } else if (type_out == DNS_TYPE_CNAME) {
                parse_cname(p, hostname);
                printf("CNAME: %s\n", hostname);
            } else if (type_out == DNS_TYPE_MX) {
                parse_mx(p, hostname);
                printf("MX: %s\n", hostname);
            } else {
                printf("Unsupported type(0x%04x).\n", type_out);
            }

#if 0
            // only for debug
            while (j < rrdata_len) {
                printf("%02x ", (uint8_t)rrdata[j]);
                if ((j + 1) % 16 == 0) printf("\n");
                else if ((j + 1) % 8 == 0) printf("- ");
                ++j;
            }
            printf("\n");
#endif
        } while (dbus_message_iter_next(&array_iter));      // next RR(Resource Record of DNS)
NEXT_ITEM:
        ;
    } while (dbus_message_iter_next(&iter));

    dbus_message_unref(reply);      // release the message

    return 0;
}
