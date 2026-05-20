#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "rfid_struct.h"

#define MAX_PASSWORD_LENGTH        20
#define MAX_ADMIN_PASSWORD_LENGTH  16
#define MAX_RFID_CARDS             20

static char g_password[MAX_PASSWORD_LENGTH + 1];
static char g_admin_password[MAX_ADMIN_PASSWORD_LENGTH + 1];

static RFID_UID_t g_rfid_list[MAX_RFID_CARDS];
static uint8_t g_rfid_count = 0;

static bool auth_compare_uid(const RFID_UID_t *uid1, const RFID_UID_t *uid2)
{
    if ((uid1 == NULL) || (uid2 == NULL)) return false;
    return (memcmp(uid1, uid2, sizeof(RFID_UID_t)) == 0);
}

void auth_init(void){
    
}