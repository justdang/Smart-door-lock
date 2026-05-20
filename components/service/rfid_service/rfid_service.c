#include <stdint.h>
#include <stdbool.h>
#include "rfid_struct.h"
#include "rfid_service.h"

static const RFID_Driver_t *rfid = &ESP32_RFID_Driver;

void rfid_init(void){
    if(rfid && rfid->Init) rfid->Init();
}

bool rfid_cardPresent(void){
    if(rfid && rfid->IsCardPresent) return rfid->IsCardPresent();
    return false; 
    //trả về false nếu driver chưa được gán or thiếu hàm
}

bool rfid_halt(void){
    if(rfid && rfid->Halt) rfid->Halt();
}

bool readUID(RFID_UID_t *uid user){
    if(rfid && rfid->ReadUID) return rfid->ReadUID();
    return false;
    //trả về false nếu driver chưa được gán or thiếu hàm
}

void rfid_process(void){
    RFID_UID_t uid;
    if(!rfid_cardPresent()) return; //thoát sớm tránh lãng phí tài nguyên cpu.
    else{
        if(rfid_readUID(uid)){

        }
    }
}
//Xây dựng rfid_process sau, khi hoàn thành được auth, storage.