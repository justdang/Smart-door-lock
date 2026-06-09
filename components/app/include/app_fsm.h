#ifndef APP_FSM_H
#define APP_FSM_H
#include <stdint.h>
#include <stdbool.h>

//FSM
//system fsm


//UI fsm
typedef enum{
    STATE_UI_INIT,
    STATE_UI_IDLE,
    STATE_UI_INPUT,
    STATE_UI_VERIFY,
    STATE_UI_DENY,
    STATE_UI_UNLOCK,
    STATE_UI_LOCK,
    STATE_UI_ERROR,
    STATE_UI_MENU,
    STATE_UI_ENTER_NEW,
    STATE_UI_CONFIRM_NEW,
    STATE_UI_SAVE,
    STATE_UI_WAIT_UID,
    STATE_UI_VERIFY_UID,
    STATE_UI_SAVE_UID,
    STATE_UI_DUPLICATE,
    STATE_UI_SUCCESS
} uiState;



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