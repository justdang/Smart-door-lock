#ifndef APP_FSM_H
#define APP_FSM_H
#include <stdint.h>
#include <stdbool.h>

//Dinh nghia state, fsm (fsm co the se duoc dua vao README neu can)
typedef enum {
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_INPUT,
    STATE_VERIFY,
    STATE_UNLOCK,
    STATE_WAIT_TO_LOCK,
    STATE_LOCK,
    STATE_ENROLL,
    STATE_ERROR
} appState;

//Dinh nghia event
typedef enum{
    EVENT_NONE,
    //Input
    EVENT_KEYPAD_IN,
    EVENT_RFID_IN,
    EVENT_WEB_IN,
    //Cai dat thoi gian tu dong khoa, cho khoa
    EVENT_WAIT_TO_LOCK_SETTING,
    //Auth
    EVENT_AUTH_SUCCESS,
    EVENT_AUTH_FAIL,
    EVENT_TIMEOUT
} appEvent;

//public API
void appFSM_Init(void);               //khoi tao 
void appFSM_Process(void);            //chay FSM 
appState appFSM_getState(void);       //lay state
void appFSM_setEvent(appEvent event); //
#endif