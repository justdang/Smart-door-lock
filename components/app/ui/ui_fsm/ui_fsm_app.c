#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ui_fsm_app.h"
#include "freertos/FreeRTOS.h" 
#include "esp_log.h"

#define PIN_MAX_LEN         6
#define PIN_MIN_LEN         4
#define FEEDBACK_DELAY_MS   1500

static UIState s_current_state = STATE_UI_INIT;
static char     s_pin_buf[7]    = {0};  
static char     s_pin_first[7]  = {0};  
static uint8_t  s_pin_len       = 0;
static uint8_t  s_last_key      = 0; 

static void ui_transition(UIState new_state);
static void ui_enter_state(UIState new_state);
static void ui_handle_idle(UIEvent event);
static void ui_handle_pin_input(UIEvent event);
static void ui_handle_pin_verify(UIEvent event);
static void ui_handle_pin_deny(UIEvent event);
static void ui_handle_unlocked(UIEvent event);
static void ui_handle_locked(UIEvent event);
static void ui_handle_admin_menu(UIEvent event);
static void ui_handle_enter_new_pin(UIEvent event);
static void ui_handle_confirm_new_pin(UIEvent event);
static void ui_handle_save_pin(UIEvent event);
static void ui_handle_wait_uid(UIEvent event);
static void ui_handle_verify_uid(UIEvent event);
static void ui_handle_duplicate_uid(UIEvent event);
static void ui_handle_save_uid(UIEvent event);
static void ui_handle_success(UIEvent event);

static void ui_transition(UIState new_state) {ui_enter_state(new_state);}

