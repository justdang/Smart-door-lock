#ifndef RFID_SERVICE_H
#define RFID_SERVICE_H
#include <stdint.h>
#include <stdbool.h>

#include "rfid_struct.h"

#define RFID_MAX_LENGTH  10

void rfid_init(void);
bool rfid_cardPresent(void); //kiểm tra xem thẻ có ở gần không 
void rfid_halt(void); //đưa thẻ vào trạng thái ngủ 
bool rfid_readUID(RFID_UID_t uid_user); // đọc mã UID từ phần cứng
void rfid_process(void); //Hàm xử lý luồng
#endif