#include <stdint.h>
#include <stdbool.h>
#include "rfid_service.h"
#include "keypad_service.h"
#include "zigbee_service.h"
#include "freertos/FreeRTOS.h" 
#include "esp_log.h"
#include "auth_service.h"

#define SERVO_ANGLE_LOCKED    0
#define SERVO_ANGLE_UNLOCKED  90
#define SERVO_TIMEOUT_MS      60000 //1min

static LockState s_lock_state = LOCK_STATE_LOCKED

static void lock_report_error(void)
{
    AppEventMessage msg = {
        .event      = APP_EVENT_ERROR,
        .data.error = {
            .code      = APP_ERR_SERVO_FAILED,
            .module_id = MODULE_ID_LOCK,
            .timestamp = hal_get_tick(),
        }
    };
    system_event_post(&msg);
    s_lock_state = LOCK_STATE_ERROR;
}

void lock_controller_init(void){
    servo_init();
    servo_lock();
    s_lock_state = LOCK_STATE_LOCKED;
}

void lock_controller_lock(void){
    if(s_lock_state == LOCK_STATE_LOCKED) return;
    bool ok = //ham wait to lock ->bool
    if(!ok){
        lock_report_error();
        return;
    }
    s_lock_state = LOCK_STATE_LOCKED;
}

void lock_controller_unlock(void){
    if(s_lock_state == LOCK_STATE_UNLOCKED) return;
    bool ok, auth_pin, auth_rfid; 
    
    if(!ok){
        lock_report_error();
        return;
    }
    s_lock_state = LOCK_STATE_UNLOCKED;
}

LockState lock_controller_get_state(void) return s_lock_state;

bool lock_controller_is_locked(void) return(s_lock_state == LOCK_STATE_LOCKED);