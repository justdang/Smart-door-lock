#include "display_driver.h"
#include "system_config.h"
<<<<<<< Updated upstream
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
=======
#include "driver/i2c.h" //khong co thu vien nay
>>>>>>> Stashed changes

#define TAG_DISP "DISPLAY_DRIVER"

static spi_device_handle_t display_spi_handle;

// --- CÁC HÀM LOW-LEVEL TRUYỀN DỮ LIỆU QUA SPI ---

// Hàm gửi Lệnh (Command) tới màn hình (Chân D/C kéo xuống LOW)
static void tft_send_cmd(uint8_t cmd) {
    gpio_set_level(PIN_DISPLAY_DC, 0); 
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(display_spi_handle, &t);
}

// Hàm gửi Dữ liệu (Data) tới màn hình (Chân D/C kéo lên HIGH)
static void tft_send_data(uint8_t data) {
    gpio_set_level(PIN_DISPLAY_DC, 1); 
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_transmit(display_spi_handle, &t);
}

// Hàm gửi một mảng Dữ liệu liên tục (Data buffer)
static void tft_send_data_buf(const uint8_t *buf, size_t len) {
    if (len == 0) return;
    gpio_set_level(PIN_DISPLAY_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = buf,
    };
    spi_device_transmit(display_spi_handle, &t);
}

// Hàm thiết lập vùng hiển thị (Window) trên màn hình TFT để vẽ/viết chữ
static void tft_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    tft_send_cmd(0x2A); // Column Address Set
    tft_send_data(x0 >> 8);
    tft_send_data(x0 & 0xFF);
    tft_send_data(x1 >> 8);
    tft_send_data(x1 & 0xFF);

    tft_send_cmd(0x2B); // Row Address Set
    tft_send_data(y0 >> 8);
    tft_send_data(y0 & 0xFF);
    tft_send_data(y1 >> 8);
    tft_send_data(y1 & 0xFF);

    tft_send_cmd(0x2C); // Memory Write (Bắt đầu nạp màu)
}

// --- FONT CHỮ ĐƠN GIẢN (ASCII 8x16) ĐỂ IN TEXT LÊN TFT ---
// Ma trận pixel cho các ký tự từ Space (0x20) đến '~' (0x7E)
extern const uint8_t font_8x16[][16]; 
// Ghi chú: Nếu chưa có file font, bạn có thể định nghĩa một mảng font 8x16 tiêu chuẩn tại đây, 
// hoặc tạm thời dùng hàm vẽ ký tự thô. Để đảm bảo code biên dịch được ngay, ta dùng một font mẫu tối giản:
const uint8_t font_8x16[][16] = {
    [0x20 - 0x20] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Khoảng trắng
    ['0' - 0x20]  = {0x00,0x00,0x3c,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3c,0x00,0x00,0x00}, // Số 0
    ['A' - 0x20]  = {0x00,0x00,0x18,0x24,0x42,0x42,0x7e,0x42,0x42,0x42,0x42,0x42,0x42,0x00,0x00,0x00}, // Chữ A
    // ... Bạn có thể bổ sung đầy đủ bảng font 8x16 chuẩn ASCII vào đây hoặc include từ file font.h ...
};

// Hàm vẽ một ký tự (Character) lên màn hình tại tọa độ (x, y) với màu sắc chỉ định
static void tft_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color) {
    if (c < 0x20 || c > 0x7E) c = ' '; // Vượt quá bảng font thì đổi thành khoảng trắng
    uint8_t c_index = c - 0x20;
    
    // Tạo buffer màu cho 1 ký tự 8x16 (mỗi pixel chiếm 2 byte màu RGB565)
    uint16_t line_buf[8]; 

    for (int row = 0; row < 16; row++) {
        uint8_t bits = font_8x16[c_index][row];
        for (int col = 0; col < 8; col++) {
            // Kiểm tra bit từ MSB tới LSB để quyết định tô màu chữ hay màu nền
            if (bits & (0x80 >> col)) {
                line_buf[col] = (color >> 8) | (color << 8); // Swap byte cho đúng định dạng SPI TFT
            } else {
                line_buf[col] = (bg_color >> 8) | (bg_color << 8);
            }
        }
        tft_set_address_window(x, y + row, x + 7, y + row);
        tft_send_data_buf((uint8_t*)line_buf, 8 * 2);
    }
}

// --- IMPLEMENTATION CÁC HÀM TRONG ĐỐI TƯỢNG DRIVER ---

