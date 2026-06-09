#ifndef UI_SCREEN
#define UI_SCREEN_H
#include <stdint.h>
#include <stdbool.h>

typedef enum{
    SCREEN_LOCKED,
    SCREEN_AUTH,
    SCREEN_VERIFYING,
    SCREEN_DENIED,
    SCREEN_UNLOCKED,
    SCREEN_LOCKED_OUT,
    SCREEN_ADMIN,
    SCREEN_ENTER_NEW_PIN,
    SCREEN_CONFIRM_NEW_PIN,
    SCREEN_SUCCESS_PIN,
    SCREEN_FAIL_PIN,//trùng, không hợp lệ, quá số ký tự...
    SCREEN_MISMATCH_PIN,
    SCREEN_SCAN_CARD,
    SCREEN_UID_DUPLICATE,
    SCREEN_SUCCESS_UID,
    SCREEN_FAIL_UID,
    SCREEN_SCREEN_ERROR
}ScreenID;

void ui_screen_init(void);
void ui_screen_show(ScreenID screen);
void ui_screen_update_pin_mask(uint8_t pin_len);
void ui_screen_update_lockout_timer(uint32_t seconds_remaining);
void ui_screen_update_autolock_timer(uint32_t seconds_remaining);
void ui_screen_clear(void);
#endif //UI_SCREEN_H