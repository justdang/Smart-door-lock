// File: servo_driver.h
// Driver cho động cơ servo
#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include <stdint.h>

// Cấu trúc Driver trừu tượng cho Servo
typedef struct {
    void (*Init)(void);                   // Khởi tạo PWM cho chân Servo
    void (*SetAngle)(uint8_t angle);      // Xoay Servo tới một góc cụ thể (0 - 180 độ)

    // phần dưới này có thể bỏ thì thực ra xoay đến 1 góc cụ thể là đủ góc mở góc đóng rồi 
    // void (*OpenLock)(void);               // Hàm tiện ích: Tự động xoay đến góc mở chốt
    // void (*CloseLock)(void);              // Hàm tiện ích: Tự động xoay đến góc đóng chốt
} Servo_Driver_t;

#endif // SERVO_DRIVER_H