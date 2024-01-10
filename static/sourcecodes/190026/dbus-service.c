/*
 * File: dbus-service.c
 * Author: Songqing Hua
 * email: hengch@163.com
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * implementation of D-Bus standard interface and service interface
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g dbus-service.c -o dbus-service `pkg-config --libs --cflags dbus-1`
 * Usage: $ ./dbus-service
 *
 * Example source code for article 《IPC之十六：D-Bus的标准接口和服务接口的具体实现》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <dbus/dbus.h>

#define SERVER_BUS_NAME             "cn.whowin.TestDbus"
#define OBJECT_PATH_NAME            "/cn/whowin/TestObject"
#define INTERFACE_NAME              "cn.whowin.TestInterface"
#define DISPATCH_TIMEOUT_MS         100

#define PROPERTY_VERSION            "Version"
#define PROPERTY_AUTHOR             "Author"
#define MAX_PROPERTY_LEN            (strlen(PROPERTY_VERSION)>strlen(PROPERTY_AUTHOR))?strlen(PROPERTY_VERSION):strlen(PROPERTY_AUTHOR)

#define DBUS_INTROSPECTION          "Introspect"            // 'Introspect' mothod on standard interface
#define DBUS_GET_PROPERTIES         "Get"                   // 'Get' mothod on standard interface
#define DBUS_GETALL_PROPERTIES      "GetAll"                // 'GetAll' mothod on standard interface
#define MYDBUS_HELLO_METHOD         "Hello"                 // 'Hello' method on cn.whowin.TestInterface
#define MYDBUS_EMITSIGNAL_METHOD    "EmitSignal"            // 'EmitSignal' method on cn.whowin.TestInterface
#define MYDBUS_QUIT_METHOD          "Quit"                  // 'Quit' method on cn.whowin.TestInterface
#define MYDBUS_ONEMITSIGNAL_SIGNAL  "OnEmitSignal"          // a signal on cn.whowin.TestInterface

const char *version = "0.123";
const char *author = "whowin";
static dbus_uint32_t client_serial = 1;
static int loop_quit = 0;

/************************************************************************************************
 * The commands below can use for testing the program
 *-----------------------------------------------------
 * dbus-send --print-reply --session --dest=cn.whowin.TestDbus /cn/whowin/TestObject org.freedesktop.DBus.Introspectable.Introspect
 * dbus-send --print-reply --session --dest=cn.whowin.TestDbus /cn/whowin/TestObject org.freedesktop.DBus.Properties.Get string:"cn.whowin.TestInterface" string:"Version"
 * dbus-send --print-reply --session --dest=cn.whowin.TestDbus /cn/whowin/TestObject org.freedesktop.DBus.Properties.Get string:"cn.whowin.TestInterface" string:"Author"
 * dbus-send --print-reply --session --dest=cn.whowin.TestDbus /cn/whowin/TestObject org.freedesktop.DBus.Properties.GetAll
 * dbus-send --print-reply --session --dest=cn.whowin.TestDbus /cn/whowin/TestObject cn.whowin.TestInterface.Hello string:"whowin"
 * dbus-send --print-reply --session --dest=cn.whowin.TestDbus /cn/whowin/TestObject cn.whowin.TestInterface.EmitSignal
 * dbus-send --print-reply --session --dest=cn.whowin.TestDbus /cn/whowin/TestObject cn.whowin.TestInterface.Quit
 * gdbus introspect --session --dest cn.whowin.TestDbus --object-path /cn/whowin/TestObject
 * dbus-monitor "type='signal',sender='cn.whowin.TestDbus',interface='cn.whowin.TestInterface'"
 ************************************************************************************************/

/********************************************************************************
 * Function: void print_dbus_error(DBusError *dbus_error_p, char *str)
 * Description: Print the error message when involking dbus functions
 ********************************************************************************/