static void Display_Init_HW(void) {
    ESP_LOGI(TAG_DISP, "Initializing TFT Display via SPI Bus...");

    // 1. Cấu hình chân điều khiển Data/Command (D/C) và Reset (RST)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_DISPLAY_DC) | (1ULL << PIN_DISPLAY_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 2. Đăng ký màn hình TFT vào Bus SPI2 chung đã khởi tạo bên RFID
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // Tốc độ SPI 10MHz mượt mà cho TFT
        .mode = 0,                          // SPI Mode 0 trùng với RFID
        .spics_io_num = PIN_DISPLAY_CS,     // Chân CS riêng biệt để phân biệt với RFID
        .queue_size = 7
    };
    
    // Thêm thiết bị vào Bus SPI2_HOST (Bus này do rfid_driver khởi tạo trước)
    spi_bus_add_device(SPI2_HOST, &devcfg, &display_spi_handle);

    // 3. Thực hiện chu trình Reset cứng màn hình
    gpio_set_level(PIN_DISPLAY_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_DISPLAY_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // 4. Gửi chuỗi byte lệnh khởi tạo chuẩn cho chip TFT (Ví dụ: ST7735)
    tft_send_cmd(0x01); // Software Reset
    vTaskDelay(pdMS_TO_TICKS(120));

    tft_send_cmd(0x11); // Sleep Out
    vTaskDelay(pdMS_TO_TICKS(120));

    tft_send_cmd(0x3A); // Set Interface Pixel Format
    tft_send_data(0x05); // Định dạng màu 16-bit/pixel (RGB565)

    tft_send_cmd(0x29); // Display On
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // Xóa màn hình về màu đen sau khi bật
    Display_Clear_HW();
    ESP_LOGI(TAG_DISP, "TFT Display initialized successfully.");
}

static void Display_Clear_HW(void) {
    // Xóa toàn bộ màn hình bằng cách tô màu Đen (Màu RGB565: 0x0000)
    // Giả định màn hình có độ phân giải phổ thông 128x160 (Thay đổi theo màn hình của bạn)
    uint16_t width = 128;
    uint16_t height = 160;
    
    tft_set_address_window(0, 0, width - 1, height - 1);

    // Khởi tạo một hàng pixel màu đen để đẩy nhanh qua DMA/SPI thay vì đẩy từng pixel đơn lẻ
    uint16_t *row_buf = malloc(width * sizeof(uint16_t));
    if (row_buf == NULL) return;
    memset(row_buf, 0, width * sizeof(uint16_t));

    for (int i = 0; i < height; i++) {
        tft_send_data_buf((uint8_t*)row_buf, width * 2);
    }

    free(row_buf);
}

static void Display_PrintText_HW(uint8_t x, uint8_t y, const char *text) {
    if (text == NULL) return;

    // Quy đổi tọa độ: Ký tự font 8x16. 
    // Nếu tầng trên truyền x, y theo dạng "Cột, Hàng" của LCD1602, ta nhân tương ứng sang Pixel của TFT.
    uint16_t pixel_x = x * 8; 
    uint16_t pixel_y = y * 16;
    
    uint16_t text_color = 0xFFFF; // Màu chữ: Trắng
    uint16_t bg_color   = 0x0000; // Màu nền chữ: Đen

    while (*text) {
        // Kiểm tra tràn màn hình theo chiều ngang (giả định màn rộng 128 pixel)
        if (pixel_x + 8 > 128) {
            pixel_x = 0;
            pixel_y += 16; // Tự động xuống dòng
        }
        // Kiểm tra tràn màn hình theo chiều dọc (giả định màn cao 160 pixel)
        if (pixel_y + 16 > 160) {
            break; 
        }

        tft_draw_char(pixel_x, pixel_y, *text, text_color, bg_color);
        pixel_x += 8; // Dịch sang phải 8 pixel cho ký tự tiếp theo
        text++;
    }
}

static void Display_ShowStatus_HW(const char *status) {
    Display_Clear_HW();
    // In chuỗi trạng thái ra chính giữa màn hình (Ví dụ: dòng số 3)
    Display_PrintText_HW(0, 3, status);
}

// Đối tượng Driver cấu trúc liên kết trỏ hàm
const Display_Driver_t ESP32_Display_Driver = {
    .Init       = Display_Init_HW,
    .Clear      = Display_Clear_HW,
    .PrintText  = Display_PrintText_HW,
    .ShowStatus = Display_ShowStatus_HW
};