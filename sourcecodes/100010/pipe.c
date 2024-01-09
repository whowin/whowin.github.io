/*
 * File: pipe.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Set up a pipe between parent and child.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall pipe.c -o pipe
 * Usage: $ ./pipe
 *
 * Example source code for article 《IPC之一：使用无名管道进行父子进程间通信的例子》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

int main(void) {
    int     fd[2], nbytes;
    pid_t   childpid;
    char    string[] = "Hello, world!\n";
    char    readbuffer[80];

    if (pipe(fd) == -1) {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }
    
    if ((childpid = fork()) == -1) {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    if (childpid == 0) {
        /* Child process closes up input side of pipe */
        close(fd[0]);

        /* Send "string" through the output side of pipe */
        write(fd[1], string, (strlen(string) + 1));
        exit(EXIT_SUCCESS);
    } else {
        /* Parent process closes up output side of pipe */
        close(fd[1]);

        /* Read in a string from the pipe */
        nbytes = read(fd[0], readbuffer, sizeof(readbuffer));
        printf("Received %d bytes: %s", nbytes, readbuffer);
    }
    
    return(EXIT_SUCCESS);
}
