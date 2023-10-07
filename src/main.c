#include "cmmutils.h"
#include "database.h"
#include "socket.h"
#include "whdlr.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv)
{
    CfgEnvLoader("../.env");

    // MongodbInit();

    struct Server_t *server = SocketInit();

    const struct SocketRoute_t routes[1] = {GetWeatherApiRoutes()};
    server->routes = routes;
    server->total_routes = 1;

    pthread_join(server->thread_id, NULL);

    // bson_t *insert = BCON_NEW("hello", BCON_UTF8("world from method"));
    // InsertDocument("db_name", "collection", insert);

    MongodbDestroy();
}
