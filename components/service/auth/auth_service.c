#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "keypad_service.h"
#include "zigbee_service.h"
#include "rfid_service.h"
#include "rfid_struct.h"
#include "storage.h"

#define MAX_AUTH_ATTEMPTS   5
#define AUTH_TIMEOUT_MS     30000 //30s
#define MAX_PINS            10
#define MAX_RFID            10
#define PIN_MAX_LEN         7           // 6 digits + null
#define UID_MAX_LEN         10

typedef enum{
    AUTH_STATE_IDLE,
    AUTH_STATE_WAITING,
} AuthState;

static AuthState s_state = AUTH_STATE_IDLE;
static uint8_t   s_fail_count = 0;
static uint32_t  s_start_time = 0;
static AuthEventCallback s_callback  = NULL;

static void auth_post_event(AppEvent event) {if(s_callback != NULL) s_callback(event);}      
static bool auth_verify_password(const char *input_pin){
   uint8_t count = storage_getPasswordCount();
    char    stored[PIN_MAX_LEN];
    for (uint8_t i = 0; i < count; i++) {
        if (storage_get_pin(i, stored, sizeof(stored))) {
            if (strcmp(input_pin, stored) == 0) {
                return true;
            }
        }
    }
    return false;
}
static bool auth_verify_rfid(const uint8_t *uid, uint8_t uid_len)
{
    RFID_UID_t target;
    target.length = uid_len;
    memcpy(target.uid, uid, uid_len);
    return storage_findUID(&target);
}

static void auth_handle_result(bool valid)
{
    if (valid) {
        s_fail_count = 0;
        s_state      = AUTH_STATE_IDLE;
        auth_post_event(APP_EVENT_AUTH_SUCCESS);
    } 
    else {
        s_fail_count++;
        s_state = AUTH_STATE_IDLE;
        if (s_fail_count >= MAX_AUTH_ATTEMPTS) auth_post_event(APP_EVENT_LOCKOUT);
        else auth_post_event(APP_EVENT_AUTH_FAIL);
    }
}

//API
void auth_service_init(void)
{
    s_state      = AUTH_STATE_IDLE;
    s_fail_count = 0;
    s_start_time = 0;
    s_callback   = callback;            
}

void auth_service_start(void)
{
    if (s_state != AUTH_STATE_IDLE) return;
    s_state      = AUTH_STATE_WAITING;
    s_start_time = hal_get_tick();
}

void auth_service_cancel(void) s_state = AUTH_STATE_IDLE;
void auth_service_reset_attempts(void) s_fail_count = 0;
void auth_service_submit_credential(const Credential *credential)
{
    if (s_state != AUTH_STATE_WAITING) return;
    if ((hal_get_tick() - s_start_time) >= AUTH_TIMEOUT_MS) {
        s_state = AUTH_STATE_IDLE;
        auth_post_event(APP_EVENT_AUTH_TIMEOUT);
        return;
    }
    bool valid = false;
    switch (credential->type) {
        case CREDENTIAL_TYPE_PIN:
            valid = auth_verify_password(credential->data.pin.digits);
            break;
        case CREDENTIAL_TYPE_RFID:
            valid = auth_verify_rfid(
                credential->data.rfid.uid,
                credential->data.rfid.uid_len
            );
            break;
        default:
            break;
    }
    auth_handle_result(valid);
}
