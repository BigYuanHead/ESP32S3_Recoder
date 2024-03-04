

#include <sys/time.h>
#include "esp_log.h"


int64_t audio_sys_get_time_ms(void)
{
    struct timeval te;
    gettimeofday(&te, NULL);
    int64_t milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
    return milliseconds;
}

char* get_time(void){
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm = NULL;
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    char amz_date[32];
    char date_stamp[32];

    strftime(amz_date, sizeof amz_date, "%Y%m%dT%H%M%SZ", nowtm);
    // strftime(date_stamp, sizeof date_stamp, "%Y%m%d", nowtm);
    return amz_date;
}
