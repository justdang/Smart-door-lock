#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "display_service.h"
#include "display_driver.h"

static const Display_Driver_t *display = &ESP32_Display_Driver;

void display_Init(void){
    if (display && display->Init) display->Init();
}
void display_clear(void){
    if (display && display->Clear) display->Clear();
}

void display_print(uint8_t x, uint8_t y, const char *text){
    if(display && display->PrintText) display->PrintText();
}

void display_idle(void){
    display_clear();
    display_print(0,0,"QUET THE HOAC NHAP MAT KHAU DE MO CUA");
}

void display_input(uint32_t length){
     char masked[17];
    if (length > 20) length = 20;
    memset(masked, '*', length);
    masked[length] = '\0';
    display_clear();
    display_print(0,0,"NHAP MAT KHAU:");
}

void display_verifying(void){
    display_clear();
    display_print(0,0,"DANG XAC THUC...");
}

void display_granted(void){
    display_clear();
    display_print(0,0,"DA MO CUA!");
    display_print(0,0,"CHAO MUNG:")
}

void display_deny(void){
    display_clear();
    display_print(0,0,"MO CUA KHONG THANH CONG, VUI LONG THU LAI!");
}

void display_error(void){
    display_clear();
    display_print(0,0,"LOI");
}

