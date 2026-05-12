#include "rtc_driver.h"
#include <time.h>
#include <sys/time.h>

static void RTC_Init_HW(void) {
    // Khởi tạo múi giờ (Ví dụ: Việt Nam GMT+7)
    setenv("TZ", "CST-7", 1); 
    tzset();
    // (Việc kết nối SNTP qua Wifi sẽ do Wifi Driver thực hiện)
}

static bool RTC_GetTime_HW(DateTime_t *current_time) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2020 - 1900)) return false; // Chưa đồng bộ

    current_time->seconds = timeinfo.tm_sec;
    current_time->minutes = timeinfo.tm_min;
    current_time->hours   = timeinfo.tm_hour;
    current_time->day     = timeinfo.tm_wday; // 0 = Sunday
    current_time->date    = timeinfo.tm_mday;
    current_time->month   = timeinfo.tm_mon + 1;
    current_time->year    = timeinfo.tm_year + 1900;
    return true;
}

static bool RTC_SetTime_HW(const DateTime_t *new_time) {
    struct tm timeinfo = {0};
    timeinfo.tm_sec  = new_time->seconds;
    timeinfo.tm_min  = new_time->minutes;
    timeinfo.tm_hour = new_time->hours;
    timeinfo.tm_mday = new_time->date;
    timeinfo.tm_mon  = new_time->month - 1;
    timeinfo.tm_year = new_time->year - 1900;

    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
    return true;
}

static uint32_t RTC_GetTimestamp_HW(void) {
    time_t now;
    time(&now);
    return (uint32_t)now;
}

static bool RTC_IsSynced_HW(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    return (timeinfo.tm_year >= (2020 - 1900));
}

const RTC_Driver_t ESP32_RTC_Driver = {
    .Init         = RTC_Init_HW,
    .GetTime      = RTC_GetTime_HW,
    .SetTime      = RTC_SetTime_HW,
    .GetTimestamp = RTC_GetTimestamp_HW,
    .IsSynced     = RTC_IsSynced_HW
};
