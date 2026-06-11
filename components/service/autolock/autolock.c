#include "autolock.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"

#define TAG                 "AUTOLOCK"
#define AUTOLOCK_DELAY_MS   5000

static TimerHandle_t    s_timer      = NULL;
static AutolockCallback s_callback   = NULL;
static bool             s_running    = false;

static void autolock_timer_cb(TimerHandle_t xTimer){
    s_running = false;
    ESP_LOGI(TAG, "Autolock timer expired — posting timeout event");
    if (s_callback != NULL) {s_callback(APP_EVENT_AUTOLOCK_TIMEOUT);}
}
void autolock_init(AutolockCallback callback){
    if (callback == NULL) ESP_LOGE(TAG, "Callback is NULL — autolock will not post events");
    s_callback = callback;
    s_running  = false;
    s_timer = xTimerCreate(
        "autolock",                         // timer name for debug
        pdMS_TO_TICKS(AUTOLOCK_DELAY_MS),   // period
        pdFALSE,                            // one-shot — does not repeat
        NULL,                               // timer ID not used
        autolock_timer_cb                   // callback on expiry
    );
    if (s_timer == NULL) ESP_LOGE(TAG, "Failed to create autolock timer");
}
void autolock_start(void){
    if (s_timer == NULL) {
        ESP_LOGE(TAG, "Timer not initialized");
        return;
    }
    if (s_running){
        /* already running — restart the countdown from zero */
        xTimerReset(s_timer, 0);
        ESP_LOGI(TAG, "Autolock timer reset");
        return;
    }
    s_running = true;
    xTimerStart(s_timer, 0);
    ESP_LOGI(TAG, "Autolock timer started (%d ms)", AUTOLOCK_DELAY_MS);
}
void autolock_stop(void){
    if (s_timer == NULL) return;
    s_running = false;
    xTimerStop(s_timer, 0);
    ESP_LOGI(TAG, "Autolock timer stopped");
}
bool autolock_is_running(void) {return s_running;}