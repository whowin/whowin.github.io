/*
 * File: pipe-popen.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Create a pipe using popen().
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall pipe-popen.c -o pipe-popen
 * Usage: $ ./pipe-popen
 *
 * Example source code for article 《IPC之一：使用无名管道进行父子进程间通信的例子》
 * https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
 * 
 */
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *pipe_fp;
    char string[] = "metabolic\nheart\nproblem\nbetween\nstrong\n";

    unlink("sort.log");
    if ((pipe_fp = popen("sort -o sort.log", "w")) == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    fputs(string, pipe_fp);    
    pclose(pipe_fp);
    
    return(EXIT_SUCCESS);
}