#include "cmmutils.h"
#include "cJSON.h"
#include "database.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 256

#define TERMLOG(l, str, c) \
    case l:                \
        levelstr = str;    \
        color    = c;      \
        break;

static void Logger_GetTimestamp(char *timestamp)
{
    time_t     rawtime;
    struct tm *info;

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", info);
}

void Logger_LogMessage(LogLevel_t level, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char timestamp[20];
    Logger_GetTimestamp(timestamp);

    char        logmsg[100] = {0};
    const char *levelstr;
    switch (level)
    {
        case DEBUG: {
            fprintf(stdout, KCYN("[%s] [%s]") " ", timestamp, "DEBUG");
            break;
        }
        case INFO: {
            fprintf(stdout, KWHT("[%s] [%s]") " ", timestamp, "INFO");
            break;
        }
        case WARNING: {
            fprintf(stdout, KYEL("[%s] [%s]") " ", timestamp, "WARNING");
            break;
        }
        case ERROR: {
            fprintf(stdout, KRED("[%s] [%s]") " ", timestamp, "ERROR");
            break;
        }
        default:
            levelstr = "UNKNOWN";
    }

    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");

    va_end(args);
}

int Cfg_EnvLoader(const char *filepath)
{
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        Logger_LogMessage(ERROR, "no env file found for the specified path...exit");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (line[0] == '#') continue;

        char key[MAX_LINE_LENGTH], value[MAX_LINE_LENGTH];
        if (sscanf(line, "%[^=]=%s", key, value) == 2)
        {
            setenv(key, value, 1);
        }
    }

    fclose(file);
    return 0;
}

char *Tools_HttpdBodyParser(const char *req)
{
    if (req == NULL) return 0;
    char *retaddr = NULL;
    retaddr       = strstr(req, "\r\n\r\n");
    if (retaddr == NULL) return NULL;
    return retaddr += 4; // +4 to skip the "\r\n\r\n"
}

int Tools_HttpdParamParser(const char *req, HttpReqParam_t *params, int max)
{
    int   count = 0;
    char *tok, *otok, *str = NULL;
    if (req == NULL) goto cleanup;

    str = strdup(req);

    for (tok = strtok(str, "&"); tok != NULL; tok = strtok(tok, "&"))
    {
        if (count >= max)
        {
            Logger_LogMessage(ERROR, "Exceeded the maximum number of parameters");
            break;
        }
        otok = tok + strlen(tok) + 1;
        tok  = strtok(tok, "=");
        if (tok != NULL) params[count].param = strdup(tok);

        tok = strtok(NULL, "=");
        if (tok != NULL) params[count].value = strdup(tok);
        tok = otok;

        count++;
    };

cleanup:
    if (str != NULL) free(str);
    return count;
}

int Tools_HostnameToIp(char *hostname, char *ip)
{
    int                 sockfd;
    struct addrinfo     hints, *servinfo, *p;
    struct sockaddr_in *h;
    int                 rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, "https", &hints, &servinfo)) != 0)
    {
        Logger_LogMessage(ERROR, "HostnameToIp getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first possible
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        h = (struct sockaddr_in *)p->ai_addr;
        strcpy(ip, inet_ntoa(h->sin_addr));
    }

    if (servinfo != NULL) freeaddrinfo(servinfo);
    return 0;
}

bool Tools_CheckJsonErr(cJSON *obj, const char *key)
{
    if (obj == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        Logger_LogMessage(
            ERROR, "Failed to parse JSON @ key '%s' - error_code: (%s)", key, error_ptr != NULL ? error_ptr : "");
        cJSON_Delete(obj);
        return false;
    }
    return true;
}

char strgbff[1024];
void Tools_WritePaginationsAsJson(const int client_socket, DbPagination_t *pagination_data)
{
    char he[100];
    sprintf(
        he, "{\"total_docs\":%d,\"total_pages\":%d,\"has_next_page\":%s,\"docs\":[", pagination_data->total_docs,
        pagination_data->total_pages, pagination_data->has_next_page ? "true" : "false");
    write(client_socket, he, strlen(he));

    const bson_t    *docs   = pagination_data->docs;
    mongoc_cursor_t *cursor = pagination_data->cursor;

    if (docs != NULL)
    {
        while (mongoc_cursor_next(cursor, &docs))
        {
            snprintf(strgbff, sizeof(strgbff), "%s,", bson_as_json(docs, NULL));
            write(client_socket, strgbff, strlen(strgbff));
        }
    }

    write(client_socket, "]}", 2);
}