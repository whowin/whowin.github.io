/*
 * File: ipc-key.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Just verify how to combine inode, device no and project id to KEY on ftok().
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall ipc-key.c -o ipc-key
 * Usage: $ ./ipc-key
 *
 * Example source code for article 《IPC之三：使用消息队列进行进程间通信的实例》
 * 
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
    struct stat st;
    int proj_id = 0;

    if (argc != 3) {
        printf("usage: %s [pathname] [proj_id]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (access(argv[1], F_OK) == -1) {
        printf("The file/directory %s does not exist.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    proj_id = atoi(argv[2]);
    if (proj_id == 0) {
        printf("The second parameter can not be zero.\n");
        exit(EXIT_FAILURE);
    }

    if (stat(argv[1], &st) == -1) {
        perror("stat()");
        exit(EXIT_FAILURE);
    }

    printf("The inode of \"%s\" is 0x%08lx\n", argv[1], st.st_ino);
    printf("The device No. of \"%s\" is 0x%08lx\n", argv[1], st.st_dev);
    printf("the project ID is 0x%04x\n", atoi(argv[2]));
    key_t ipc_key = (st.st_ino & 0xffff) | ((st.st_dev & 0xff) << 16) | ((proj_id & 0xff) << 24);
    printf("The key combined by myself is 0x%08x\n", ipc_key);
    printf("The ket created  by ftok() is 0x%08x\n", ftok(argv[1], proj_id));

    return EXIT_SUCCESS;
}