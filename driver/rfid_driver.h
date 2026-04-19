// File: rfid_driver.h
//Driver cho thẻ từ RFID
#ifndef RFID_DRIVER_H
#define RFID_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define RFID_MAX_LENGTH 10

typedef struct{
    uint8_t uid[RFID_MAX_LENGTH];
    uint8_t length;
} rfid_uid_t;

// Cấu trúc Driver trừu tượng cho RFID
typedef struct {
    void (*Init)(void);                                  // Khởi tạo giao tiếp (SPI/I2C)
    bool (*IsCardPresent)(void);                         // Kiểm tra xem có thẻ ở gần không
    bool (*ReadUID)(rfid_uid_t uid_user);  // Đọc mã UID vào buffer
    void (*Halt)(void);                                  // Đưa thẻ vào trạng thái ngủ để tránh đọc lặp lại
} RFID_Driver_t;

#endif // RFID_DRIVER_H