#include "whdlr.h"
#include "cJSON.h"
#include "socket.h"
#include <stdio.h>
#include <string.h>

static void getHandler(int client_socket, char *data __attribute__((unused)))
{
    const char response[] = "Hello, World!\n";
    const char header[]   = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n";

    // Calculate the content length
    char content_length_header[50];
    sprintf(content_length_header, "Content-Length: %zu\r\n", strlen(response));

    write(client_socket, header, strlen(header)); // Header
    write(client_socket, content_length_header,
          strlen(content_length_header));             // Content len
    write(client_socket, "\r\n\r\n", 2);              // End of header
    write(client_socket, response, strlen(response)); // Body
}

static void postHandler(int client_socket, char *data)
{
    cJSON *root = cJSON_Parse(data);

    getHandler(client_socket, data);
}

SocketRoute_t Whdlr_GetWeatherApiRoutes(void)
{
    return (SocketRoute_t){
        .path         = "/whdlr",
        .handleGet    = &getHandler,
        .handlePut    = NULL,
        .handlePost   = &postHandler,
        .handleDelete = NULL,
    };
}
