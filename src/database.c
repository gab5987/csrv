#include "database.h"
#include "cmmutils.h"

#include <math.h>
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
    mongoc_collection_destroy(collection);
    return 0;
}

const bson_t *Db_FindDocument(const char *dbname, const char *collname, mongoc_cursor_t **cursor, bson_t *query)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(database_client, dbname, collname);
    const bson_t        *docs;

    *cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);

    mongoc_collection_destroy(collection);
    bson_destroy(query);
    return docs;
}

const bson_t *Db_FindDocumentById(const char *dbname, const char *collname, mongoc_cursor_t **cursor, bson_oid_t *oid)
{
    bson_t       *query = BCON_NEW("_id", BCON_OID(oid));
    bson_error_t  error;
    const bson_t *doc = Db_FindDocument(dbname, collname, cursor, query);

    mongoc_cursor_next(*cursor, &doc);
    if (mongoc_cursor_error(*cursor, &error))
        Logger_LogMessage(ERROR, "Unable to find document with the given id, %s", error.message);

    return doc;
}

DbPagination_t *Db_Paginate(const char *dbname, const char *collname, int limit, int page, bson_t *query)
{
    if (query == NULL) query = bson_new();

    int     skip = page * limit;
    bson_t *opts = BCON_NEW("limit", BCON_INT64(limit), "skip", BCON_INT64(skip));

    mongoc_collection_t *collection      = mongoc_client_get_collection(database_client, dbname, collname);
    DbPagination_t      *pagination_data = malloc(sizeof(DbPagination_t));

    if (pagination_data == NULL)
    {
        Logger_LogMessage(ERROR, "Error allocating space for pagination_data");
        goto cleanup;
    }

    pagination_data->docs = bson_new();

    pagination_data->total_docs = mongoc_collection_count_documents(collection, query, opts, NULL, NULL, NULL);
    pagination_data->cursor     = mongoc_collection_find_with_opts(collection, query, opts, NULL);

    pagination_data->total_pages   = (int)ceil(pagination_data->total_docs / limit);
    pagination_data->has_next_page = page < pagination_data->total_pages;

cleanup:
    bson_destroy(opts);
    mongoc_collection_destroy(collection);
    return pagination_data;
}

void Db_PaginationFree(const DbPagination_t *pagination_data)
{
    if (pagination_data == NULL) return;

    // bson_t          *docs   = pagination_data->docs;
    // mongoc_cursor_t *cursor = pagination_data->cursor;

    // if (docs != NULL) bson_destroy(docs);
    // if (cursor != NULL) mongoc_cursor_destroy(cursor);
    // free((void *)pagination_data);
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
