#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PIN_OP_ENROLL,
    PIN_OP_DELETE,
} PinOperation;

typedef struct {
    PinOperation op;
    union {
        struct {
            char pin[7];        // for ENROLL
        } enroll;
        struct {
            uint8_t index;      // for DELETE
        } del;
    } data;
} PinRequest;

//them mot ham nhu khoi tao de chon giua viec enroll hay delete
bool    pin_manager_execute(const PinRequest *req);
uint8_t pin_manager_count(void);

#endif