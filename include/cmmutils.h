#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "cJSON.h"

#include <stdbool.h>

typedef enum
{
    DEBUG,
    INFO,
    WARNING,
    ERROR
} LogLevel_t;

int   Cfg_EnvLoader(const char *filepath);
void  Logger_LogMessage(LogLevel_t level, const char *format, ...);
char *Tools_HttpdBodyParser(const char *req);
int   Tools_HostnameToIp(char *hostname, char *ip);
bool  Tools_CheckJsonErr(cJSON *obj, const char *key);

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

#endif // !_CONFIG_H_
