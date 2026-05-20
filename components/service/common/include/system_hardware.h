// File: system_hardware.h
#ifndef SYSTEM_HARDWARE_H
#define SYSTEM_HARDWARE_H
#include "servo_driver.h"
#include "rtc_driver.h"
#include "rfid_driver.h"
#include "keypad_driver.h"
#include "display_driver.h"

// Cấu trúc chứa toàn bộ API giao tiếp với ngoại vi
typedef struct {
    const Servo_Driver_t   *Servo;
    const RTC_Driver_t     *RTC;
    const RFID_Driver_t    *RFID;
    const Keypad_Driver_t  *Keypad;
    const Display_Driver_t *Display;
} System_Hardware_t;

extern System_Hardware_t Hardware;
#endif