static void pin_buf_clear(void){
    memset(s_pin_buf,   0, sizeof(s_pin_buf));
    memset(s_pin_first, 0, sizeof(s_pin_first));
    s_pin_len = 0;
}
static void pin_buf_append(uint8_t digit){
    if (s_pin_len >= PIN_MAX_LEN) return;
    s_pin_buf[s_pin_len] = '0' + digit;
    s_pin_len++;
    s_pin_buf[s_pin_len] = '\0';
}
static void post_system_event(AppEvent event){
    AppEventMessage msg = { .event = event };
    system_event_post(&msg);
}
static void ui_enter_state(UIState new_state){
    s_state = new_state;
    switch (new_state) {
        case STATE_UI_IDLE:
            pin_buf_clear();
            ui_screen_show(SCREEN_LOCKED);
            break;
        case STATE_UI_PIN_INPUT:
            pin_buf_clear();
            ui_screen_show(SCREEN_AUTH);
            ui_screen_update_pin_mask(0);   // show empty input
            break;
        case STATE_UI_PIN_VERIFY:
            ui_screen_show(SCREEN_VERIFYING);
            /* submit credential to auth_service */
            {
                Credential cred = { .type = CREDENTIAL_TYPE_PIN };
                strncpy(cred.data.pin.digits, s_pin_buf,
                        sizeof(cred.data.pin.digits) - 1);
                auth_service_submit_credential(&cred);
            }
            break;
        case STATE_UI_PIN_DENY:
            ui_screen_show(SCREEN_DENIED);
            break;
        case STATE_UI_UNLOCKED:
            ui_screen_show(SCREEN_UNLOCKED);
            break;
        case STATE_UI_LOCKED:
            ui_screen_show(SCREEN_LOCKED);
            break;
        case STATE_UI_ADMIN_MENU:
            pin_buf_clear();
            ui_screen_show(SCREEN_ADMIN);
            break;
        case STATE_UI_ENTER_NEW_PIN:
            pin_buf_clear();
            ui_screen_show(SCREEN_ENTER_NEW_PIN);
            break;
        case STATE_UI_CONFIRM_NEW_PIN:
            /* save first entry, clear buffer for second */
            strncpy(s_pin_first, s_pin_buf, sizeof(s_pin_first) - 1);
            pin_buf_clear();
            ui_screen_show(SCREEN_CONFIRM_PIN);
            break;
        case STATE_UI_SAVE_PIN:
            /* compare first and second entries */
            if (strcmp(s_pin_buf, s_pin_first) == 0) {
                if (pin_manager_enroll(s_pin_buf)) {
                    ui_transition(STATE_UI_SUCCESS);
                } else {
                    ui_screen_show(SCREEN_SAVE_FAIL);
                    ui_transition(STATE_UI_ADMIN_MENU);
                }
            } 
            else {
                /* mismatch — go back to enter new PIN */
                ui_screen_show(SCREEN_PIN_MISMATCH);
                ui_transition(STATE_UI_ENTER_NEW_PIN);
            }
            pin_buf_clear();
            break;
        case STATE_UI_WAIT_UID:
            ui_screen_show(SCREEN_SCAN_CARD);
            break;
        case STATE_UI_VERIFY_UID:
            ui_screen_show(SCREEN_VERIFYING);
            break;
        case STATE_UI_DUPLICATE_UID:
            ui_screen_show(SCREEN_UID_DUPLICATE);
            break;
        case STATE_UI_SAVE_UID:
            ui_screen_show(SCREEN_SAVE_UID);
            break;
        case STATE_UI_SUCCESS:
            ui_screen_show(SCREEN_SUCCESS);
            break;
        default:
            break;
    }
}
static void ui_handle_idle(UIEvent event){
    switch (event) {
        case EVENT_UI_KEY_PRESSED:
            pin_buf_append(s_last_key);
            ui_transition(STATE_UI_PIN_INPUT);
            break;
        default:
            break;
    }
}
static void ui_handle_pin_input(UIEvent event){
    switch (event) {
        case EVENT_UI_KEY_PRESSED:
            pin_buf_append(s_last_key);
            ui_screen_update_pin_mask(s_pin_len);
            break;
        case EVENT_UI_SUBMIT:
            if (s_pin_len >= PIN_MIN_LEN) {
                ui_transition(STATE_UI_PIN_VERIFY);
            }
            /* if too short — stay and wait for more input */
            break;
        case EVENT_UI_CANCEL:
            ui_transition(STATE_UI_IDLE);
            break;
        default:
            break;
    }
}
static void ui_handle_pin_verify(UIEvent event){
    switch (event) {
        case EVENT_UI_CORRECT:
            /* post to system FSM — it will unlock the door */
            post_system_event(APP_EVENT_AUTH_SUCCESS);
            ui_transition(STATE_UI_UNLOCKED);
            break;
        case EVENT_UI_WRONG:
            post_system_event(APP_EVENT_AUTH_FAIL);
            ui_transition(STATE_UI_PIN_DENY);
            break;
        default:
            break;
    }
}
static void ui_handle_pin_deny(UIEvent event){
    switch (event) {
        case EVENT_UI_CANCEL:
            ui_transition(STATE_UI_IDLE);
            break;
        case EVENT_UI_KEY_PRESSED:
            /* user tries again */
            pin_buf_clear();
            pin_buf_append(s_last_key);
            ui_transition(STATE_UI_PIN_INPUT);
            break;
        default:
            break;
    }
}
static void ui_handle_unlocked(UIEvent event){
    switch (event) {
        case EVENT_UI_DOOR_CLOSED:
            ui_transition(STATE_UI_LOCKED);
            break;
        default:
            break;
    }
}
static void ui_handle_locked(UIEvent event){
    switch (event) {
        case EVENT_UI_KEY_PRESSED:
            pin_buf_append(s_last_key);
            ui_transition(STATE_UI_PIN_INPUT);
            break;
        default:
            break;
    }
}
static void ui_handle_admin_menu(UIEvent event){
    switch (event) {
        case EVENT_UI_CHANGE_PASSWORD:
            ui_transition(STATE_UI_ENTER_NEW_PIN);
            break;
        case EVENT_UI_ENROLL_UID:
            ui_transition(STATE_UI_WAIT_UID);
            break;
        case EVENT_UI_EXIT:
            post_system_event(APP_EVENT_EXIT_ADMIN);
            ui_transition(STATE_UI_IDLE);
            break;
        default:
            break;
    }
}
static void ui_handle_enter_new_pin(UIEvent event){
    switch (event) {
        case EVENT_UI_KEY_PRESSED:
            pin_buf_append(s_last_key);
            ui_screen_update_pin_mask(s_pin_len);
            break;
        case EVENT_UI_SUBMIT:
            if (s_pin_len >= PIN_MIN_LEN) {
                ui_transition(STATE_UI_CONFIRM_NEW_PIN);
            }
            break;
        case EVENT_UI_CANCEL:
            ui_transition(STATE_UI_ADMIN_MENU);
            break;
        default:
            break;
    }
}
static void ui_handle_confirm_new_pin(UIEvent event){
    switch (event) {
        case EVENT_UI_KEY_PRESSED:
            pin_buf_append(s_last_key);
            ui_screen_update_pin_mask(s_pin_len);
            break;
        case EVENT_UI_SUBMIT:
            if (s_pin_len >= PIN_MIN_LEN) {
                ui_transition(STATE_UI_SAVE_PIN);
            }
            break;
        case EVENT_UI_CANCEL:
            ui_transition(STATE_UI_ADMIN_MENU);
            break;
        default:
            break;
    }
}
static void ui_handle_save_pin(UIEvent event)
{
    /* STATE_UI_SAVE_PIN transitions happen inside ui_enter_state */
    /* no events expected here — transition fires immediately on entry */
    (void)event;
}
static void ui_handle_wait_uid(UIEvent event){
    switch (event) {
        case EVENT_UI_UID_DETECTED:
            ui_transition(STATE_UI_VERIFY_UID);
            break;
        case EVENT_UI_CANCEL:
            ui_transition(STATE_UI_ADMIN_MENU);
            break;
        default:
            break;
    }
}
static void ui_handle_verify_uid(UIEvent event){
    switch (event) {
        case EVENT_UI_UID_EXISTS:
            ui_transition(STATE_UI_DUPLICATE_UID);
            break;
        case EVENT_UI_UID_VALID:
            ui_transition(STATE_UI_SAVE_UID);
            break;
        default:
            break;
    }
}
static void ui_handle_duplicate_uid(UIEvent event){
    switch (event) {
        case EVENT_UI_CANCEL:
            ui_transition(STATE_UI_ADMIN_MENU);
            break;
        default:
            break;
    }
}
static void ui_handle_save_uid(UIEvent event){
    switch (event) {
        case EVENT_UI_SAVE_DONE:
            ui_transition(STATE_UI_SUCCESS);
            break;
        case EVENT_UI_CANCEL:
            ui_transition(STATE_UI_ADMIN_MENU);
            break;
        default:
            break;
    }
}
static void ui_handle_success(UIEvent event){
    switch (event) {
        case EVENT_UI_CANCEL:
        case EVENT_UI_KEY_PRESSED:
            ui_transition(STATE_UI_ADMIN_MENU);
            break;
        default:
            break;
    }
}
void ui_fsm_init(void){
    pin_buf_clear();
    s_state    = STATE_UI_IDLE;
    s_last_key = 0;
    ui_screen_show(SCREEN_LOCKED);
}
void ui_fsm_process(UIEvent event){
    switch (s_state) {
        case STATE_UI_IDLE:             ui_handle_idle(&event);             break;
        case STATE_UI_PIN_INPUT:        ui_handle_pin_input(event);         break;
        case STATE_UI_PIN_VERIFY:       ui_handle_pin_verify(event);        break;
        case STATE_UI_PIN_DENY:         ui_handle_pin_deny(event);          break;
        case STATE_UI_UNLOCKED:         ui_handle_unlocked(event);          break;
        case STATE_UI_LOCKED:           ui_handle_locked(event);            break;
        case STATE_UI_ADMIN_MENU:       ui_handle_admin_menu(event);        break;
        case STATE_UI_ENTER_NEW_PIN:    ui_handle_enter_new_pin(event);     break;
        case STATE_UI_CONFIRM_NEW_PIN:  ui_handle_confirm_new_pin(event);   break;
        case STATE_UI_SAVE_PIN:         ui_handle_save_pin(event);          break;
        case STATE_UI_WAIT_UID:         ui_handle_wait_uid(event);          break;
        case STATE_UI_VERIFY_UID:       ui_handle_verify_uid(event);        break;
        case STATE_UI_DUPLICATE_UID:    ui_handle_duplicate_uid(event);     break;
        case STATE_UI_SAVE_UID:         ui_handle_save_uid(event);          break;
        case STATE_UI_SUCCESS:          ui_handle_success(event);           break;
        default:                                                            break;
    }
}
UIState ui_fsm_get_state(void) {return s_state;}
void ui_fsm_push_key(uint8_t digit){
    s_last_key = digit;
    ui_fsm_process(EVENT_UI_KEY_PRESSED);
}