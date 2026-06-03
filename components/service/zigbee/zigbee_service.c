#include <stdint.h>
#include <stdbool.h>
#include <string.h> 
#include "zigbee_service.h"                
#include "esp_log.h"                
#include "freertos/FreeRTOS.h"      
#include "freertos/semphr.h" 
#include "zigbee_low_level.h" //có thể thay đổi theo Thành

#define TAG "ZIGBEE_SERVICE"
static bool _initialized = false;
static SemaphoreHandle_t _pair_semaphore = NULL;
static zb_pair_status_t _pair_result = ZB_PAIR_FAILED;
static SemaphoreHandle_t _online_semaphore = NULL;

zigbee_service_err_t zigbee_service_init(const uint8_t aes_key[ZIGBEE_AES_KEY_SIZE]){
    if (aes_key == NULL) {
        ESP_LOGE(TAG, "init: aes_key is NULL");
        return ZB_SERVICE_ERR_PARAM;
    }
    // ── Step 1: Initialize low level UART ────────────────────
    // Must be done before driver init because driver may send
    // bytes during initialization.
    uart_zigbee_err_t uerr = zigbee_uart_init();
    if (uerr != UART_ZIGBEE_OK) {
        ESP_LOGE(TAG, "init: uart init failed (%d)", uerr);
        return ZB_SERVICE_ERR_DRIVER;
    }
    // ── Step 2: Initialize driver with AES key ────────────────
    zigbee_err_t derr = zigbee_driver_init(aes_key);
    if (derr != ZIGBEE_OK) {
        ESP_LOGE(TAG, "init: driver init failed (%d)", derr);
        return ZB_SERVICE_ERR_DRIVER;
    }
    // ── Step 3: Register this file's RX callback ─────────────
    // From now on, every valid frame from CC2530 is delivered
    // to service_zigbee_on_receive() automatically.
    zigbee_driver_register_rx_callback(service_zigbee_on_receive);
    // ── Step 4: Create semaphores ─────────────────────────────
    // Binary semaphore: starts empty (0).
    // service_zigbee_pair() takes it (blocks).
    // service_zigbee_on_receive() gives it (unblocks pair).
    _pair_semaphore   = xSemaphoreCreateBinary();
    _online_semaphore = xSemaphoreCreateBinary();
    if (_pair_semaphore == NULL || _online_semaphore == NULL) {
        ESP_LOGE(TAG, "init: semaphore creation failed");
        return ZB_SERVICE_ERR_DRIVER;
    }
    _initialized = true;
    ESP_LOGI(TAG, "Zigbee service initialized.");
    return ZB_SERVICE_OK;
}

zigbee_service_err_t zigbee_service_pair(uint32_t timeout_ms, zigbee_pair_status_t* out_status){
     if (!_initialized) {
        ESP_LOGE(TAG, "pair: not initialized");
        return ZB_SERVICE_ERR_NOT_INIT;
    }
    if (out_status == NULL) {
        ESP_LOGE(TAG, "pair: NULL out_status");
        return ZB_SERVICE_ERR_PARAM;
    }
    // ── Step 1: Reset pair result before starting ─────────────
    _pair_result = ZB_PAIR_FAILED;
    // ── Step 2: Tell CC2530 to open pairing window ────────────
    // Convert timeout_ms to seconds for CC2530 duration field.
    // CC2530 will broadcast association permit for this duration.
    uint8_t duration_s = (uint8_t)(timeout_ms / 1000);
    if (duration_s == 0) duration_s = 30; // minimum 30 seconds
    zigbee_err_t derr = zigbee_driver_start_pairing(duration_s);
    if (derr != ZIGBEE_OK) {
        ESP_LOGE(TAG, "pair: driver start_pairing failed (%d)", derr);
        *out_status = ZB_PAIR_FAILED;
        return ZB_SERVICE_ERR_DRIVER;
    }
    ESP_LOGI(TAG, "Pairing window open for %d seconds...", duration_s);
    // ── Step 3: Block and wait for CC2530 result ──────────────
    // _pair_semaphore is released by service_zigbee_on_receive()
    // when ZIGBEE_CMD_PAIR_SUCCESS or ZIGBEE_CMD_PAIR_FAILED arrives.
    // pdMS_TO_TICKS converts ms to FreeRTOS ticks.
    BaseType_t got = xSemaphoreTake(_pair_semaphore,
                                     pdMS_TO_TICKS(timeout_ms));
    if (got != pdTRUE) {
        // Semaphore was never released → no hub responded in time
        ESP_LOGW(TAG, "pair: timeout after %d ms", timeout_ms);
        *out_status = ZB_PAIR_TIMEOUT;
        return ZB_SERVICE_OK;    // attempt completed — just no hub found
    }
    // ── Step 4: Return result set by RX callback ──────────────
    *out_status = _pair_result;
    ESP_LOGI(TAG, "pair: result = %d", _pair_result);
    return ZB_SERVICE_OK;
}

