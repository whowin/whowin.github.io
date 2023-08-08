/*
 * File: pipe-popen2.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * http://whowin.cn/
 *
 * Read data from a file as input to a command that is a pipe opened with popen()
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall pipe-popen3.c -o pipe-popen3
 * Usage: $ ./pipe-popen3 [command] [filename] 
 *
 * Example source code for article 《IPC之一：使用无名管道进行父子进程间通信的例子》
 * https://whowin.gitee.io/post/blog/linux/0010-ipc-example-of-anonymous-pipe/
 * 
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    FILE *pipe_fp, *infile;
    char readbuf[128];

    if (argc != 3) {
        fprintf(stderr, "USAGE:  %s [command] [filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Open up input file */
    if ((infile = fopen(argv[2], "rt")) == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Create one way pipe line with call to popen() */
    if ((pipe_fp = popen(argv[1], "w")) == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    /* Processing loop */
    do { 
        fgets(readbuf, 80, infile);
        if (feof(infile)) break;

        fputs(readbuf, pipe_fp);
    } while(!feof(infile));

    fclose(infile); 
    pclose(pipe_fp);

    return(EXIT_SUCCESS);
}
