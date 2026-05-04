#include <stdint.h>
#include <stdbool.h>
#include "rtc_service.h"
#include "rtc_driver.h"

void rtc_init(void){
    rtc_driver_init();
}

bool rtc_getTime(DateTime_t *current_time){
    return rtc_driver_getTime(current_time);
}


