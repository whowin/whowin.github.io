/*
 * File: fifo-eof.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * How to catch EOF when reading data from FIFO
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall fifo-eof.c -o fifo-eof
 * Usage: $ ./fifo-eof
 *
 * Example source code for article 《IPC之二：使用命名管道(FIFO)进行进程间通信的例子》
 * https://whowin.gitee.io/post/blog/linux/0011-ipc-examples-of-fifo/
 * 
 */
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

// handle SIGINT
void sigint(int signum) {
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

int main(int argc, char *argv[]) {
    int nbytes = 0;
    char buf[BUF_SIZE];

    // Just create a FIFO if it's not exist
    umask(0);           // Clear umask
    if (mknod(FIFO_FILE, S_IFIFO|0666, 0) == -1) {      // Create FIFO with mknod()
        perror("mknod()");
        if ((errno != EEXIST) || (!isfifo(FIFO_FILE))) {
            exit(EXIT_FAILURE);
        }
    }

    printf("Openning FIFO with O_RDONLY.\n");
    fd = open(FIFO_FILE, (O_RDONLY));
    //fd = open(FIFO_FILE, (O_RDONLY | O_NONBLOCK));
    if (fd == -1) {
        perror("open()");
        printf("Failed to open FIFO.\n");
        exit(EXIT_FAILURE);
    }
    printf("Openning FIFO successfully.\n");

    signal(SIGPIPE, sigint);        // Catch sigpipe
    signal(SIGINT, sigint);         // Catch sigint

    int i = 0;
    int eof_count = 0;
    while (++i < 100) {
        printf("Reading data from FIFO.\n");
        nbytes= read(fd, buf, BUF_SIZE - 1);
        if (nbytes > 0) {
            buf[nbytes] = 0;
            printf("No.%d: Read %d bytes: %s\n", i, nbytes, buf);
        } else if (nbytes == 0) {
            printf("No. %d: Got an EOF\n", ++eof_count);
            sleep(3);
        } else {
            perror("read()");
        }
    }
    if (fd) close(fd);

    return(EXIT_SUCCESS);
}
