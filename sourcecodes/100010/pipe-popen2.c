/*
 * File: pipe-popen2.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Opens up two pipes using popen().
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall pipe-popen2.c -o pipe-popen2
 * Usage: $ ./pipe-popen2
 *
 * Example source code for article 《IPC之一：使用无名管道进行父子进程间通信的例子》
 * https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
 * 
 */
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *pipein_fp, *pipeout_fp;
    char readbuf[80];

    /* Create one way pipe line with call to popen() */
    if ((pipein_fp = popen("ls", "r")) == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    /* Create one way pipe line with call to popen() */
    if (( pipeout_fp = popen("sort", "w")) == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    /* Processing loop */
    while(fgets(readbuf, 80, pipein_fp)) {
        fputs(readbuf, pipeout_fp);
    }

    /* Close the pipes */
    pclose(pipein_fp);
    pclose(pipeout_fp);

    return(EXIT_SUCCESS);
}