#include "cmmutils.h"
#include "database.h"
#include "socket.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv)
{
    CfgEnvLoader("../.env");

    // MongodbInit();
    const struct Server_t *server = SocketInit();

    pthread_join(server->thread_id, NULL);

    // bson_t *insert = BCON_NEW("hello", BCON_UTF8("world from method"));
    // InsertDocument("db_name", "collection", insert);

    MongodbDestroy();
}
