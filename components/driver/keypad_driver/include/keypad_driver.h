// File: keypad_driver.h
// Driver cho bàn phím
#ifndef KEYPAD_DRIVER_H
#define KEYPAD_DRIVER_H

// Cấu trúc Driver trừu tượng cho Keypad
typedef struct {
    void (*Init)(void);           // Khởi tạo các chân GPIO cho bàn phím
    char (*GetPressedKey)(void);  // Trả về ký tự được nhấn (hoặc '\0' nếu không có phím nào)
} Keypad_Driver_t;

#endif // KEYPAD_DRIVER_H