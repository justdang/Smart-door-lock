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

void storage_reset(void); //xóa hết bộ nhớ


#endif