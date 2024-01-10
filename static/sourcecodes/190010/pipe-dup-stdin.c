/*
 * File: pipe-dup-stdin.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Duplicate the pipe descriptor of child process onto standard input.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall pipe-dup-stdin.c -o pipe-dup-stdin
 * Usage: $ ./pipe-dup-stdin
 *
 * Example source code for article 《IPC之一：使用无名管道进行父子进程间通信的例子》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define SORT_OUTPUT      "sort.log"

int main(void) {
    int     fd[2];
    pid_t   childpid;
    char    string[] = "metabolic\nheart\nproblem\nbetween\nstrong\n";

    unlink(SORT_OUTPUT);
    if (pipe(fd) == -1) {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }
    
    if ((childpid = fork()) == -1) {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    if (childpid == 0) {
        /* Child process closes up output side of pipe */
        close(fd[1]);

        /* Duplicate output descriptor of pipe onto standard input*/
        //close(STDIN_FILENO);
        //dup(fd[0]);
        dup2(fd[0], STDIN_FILENO);
        execlp("sort", "sort", "-o", "sort.log", NULL);
        exit(EXIT_SUCCESS);
    } else {
        /* Parent process closes up input side of pipe */
        close(fd[0]);

        write(fd[1], string, (strlen(string)));
    }
    
    return(EXIT_SUCCESS);
}
