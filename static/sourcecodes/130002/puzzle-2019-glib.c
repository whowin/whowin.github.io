/*
 * File: puzzle-2019-glib.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * A solution for the first puzzle of the Advent of Code 2019 puzzles using GLib.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g puzzle-2019-glib.c -o puzzle-2019-glib `pkg-config --cflags --libs glib-2.0`
 * Usage: $ ./puzzle-2019 2024
 * 
 * Example source code for article 《使用glib进行C语言编程的实例(一)》
 *
 */
//#include <stdio.h>
#include <glib.h>

// Calculate the fuel required
gint calculate_fuel(gint weight, GString *message) {
    gint additional_weight = MAX(weight / 3 - 2, 0);
    g_autoptr(GString) temp_str = g_string_new(NULL);

    g_string_printf(temp_str, "Weight: %d\tAdditional weight(Fuel required): %d\n", weight, additional_weight);
    g_string_append(message, temp_str->str);

    if (additional_weight > 0) {
        additional_weight += calculate_fuel(additional_weight, message);
    }

    return additional_weight;
}

gint main(gint argc, gchar *argv[]) {
    if (argc < 2) {
        g_print("USAGE: %s <mass>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    gint mass = g_ascii_strtoll(argv[1], NULL, 10);         // Convert an ASCII string to a number
    if (mass <= 0) {
        g_print("The mass can not be zero.\n");
        exit(EXIT_FAILURE);
    }

    GString *message = g_string_new(NULL);                  // Create a string object
    gint fuel_required = calculate_fuel(mass, message);
    g_print("%sTotal fuel required: %d\n", message->str, fuel_required);
    g_string_free(message, TRUE);                           // Release the string object

    return EXIT_SUCCESS;
}
