#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H
#include <stdint.h>

void display_Init(void);
void display_clear(void);
void display_print(uint8_t row, uint8_t col, const char *text);
void display_idle(void);
void display_input(void);
void display_verifying(void);
void display_granted(void); //unlock
void display_deny(void);
void display_locking(void);
void display_error(void);
#endif