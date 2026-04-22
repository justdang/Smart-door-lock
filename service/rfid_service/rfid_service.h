#ifndef RFID_SERVICE_H
#define RFID_SERVICE_H
#include <stdint.h>
#include <stdbool.h>

#include "rfid_struct.h"

#define RFID_MAX_LENGTH  10

void rfid_init(void);
void rfid_process(void);
bool rfid_cardPresent(void); //kiểm tra xem thẻ có ở gần không ?
void rfid_halt(void); //đưa thẻ vào trạng thái ngủ 
void rfid_getUID(RFID_UID_t uid_user);  
bool rfid_readUID(RFID_UID_t uid_user); // đọc mã UID

#endif