#include "rtc_driver.h"
#include "system_config.h"
#include "i2c.h" //low level

#define RTC_I2C_ADDR 0x68 // Địa chỉ mặc định của DS3231

static void RTC_Init_HW(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_RTC_SDA,
        .scl_io_num = PIN_RTC_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_flags = 0, 
    };
    conf.master.clk_speed = 100000; // Đưa tốc độ ra đây để đúng cú pháp ESP-IDF
    
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

// Hàm đọc thời gian từ DS3231
static bool RTC_GetTime_HW(DateTime_t *current_time) {
    uint8_t reg = 0x00;
    uint8_t buffer[7];
    
    // Gửi lệnh đọc thanh ghi từ 0x00
    if (i2c_master_write_read_device(I2C_NUM_0, RTC_I2C_ADDR, &reg, 1, buffer, 7, 1000/portTICK_PERIOD_MS) != ESP_OK) {
        return false;
    }

    // Convert BCD sang số thập phân
    current_time->seconds = ((buffer[0] >> 4) * 10) + (buffer[0] & 0x0F);
    current_time->minutes = ((buffer[1] >> 4) * 10) + (buffer[1] & 0x0F);
    current_time->hours   = ((buffer[2] >> 4) * 10) + (buffer[2] & 0x0F);
    current_time->date    = ((buffer[4] >> 4) * 10) + (buffer[4] & 0x0F);
    current_time->month   = ((buffer[5] >> 4) * 10) + (buffer[5] & 0x0F);
    current_time->year    = 2000 + ((buffer[6] >> 4) * 10) + (buffer[6] & 0x0F);

    return true;
}

const RTC_Driver_t ESP32_RTC_Driver = {
    .Init = RTC_Init_HW,
    .GetTime = RTC_GetTime_HW
    
};