zigbee_service_err_t zigbee_service_isConnected(bool* out_online){
    if (!_initialized) {
        ESP_LOGE(TAG, "check_online: not initialized");
        return ZB_SERVICE_ERR_NOT_INIT;
    }
    if (out_online == NULL) {
        ESP_LOGE(TAG, "check_online: NULL param");
        return ZB_SERVICE_ERR_PARAM;
    }
    // ── Step 1: Send ping (REQUEST_STATUS) to hub ─────────────
    zigbee_err_t derr = zigbee_driver_send_ping();
    if (derr != ZIGBEE_OK) {
        ESP_LOGW(TAG, "check_online: send ping failed (%d)", derr);
        *out_online = false;
        return ZB_SERVICE_ERR_DRIVER;
    }
    // ── Step 2: Wait for ACK from hub (max 2 seconds) ─────────
    // If hub is online, it replies with ZIGBEE_CMD_ACK.
    // service_zigbee_on_receive() will release _online_semaphore.
    BaseType_t got = xSemaphoreTake(_online_semaphore,
                                     pdMS_TO_TICKS(2000));
    // ── Step 3: Determine online status ───────────────────────
    *out_online = (got == pdTRUE);  // true if semaphore released in time
    ESP_LOGI(TAG, "check_online: hub is %s",
             *out_online ? "ONLINE" : "OFFLINE");
    return ZB_SERVICE_OK;
}

zigbee_service_err_t zigbee_service_send_lock_state(zigbee_lock_state_t state){
     if (!_initialized) {
        ESP_LOGE(TAG, "send_state: not initialized");
        return ZB_SERVICE_ERR_NOT_INIT;
    }
    if (state == NULL) {
        ESP_LOGE(TAG, "send_state: NULL param");
        return ZB_SERVICE_ERR_PARAM;
    }
    // Send each state component as a separate encrypted frame.
    // Even if one fails, attempt to send the others.
    // ── Lock state ────────────────────────────────────────────
    zigbee_err_t err = zigbee_driver_send_lock_state(state->lock);
    if (err != ZIGBEE_OK) {
        ESP_LOGW(TAG, "send_state: lock state failed (%d)", err);
        return ZB_SERVICE_ERR_DRIVER;
    }
    // ── Door state ────────────────────────────────────────────
    err = zigbee_driver_send_door_state(state->door);
    if (err != ZIGBEE_OK) {
        ESP_LOGW(TAG, "send_state: door state failed (%d)", err);
        return ZB_SERVICE_ERR_DRIVER;
    }
    ESP_LOGI(TAG, "send_state: all states sent successfully.");
    return ZB_SERVICE_OK;
}

