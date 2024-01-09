/*
 * File: ipc-files.c
 * Author: Songqing Hua
 * 
 * (C) 2023 Songqing Hua.
 * https://blog.csdn.net/whowin/
 *
 * Demonstrate how to implement IPC using shared files.
 * Compiled with gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1).
 * Tested on Linux 5.4.0-139-generic #156-Ubuntu SMP Fri Jan 20 17:27:18 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux.
 *
 * To compile: $ gcc -Wall -g ipc-files.c -o ipc-files
 * Usage: $ ./ipc-files
 *
 * IPC examples using shared files 《IPC之十：使用共享文件进行进程间通信的实例》
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

#include <sys/wait.h>

#define CLIENT_NUM              3       // how many client processes 
#define SERVER_FILE_READING     "./server_shared_file_reading.ipc"
#define SERVER_FILE_WRITING     "./server_shared_file_writing.ipc"
#define STR_SIZE                64

#define CMD_SERVER_ONLINE       0       // The server tells a client that the server is online
#define CMD_SERVER_OFFLINE      1       // The server sends this message before it quits
#define CMD_SERVER_STATUS       2       // The client asks server's status
#define CMD_STRING              3       // The client sends a string to server
#define CMD_STRING_OK           4       // The server responds ACK when it received a string message

#define BROADCAST_PROCESS_ID    0       // this is a broadcast message

#define PREFIX_LEN              (sizeof(int) * 4 + sizeof(ushort))

struct ipc_msg {
    int len;            // total length including itself
    int src_pid;        // source PID
    int dest_pid;       // destination PID
    uint seq_num;       // sequence number of the current message
    ushort cmd;         // command code
    char msg[1];        // the auxiliary information
};

pid_t pid[CLIENT_NUM + 1] = {0};    // PIDs
int readfd = -1, writefd = -1;      // file descriptor

/************************************************************************************
 * Function: void error(const char* msg)
 * Description: Error handler. 
 *              Display errmessage and then send a SIGINT signal to current process
 ************************************************************************************/
void error(const char *msg) {
    perror(msg);
    kill(getpid(), SIGINT);
}
/**************************************************************************
 * Function: void reading_lock(int fd)
 * Description: Put a reading lock on the file
 *              it will block until the lock is put successfully
 *              exit if error occurs.
 **************************************************************************/
void reading_lock(int fd) {
    struct flock lock;

    lock.l_type = F_RDLCK;      // reading lock
    lock.l_whence = SEEK_SET;   // offset based on the start of the file
    lock.l_start = 0;           // offset
    lock.l_len = 0;             // 0 here means 'until EOF'
    lock.l_pid = getpid();      // process id

    if (fcntl(fd, F_SETLKW, &lock) < 0) {    // Acquire the lock
        perror("fcntl(F_RDLCK)");
        kill(getpid(), SIGINT);
    }
    return;
}
/**************************************************************************
 * Function: void writing_lock(int fd)
 * Description: Put a writing lock on the file
 *              it will block until the lock is put successfully
 *              exit if error occurs.
 **************************************************************************/
void writing_lock(int fd) {
    struct flock lock;

    lock.l_type = F_WRLCK;      // writing lock
    lock.l_whence = SEEK_SET;   // offset based on the start of the file
    lock.l_start = 0;           // offset
    lock.l_len = 0;             // 0 here means 'until EOF'
    lock.l_pid = getpid();      // process id

    if (fcntl(fd, F_SETLKW, &lock) < 0) {    // Acquire the lock
        perror("fcntl(F_WRLCK)");
        kill(getpid(), SIGINT);
    }
    return;
}
/**************************************************************************
 * Function: void release_lock(int fd)
 * Description: Release the lock from the file
 *              exit if error occurs.
 **************************************************************************/
