#ifndef SERVO_SERVICE_H
#define SERVO_SERVICE_H
#include <stdint.h>

void servo_init(void);
void servo_close(void);
void servo_open(void);
void servo_autolock(void); // tu dong khoa sau 1 khoang thoi gian

#endif