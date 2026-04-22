#ifndef RFID_STRUCT_H
#define RFID_STRUCT_H
#include <stdint.h>
#include <stdbool.h>
#define RFID_MAX_LENGTH  10

typedef struct{
    uint8_t uid[RFID_MAX_LENGTH];
    uint8_t length;
} RFID_UID_t;

#endif