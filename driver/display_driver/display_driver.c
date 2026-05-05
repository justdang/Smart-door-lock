#include "display_driver.h"
#include "system_config.h"
#include "driver/i2c.h"

#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

static void Display_Init_HW(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // TODO: Gửi chuỗi byte khởi tạo màn hình LCD/OLED
}

static void Display_Clear_HW(void) {
    // TODO: Gửi lệnh Clear Display (VD LCD 1602 là lệnh 0x01)
}

static void Display_PrintText_HW(uint8_t x, uint8_t y, const char *text) {
    // TODO: Set con trỏ I2C tới tọa độ x, y
    // TODO: Gửi từng byte trong mảng ký tự text[] qua đường I2C
}

static void Display_ShowStatus_HW(const char *status) {
    Display_Clear_HW();
    // Căn giữa tương đối chuỗi status và in ra màn hình
    Display_PrintText_HW(0, 0, status);
}

const Display_Driver_t ESP32_Display_Driver = {
    .Init       = Display_Init_HW,
    .Clear      = Display_Clear_HW,
    .PrintText  = Display_PrintText_HW,
    .ShowStatus = Display_ShowStatus_HW
};
