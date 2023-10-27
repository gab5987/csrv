#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "cJSON.h"
#include "database.h"

#include <stdbool.h>

typedef enum
{
    DEBUG,
    INFO,
    WARNING,
    ERROR
} LogLevel_t;

typedef struct
{
    char *param;
    char *value;
} HttpReqParam_t;

int   Cfg_EnvLoader(const char *filepath);
void  Logger_LogMessage(LogLevel_t level, const char *format, ...);
char *Tools_HttpdBodyParser(const char *req);
int   Tools_HostnameToIp(char *hostname, char *ip);
bool  Tools_CheckJsonErr(cJSON *obj, const char *key);
int   Tools_HttpdParamParser(const char *req, HttpReqParam_t *params, int max);
void  Tools_WritePaginationsAsJson(const int client_socket, DbPagination_t *pagination_data);

#define cJSON_get_value_number(jsonptr, variable, from_key, is_float)                      \
    {                                                                                      \
        cJSON *item = cJSON_GetObjectItemCaseSensitive(jsonptr, from_key);                 \
        if (item != NULL) variable = is_float ? (float)item->valuedouble : item->valueint; \
    }

#define cJSON_get_value_string(jsonptr, variable, from_key)                                  \
    {                                                                                        \
        cJSON *item = cJSON_GetObjectItemCaseSensitive(jsonptr, from_key);                   \
        if (item->valuestring != NULL && cJSON_IsString(item)) variable = item->valuestring; \
    }

#define KNRM(x) "\033[0m" x "\033[0m"
#define KRED(x) "\033[31m" x "\033[0m"
#define KGRN(x) "\033[32m" x "\033[0m"
#define KYEL(x) "\033[33m" x "\033[0m"
#define KBLU(x) "\033[34m" x "\033[0m"
#define KMAG(x) "\033[35m" x "\033[0m"
#define KCYN(x) "\033[36m" x "\033[0m"
#define KWHT(x) "\033[37m" x "\033[0m"

#endif // !_CONFIG_H_
