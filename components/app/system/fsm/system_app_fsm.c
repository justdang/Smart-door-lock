#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "system_app_event.h"
#include "system_app_fsm.h"
#include "freertos/FreeRTOS.h" 
#include "esp_log.h"
//them include cho cac header o service
//them API service   

//dinh nghia ham noi bo
static systemState s_current_state = STATE_SYS_INIT;
static void fsm_handle_init(AppEventMessage *msg);
static void fsm_handle_locked(AppEventMessage *msg);
static void fsm_handle_auth(AppEventMessage *msg);
static void fsm_handle_unlocked(AppEventMessage *msg);
static void fsm_handle_door_open(AppEventMessage *msg);
static void fsm_handle_wait_autolock(AppEventMessage *msg);
static void fsm_handle_locked_out(AppEventMessage *msg);
static void fsm_handle_admin_mode(AppEventMessage *msg);
static void fsm_handle_error(AppEventMessage *msg);

static void fsm_transition(systemState new_state) s_current_state = new_state;

static void fsm_handle_init(AppEventMessage *msg){
    if(msg->event == APP_EVENT_SYSTEM_READY) fsm_transition(STATE_SYS_LOCKED);
    if(msg->event == APP_EVENT_ERROR) fsm_transition(STATE_SYS_ERROR); //loop?
}

static void fsm_handle_locked(AppEventMessage *msg){
    if(msg->event == APP_EVENT_AUTH_REQUEST) fsm_transition(STATE_SYS_AUTHENTICATING);
    if(msg->event == APP_EVENT_ERROR) fsm_transition(STATE_SYS_ERROR);
}

static void fsm_handle_auth(AppEventMessage *msg){
    switch (msg->event){
         case APP_EVENT_AUTH_FAIL:
            fsm_transition(STATE_SYS_LOCKED);
            break;
        case APP_EVENT_AUTH_SUCCESS:
            fsm_transition(STATE_SYS_UNLOCKED);
            break;
        case APP_EVENT_LOCKOUT:
            fsm_transition(STATE_SYS_LOCKED_OUT);
            break;
        case APP_EVENT_ERROR:
            fsm_transition(STATE_SYS_ERROR);
            break;
        default: 
            break;
    }
}
static void fsm_handle_init(AppEventMessage *msg)
{
    switch (msg->event) {
        case APP_EVENT_SYSTEM_READY:    fsm_transition(STATE_SYS_LOCKED);   break;
        case APP_EVENT_ERROR:           fsm_transition(STATE_SYS_ERROR);    break;
        default:                                                             break;
    }
}
static void fsm_handle_locked(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: LOCKED → AUTHENTICATING on AUTH_REQUEST */
        case APP_EVENT_AUTH_REQUEST:    fsm_transition(STATE_SYS_AUTH);     break;
        case APP_EVENT_ERROR:           fsm_transition(STATE_SYS_ERROR);    break;
        default:                                                             break;
    }
}
static void fsm_handle_auth(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: AUTHENTICATING → UNLOCKED on AUTH_SUCCESS */
        case APP_EVENT_AUTH_SUCCESS:    fsm_transition(STATE_SYS_UNLOCKED);     break;
        /* diagram: AUTHENTICATING → LOCKED on AUTH_FAIL */
        case APP_EVENT_AUTH_FAIL:       fsm_transition(STATE_SYS_LOCKED);       break;
        /* diagram: AUTHENTICATING → LOCKED_OUT on LOCKOUT */
        case APP_EVENT_LOCKOUT:         fsm_transition(STATE_SYS_LOCKED_OUT);   break;
        case APP_EVENT_ERROR:           fsm_transition(STATE_SYS_ERROR);        break;
        default:                                                                 break;
    }
}
static void fsm_handle_unlocked(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: UNLOCKED → DOOR_OPEN on DOOR_OPENED */
        case APP_EVENT_DOOR_OPENED:     fsm_transition(STATE_SYS_DOOR_OPEN);    break;
        /* diagram: UNLOCKED → LOCKED on MANUAL_LOCK */
        case APP_EVENT_MANUAL_LOCK:     fsm_transition(STATE_SYS_LOCKED);       break;
        /* diagram: UNLOCKED → ADMIN_MODE on ADMIN_MODE */
        case APP_EVENT_ADMIN_MODE:      fsm_transition(STATE_SYS_ADMIN_MODE);   break;
        case APP_EVENT_ERROR:           fsm_transition(STATE_SYS_ERROR);        break;
        default:                                                                 break;
    }
}
static void fsm_handle_door_open(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: DOOR_OPEN → WAIT_AUTOLOCK on DOOR_CLOSED */
        case APP_EVENT_DOOR_CLOSED:     fsm_transition(STATE_SYS_WAIT_AUTOLOCK); break;
        case APP_EVENT_ERROR:           fsm_transition(STATE_SYS_ERROR);         break;
        default:                                                                  break;
    }
}
static void fsm_handle_wait_autolock(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: WAIT_AUTOLOCK → UNLOCKED on AUTOLOCK_TIMEOUT */
        case APP_EVENT_AUTOLOCK_TIMEOUT: fsm_transition(STATE_SYS_UNLOCKED);    break;
        /* diagram: WAIT_AUTOLOCK → DOOR_OPEN on DOOR_REOPENED */
        case APP_EVENT_DOOR_REOPENED:    fsm_transition(STATE_SYS_DOOR_OPEN);   break;
        case APP_EVENT_ERROR:            fsm_transition(STATE_SYS_ERROR);       break;
        default:                                                                 break;
    }
}
static void fsm_handle_locked_out(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: LOCKED_OUT → LOCKED on LOCKOUT_TIMEOUT */
        case APP_EVENT_LOCKOUT_TIMEOUT:  fsm_transition(STATE_SYS_LOCKED);     break;
        case APP_EVENT_ERROR:            fsm_transition(STATE_SYS_ERROR);      break;
        default:                        /* ignore all input while locked out */ break;
    }
}
static void fsm_handle_admin_mode(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: ADMIN_MODE → UNLOCKED on EXIT_ADMIN */
        case APP_EVENT_EXIT_ADMIN:       fsm_transition(STATE_SYS_UNLOCKED);   break;
        case APP_EVENT_ERROR:            fsm_transition(STATE_SYS_ERROR);      break;
        default:                                                                break;
    }
}
static void fsm_handle_error(AppEventMessage *msg)
{
    switch (msg->event) {
        /* diagram: ERROR has no exit — only system restart recovers */
        default:                        /* stay in error permanently */         break;
    }
}
static void fsm_enter_state(systemState new_state){
    switch (new_state) {
        case STATE_SYS_INIT:
            /* nothing to do — waiting for SYSTEM_READY */
            break;
        case STATE_SYS_LOCKED:
            lock_controller_lock(); //tiep tuc lock 
            auth_service_cancel(); //cancel
            auth_service_reset_attempts(); //
            ui_screen_show(SCREEN_LOCKED);
            break;
        case STATE_SYS_AUTH:
            auth_service_start();
            ui_screen_show(SCREEN_AUTH);
            //trong auth co cong doan la quet the va nhap mat khau, co ui screen show ra khac nhau 
            //vay thi o day co can thay doi gi khong vi khong co state do 
            break;
        case STATE_SYS_UNLOCKED:
            lock_controller_unlock();
            ui_screen_show(SCREEN_UNLOCKED);
            break;
        case STATE_SYS_DOOR_OPEN:
            //ui_screen_show(SCREEN_DOOR_OPEN);
            //cân nhắc thêm hàm với công tắc lưỡi gà cảm biến đóng mở cửa
            break;
        case STATE_SYS_WAIT_AUTOLOCK:
            autolock_start();
            //ui_screen_show(SCREEN_WAIT_AUTOLOCK);
            break;
        case STATE_SYS_LOCKED_OUT:
            lockout_timer_start();
            ui_screen_show(SCREEN_LOCKED_OUT);
            break;
        case STATE_SYS_ADMIN_MODE: //chia nho ra 
            ui_screen_show(SCREEN_ADMIN);
            //thêm các hàm xử lý phần admin (nếu cần)
            break;
        case STATE_SYS_PIN_MANAGER:
            ui_screen_show(SCREEN_PIN_MANAGER);
            pin_manager_execute();
            break;
        case STATE_SYS_RFID_MANAGER:
            ui_screen_show(SCREEN_RFID_MANAGER);
            rfid_manager_execute();
            break;
        case STATE_SYS_ERROR:
            lock_controller_lock(const PinRequest *req);         // fail secure
            ui_screen_show(SCREEN_ERROR);
            break;
        default:
            break;
    }
}

