#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// Định nghĩa cấu trúc trỏ hàm (Interface) cho Display Driver
typedef struct {
    void (*Init)(void);
    void (*Clear)(void);
    void (*PrintText)(uint8_t x, uint8_t y, const char *text);
    void (*ShowStatus)(const char *status);
} Display_Driver_t;

// Khai báo biến extern để các file khác (như main.c) có thể gọi driver này
extern const Display_Driver_t ESP32_Display_Driver;

#endif // DISPLAY_DRIVER_H