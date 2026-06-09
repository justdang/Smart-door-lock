#ifndef AUTH_SERVICE
#define AUTH_SERVICE
#include <stdint.h>
#include <stdbool.h>
// #include <string.h>
// #include "keypad_service.h"
// #include "zigbee_service.h"
// #include "rfid_service.h"
// #include "rfid_struct.h"
// #include "storage.h"

typedef enum{
    CREDENTIAL_TYPE_PIN,
    CREDENTIAL_TYPE_RFID
} CredentialType;

typedef struct{
    CredentialType type;
    union{
        struct {
            char digits[7]; //6 ky tu + ky tu /0
        } pin;
        struct {
            uint8_t uid[10];
            uint8_t uid_len;
        } rfid;
    } data;
} Credential;
typedef void (*AuthEventCallback)(AppEvent event);
void auth_service_init(void);
void auth_service_start(void);
void auth_service_cancel(void);
void auth_service_reset_attempts(void);
void auth_service_submit_credential(const Credential *credential);

#endif //AUTH_SERVICE