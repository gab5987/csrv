#include "cmmutils.h"
#include "database.h"
#include "owalogger.h"
#include "socket.h"
#include "whdlr.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char **argv)
{
    Cfg_EnvLoader("../.env");

    SocketRoute_t routes[1]       = {Whdlr_GetWeatherApiRoutes()};
    Server_t     *server          = NULL;
    int           binding_retries = 0;

init:
    if (binding_retries > 10)
    {
        Logger_LogMessage(ERROR, "Server failed to boot");
        goto exit;
    }

    Db_MongoInitialize();

    pthread_t *owathread = Owa_Init();
    pthread_join(*owathread, NULL);

    // bson_t *insert = BCON_NEW("hello", BCON_UTF8("world from method"));
    // Db_InsertDocument("db_name", "collection", insert);

    // server = Soc_SocketInit(routes, 1);
    // if (server == NULL)
    // {
    //     binding_retries++;
    //     sleep(1);
    //     goto init; // Reinit in case of failure
    // }

    // pthread_join(server->thread_id, NULL);

exit:
    if (server != NULL) free(server);
    Db_MongoDestroy();
}
