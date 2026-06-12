#include "access_control.h"
#include <stdint.h>
bool unlock_role(UserRole role){
    return (role == NORMAL_USER || role == ADMIN);
}
bool enroll_delete_changePIN_role(UserRole role){
    return (role == ADMIN);
}
bool factory_reset_role(UserRole role){
    return (role == ADMIN);
}