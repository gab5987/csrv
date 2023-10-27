#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void (*SocketHandlerOp)(int client_socket, char *args, char *data);

typedef struct
{
    const char     *path;
    SocketHandlerOp handleGet;
    SocketHandlerOp handlePost;
    SocketHandlerOp handlePut;
    SocketHandlerOp handleDelete;
} SocketRoute_t;

typedef struct
{
    int                  server_socket, client_socket;
    struct sockaddr_in   server_addr, client_addr;
    pthread_t            thread_id;
    const SocketRoute_t *routes;
    int                  total_routes;
} Server_t;

Server_t *Soc_SocketInit(const SocketRoute_t *routes, int total_routes);

#endif // !_SOCKET_H_
