#include <stdint.h>
#include <stdbool.h>
#include "rtc_service.h"
#include "rtc_driver.h"

//Ten cac API lay tu driver dang la ten tam thoi do chua co source code cua driver
void rtc_init(void){
    if(rtc && rtc->Init) rtc->Init();
}

bool rtc_getTime(DateTime_t *current_time){
    if(rtc && rtc->GetTime) return rtc->GetTime();
    return false;
}

bool rtc_setTime(DateTime_t *new_time){
    if(rtc && rtc->SetTime) return rtc->SetTime();
    return false;
}

uint32_t rtc_getTimeStamp(void){
    if(rtc && rtc->GetTimestamp) return rtc->GetTimestamp();
    return 0;
}

bool rtc_synced(void){
    if(rtc && rtc->IsSynced) return rtc->IsSynced();
    return false;
}