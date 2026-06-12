#ifndef AUTOLOCK_H
#define AUTOLOCK_H
#include <stdint.h>
#include <stdbool.h>
#include "system_app_event.h"
typedef void (*AutolockCallback)(AppEvent event);
void autolock_init(AutolockCallback callback);
void autolock_start(void);
void autolock_stop(void);
bool autolock_is_running(void);
#endif 
