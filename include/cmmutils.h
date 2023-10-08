#ifndef _CONFIG_H_
#define _CONFIG_H_

int CfgEnvLoader(const char *filepath);

typedef enum
{
    DEBUG,
    INFO,
    WARNING,
    ERROR
} LogLevel_t;

void LoggerMessage(LogLevel_t level, const char *format, ...);
char *HttpdBodyParser(const char *req);

#endif // !_CONFIG_H_
