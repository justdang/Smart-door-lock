#ifndef SYSTEM_APP_EVENTS_H
#define SYSTEM_APP_EVENTS_H
#include <stdint.h>

typedef enum {
    APP_EVENT_SYSTEM_READY,
    APP_EVENT_AUTH_REQUEST,
    APP_EVENT_AUTH_FAIL,
    APP_EVENT_ERROR,
    APP_EVENT_LOCKOUT_TIMEOUT,
    APP_EVENT_MANUAL_LOCK,
    APP_EVENT_LOCKOUT,
    APP_EVENT_AUTH_SUCCESS,
    APP_EVENT_AUTOLOCK_TIMEOUT,
    APP_EVENT_EXIT_ADMIN,
    APP_EVENT_ADMIN_MODE,
    APP_EVENT_DOOR_OPENED,
    APP_EVENT_DOOR_CLOSED,
    APP_EVENT_DOOR_REOPENED
} AppEvent;

typedef enum{
    APP_ERR_SERVO_FAILED,
    APP_ERR_STORAGE_FAILED,
    //them sau
} AppErrorCode;

typedef struct{
    AppErrorCode  code;
    uint8_t       module_id;
    uint32_t      timestamp;
} AppError;

typedef struct{
    AppEvent event;
    union{
        AppError error;
        //trong truong hop can mo rong sau 
    }
} AppEventMessage;

void system_event_init(void); //chuc nang giong flush, co the init va reset
bool system_event_post(AppEventMessage* msg);
bool system_event_get(AppEventMessage* msg);
bool system_pending(void);

#endif // SYSTEM_APP_EVENTS_H