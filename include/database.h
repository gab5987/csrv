#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <mongoc/mongoc.h>

int  Db_MongoInitialize(void);
void Db_MongoDestroy(void);
int Db_InsertDocument(const char *dbname, const char *collname, bson_t *insert);

#endif // !_DATABASE_H_
