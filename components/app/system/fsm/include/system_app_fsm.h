#ifndef SYSTEM_APP_FSM_H
#define SYSTEM_APP_FSM_H
#include "system_event.h"
#include <stdint.h>

typedef enum {
    STATE_SYS_INIT,
    STATE_SYS_LOCKED, //vai trò tương tự idle
    STATE_SYS_AUTH,
    STATE_SYS_UNLOCKED,
    STATE_SYS_DOOR_OPEN,
    STATE_SYS_WAIT_AUTOLOCK, //cho 3-5s sau khi cua dong roi moi khoa
    STATE_SYS_LOCKED_OUT,
    STATE_SYS_ADMIN_MODE,
    STATE_SYS_ERROR
} systemState;

void appFSM_Init(void);               //khoi tao 
void appFSM_Process(void);            //chay FSM 
systemState appFSM_getState(void);       //lay state
void appFSM_setEvent(AppEvent event);

#endif // SYSTEM_APP_FSM_H