void release_lock(int fd) {
    struct flock lock;

    lock.l_type = F_UNLCK;      // release lock
    lock.l_whence = SEEK_SET;   // offset based on the start of the file
    lock.l_start = 0;           // offset
    lock.l_len = 0;             // 0 here means 'until EOF'
    lock.l_pid = getpid();      // process id

    if (fcntl(fd, F_SETLK, &lock) < 0) {    // Acquire the lock
        perror("fcntl(F_UNLCK)");
        kill(getpid(), SIGINT);
    }
    return;
}
/*******************************************************************************
 * Function: void parent_sigint(int signum)
 * Description: Catch ctrl+c signal for the parent process
 *******************************************************************************/
void parent_sigint(int signum) {
    kill(pid[0], SIGINT);           // send SIGINT signal to server process
    return;
}
/*******************************************************************************
 * Function: void server_sigint(int signum)
 * Description: Catch ctrl+c signal for the server process
 *******************************************************************************/
void server_sigint(int signum) {
    struct ipc_msg offline_msg;
    if (writefd > 0) {
        // Broadcast a offline message 
        offline_msg.cmd = CMD_SERVER_OFFLINE;
        offline_msg.src_pid = getpid();
        offline_msg.dest_pid = BROADCAST_PROCESS_ID;
        offline_msg.seq_num = 0;
        offline_msg.len = PREFIX_LEN + 1;
        offline_msg.msg[0] = '\0';

        writing_lock(writefd);
        int nbytes = write(writefd, &offline_msg, offline_msg.len);
        if (nbytes < 0) {
            error("serverint-write()");
        }
        release_lock(writefd);
        sleep(5);
        close(writefd);
    }
    if (readfd > 0) close(readfd);

    //unlink(SERVER_FILE_NAME);
    printf("\nserver(%d) - Terminated by USER.\n", getpid());
    exit(EXIT_SUCCESS);
}
/*******************************************************************************
 * Function: void client_sigint(int signum)
 * Description: Catch ctrl+c signal for the client process
 *******************************************************************************/
void client_sigint(int signum) {
    if (readfd > 0) close(readfd);
    if (writefd > 0) close(writefd);

    printf("\nclient(%d) - Terminated by USER.\n", getpid());
    exit(EXIT_SUCCESS);
}
#if 0
void dump(uint8_t *p, int len) {
    int i;
    for (i = 0; i < len; ++i) {
        printf("%02x ", p[i]);
    }
    printf("\n");
}
#endif
/*******************************************************************************
 * Function: int read_a_msg(int fd, struct ipc_msg **pmsg)
 * Description: Read a message from the shared file as per struct ipc_msg
 * 
 * Arguments:   fd      opened file descriptor
 *              pmsg    a pointer that point to a pointer of struct ipc_msg
 * Return:      0       no message to read
 *              >0      number of bytes that have been read. 
 *                      Data is stored in pmsg
 *              exit if error occurs
 *******************************************************************************/
int read_a_msg(int fd, struct ipc_msg **pmsg) {
    int msg_len = 0;
    int nbytes = 0;
    struct ipc_msg *msg;

    reading_lock(fd);           // acquire file lock for reading
    nbytes = read(fd, &msg_len, sizeof(int));   // get length of the message
    if (nbytes < 0) {
        error("server-read()");
    } else if (nbytes == 0) {
        release_lock(fd);
        return 0;
    } else if (nbytes != sizeof(int)) {
        printf("server: data(int) received may be corrupt. - %d - %d\n", nbytes, msg_len);
        kill(getpid(), SIGINT);
        sleep(1);
    }

    msg = malloc(msg_len);      // allocate memory for the message
    memset(msg, 0, msg_len);
    msg->len = msg_len;
    nbytes = read(fd, &msg->src_pid, msg_len - sizeof(int));    // read the message
    if (nbytes < 0) {
        error("server-read()");
    } else if (nbytes != (msg_len - sizeof(int))) {
        printf("server: data received may be corrupt. - %d - %d\n", nbytes, msg_len);
        kill(getpid(), SIGINT);
        sleep(1);
    }
    release_lock(fd);

    if ((msg->dest_pid != getpid()) && (msg->dest_pid != BROADCAST_PROCESS_ID)) {
        // the destination is not mine, throw it away.
        free(msg);
        msg_len = 0;
        *pmsg = NULL;
    } else {
        *pmsg = msg;
    }
    return msg_len;
}
/************************************************************************
 * Function: void server_process()
 * Description: Server process
 ************************************************************************/
