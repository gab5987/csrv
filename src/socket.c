#include "socket.h"
#include "cmmutils.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT            8080
#define MAX_BUFFER_SIZE 1024 * 1000 /* ~1Mb */
#define MAX_ROUTE_LEN   32          /* 32 chars */
#define MAX_ARGS_LEN    128

#define SA struct sockaddr

#define RESPONSEMET(hand, c, a, d) \
    if (hand != NULL)              \
        hand(c, a, d);             \
    else                           \
        goto not_implemented;

const char response404[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n";
const char response501[] = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain\r\n\r\n";

typedef struct
{
    Server_t *server;
    pthread_t thr;
    int       pclient_socket;
} TheradParams_t;

static void *Soc_ConnectionHandler(void *thread_params)
{
    TheradParams_t *params = (TheradParams_t *)thread_params;
    Server_t       *server = params->server;

    if (server == NULL) pthread_cancel(params->thr);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    int     client_socket        = server->client_socket;
    char   *buffer               = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    char    method[5]            = {0};
    char    route[MAX_ROUTE_LEN] = {0};
    char   *args                 = NULL;
    ssize_t bytes_received;

    if ((bytes_received = read(client_socket, buffer, MAX_BUFFER_SIZE)) < 0)
    {
        Logger_LogMessage(ERROR, "error reading from socket (buffer reads less than 0)");
        goto cleanup;
    }

    buffer[bytes_received] = '\0'; // ensures that the last byte is a null terminator

    sscanf(buffer, "%s", method);
    sscanf((buffer + strlen(method)), "%s", route);
    args = strchr(route, '?');
    if (args != NULL)
    {
        route[args - route] = '\0';
        args++;
    }

    char *data = strchr(buffer, '\n');
    if (data != NULL) data++;

    for (int i = 0; i <= server->total_routes; i++)
    {
        // If no route matches, return a 404 Not Found response
        if (i == server->total_routes)
        {
            write(client_socket, response404, strlen(response404));
            break;
        }

        const SocketRoute_t *current_route    = &server->routes[i];
        size_t               current_path_len = strlen(current_route->path), req_path_len = strlen(route);

        if (current_path_len == req_path_len && strncmp(route, current_route->path, current_path_len) == 0)
        {
            data = Tools_HttpdBodyParser(data);
            if (strcmp(method, "GET") == 0)
            {
                RESPONSEMET(current_route->handleGet, client_socket, args, data);
            }
            else if (strcmp(method, "POST") == 0)
            {
                RESPONSEMET(current_route->handlePost, client_socket, args, data);
            }
            else if (strcmp(method, "PUT") == 0)
            {
                RESPONSEMET(current_route->handlePut, client_socket, args, data);
            }
            else
            {
            not_implemented:
                write(client_socket, response501, strlen(response501));
            }
            break;
        }
    }

cleanup:
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    Logger_LogMessage(
        INFO, KGRN("%s %s %s SocketRuntime/::%d %ldms"), method, route, args != NULL ? args : "", client_socket,
        delta_ms);

    if (buffer != NULL) free(buffer);
    if (client_socket >= 0) close(client_socket);
    if (params != NULL) free(params);
    pthread_exit(NULL);
}

void *Soc_ServerAccept(void *args)
{
    Server_t *server   = (Server_t *)args;
    socklen_t addr_len = sizeof(server->client_addr);
    while (1)
    {
        server->client_socket = accept(server->server_socket, (SA *)&server->client_addr, &addr_len);
        if (server->client_socket < 0)
        {
            Logger_LogMessage(ERROR, "Error accepting connection");
            continue;
        }

        TheradParams_t *pool = malloc(sizeof(TheradParams_t));
        pool->pclient_socket = server->client_socket;
        pool->server         = server;

        if (pthread_create(&pool->thr, NULL, Soc_ConnectionHandler, pool) < 0)
            Logger_LogMessage(WARNING, "failed to create thread for client %d", pool->pclient_socket);
    }
}

Server_t *Soc_SocketInit(const SocketRoute_t *routes, int total_routes)
{
    Server_t *server = (Server_t *)malloc(sizeof(Server_t));

    server->routes       = routes;
    server->total_routes = total_routes;

    if ((server->server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        Logger_LogMessage(ERROR, "Error creating socket");
        goto exit_err;
    }

    if (setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        Logger_LogMessage(WARNING, "Setsockopt(SO_REUSEADDR) failed");

    server->server_addr.sin_family      = AF_INET;
    server->server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr.sin_port        = htons(PORT);

    if (bind(server->server_socket, (SA *)&server->server_addr, sizeof(server->server_addr)) == -1)
    {
        Logger_LogMessage(ERROR, "Error binding socket");
        goto exit_err;
    }

    if (listen(server->server_socket, 10) == -1)
    {
        Logger_LogMessage(ERROR, "Error listening for connections");
        goto exit_err;
    }

    Logger_LogMessage(INFO, "Server listening on port %d...\n", PORT);

    if (pthread_create(&server->thread_id, NULL, Soc_ServerAccept, server) < 0)
    {
        Logger_LogMessage(ERROR, "failed to create server thread");
        goto exit_err;
    }

    return server;
exit_err:
    if (server != NULL) free(server);
    return NULL;
}
