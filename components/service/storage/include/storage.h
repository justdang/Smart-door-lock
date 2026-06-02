#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h" // có sẵn của espidf
#include "rfid_struct.h"
#include "storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

void storage_init(void);
//password 
bool storage_savePassword(char* password); // lấy pass mới và xóa pass cũ
bool storage_getPassword(void);
//rfid
bool storage_addUID(RFID_UID_t* uid);
bool storage_removeUID(RFID_UID_t* uid); // tìm và xóa UID
bool storage_findUID(RFID_UID_t* uid);
bool storage_getUIDs(void); //lấy tất cả UID đã lưu dưới dạng chuỗi (để hiển thị hoặc debug)
//admin
bool storage_saveAdminPassword(char* admin_password);
bool storage_getAdminPassword(void);

//void storage_reset(void); //xóa hết bộ nhớ


#endif