#include "rfid_manager.h"
#include "storage.h"
#include <string.h>

static bool is_valid_uid(const RFID_UID_t *uid)
{
    if (uid == NULL)             return false;
    if (uid->length == 0)        return false;
    if (uid->length > RFID_MAX_LENGTH) return false;
    bool all_zero = true;
    for (uint8_t i = 0; i < uid->length; i++) {
        if (uid->uid[i] != 0x00) {
            all_zero = false;
            break;
        }
    }
    return !all_zero;
}
static bool rfid_enroll(const RFID_UID_t *uid)
{
    if (!is_valid_uid(uid)) {return false;}
    if (uid_count >= MAX_UIDS) {return false;}
    if (storage_findUID((RFID_UID_t *)uid)) {return false;}
    return storage_addUID((RFID_UID_t *)uid);
}
static bool rfid_delete(uint8_t index)
{
    if (index >= MAX_UIDS) {return false;}
    if (rfid_manager_count() == 0) {return false;}
    RFID_UID_t target;
    if (!storage_getUID(index, &target)) {return false;}
    return storage_removeUID(&target);
}
bool rfid_manager_execute(const RfidRequest *req)
{
    if (req == NULL) return false;
    switch (req->op) {
        case RFID_OP_ENROLL:
            return rfid_enroll(&req->data.enroll.uid);
        case RFID_OP_DELETE:
            return rfid_delete(req->data.del.index);
        default:
            return false;
    }
}
uint8_t rfid_manager_count(void){return (uint8_t)uid_count;}