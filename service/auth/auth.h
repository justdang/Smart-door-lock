#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "storage.h"
#include "rfid_struct.h"

void auth_init(void);
//password
bool auth_checkPassword(char* password);
bool auth_setPassword(char* old_pass, char* new_pass, char* confirm_pass); // đổi mật khẩu 
//rfid
bool auth_checkRFID(RFID_UID_t* uid);
bool auth_setRFID(RFID_UID_t* uid); // thêm thẻ mới vào hệ thống
bool auth_removeRFID(RFID_UID_t* uid); // xóa thẻ khỏi hệ thống
//admin
bool auth_checkAdminPassword(char* password);
bool auth_setAdminPassword(char* old_admin_pass, char* new_admin_pass, char* confirm_admin_pass); // đổi mật khẩu admin
//web

#endif