void print_dbus_error(DBusError *dbus_error_p, char *str) {
    printf("%s: %s\n", str, dbus_error_p->message);
    dbus_error_free(dbus_error_p);
}
/********************************************************************************
 * This is the XML string describing the interfaces, methods and signals 
 * implemented by our 'Server' object. It's used by the 'Introspect' method of 
 * 'org.freedesktop.DBus.Introspectable' interface.
 *
 * Currently this server implements only 3 standard interfaces:
 *      - org.freedesktop.DBus.Peer
 *      - org.freedesktop.DBus.Introspectable
 *      - org.freedesktop.DBus.Properties
 * 
 * One our own interface
 *      - cn.whowin.TestInterface
 *
 * 'cn.whowin.TestInterface' offers 3 methods:
 *    - Hello(): replies the string "Hello " plus the passed string argument.
 *    - EmitSignal(): send a signal 'OnEmitSignal'
 *    - Quit(): makes the server exit. It takes no arguments.
 ***********************************************************************************/

const char *server_introspection_xml =
    DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
    "<node>\n"

    "  <interface name='org.freedesktop.DBus.Peer'>\n"
    "    <method name='Ping' />\n"
    "    <method name='GetMachineId'>\n"
    "      <arg type='s' name='machine_uuid' direction='out' />\n"
    "    </method>\n"
    "  </interface>\n"
    
    "  <interface name='org.freedesktop.DBus.Introspectable'>\n"
    "    <method name='Introspect'>\n"
    "      <arg name='data' type='s' direction='out' />\n"
    "    </method>\n"
    "  </interface>\n"
    
    "  <interface name='org.freedesktop.DBus.Properties'>\n"
    "    <method name='Get'>\n"
    "      <arg name='interface' type='s' direction='in' />\n"
    "      <arg name='property'  type='s' direction='in' />\n"
    "      <arg name='value'     type='s' direction='out' />\n"
    "    </method>\n"
    "    <method name='GetAll'>\n"
    "      <arg name='interface'  type='s'     direction='in'/>\n"
    "      <arg name='properties' type='a{sv}' direction='out'/>\n"
    "    </method>\n"
    "  </interface>\n"
    
    "  <interface name='cn.whowin.TestInterface'>\n"
    "    <property name='Version' type='s' access='read' />\n"
    "    <property name='Author' type='s' access='read' />\n"
    "    <method name='Hello'>\n"
    "      <arg name='name' direction='in' type='s'/>\n"
    "      <arg name='hello' type='s' direction='out' />\n"
    "    </method>\n"
    "    <method name='EmitSignal' />\n"
    "    <method name='Quit' />\n"
    "    <signal name='OnEmitSignal' />\n"
    "  </interface>\n"
    
    "</node>\n";

/*******************************************************************************************
 * Function: DBusHandlerResult send_reply(DBusConnection *conn, DBusMessage *reply)
 * Description: This function sends the reply and return DBusHandlerResult
 *******************************************************************************************/
DBusHandlerResult send_reply(DBusConnection *conn, DBusMessage *reply) {
    dbus_bool_t ret = FALSE;

    ret = dbus_connection_send(conn, reply, &client_serial);
    dbus_message_unref(reply);
    if (ret == FALSE) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    ++client_serial;
    return DBUS_HANDLER_RESULT_HANDLED;
}
/**********************************************************************************************
 * Function: DBusHandlerResult introspection(DBusConnection *conn, DBusMessage *message)
 * Description: This function implements 'Introspect' method of DBUS_INTERFACE_INTROSPECTABLE
 **********************************************************************************************/
DBusHandlerResult introspection(DBusConnection *conn, DBusMessage *message) {
    DBusMessage *reply = NULL;

    if (!(reply = dbus_message_new_method_return(message))) {
        printf("Can't get the reply.\n");
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &server_introspection_xml, DBUS_TYPE_INVALID);
    return send_reply(conn, reply);
}

/******************************************************************************************
 * Function: DBusHandlerResult get_properties(DBusConnection *conn, DBusMessage *message)
 * Description: This function implements 'Get' method of DBUS_INTERFACE_PROPERTIES 
 *              so a client can inspect the properties/attributes of 'TestInterface'.
 ******************************************************************************************/
