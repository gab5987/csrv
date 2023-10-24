#include "cmmutils.h"
#include "cJSON.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 256

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

    const char *levelstr;
    switch (level)
    {
        case DEBUG:
            levelstr = "DEBUG";
            break;
        case INFO:
            levelstr = "INFO";
            break;
        case WARNING:
            levelstr = "WARNING";
            break;
        case ERROR:
            levelstr = "ERROR";
            break;
        default:
            levelstr = "UNKNOWN";
    }

    fprintf(stdout, "[%s] [%s] ", timestamp, levelstr);
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