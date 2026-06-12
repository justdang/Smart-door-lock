#include "display_driver.h"
#include "system_config.h"
#include "driver/i2c.h" 
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG_DISP "OLED_I2C_DRIVER"

// Cấu hình các thông số I2C cho màn hình OLED
#define I2C_MASTER_NUM              I2C_NUM_0    // Sử dụng bộ ngoại vi I2C số 0 của ESP32
#define OLED_I2C_ADDRESS            0x3C         // Địa chỉ I2C phổ thông của màn hình OLED SSD1306
#define I2C_MASTER_FREQ_HZ          400000       // Tốc độ Bus I2C: Fast Mode (400kHz) giúp hiển thị mượt mà
#define I2C_MASTER_TX_BUF_DISABLE   0            // Không cần bộ đệm TX
#define I2C_MASTER_RX_BUF_DISABLE   0            // Không cần bộ đệm RX

// Kích thước màn hình OLED đơn sắc
#define OLED_WIDTH                  128
#define OLED_HEIGHT                 64

// --- BẢNG FONT CHỮ TIÊU CHUẨN 5x7 (ASCII từ ' ' đến 'Z') ---
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Khoảng trắng (Space)
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // @
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // A
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // Q
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // V
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

// --- CÁC HÀM GIAO TIẾP TẦNG THẤP (LOW-LEVEL I2C) ---

static esp_err_t oled_send_cmd(uint8_t cmd) {
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x00, true); 
    i2c_master_write_byte(link, cmd, true);
    i2c_master_stop(link);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, link, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(link);
    return ret;
}

static esp_err_t oled_send_data(uint8_t data) {
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x40, true); 
    i2c_master_write_byte(link, data, true);
    i2c_master_stop(link);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, link, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(link);
    return ret;
}

static void oled_write_char(char c) {
    if (c < 32 || c > 90) c = ' '; 
    uint8_t index = c - 32;

    for (uint8_t i = 0; i < 5; i++) {
        oled_send_data(font_5x7[index][i]); 
    }
    oled_send_data(0x00); 
}

static void oled_print_string(uint8_t page, uint8_t col, const char* str) {
    if (page > 7) page = 7;
    oled_send_cmd(0xB0 + page);                    
    oled_send_cmd(col & 0x0F);                     
    oled_send_cmd(0x10 | ((col >> 4) & 0x0F));      

    while (*str) {
        oled_write_char(*str);
        str++;
    }
}

// --- IMPLEMENTATION CÁC HÀM LOGIC CỦA DRIVER ---

static void Display_Init_HW(void) {
    ESP_LOGI(TAG_DISP, "Initializing OLED Display via I2C Bus...");

    // ĐÃ XÓA KHỐI CÀI ĐẶT DRIVER I2C Ở ĐÂY ĐỂ DÙNG CHUNG VỚI RTC ĐÚNG THEO YÊU CẦU TRÁNH TREO MẠCH

    vTaskDelay(pdMS_TO_TICKS(100)); 
    
    oled_send_cmd(0xAE); // Tắt màn hình tạm thời
    oled_send_cmd(0x20); // Đặt chế độ định địa chỉ bộ nhớ (Memory Addressing Mode)
    oled_send_cmd(0x10); // Chọn chế độ Page Addressing Mode giống LCD thông thường
    oled_send_cmd(0xB0); // Reset con trỏ về Page 0
    oled_send_cmd(0xC8); // Quét hàng ngược (Lật đúng chiều màn hình không bị ngược chữ)
    oled_send_cmd(0x00); // Set con trỏ cột thấp
    oled_send_cmd(0x10); // Set con trỏ cột cao
    oled_send_cmd(0x40); // Set dòng bắt đầu hiển thị trong RAM
    oled_send_cmd(0x81); // Thiết lập độ tương phản (Contrast)
    oled_send_cmd(0x7F); // Mức trung bình
    oled_send_cmd(0xA1); // Đặt hướng cột xuôi/ngược (Lật chiều ngang)
    oled_send_cmd(0xA6); // Chế độ hiển thị bình thường (Không đảo ngược màu đen/trắng)
    oled_send_cmd(0xA8); // Set tỷ lệ Multiplex Ratio
    oled_send_cmd(0x3F); // 1/64 duty tương ứng 64 hàng pixel
    oled_send_cmd(0xA4); // Xuất nội dung ra dựa theo dữ liệu nạp trong RAM
    oled_send_cmd(0xD3); // Đặt độ lệch màn hình (Display Offset)
    oled_send_cmd(0x00); // Không lệch
    oled_send_cmd(0xD5); // Đặt tần số quét màn hình
    oled_send_cmd(0x80);
    oled_send_cmd(0x8D); // Kích hoạt mạch sạc bơm nguồn tích hợp (Charge Pump)
    oled_send_cmd(0x14); // Bật mạch sạc
    oled_send_cmd(0xAF); // CHÍNH THỨC BẬT MÀN HÌNH

    Display_Clear_HW();
    ESP_LOGI(TAG_DISP, "OLED I2C Display initialized successfully.");
}

static void Display_Clear_HW(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page); 
        oled_send_cmd(0x00);        
        oled_send_cmd(0x10);
        for (uint8_t i = 0; i < OLED_WIDTH; i++) {
            oled_send_data(0x00);   
        }
    }
}

static void Display_PrintText_HW(uint8_t x, uint8_t y, const char *text) {
    if (text == NULL) return;
    
    uint8_t pixel_col = x * 6;
    uint8_t page_row  = y;

    if (pixel_col >= OLED_WIDTH)  pixel_col = 0;
    if (page_row >= 8)            page_row = 7;

    oled_print_string(page_row, pixel_col, text);
}

static void Display_ShowStatus_HW(const char *status) {
    Display_Clear_HW();
    oled_print_string(0, 0, "=== SYSTEM STATUS ===");
    oled_print_string(3, 20, status);
    oled_print_string(7, 0, "Ghi chu: Nhap pass...");
}

const Display_Driver_t ESP32_Display_Driver = {
    .Init       = Display_Init_HW,
    .Clear      = Display_Clear_HW,
    .PrintText  = Display_PrintText_HW,
    .ShowStatus = Display_ShowStatus_HW
};