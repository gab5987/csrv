#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <mongoc/mongoc.h>

int MongodbInit(void);
void MongodbDestroy(void);
int InsertDocument(const char *dbname, const char *collname, bson_t *insert);

#endif // !_DATABASE_H_
