/*
 * File: fifo-client.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Writing data into the FIFO as if it is a stream file.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall fifo-client.c -o fifo-client
 * Usage: $ ./fifo-client
 *
 * Example source code for article 《IPC之二：使用命名管道(FIFO)进行进程间通信的例子》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FIFO_FILE       "myfifo"

int main(int argc, char *argv[]) {
    FILE *fp;

    if (argc != 2) {
        printf("USAGE: %s [string]\n", argv[0]);
        exit(1);
    }

    if ((fp = fopen(FIFO_FILE, "w")) == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fputs("Hello world.", fp);
    fflush(fp);
    
    fclose(fp);

    return(EXIT_SUCCESS);
}