DBusHandlerResult get_properties(DBusConnection *conn, DBusMessage *message) {
    DBusMessage *reply = NULL;
    DBusError err;
    const char *interface, *property;

    dbus_error_init(&err);
    dbus_message_get_args(message, &err,
                          DBUS_TYPE_STRING, &interface,
                          DBUS_TYPE_STRING, &property,
                          DBUS_TYPE_INVALID);
    if (dbus_error_is_set(&err)) {
        reply = dbus_message_new_error(message, err.name, err.message);
        dbus_error_free(&err);
    } else {
        reply = dbus_message_new_method_return(message);
        if (reply == NULL) {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        if (strcmp(property, "Version") == 0) {
            dbus_message_append_args(reply,
                                    DBUS_TYPE_STRING, &version,
                                    DBUS_TYPE_INVALID);
        } else if (strcmp(property, "Author") == 0) {
            dbus_message_append_args(reply,
                                    DBUS_TYPE_STRING, &author,
                                    DBUS_TYPE_INVALID);

        } else {
            // Unknown property
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    }
    return send_reply(conn, reply);
}
/*********************************************************************************************************
 * Function: DBusHandlerResult get_all_properties(DBusConnection *conn, DBusMessage *message)
 * Description: This implements 'GetAll' method of DBUS_INTERFACE_PROPERTIES. 
 *********************************************************************************************************/
DBusHandlerResult get_all_properties(DBusConnection *conn, DBusMessage *message) {
    DBusMessage *reply = NULL;

    reply = dbus_message_new_method_return(message);
    if (reply == NULL) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    DBusHandlerResult result;
    DBusMessageIter array, dict, iter, variant;
    //printf("MAX_PROPERTY_LEN: %ld\n", MAX_PROPERTY_LEN);
    char *property = malloc(MAX_PROPERTY_LEN + 1);

    //result = DBUS_HANDLER_RESULT_NEED_MEMORY;

    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &array);

    /* Append all properties name/value pairs */
    strcpy(property, PROPERTY_VERSION);
    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &dict);
    dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);
    dbus_message_iter_open_container(&dict, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &version);
    dbus_message_iter_close_container(&dict, &variant);
    dbus_message_iter_close_container(&array, &dict);

    strcpy(property, PROPERTY_AUTHOR);
    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &dict);
    dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);
    dbus_message_iter_open_container(&dict, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &author);
    dbus_message_iter_close_container(&dict, &variant);
    dbus_message_iter_close_container(&array, &dict);

    dbus_message_iter_close_container(&iter, &array);

    result = send_reply(conn, reply);
    free(property);
    return result;
}
/*******************************************************************************************
 * Function: DBusHandlerResult hello_handler(DBusConnection *conn, DBusMessage *message)
 * Description: This function implements "Hello" method on INTERFACE_NAME.
 *              It adds "Hello" in front of the passed parameters and 
 *              reply the string to the client.
 *******************************************************************************************/
DBusHandlerResult hello_handler(DBusConnection *conn, DBusMessage *message) {
    DBusError err;
    DBusMessage *reply = NULL;
    const char *name;
    char *answer = NULL;
    DBusHandlerResult result;

    dbus_error_init(&err);
    dbus_message_get_args(message, &err, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&err)) {
        reply = dbus_message_new_error(message, err.name, err.message);
        dbus_error_free(&err);
    } else {
        reply = dbus_message_new_method_return(message);
        if (reply == NULL) {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        answer = malloc(strlen(name) + 16);
        sprintf(answer, "Hello %s", name);
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &answer, DBUS_TYPE_INVALID);
    }
    result = send_reply(conn, reply);
    if (answer) free(answer);
    return result;
}
/***************************************************************************************************
 * Function: DBusHandlerResult emit_signal_handler(DBusConnection *conn, DBusMessage *message)
 * Description: This function implements "EmitSignal" method on INTERFACE_NAME
 *              It broadcasts 'OnEmitSignal' signal and then replies an empty message to the client.
 ***************************************************************************************************/
