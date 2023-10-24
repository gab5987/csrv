#include "cJSON.h"
#include "cmmutils.h"
#include "socket.h"
#include "whdlr.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#define SA            struct sockaddr
#define SAI           struct sockaddr_in
#define MAX_RECV_LINE 4096 * 2
#define BASE_FMT_LEN  125

int Owa_GetData(void)
{
    char *host   = "82.196.7.246"; // getenv("OPEN_WEATHER_KEY");
    char *apikey = getenv("OPEN_WEATHER_KEY");
    char *message_fmt =
        "GET /data/3.0/onecall?lat=-27.5752&lon=-48.4326&exclude=minutely,hourly,daily&appid=%s&lang=pt_br "
        "Accept: */*\r\n HTTP/1.1\r\n\r\n";

    int  socketfd, n, sendbytes;
    SAI  servaddr;
    char sendline[512], recvline[MAX_RECV_LINE];

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        Logger_LogMessage(ERROR, "Error while creating owa socket");
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(80);

    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0)
    {
        Logger_LogMessage(ERROR, "Error int socket pton");
        return -1;
    }

    if (connect(socketfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
    {
        Logger_LogMessage(ERROR, "Failed to connect to owa server");
        return -1;
    }

    sprintf(sendline, message_fmt, apikey);
    sendbytes = strlen(apikey) + BASE_FMT_LEN;

    if (write(socketfd, sendline, sendbytes) != sendbytes)
    {
        Logger_LogMessage(ERROR, "Failed to write to owa server");
        return -1;
    }

    memset(recvline, 0, MAX_RECV_LINE);

    while ((n = read(socketfd, recvline, MAX_RECV_LINE - 1)) > 0)
    {
        printf("%s", recvline);
    }
    return 0;
}

void Owa_Init(void)
{
    Owa_GetData();
}