#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <mongoc/mongoc.h>

typedef struct
{
    bson_t          *docs;
    mongoc_cursor_t *cursor;
    int              total_docs;
    int              total_pages;
    bool             has_next_page;
} DbPagination_t;

int           Db_MongoInitialize(void);
void          Db_MongoDestroy(void);
int           Db_InsertDocument(const char *dbname, const char *collname, bson_t *insert);
const bson_t *Db_FindDocument(const char *dbname, const char *collname, mongoc_cursor_t **cursor, bson_t *query);
const bson_t *Db_FindDocumentById(const char *dbname, const char *collname, mongoc_cursor_t **cursor, bson_oid_t *oid);
DbPagination_t *Db_Paginate(const char *dbname, const char *collname, int limit, int page, bson_t *query);
void            Db_PaginationFree(const DbPagination_t *pagination_data);

#endif // !_DATABASE_H_
