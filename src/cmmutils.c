#include "cmmutils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LINE_LENGTH 256

static void getTimestamp(char *timestamp)
{
    time_t rawtime;
    struct tm *info;

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", info);
}

void LoggerMessage(LogLevel_t level, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char timestamp[20];
    getTimestamp(timestamp);

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

int CfgEnvLoader(const char *filepath)
{
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (line[0] == '#')
            continue;

        char key[MAX_LINE_LENGTH], value[MAX_LINE_LENGTH];
        if (sscanf(line, "%[^=]=%s", key, value) == 2)
        {
            setenv(key, value, 1);
        }
    }

    fclose(file);
    return 0;
}
