#ifndef LOCK_CONTROLLER
#define LOCK_CONTROLLER
#include <stdint.h>
#include <stdbool.h>
#include "rfid_service.h"
#include "keypad_service.h"
#include "zigbee_service.h"

typedef enum{
    LOCK_STATE_LOCKED,
    LOCK_STATE_UNLOCKED,
    LOCK_STATE_ERROR,
} LockState;

void lock_controller_init(void);
void lock_controller_lock(void);
void lock_controller_unlock(void);
LockState lock_controller_get_state(void);
bool lock_controller_is_locked(void);

#endif //LOCK_CONTROLLER