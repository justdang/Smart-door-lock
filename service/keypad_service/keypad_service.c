#include <stdint.h>
#include <stdbool.h>
#include "keypad_service.h"
#include "keypad_driver.h"
#include "system_hardware.h"
#include <string.h>
#include "auth.h"

#define KEYPAD_BUFFER_SIZE 6
static char keypad_buffer[KEYPAD_BUFFER_SIZE];
static uint8_t buffer_count = 0;

void keypad_init(void){
    memset(keypad_buffer, 0, sizeof(keypad_buffer));
    buffer_count = 0;
}

char keypad_getPressedKey(void){
    return (Hardware.Keypad->GetPressedKey());
}

void keypad_process(void){}

void keypad_resetBuffer(void){
    memset(keypad_buffer, 0, sizeof(keypad_buffer));
    buffer_count = 0;
}

bool keypad_addKey(void){
    char key = keypad_getPressedKey();
    if(key != '\0' && buffer_count < KEYPAD_BUFFER_SIZE - 1){ //Buffer từ 0 -> 5, là số ký tự nhập vào
        keypad_buffer[buffer_count++] = key;
        return true;
    }
    return false;
}

bool keypad_bufferFull(void){
    return buffer_count >= KEYPAD_BUFFER_SIZE - 1;
}

char* keypad_getBuffer(void){
    return keypad_buffer;
}

