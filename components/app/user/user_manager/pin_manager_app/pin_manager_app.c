#include "pin_manager.h"
#include "storage.h"
#include <string.h>

#define PIN_MIN_LEN   4
#define PIN_MAX_LEN   6

static bool is_valid_pin(const char *pin)
{
    if (pin == NULL) return false;
    size_t len = strlen(pin);
    if (len < PIN_MIN_LEN || len > PIN_MAX_LEN) return false;
    /* only digits allowed */
    for (size_t i = 0; i < len; i++) {
        if (pin[i] < '0' || pin[i] > '9') return false;
    }
    return true;
}

static bool pin_manager_enroll(const char *pin)
{
    if (!is_valid_pin(pin)) {return false;}
    if (storage_getPasswordCount() >= MAX_PASSWORDS) {return false;} //storage full
    return storage_savePassword(pin);
}

static bool pin_manager_delete(uint8_t index)
{
    if (index >= MAX_PASSWORDS) {return false;}
    if (storage_getPasswordCount() == 0) {return false;} //khong co gi de xoa
    return storage_deletePassword(index);
}
bool pin_manager_execute(const PinRequest *req)
{
    if (req == NULL) return false;
    switch (req->op) {
        case PIN_OP_ENROLL:
            return pin_manager_enroll(req->data.enroll.pin);
        case PIN_OP_DELETE:
            return pin_manager_delete(req->data.del.index);
        default:
            return false;
    }
}
uint8_t pin_manager_count(void) {return storage_getPasswordCount();}