void server_process() {
    struct ipc_msg *msg;
    int msg_len;
    int nbytes = 0;
    uint seq_num = 0;

    signal(SIGINT, server_sigint);
    unlink(SERVER_FILE_READING);
    unlink(SERVER_FILE_WRITING);

    // 1. Create shared files and open them with READ & WRITE
    //=========================================================
    readfd = open(SERVER_FILE_READING, O_RDONLY | O_CREAT, 0666);
    if (readfd < 0) {
        error("server-open(READ)");
    }
    writefd = open(SERVER_FILE_WRITING, O_WRONLY | O_CREAT, 0666);
    if (writefd < 0) {
        error("server-open(WRITE)");
    }

    while (1) {
        // 2. Read a message from the shared file
        //========================================
        msg_len = read_a_msg(readfd, &msg);
        if (msg_len == 0) {
            sleep(1);
            continue;
        }
        // 3. process received messages
        //===============================
        switch (msg->cmd) {
            case CMD_SERVER_STATUS:
                // Client asks for server's status. respond CMD_SERVER_ONLINE to the client.
                nbytes = 0;
                struct ipc_msg online_msg;
                online_msg.cmd = CMD_SERVER_ONLINE;
                online_msg.src_pid = getpid();
                online_msg.dest_pid = msg->src_pid;
                online_msg.seq_num = ++seq_num;
                online_msg.len = PREFIX_LEN + 1;
                online_msg.msg[0] = '\0';

                writing_lock(writefd);
                lseek(writefd, 0, SEEK_END);
                nbytes = write(writefd, &online_msg, online_msg.len);
                if (nbytes < 0) {
                    error("server-write()");
                }
                release_lock(writefd);
                break;

            case CMD_STRING:
                // Client sends a string message, respond ACK.
                if (msg->len > PREFIX_LEN) {
                    msg_len = msg->len - PREFIX_LEN - 1;
                    msg->msg[msg_len] = '\0';
                    printf("server: received a string(%d) - %s\n", msg->src_pid, msg->msg);

                    nbytes = 0;
                    struct ipc_msg string_msg;
                    string_msg.cmd = CMD_STRING_OK;
                    string_msg.src_pid = getpid();
                    string_msg.dest_pid = msg->src_pid;
                    string_msg.seq_num = ++seq_num;
                    string_msg.len = PREFIX_LEN + 1;
                    string_msg.msg[0] = '\0';

                    writing_lock(writefd);
                    lseek(writefd, 0, SEEK_END);
                    nbytes = write(writefd, &string_msg, string_msg.len);
                    if (nbytes < 0) {
                        error("server-write()");
                    }
                    release_lock(writefd);
                }
                break;

            default:        // others are gabage
                printf("server: unknown command.\n");
                printf("src_pid=%d\t\tdest_pid=%d\t\tcmd=%d\n", msg->src_pid, msg->dest_pid, msg->cmd);
                break;
        }
        free(msg);
    }

    close(readfd); 
    close(writefd); 
    return;
}
/**********************************************************************
 * Function: void client_process()
 * Description: Client process
 **********************************************************************/
