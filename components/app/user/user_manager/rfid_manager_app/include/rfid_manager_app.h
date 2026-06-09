#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "rfid_struct.h"

typedef enum {
    RFID_OP_ENROLL,
    RFID_OP_DELETE,
} RfidOperation;
typedef struct {
    RfidOperation op;
    union {
        struct {
            RFID_UID_t uid;     // for ENROLL
        } enroll;
        struct {
            uint8_t index;      // for DELETE
        } del;
    } data;
} RfidRequest;
bool    rfid_manager_execute(const RfidRequest *req);
uint8_t rfid_manager_count(void);
#endif // RFID_MANAGER_H