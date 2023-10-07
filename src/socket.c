#include "socket.h"
#include "cmmutils.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024 * 1000 /* ~1Mb */
#define SA struct sockaddr
#define ST struct Server_t

typedef struct
{
    int client_socket;
} ThreadArgs;

void handleGet(int client_socket)
{
    const char response[] = "Hello, World!\n";
    const char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n";

    // Calculate the content length
    char content_length_header[50];
    sprintf(content_length_header, "Content-Length: %zu\r\n", strlen(response));

    write(client_socket, header, strlen(header));                               // Header
    write(client_socket, content_length_header, strlen(content_length_header)); // Content len
    write(client_socket, "\r\n", 2);                                            // End of header
    write(client_socket, response, strlen(response));                           // Body
}

void handlePost(int client_socket, char *data)
{
    char response[MAX_BUFFER_SIZE] = "teste aaaaa\n";
    LoggerMessage(INFO, "received POST request: %s", data);
    write(client_socket, response, strlen(response));
}

void handlePut(int client_socket, char *data)
{
    char response[MAX_BUFFER_SIZE];
    LoggerMessage(INFO, "Received PUT data: %s", data);
    write(client_socket, response, strlen(response));
}

static void *connectionHandler(void *p_args)
{
    ThreadArgs *args = (ThreadArgs *)p_args;
    if (args == NULL)
        goto exit;

    int client_socket = args->client_socket;
    char *buffer = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    ssize_t bytes_received;

    if ((bytes_received = read(client_socket, buffer, sizeof(buffer) - 1)) < 0)
    {
        LoggerMessage(ERROR, "error reading from socket (buffer reads less than 0)");
        goto cleanup;
    }

    buffer[bytes_received] = '\0'; // ensures that the last byte is a null terminator

    char *method = (char *)malloc(sizeof(char) * 5);
    sscanf(buffer, "%s", method);

    char *data = strchr(buffer, '\n');
    if (data != NULL)
        data++;

    if (strcmp(method, "GET") == 0)
        handleGet(client_socket);
    else if (strcmp(method, "POST") == 0)
        handlePost(client_socket, data);
    else if (strcmp(method, "PUT") == 0)
        handlePut(client_socket, data);
    else
    {
        char response[] = "HTTP/1.1 501 Not Implemented\r\n\r\n";
        write(client_socket, response, strlen(response));
    }

cleanup:
    if (buffer != NULL)
        free(buffer);
exit:
    if (args != NULL)
        free(args);
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

        ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        if (args == NULL)
        {
            LoggerMessage(ERROR, "Error allocating memory for thread args");
            close(server->client_socket);
            continue;
        }

        args->client_socket = server->client_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, connectionHandler, (void *)args) != 0)
        {
            LoggerMessage(ERROR, "Error creating thread");
            close(server->client_socket);
            free(args);
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

