#include "servo_service.h"
#define LOCK_ANGLE 0
#define UNLOCK_ANGLE 90 

static const Servo_Driver_t *servo = &ESP32_Servo_Driver;

void servo_init(void){
    if(servo && servo->Init) servo->Init();
}

void servo_lock(void){
    if(servo && servo->SetAngle) servo->SetAngle(LOCK_ANGLE);
}

void servo_unlock(void){
    if(servo && servo->SetAngle) servo->SetAngle(UNLOCK_ANGLE);
}

