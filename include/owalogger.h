#ifndef _OWA_LOGGER_H_
#define _OWA_LOGGER_H_

#include <pthread.h>

#define OWA_LOGTIME (int)15 * 60

typedef struct
{
    unsigned long  dt;
    unsigned long  sunrise;
    unsigned long  sunset;
    float          temp;
    float          feels_like;
    float          uvi;
    unsigned int   pressure;
    unsigned short humidity;
    unsigned short clouds;
    float          wind_speed;
    unsigned short wind_deg;
} WeatherapiDescCurrentWeather_t;

pthread_t *Owa_Init(void);

#endif // !_OWA_LOGGER_H_