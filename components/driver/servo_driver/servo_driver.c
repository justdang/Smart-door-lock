#include "servo_driver.h"
#include "system_config.h" 
#include "driver/ledc.h"

static void Servo_Init_HW(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT, 
        .freq_hz          = 50, // 50Hz chuẩn cho Servo
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t channel_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = PIN_SERVO, 
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&channel_cfg);
}

static void Servo_SetAngle_HW(uint8_t angle) {
    if (angle > 180) angle = 180;
    // Map 0-180 độ sang duty cycle của 13-bit (50Hz)
    uint32_t duty = (uint32_t)(204 + (angle * 819 / 180)); 
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

const Servo_Driver_t ESP32_Servo_Driver = {
    .Init     = Servo_Init_HW,
    .SetAngle = Servo_SetAngle_HW
};
