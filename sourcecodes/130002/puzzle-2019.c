/*
 * File: puzzle-2019.c
 * Author: Songqing Hua
 *
 * (C) 2024 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * A solution for the first puzzle of the Advent of Code 2019 puzzles using glibc.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall puzzle-2019.c -o puzzle-2019 -lm
 * Usage: $ ./puzzle-2019 2024
 * 
 * Example source code for article 《使用glib进行C语言编程的实例(一)》
 *
 */
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

// Calculate the fuel required
int calculate_fuel(int weight) {
    int additional_weight = fmax(weight / 3 - 2, 0);

    printf("Weight: %d\tAdditional weight(Fuel required): %d\n", weight, additional_weight);
    if (additional_weight > 0) {
        additional_weight += calculate_fuel(additional_weight);
    }

    return additional_weight;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("USAGE: %s <mass>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int mass = atoi(argv[1]);
    if (mass <= 0) {
        printf("The mass can not be zero.\n");
        exit(EXIT_FAILURE);
    }

    int fuel_required = calculate_fuel(mass);
    printf("Total fuel reqired: %d\n", fuel_required);

    return EXIT_SUCCESS;
}
