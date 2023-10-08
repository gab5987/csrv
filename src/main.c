#include "cmmutils.h"
#include "database.h"
#include "socket.h"
#include "whdlr.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char **argv)
{
    CfgEnvLoader("../.env");

    struct SocketRoute_t routes[1] = {GetWeatherApiRoutes()};
    struct Server_t *server = NULL;
    int binding_reties = 0;

init:
    // MongodbInit();
    if (binding_reties > 10)
    {
        LoggerMessage(ERROR, "Server failed to boot");
        goto exit;
    }

    server = SocketInit(routes, 1);
    if (server == NULL)
    {
        binding_reties++;
        sleep(1);
        goto init; // Reinit in case of failure
    }

    pthread_join(server->thread_id, NULL);

    // bson_t *insert = BCON_NEW("hello", BCON_UTF8("world from method"));
    // InsertDocument("db_name", "collection", insert);

exit:
    if (server != NULL)
        free(server);
    MongodbDestroy();
}
