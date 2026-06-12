#ifndef ACCESS_CONTROL
#define ACCESS_CONTROL
#include <stdint.h>
#include <stdbool.h>
typedef enum {
    NORMAL_USER,     
    ADMIN,        
} UserRole;
bool unlock_role(UserRole role);
bool enroll_delete_changePIN_role(UserRole role);
bool factory_reset_role(UserRole role);
#endif