#ifndef RTC_SERVICE_H
#define RTC_SERVICE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint16_t year;
} DateTime_t; // cân nhắc tách ra để dùng cho nhiều module

void rtc_init(void);
bool rtc_getTime(DateTime_t *current_time);
bool rtc_setTime(DateTime_t *new_time);
uint32_t rtc_getTimeStamp(void);
bool rtc_synced(void);

#endif