// File: rtc_driver.h
// Driver cho thời gian thực
#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// Cấu trúc lưu trữ thời gian thống nhất
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint16_t year;
} DateTime_t;

// Cấu trúc Driver trừu tượng cho RTC
typedef struct {
    void (*Init)(void);                           // Khởi tạo module thời gian (I2C hoặc Wifi/SNTP)
    bool (*GetTime)(DateTime_t *current_time);    // Lấy thời gian hiện tại gán vào struct DateTime_t
    bool (*SetTime)(DateTime_t *new_time);        // Cập nhật lại thời gian cho hệ thống
    uint32_t (*GetTimestamp)(void);               // Chuyển dạng hiển thị thời gian, để check auth
    bool (*IsSynced)(void);                       // Kiểm tra xem thời gian có đồng bộ không
} RTC_Driver_t;

#endif // RTC_DRIVER_H