void service_zigbee_on_receive(const zigbee_frame_t* frame){
    if (frame == NULL) {
        ESP_LOGW(TAG, "on_receive: NULL frame");
        return;
    }
    ESP_LOGI(TAG, "on_receive: cmd=0x%04X", frame->cmd);
    switch (frame->cmd) {
        // ── Hub commands Hub→Door ─────────────────────────────
        case ZIGBEE_CMD_LOCK:
            // Hub wants to lock the door.
            // Delegate to main service layer which handles
            // motor control and storage update.
            ESP_LOGI(TAG, "CMD: LOCK");
            service_lock();
            // Report new state back to hub
            zigbee_driver_send_lock_state(ZIGBEE_LOCK_STATE_LOCKED);
            break;
        case ZIGBEE_CMD_UNLOCK:
            // Hub wants to unlock the door.
            ESP_LOGI(TAG, "CMD: UNLOCK");
            service_unlock();
            zigbee_driver_send_lock_state(ZIGBEE_LOCK_STATE_UNLOCKED);
            break;
        case ZIGBEE_CMD_SET_USER_PASSWORD:
            // Hub sends new user password.
            // payload[0..5] = 6 ASCII characters of new password.
            // We pass it as a string — storage expects null-terminated.
            ESP_LOGI(TAG, "CMD: SET_USER_PASSWORD");
            {
                // Copy payload to null-terminated buffer
                char new_pw[7] = {0};   // 6 chars + '\0'
                memcpy(new_pw, frame->payload,
                       frame->payload_len < 6 ? frame->payload_len : 6);
                // Admin resets user password (hub has admin authority)
                // Admin password is verified internally in service layer
                service_adminResetUserPassword("admin_pw_placeholder", new_pw);
                zigbee_driver_send_password_changed(ZIGBEE_PASSWORD_USER);
            }
            break;
 
        case ZIGBEE_CMD_SET_ADMIN_PASSWORD:
            // Hub sends new admin password.
            ESP_LOGI(TAG, "CMD: SET_ADMIN_PASSWORD");
            {
                char new_pw[7] = {0};
                memcpy(new_pw, frame->payload,
                       frame->payload_len < 6 ? frame->payload_len : 6);
                service_changeAdminPassword("old_admin_pw_placeholder", new_pw);
                zigbee_driver_send_password_changed(ZIGBEE_PASSWORD_ADMIN);
            }
            break;
 
        case ZIGBEE_CMD_REQUEST_STATUS:
            // Hub is asking for full device status report.
            // Build current state and send all three state frames.
            ESP_LOGI(TAG, "CMD: REQUEST_STATUS");
            {
                zb_device_state_t current_state = {
                    .lock    = service_get_lock_state(),
                    .door    = service_get_door_state(),
                    .battery = service_get_battery_status(),
                };
                service_zigbee_send_state(&current_state);
            }
            break;
 
        // ── Pairing results (CC2530 → driver → here) ──────────
 
        case ZIGBEE_CMD_PAIR_SUCCESS:
            // CC2530 reports hub joined successfully.
            // Release the semaphore so service_zigbee_pair() can return.
            ESP_LOGI(TAG, "CMD: PAIR_SUCCESS");
            _pair_result = ZB_PAIR_SUCCESS;
            xSemaphoreGive(_pair_semaphore);
            break;
 
        case ZIGBEE_CMD_PAIR_FAILED:
            // CC2530 reports pairing failed.
            ESP_LOGW(TAG, "CMD: PAIR_FAILED");
            _pair_result = ZB_PAIR_FAILED;
            xSemaphoreGive(_pair_semaphore);
            break;
 
        // ── ACK from hub (response to our ping) ───────────────
 
        case ZIGBEE_CMD_ACK:
            // Hub acknowledged something we sent.
            // Release online semaphore so check_online() can return true.
            ESP_LOGD(TAG, "CMD: ACK received");
            xSemaphoreGive(_online_semaphore);
            break;
 
        default:
            ESP_LOGW(TAG, "on_receive: unhandled cmd 0x%04X", frame->cmd);
            break;
    }
}




