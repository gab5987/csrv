#include "socket.h"
#include "cmmutils.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_null.h>
#include <unistd.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024 * 1000 /* ~1Mb */
#define MAX_ROUTE_LEN 32            /* 32 chars */
#define SA struct sockaddr
#define ST struct Server_t

#define RESPONSEMET(hand, c, d)                                                                                        \
    if (hand != NULL)                                                                                                  \
        hand(c, d);                                                                                                    \
    else                                                                                                               \
        goto not_implemented;

static const char response404[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n";
static const char response501[] = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain\r\n\r\n";

static void *connectionHandler(void *p_args)
{
    ST *server = (ST *)p_args;
    if (server == NULL)
        goto exit;

    int client_socket = server->client_socket;
    char *buffer = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    char *method = (char *)malloc(sizeof(char) * 5);
    char *route = (char *)malloc(sizeof(char) * MAX_ROUTE_LEN);
    ssize_t bytes_received;

    if ((bytes_received = read(client_socket, buffer, MAX_BUFFER_SIZE - 1)) < 0)
    {
        LoggerMessage(ERROR, "error reading from socket (buffer reads less than 0)");
        goto cleanup;
    }

    buffer[bytes_received] = '\0'; // ensures that the last byte is a null terminator

    sscanf(buffer, "%s", method);
    sscanf(buffer + strlen(method), "%s", route);

    char *data = strchr(buffer, '\n');
    if (data != NULL)
        data++;

    for (int i = 0; i <= server->total_routes; i++)
    {
        // If no route matches, return a 404 Not Found response
        if (i == server->total_routes)
        {
            write(client_socket, response404, strlen(response404));
            break;
        }

        const struct SocketRoute_t *current_route = &server->routes[i];
        if (strncmp(route, current_route->path, strlen(current_route->path)) == 0)
        {
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
    if (buffer != NULL)
        free(buffer);
    if (method != NULL)
        free(method);
    if (route != NULL)
        free(route);
    if (client_socket >= 0)
        close(client_socket);
exit:
    pthread_exit(NULL);
}

static void *socketAccept(void *args)
{
    ST *server = (ST *)args;
    socklen_t addr_len = sizeof(server->client_addr);
    while (1)
    {
        server->client_socket = accept(server->server_socket, (SA *)&server->client_addr, &addr_len);
        if (server->client_socket < 0)
        {
            LoggerMessage(ERROR, "Error accepting connection");
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, connectionHandler, (void *)server) != 0)
        {
            LoggerMessage(ERROR, "Error creating thread");
            close(server->client_socket);
        }
    }

    return NULL;
}

ST *SocketInit(void)
{
    ST *server = (ST *)malloc(sizeof(ST));

    if ((server->server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        LoggerMessage(ERROR, "Error creating socket");
        exit(EXIT_FAILURE);
    }

    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr.sin_port = htons(PORT);

    if (bind(server->server_socket, (SA *)&server->server_addr, sizeof(server->server_addr)) == -1)
    {
        LoggerMessage(ERROR, "Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server->server_socket, 10) == -1)
    {
        LoggerMessage(ERROR, "Error listening for connections");
        exit(EXIT_FAILURE);
    }

    LoggerMessage(INFO, "Server listening on port %d...\n", PORT);

    if (pthread_create(&server->thread_id, NULL, socketAccept, (void *)server) != 0)
    {
        LoggerMessage(ERROR, "Error creating thread");
        close(server->client_socket);
    }

    return server;
}

