#include <stdint.h>
#include <stdbool.h>
#include "rtc_service.h"
#include "rtc_driver.h"

//Ten cac API lay tu driver dang la ten tam thoi do chua co source code cua driver
void rtc_init(void){
    rtc_driver_init();
}

bool rtc_getTime(DateTime_t *current_time){
    return rtc_driver_getTime(current_time);
}

bool rtc_setTime(DateTime_t *new_time){
    return rtc_driver_setTime(new_time);
}

uint32_t rtc_getTimeStamp(void){
    return rtc_driver_getTimeStamp();
}

bool rtc_synced(void){
    return rtc_isSynced();
}