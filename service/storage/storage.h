#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "rfid_struct.h"

void storage_init(void);
//password 
bool storage_savePassword(char* password); // lấy pass mới và xóa pass cũ
char* storage_getPassword(void);
//rfid
bool storage_addUID(RFID_UID_t* uid);
bool storage_removeUID(RFID_UID_t* uid); // tìm và xóa UID
bool storage_findUID(RFID_UID_t* uid);
char* storage_getUIDs(void); //lấy tất cả UID đã lưu dưới dạng chuỗi (để hiển thị hoặc debug)
//admin
bool storage_saveAdminPassword(char* admin_password);
char* storage_getAdminPassword(void);

//void storage_reset(void); //xóa hết bộ nhớ


#endif