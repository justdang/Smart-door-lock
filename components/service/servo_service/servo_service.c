#include <stdbool.h>
#include "servo_service.h"
#define LOCK_ANGLE 0
#define UNLOCK_ANGLE 90 
#define AUTOLOCK_TIME 3000 //3 giay

static TimerHandle_t autoLockTimer;

void servo_init(void){
    servo_init();
}

void servo_lock(void){
    servo_setAngle(LOCK_ANGLE);
}

void servo_unlock(void){
    servo_setAngle(UNLOCK_ANGLE);
}

void servo_autoLockCallBack(void){

}
void servo_autolock(void){

}