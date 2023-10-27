#include "whdlr.h"
#include "cJSON.h"
#include "cmmutils.h"
#include "database.h"
#include "socket.h"

#include <stdio.h>
#include <string.h>

#define ROUTE "/whdlr"

static void getHandler(int client_socket, char *args, char *data __attribute__((unused)))
{
    int            page = 0, limit = 10;
    HttpReqParam_t params[5] = {0};
    char           h[75];
    const char     base_header[] = "HTTP/1.1 %s\r\nContent-Type: application/json\r\n\r\n";

    for (int i = 0; i < Tools_HttpdParamParser(args, params, 5); i++)
    {
        register HttpReqParam_t *p = &params[i];
        if (strcmp("page", p->param) == 0)
            page = atoi(p->value);
        else if (strcmp("limit", p->param) == 0)
            limit = atoi(p->value);
    }

    bson_error_t   *err;
    DbPagination_t *pagination_data = Db_Paginate("weather", "owa", limit, page, NULL);
    bool            has_error       = mongoc_cursor_error(pagination_data->cursor, err);

    if (has_error)
    {
        sprintf(h, base_header, "400 BAD REQUEST");
        write(client_socket, h, strlen(h));
        // char *str = bson_as_canonical_extended_json(err, NULL);
        // write(client_socket, str, strlen(str));
        goto cleanup;
    }

    sprintf(h, base_header, "200 OK");
    write(client_socket, h, strlen(h));
    Tools_WritePaginationsAsJson(client_socket, pagination_data);

cleanup:
    Db_PaginationFree(pagination_data);
    // if (err != NULL) bson_(err);
}

static void postHandler(int client_socket, char *args, char *data)
{
    // cJSON *root = cJSON_Parse(data);

    // getHandler(client_socket, NULL, data);
}

SocketRoute_t Whdlr_GetWeatherApiRoutes(void)
{
    return (SocketRoute_t){
        .path         = ROUTE,
        .handleGet    = &getHandler,
        .handlePut    = NULL,
        .handlePost   = &postHandler,
        .handleDelete = NULL,
    };
}
