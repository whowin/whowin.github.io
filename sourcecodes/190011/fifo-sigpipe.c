/*
 * File: fifo-sigpipe.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * How to catch SIGPIPE signal when writing data into a FIFO.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall fifo-sigpipe.c -o fifo-sigpipe
 * Usage: $ ./fifo-sigpipe
 *
 * Example source code for article 《IPC之二：使用命名管道(FIFO)进行进程间通信的例子》
 * 
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>


#define FIFO_FILE       "myfifo"
#define BUF_SIZE        128
#define TRUE            1
#define FALSE           0

int fd = 0;     // for FIFO special file

// handle SIGPIPE and SIGINT
void sigint(int signum) {
    static int count = 0;
    if (signum == SIGPIPE) {
        printf("No. %d: Get a SIGPIPE signal.\n", ++count);
        return;
    }
    if (signum == SIGINT) {
        if (fd) close(fd);
        printf("Terminated by user.\n");
        exit(EXIT_SUCCESS);
    }
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

int main() {
    int nbytes = 0;
    char buf[BUF_SIZE];

    signal(SIGPIPE, sigint);        // Catch sigpipe
    signal(SIGINT, sigint);         // Catch sigint

    // Just create a FIFO if it's not exist
    umask(0);           // Clear umask
    //if (mkfifo(FIFO_FILE, S_IFIFO|0666, 0) == -1) {     // Create FIFO with mkfifo()
    if (mknod(FIFO_FILE, S_IFIFO|0666, 0) == -1) {      // Create FIFO with mknod()
        perror("mknod()");
        if ((errno != EEXIST) || (!isfifo(FIFO_FILE))) {
            exit(EXIT_FAILURE);
        }
    }

    printf("Open FIFO with O_WRONLY.\n");
    fd = open(FIFO_FILE, O_WRONLY);
    if (fd == -1) {
        perror("open()");
        printf("Failed to open FIFO.\n");
    } else {
        printf("Open FIFO successfully.\n");
        printf("Write data into FIFO that is openned with O_WRONLY.\n");
        int i = 0;
        while (++i < 100) {
            sprintf(buf, "No. %d: Hello World!\n", i);
            nbytes = write(fd, buf, strlen(buf));
            if (nbytes == -1) {
                perror("write()");
            } else {
                printf("No. %d: Write %d bytes.\n", i, nbytes);
            }
            sleep(5);
        }
        close(fd);
    }


    return(EXIT_SUCCESS);
}