DBusHandlerResult emit_signal_handler(DBusConnection *conn, DBusMessage *message) {
    DBusMessage *reply = NULL;
    DBusHandlerResult result;

    reply = dbus_message_new_signal(OBJECT_PATH_NAME, INTERFACE_NAME, MYDBUS_ONEMITSIGNAL_SIGNAL);
    if (reply == NULL) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    result = send_reply(conn, reply);
    if (result != DBUS_HANDLER_RESULT_HANDLED) {
        return result;
    }

    // Send a METHOD_RETURN reply
    reply = dbus_message_new_method_return(message);
    return send_reply(conn, reply);
}
/**********************************************************************************************
 * Function: DBusHandlerResult quit_handler(DBusConnection *conn, DBusMessage *message)
 * Description: This function implements "Quit" method on INTERFACE_NAME
 *              It replies an empty message to the client and then exits the server.
 **********************************************************************************************/
DBusHandlerResult quit_handler(DBusConnection *conn, DBusMessage *message) {
    DBusMessage *reply = NULL;
    DBusHandlerResult result;

    reply = dbus_message_new_method_return(message);
    result = send_reply(conn, reply);
    loop_quit = TRUE;
    return result;
}
/*******************************************************************************************************************
 * Function: DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *data)
 * Description:
 * 
 * This function implements the 'TestInterface' interface for the 'Server' DBus object.
 *
 * It also implements 
 *      - 'Introspect' method of 'org.freedesktop.DBus.Introspectable' interface
 *      - 'Get' and 'GetAll' methods of 'org.freedesktop.DBus.Properties' inteface
 * 
 * This can be queried by:
 *      $ gdbus introspect --session --dest cn.whowin.TestDbus --object-path /cn/whowin/TestObject
 ******************************************************************************************************************/
DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *data) {
    DBusError err;

    printf("Got D-Bus request: %s.%s on %s\n", 
            dbus_message_get_interface(message), 
            dbus_message_get_member(message),
            dbus_message_get_path(message));

    dbus_error_init(&err);

    // Step 01: Standard interface: org.freedesktop.DBus.Introspectable
    //==================================================================
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, DBUS_INTROSPECTION)) {
        // 'Introspect' method
        return introspection(conn, message);
    } 
    // Step 02: Standard nterface: org.freedesktop.DBus.Properties
    //=============================================================
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, DBUS_GET_PROPERTIES)) {
        // 'Get' method
        return get_properties(conn, message);
    }
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, DBUS_GETALL_PROPERTIES)) {
        // 'GetAll' method
        return get_all_properties(conn, message);
    }
    // Step 03: Implementation on cn.whowin.TestInterface
    //===================================================== 
    if (dbus_message_is_method_call(message, INTERFACE_NAME, MYDBUS_HELLO_METHOD)) {
        // 'Hello' method
        return hello_handler(conn, message);
    }
    if (dbus_message_is_method_call(message, INTERFACE_NAME, MYDBUS_EMITSIGNAL_METHOD)) {
        // 'EmitSignal' method
        return emit_signal_handler(conn, message);
    }
    if (dbus_message_is_method_call(message, INTERFACE_NAME, MYDBUS_QUIT_METHOD)) {
        // 'Quit' method
        return quit_handler(conn, message);
    } else {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

}

const DBusObjectPathVTable server_vtable = {
    .message_function = server_message_handler
};


int main(void) {
    DBusConnection *conn;
    DBusError err;
    int ret;

    dbus_error_init(&err);

    // connects to the daemon bus
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn) {
        print_dbus_error(&err, "dbus_bus_get()");
        exit(EXIT_FAILURE);
    }
    // requests bus name
    ret = dbus_bus_request_name(conn, SERVER_BUS_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        print_dbus_error(&err, "dbus_bus_request_name()");
        exit(EXIT_FAILURE);
    }
    // registers the object
    if (!dbus_connection_register_object_path(conn, OBJECT_PATH_NAME, &server_vtable, NULL)) {
        print_dbus_error(&err, "dbus_connection");
        exit(EXIT_FAILURE);
    }

    printf("Starting dbus server v%s\n", version);
    while (dbus_connection_read_write_dispatch(conn, DISPATCH_TIMEOUT_MS)) {
        // main loop
        if (loop_quit) break;
    }

    return EXIT_SUCCESS;
}
