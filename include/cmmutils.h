#ifndef _CONFIG_H_
#define _CONFIG_H_

int Cfg_EnvLoader(const char *filepath);

typedef enum
{
    DEBUG,
    INFO,
    WARNING,
    ERROR
} LogLevel_t;

void  Logger_LogMessage(LogLevel_t level, const char *format, ...);
char *Parser_HttpdBodyParser(const char *req);

#endif // !_CONFIG_H_
