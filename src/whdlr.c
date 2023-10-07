#include "whdlr.h"
#include "socket.h"
#include <string.h>

static void getHandler(int client_socket, char *data)
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

struct SocketRoute_t GetWeatherApiRoutes(void)
{
    return (struct SocketRoute_t){
        .path = "/whdlr",
        .handleGet = &getHandler,
        .handlePut = NULL,
        .handlePost = NULL,
        .handleDelete = NULL,
    };
}