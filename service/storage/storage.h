#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "rfid_service.h"

void storage_init(void);
//password 
bool storage_savePassword(char* password);
bool storage_getPassword(char* password);
//rfid
bool storage_addUID(RFID_UID_t* uid);
bool storage_removeUID(RFID_UID_t* uid);
bool storage_findUID(RFID_UID_t* uid);
//admin
bool storage_saveAdminPassword(char* admin_password);
bool storage_getAdminPassword(char* admin_password);

void storage_reset(void); //xóa hết bộ nhớ


#endif