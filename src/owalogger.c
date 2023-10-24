#include "owalogger.h"
#include "cJSON.h"
#include "cmmutils.h"
#include "database.h"
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

#define SANITY_CHECK(cond, x)        \
    if (cond)                        \
    {                                \
        Logger_LogMessage(ERROR, x); \
        goto error;                  \
    }

static char *Owa_GetData(void)
{
    char  host[100], sendline[256], *recvline = malloc(sizeof(char) * MAX_RECV_LINE);
    char *apikey = getenv("OPEN_WEATHER_KEY");
    char *message_fmt =
        "GET /data/3.0/onecall?lat=-27.5752&lon=-48.4326&exclude=minutely,hourly,daily&appid=%s&lang=pt_br "
        "Accept: */*\r\n HTTP/1.1\r\n\r\n";

    int socketfd, sendbytes;
    SAI servaddr;

    SANITY_CHECK(Tools_HostnameToIp(getenv("OPEN_WEATHER_HOST"), host) != 0, "Couldnt resolve owa hostname");

    SANITY_CHECK((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0, "Error while creating owa socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(80);

    SANITY_CHECK(inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0, "Error in socket pton");

    SANITY_CHECK(connect(socketfd, (SA *)&servaddr, sizeof(servaddr)) < 0, "Failed to connect to owa server");

    sprintf(sendline, message_fmt, apikey);
    sendbytes = strlen(apikey) + BASE_FMT_LEN;

    SANITY_CHECK(write(socketfd, sendline, sendbytes) != sendbytes, "Failed to write to owa server");

    do
    {
    } while ((read(socketfd, recvline, MAX_RECV_LINE - 1)) > 0);
    return recvline;
error:
    if (recvline != NULL) free(recvline);
    return NULL;
}

WeatherapiDescCurrentWeather_t *Owa_ParseWeather(char *data)
{
    cJSON                          *json     = NULL;
    WeatherapiDescCurrentWeather_t *response = malloc(sizeof(WeatherapiDescCurrentWeather_t));

    json = cJSON_Parse(data);
    if (!Tools_CheckJsonErr(json, "root")) goto errprone;

    const char *current_key = "current";
    cJSON      *current     = cJSON_GetObjectItemCaseSensitive(json, current_key);

    if (Tools_CheckJsonErr(current, current_key))
    {
        cJSON_get_value_number(current, response->dt, "dt", false);
        cJSON_get_value_number(current, response->sunrise, "sunrise", false);
        cJSON_get_value_number(current, response->sunset, "sunset", false);
        cJSON_get_value_number(current, response->temp, "temp", true);
        cJSON_get_value_number(current, response->feels_like, "feels_like", true);
        cJSON_get_value_number(current, response->pressure, "pressure", false);
        cJSON_get_value_number(current, response->humidity, "humidity", false);
        cJSON_get_value_number(current, response->clouds, "clouds", false);
        cJSON_get_value_number(current, response->wind_speed, "wind_speed", false);
        cJSON_get_value_number(current, response->wind_deg, "wind_deg", false);
        cJSON_get_value_number(current, response->uvi, "uvi", true);
    }

    if (json != NULL) cJSON_free(json);
    return response;
errprone:
    if (response != NULL) free(response);
    return NULL;
}

void *Owa_Task(__attribute__((unused)) void *args)
{
    do
    {
        char *data = Owa_GetData();
        if (data == NULL) goto taskend;

        bson_t    *doc;
        bson_oid_t oid;

        WeatherapiDescCurrentWeather_t *parsed_data = Owa_ParseWeather(data);

        bson_oid_init(&oid, NULL);

        doc = BCON_NEW(
            "dt", BCON_INT32(parsed_data->dt), "sunrise", BCON_INT32(parsed_data->sunrise), "sunset",
            BCON_INT32(parsed_data->sunset), "temp", BCON_DOUBLE(parsed_data->temp), "feels_like",
            BCON_DOUBLE(parsed_data->feels_like), "uvi", BCON_DOUBLE(parsed_data->uvi), "pressure",
            BCON_INT32(parsed_data->pressure), "humidity", BCON_INT32(parsed_data->humidity), "clouds",
            BCON_INT32(parsed_data->clouds), "wind_speed", BCON_DOUBLE(parsed_data->wind_speed), "wind_deg",
            BCON_INT32(parsed_data->wind_deg));

        BSON_APPEND_OID(doc, "_id", &oid);
        if (Db_InsertDocument("weather", "owa", doc) == 0)
        {
            char str[25] = {0};
            bson_oid_to_string(&oid, str);
            Logger_LogMessage(INFO, "Inserted owa document: %s", str);
        }
        else
            Logger_LogMessage(WARNING, "Could not insert owa document");

    taskend:
        if (data != NULL) free(data);
        if (parsed_data != NULL) free(parsed_data);

        sleep(OWA_LOGTIME);
    } while (1);
}

pthread_t *Owa_Init(void)
{
    pthread_t *owathread = malloc(sizeof(pthread_t));
    if (pthread_create(owathread, NULL, Owa_Task, NULL) < 0)
        Logger_LogMessage(WARNING, "failed to create owa logger thread");

    return owathread;
}