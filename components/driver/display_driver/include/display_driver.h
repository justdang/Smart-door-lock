// File: display_driver.h
// Driver cho màn hình
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>

// Cấu trúc Driver trừu tượng cho Màn hình
typedef struct {
    void (*Init)(void);                                   // Khởi tạo màn hình
    void (*Clear)(void);                                  // Xóa toàn bộ màn hình
    void (*PrintText)(uint8_t x, uint8_t y, char *text);  // In chuỗi tại tọa độ x, y
    void (*ShowStatus)(char *status);                     // Hàm tiện ích: hiển thị dòng trạng thái lớn ở giữa
} Display_Driver_t;

#endif // DISPLAY_DRIVER_H