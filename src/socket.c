#include "socket.h"
#include "cmmutils.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT             8080
#define MAX_BUFFER_SIZE  1024 * 1000 /* ~1Mb */
#define MAX_ROUTE_LEN    32          /* 32 chars */
#define THREAD_POOL_SIZE 10

#define SA struct sockaddr

#define RESPONSEMET(hand, c, d) \
    if (hand != NULL)           \
        hand(c, d);             \
    else                        \
        goto not_implemented;

const char response404[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n";
const char response501[] = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain\r\n\r\n";

static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t              thread_pool[THREAD_POOL_SIZE];

struct QueueNode
{
    struct QueueNode *next;
    int              *client_socket;
};
typedef struct QueueNode QueueNode_t;

static QueueNode_t *queue_head = NULL;
static QueueNode_t *queue_tail = NULL;

void Soc_Enqueue(int *client_socket)
{
    QueueNode_t *newnode   = malloc(sizeof(QueueNode_t));
    newnode->client_socket = client_socket;
    newnode->next          = NULL;
    if (queue_tail == NULL)
        queue_head = newnode;
    else
        queue_tail->next = newnode;
    queue_tail = newnode;
}

int *Soc_Dequeue(void)
{
    if (queue_head == NULL) return NULL;
    int         *result = queue_head->client_socket;
    QueueNode_t *temp   = queue_head;
    queue_head          = queue_head->next;
    if (queue_head == NULL) queue_tail = NULL;
    free(temp);
    return result;
}

static void Soc_ConnectionHandler(Server_t *server)
{
    if (server == NULL) return;

    int     client_socket        = server->client_socket;
    char   *buffer               = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    char    method[5]            = {0};
    char    route[MAX_ROUTE_LEN] = {0};
    ssize_t bytes_received;

    if ((bytes_received = read(client_socket, buffer, MAX_BUFFER_SIZE)) < 0)
    {
        Logger_LogMessage(ERROR, "error reading from socket (buffer reads less than 0)");
        goto cleanup;
    }

    buffer[bytes_received] = '\0'; // ensures that the last byte is a null terminator

    sscanf(buffer, "%s", method);
    sscanf(buffer + strlen(method), "%s", route);

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
                RESPONSEMET(current_route->handleGet, client_socket, data);
            }
            else if (strcmp(method, "POST") == 0)
            {
                RESPONSEMET(current_route->handlePost, client_socket, data);
            }
            else if (strcmp(method, "PUT") == 0)
            {
                RESPONSEMET(current_route->handlePut, client_socket, data);
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
    if (buffer != NULL) free(buffer);
    if (client_socket >= 0) close(client_socket);
}

static void *Soc_ConsumeThread(void *args)
{
    Server_t *server = (Server_t *)args;
    while (1)
    {
        pthread_mutex_lock(&thread_mutex);
        int *pclient_socket = Soc_Dequeue();
        pthread_mutex_unlock(&thread_mutex);
        if (pclient_socket != NULL)
        {
            Soc_ConnectionHandler(server);
        }
    }
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

        int *pclient_socket = malloc(sizeof(int));
        *pclient_socket     = server->client_socket;

        pthread_mutex_lock(&thread_mutex);
        Soc_Enqueue(pclient_socket);
        pthread_mutex_unlock(&thread_mutex);
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

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (pthread_create(&thread_pool[i], NULL, Soc_ConsumeThread, server) < 0)
            Logger_LogMessage(WARNING, "failed to create thread %d", i);
    }

    if (pthread_create(&server->thread_id, NULL, Soc_ServerAccept, server) < 0)
    {
        Logger_LogMessage(ERROR, "failed to create server thread");
        goto exit_err;
    }

    return server;
exit_err:
    free(server);
    return NULL;
}
