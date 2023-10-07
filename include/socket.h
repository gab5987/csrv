#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_pthread/_pthread_t.h>
#include <unistd.h>

typedef void (*SocketHandlerOp)(int client_socket, char *data);

struct SocketRoute_t
{
    const char *path;
    SocketHandlerOp handleGet;
    SocketHandlerOp handlePost;
    SocketHandlerOp handlePut;
    SocketHandlerOp handleDelete;
};

struct Server_t
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    pthread_t thread_id;
    const struct SocketRoute_t *routes;
    int total_routes;
};

struct Server_t *SocketInit(void);

#endif // !_SOCKET_H_