void client_process(int flag) {
    struct ipc_msg *msg;
    int msg_len;
    int nbytes = 0;
    int server_online = 0;
    int server_pid = 0;
    int waiting_count = 0;
    uint seq_num = 0;

    signal(SIGINT, client_sigint);

    // 1. open shared files with READ & WRITE
    //=========================================
    readfd = open(SERVER_FILE_WRITING, O_RDONLY);
    if (readfd < 0) {
        error("client-open(READ)");
    }
    writefd = open(SERVER_FILE_READING, O_WRONLY);
    if (writefd < 0) {
        error("client-open(WRITE)");
    }

    while (1) {
        if (!server_online) {
            // 2. Acquire the server's status
            //=================================
            writing_lock(writefd);
            struct ipc_msg status_msg;
            status_msg.cmd = CMD_SERVER_STATUS;
            status_msg.src_pid = getpid();
            status_msg.dest_pid = BROADCAST_PROCESS_ID;
            status_msg.seq_num = ++seq_num;
            status_msg.len = PREFIX_LEN + 1;
            status_msg.msg[0] = '\0';
            msg_len = status_msg.len;

            lseek(writefd, 0, SEEK_END);
            nbytes = write(writefd, &status_msg, msg_len);
            if (nbytes < 0) {
                error("client-write()");
            }
            release_lock(writefd);
            sleep(1);
        }

        // 3. read message from the shared file
        //======================================
        msg_len = read_a_msg(readfd, &msg);
        if (msg_len > 0) {
            switch (msg->cmd) {
                case CMD_SERVER_ONLINE:
                    // The server is online
                    server_pid = msg->src_pid;
                    server_online = 1;
                    printf("client(%d): The server(%d) is online.\n", getpid(), server_pid);
                    break;

                case CMD_SERVER_OFFLINE:
                    // The server will be offline, so just quit.
                    free(msg);
                    kill(getpid(), SIGINT);
                    break;

                case CMD_STRING_OK:
                    // The ACK from the server, it means the server received my string message.
                    printf("client(%d): Received ACK from %d, SN: %d.\n", getpid(), msg->src_pid, msg->seq_num);
                    break;

                default:
                    // received a gabage
                    printf("client(%d): Unknown command - %d - %d\n", getpid(), msg->src_pid, msg->cmd);
                    break;
            }
            free(msg);
        } else if (server_online) { 
            if (++waiting_count >= 5) {
                // 4. send a string to server circularly
                //=======================================
                waiting_count = 0;
                char str_buf[STR_SIZE];
                struct ipc_msg *string_msg;
                sprintf(str_buf, "This message is from the client PID.%d. SN:%d", getpid(), ++seq_num);
                msg_len = PREFIX_LEN + strlen(str_buf) + 1;
                string_msg = malloc(msg_len);
                string_msg->cmd = CMD_STRING;
                string_msg->src_pid = getpid();
                string_msg->dest_pid = server_pid;
                string_msg->seq_num = seq_num;
                string_msg->len = PREFIX_LEN + strlen(str_buf) + 1;
                strcpy(string_msg->msg, str_buf);

                writing_lock(writefd);
                lseek(writefd, 0, SEEK_END);
                nbytes = write(writefd, string_msg, msg_len);
                if (nbytes < 0) {
                    error("client-write()");
                }
                release_lock(writefd);
                free(string_msg);
            } else {
                sleep(1);
            }
        } else {
            sleep(1);
        }
    }

    close(readfd);
    close(writefd);
    return;
}

int main() {
    int i = 0;

    signal(SIGINT, parent_sigint);
    // 1. Fork server process and client processes
    //=============================================
    for (i = 0; i < (CLIENT_NUM + 1); ++i) {
        pid[i] = fork();

        if (pid[i] == 0) {
            // Break if child process
            break;
        }
        sleep(1);
    }
 
    if (i == 0) {
        // 2. the first child is server process
        //======================================
        printf("%d. Server process. PID=%d.\n", i + 1, getpid());
        server_process();
    } else if (i <= CLIENT_NUM) {
        // 3. next CLIENT_NUM children are client processes
        //==================================================
        printf("%d. Client process. PID=%d.\n", i + 1, getpid());
        client_process(i);
    } else if (i > CLIENT_NUM) {
        // 4. the last child is parent process
        //======================================
        printf("\n=== Parent process. Waiting for child processes exiting ===\n");
        pid_t pid_wait;
        int j;
        for (j = 0; j < (CLIENT_NUM + 1); ++j) {
            pid_wait = wait(NULL);
            printf("Client exited. PID=%d.\n", pid_wait);
        }
    }

    return EXIT_SUCCESS;

}
