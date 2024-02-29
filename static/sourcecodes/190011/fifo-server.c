/*
 * File: fifo-server.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Reading data from the FIFO as if it is a stream file.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall fifo-server.c -o fifo-server
 * Usage: $ ./fifo-server
 *
 * Example source code for article 《IPC之二：使用命名管道(FIFO)进行进程间通信的例子》
 * 
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/stat.h>

#define FIFO_FILE       "myfifo"
#define BUF_SIZE        128
#define TRUE            1
#define FALSE           0

FILE *fp;

// Signal handler for catching ctrl+c
void sigint(int signum) {
    printf("\nServer terminated by user.\n");
    if (fp) fclose(fp);
    exit(EXIT_SUCCESS);
}

// check if the file is FIFO special file
int isfifo(char *filename) {
    struct stat file_stat;

    if (access(filename, F_OK) == -1) {         // Check if the file exists
        printf("The file %s does not exist.\n", filename);
        return FALSE;
    }

    if (stat(filename, &file_stat) == -1) {     // Retrieve file information
        perror("stat()");
        return FALSE;
    }
    if ((file_stat.st_mode & S_IFIFO) == S_IFIFO) {     // Check if the file is FIFO
        return TRUE;
    }
    return FALSE;
}

int main(void) {
    char buf[BUF_SIZE];

    umask(0);           // Clear umask
    //if (mkfifo(FIFO_FILE, S_IFIFO|0666, 0) == -1) {     // Create FIFO with mkfifo()
    if (mknod(FIFO_FILE, S_IFIFO|0666, 0) == -1) {      // Create FIFO with mknod()
        perror("mknod()");
        if ((errno != EEXIST) || (!isfifo(FIFO_FILE))) {
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, sigint);         // Catch ctrl+c 
    while (1) {
        if ((fp = fopen(FIFO_FILE, "r")) == NULL) {     // Open FIFO special file
            perror("fopen()");
            break;
        }
        while (fgets(buf, BUF_SIZE, fp) != NULL) {        // Read from FIFO
            printf("Received string: %s\n", buf);
        }
        fclose(fp);                 // Close FIFO special file
    }

    return(EXIT_SUCCESS);
}