//public API
void appFSM_Init(void){
    s_current_state = STATE_SYS_INIT; //giai thich ly do duoc khai bao 2 lan trong bao cao
    ESP_LOGI("SYS_FSM", "Initialized with state: %d", s_current_state);
}

void appFSM_Process(void){
    AppEventMessage msg;
    if(!system_event_get(&msg)){
        return; //khong co event de xu ly
    }
    ESP_LOGI("SYS_FSM", "Processing event: %d in state: %d", msg.event, s_current_state);
    switch(s_current_state){
        case STATE_SYS_INIT:
            fsm_handle_init(&msg);
            break;
        case STATE_SYS_LOCKED:
            fsm_handle_locked(&msg);
            break;
        case STATE_SYS_AUTH:
            fsm_handle_auth(&msg);
            break;
        case STATE_SYS_UNLOCKED:
            fsm_handle_unlocked(&msg);
            break;
        case STATE_SYS_DOOR_OPEN:
            fsm_handle_door_open(&msg);
            break;
        case STATE_SYS_WAIT_AUTOLOCK:
            fsm_handle_wait_autolock(&msg);
            break;
        case STATE_SYS_LOCKED_OUT:
            fsm_handle_locked_out(&msg);
            break;
        case STATE_SYS_ADMIN_MODE:
            fsm_handle_admin_mode(&msg);
            break;
        case STATE_SYS_ERROR:
            fsm_handle_error(&msg);
            break;
        default:
            ESP_LOGW("SYS_FSM", "Unknown state: %d", s_current_state);
    }
}

systemState appFSM_getState(void) return s_current_state;  

void appFSM_setEvent(AppEvent event){
    AppEventMessage msg = {.event = event}; //C99 designated initializer
    if(!system_event_post(&msg)){
        ESP_LOGW("SYS_FSM", "Failed to post event: %d", event);
    }
}

