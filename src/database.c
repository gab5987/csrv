#include "cmmutils.h"
#include <mongoc/mongoc.h>
#include <stdlib.h>

static mongoc_client_t *database_client;
static mongoc_uri_t    *uri;

int Db_InsertDocument(const char *dbname, const char *collname, bson_t *insert)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(database_client, dbname, collname);
    bson_error_t         error;
    if (!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error))
    {
        Logger_LogMessage(ERROR, "failed to insert document: %s", error.message);
        return -1;
    }
    bson_destroy(insert);
    return 0;
}

void Db_MongoDestroy(void)
{
    mongoc_uri_destroy(uri);
    mongoc_client_destroy(database_client);
    mongoc_cleanup();
}

int Db_MongoInitialize(void)
{
    char *db_url = getenv("DB_URI");

    bson_t      *command, reply;
    bson_error_t error;

    mongoc_init();

    uri = mongoc_uri_new_with_error(db_url, &error);
    if (!uri)
    {
        Logger_LogMessage(
            ERROR,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            db_url, error.message);
        return EXIT_FAILURE;
    }

    database_client = mongoc_client_new_from_uri(uri);
    if (!database_client)
    {
        Logger_LogMessage(ERROR, "failed to create database client");
        return EXIT_FAILURE;
    }

    char *client_db_name = getenv("DB_NAME");
    if (client_db_name == NULL)
    {
        Logger_LogMessage(WARNING, "could not get DB_NAME from env_vars");
        client_db_name = "csrv";
    }

    mongoc_client_set_appname(database_client, client_db_name);

    command = BCON_NEW("ping", BCON_INT32(1));

    bool retval = mongoc_client_command_simple(database_client, "admin", command, NULL, &reply, &error);
    if (!retval)
    {
        Logger_LogMessage(ERROR, "failed to ping database: %s\n", error.message);
        return EXIT_FAILURE;
    }

    char *str = bson_as_json(&reply, NULL);
    if (str != NULL)
        Logger_LogMessage(INFO, "database ping response: %s", str);
    else
        Logger_LogMessage(WARNING, "database did not answered the ping command");

    bson_destroy(&reply);
    bson_destroy(command);

    return EXIT_SUCCESS;
}
