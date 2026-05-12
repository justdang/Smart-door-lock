#ifndef KEYPAD_SERVICE_H
#define KEYPAD_SERVICE_H
#include <stdint.h>
#include <stdbool.h>

void keypad_init(void);
char keypad_getPressedKey(void); //nhận ký tự nhập từ keypad
void keypad_process(void); //cho hoạt động chuyển state (nếu cần)
void keypad_resetBuffer(void); //xóa toàn bộ input
bool keypad_addKey(void); //gom ký tự rời rạc thành chuỗi 
bool keypad_bufferFull(void); //kiểm tra xem buffer đã đầy chưa
char* keypad_getBuffer(void); //lấy dữ liệu buffer cho bên khác -> đổi mật khẩu, auth
#endif