#ifndef MQ_CONST_H
#define MQ_CONST_H 1

#define SERVER_QUEUE_NAME       "/posix-mq-server"
#define QUEUE_PERMISSIONS       0660
#define MAX_MESSAGES            10
#define MAX_MSG_SIZE            256
#define MSG_BUFFER_SIZE         MAX_MSG_SIZE + 16
#define QD_NAME_SIZE            255

#endif
