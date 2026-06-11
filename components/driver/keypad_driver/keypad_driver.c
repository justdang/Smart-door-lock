#include "keypad_driver.h"
#include "system_config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const int ROW_PINS[4] = {PIN_KEYPAD_R1, PIN_KEYPAD_R2, PIN_KEYPAD_R3, PIN_KEYPAD_R4};
static const int COL_PINS[4] = {PIN_KEYPAD_C1, PIN_KEYPAD_C2, PIN_KEYPAD_C3, PIN_KEYPAD_C4};
static const char KEY_MAP[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static void Keypad_Init_HW(void) {
    for (int i = 0; i < 4; i++) {
        gpio_set_direction(ROW_PINS[i], GPIO_MODE_OUTPUT);
        gpio_set_level(ROW_PINS[i], 1); 
        
        gpio_set_direction(COL_PINS[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(COL_PINS[i], GPIO_PULLUP_ONLY); 
    }
}

static char Keypad_GetPressedKey_HW(void) {
    for (int r = 0; r < 4; r++) {
        gpio_set_level(ROW_PINS[r], 0); 
        for (int c = 0; c < 4; c++) {
            if (gpio_get_level(COL_PINS[c]) == 0) { 
                vTaskDelay(pdMS_TO_TICKS(20)); // Chống rung phím (Debounce)
                if (gpio_get_level(COL_PINS[c]) == 0) {
                    
                    // SỬA ĐỔI: Thêm vTaskDelay(1) vào vòng lặp chờ nhả phím 
                    // để tránh gây lỗi đứng Task/kích hoạt Watchdog của ESP32 khi đè phím
                    while(gpio_get_level(COL_PINS[c]) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(10)); 
                    }
                    
                    gpio_set_level(ROW_PINS[r], 1);
                    return KEY_MAP[r][c];
                }
            }
        }
        gpio_set_level(ROW_PINS[r], 1); 
    }
    return '\0';
}

const Keypad_Driver_t ESP32_Keypad_Driver = {
    .Init          = Keypad_Init_HW,
    .GetPressedKey = Keypad_GetPressedKey_HW
};