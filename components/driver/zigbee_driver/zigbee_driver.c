// zigbee_driver.c
// Middle layer between service layer and low-level UART.
// Handles: frame building, AES-128 CBC encryption/decryption,
//          dispatching received commands to the service layer via callback.

#include <string.h>             // memcpy, memset
#include "esp_log.h"            // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "zigbee_driver.h"      // own header — types and API
#include "zigbee_frame.h"       // zigbee_frame_pack, zigbee_frame_unpack
#include "zigbee_crypto.h"      // zigbee_crypto_encrypt, zigbee_crypto_decrypt
#include "zigbee_uart.h"        // zigbee_uart_send — low level TX

#define TAG "ZIGBEE_DRIVER"

// ============================================================
//  Internal state
// ============================================================

// Tracks whether zigbee_driver_init() has been called.
// All public functions check this before doing anything.
static bool _initialized = false;

// Pointer to the service layer callback function.
// Set by zigbee_driver_register_rx_callback().
// Called every time a valid decrypted frame arrives.
static zigbee_rx_callback_t _rx_callback = NULL;

// ============================================================
//  Internal helper — encrypt, pack, and send a frame
// ============================================================

// This function is the core TX path.
// Every send_xxx function calls this after building their payload.
//
// Steps:
//   1. Encrypt payload with AES-128 CBC → produces IV + ciphertext
//   2. Build encrypted_payload = [IV (16B)] + [ciphertext (N B)]
//   3. Pack into raw ZNP frame bytes via zigbee_frame_pack()
//   4. Send raw bytes to CC2530 via zigbee_uart_send()
static zigbee_err_t _encrypt_and_send(zigbee_cmd_t    cmd,
                                       const uint8_t*  plaintext,
                                       size_t          plaintext_len) {

    // ── Step 1: Encrypt payload ──────────────────────────────
    uint8_t out_iv[AES128_IV_SIZE]                      = {0};
    uint8_t out_ciphertext[CRYPTO_MAX_CIPHERTEXT_SIZE]  = {0};
    size_t  encrypted_len = 0;

    crypto_err_t cerr = zigbee_crypto_encrypt(plaintext,
                                               plaintext_len,
                                               out_iv,
                                               out_ciphertext,
                                               &encrypted_len);
    if (cerr != CRYPTO_OK) {
        ESP_LOGE(TAG, "_encrypt_and_send: encrypt failed (%d)", cerr);
        return ZIGBEE_ERR_ENCRYPT;
    }

    // ── Step 2: Build encrypted_payload = IV + ciphertext ───
    // The frame payload field carries both the IV and the ciphertext.
    // Receiver knows: first 16 bytes = IV, rest = ciphertext.
    zigbee_frame_t frame;
    memset(&frame, 0, sizeof(zigbee_frame_t));

    frame.cmd = cmd;

    // Copy IV into payload starting at byte 0
    memcpy(&frame.payload[0], out_iv, AES128_IV_SIZE);

    // Copy ciphertext immediately after IV
    memcpy(&frame.payload[AES128_IV_SIZE], out_ciphertext, encrypted_len);

    // Total payload length = IV size + ciphertext length
    frame.payload_len = (uint8_t)(AES128_IV_SIZE + encrypted_len);

    // ── Step 3: Pack frame into raw bytes ────────────────────
    uint8_t raw_buf[ZIGBEE_FRAME_MAX_SIZE] = {0};
    size_t  raw_len = 0;

    frame_err_t ferr = zigbee_frame_pack(&frame, raw_buf, &raw_len);
    if (ferr != FRAME_OK) {
        ESP_LOGE(TAG, "_encrypt_and_send: frame pack failed (%d)", ferr);
        return ZIGBEE_ERR_FRAME_PACK;
    }

    // ── Step 4: Send raw bytes over UART to CC2530 ───────────
    // zigbee_uart_send() writes raw_buf to the UART peripheral.
    // CC2530 picks it up and forwards it over the Zigbee RF network.
    bool sent = zigbee_uart_send(raw_buf, raw_len);
    if (!sent) {
        ESP_LOGE(TAG, "_encrypt_and_send: UART send failed");
        return ZIGBEE_ERR_SEND;
    }

    ESP_LOGD(TAG, "Sent cmd=0x%04X plaintext=%d encrypted=%d total=%d",
             cmd, plaintext_len, encrypted_len, raw_len);

    return ZIGBEE_OK;
}

// ============================================================
//  Init
// ============================================================

zigbee_err_t zigbee_driver_init(const uint8_t aes_key[ZIGBEE_AES_KEY_SIZE]) {

    // Validate key pointer — never dereference NULL
    if (aes_key == NULL) {
        ESP_LOGE(TAG, "init: aes_key is NULL");
        return ZIGBEE_ERR_PARAM;
    }

    // Initialize crypto module with the shared AES key.
    // From this point, zigbee_crypto_encrypt/decrypt will use this key.
    crypto_err_t cerr = zigbee_crypto_init(aes_key);
    if (cerr != CRYPTO_OK) {
        ESP_LOGE(TAG, "init: crypto init failed (%d)", cerr);
        return ZIGBEE_ERR_NOT_INIT;
    }

    _initialized = true;
    ESP_LOGI(TAG, "Zigbee driver initialized.");
    return ZIGBEE_OK;
}

void zigbee_driver_register_rx_callback(zigbee_rx_callback_t callback) {
    // Store the service layer function pointer.
    // When a frame arrives, _rx_callback(frame) delivers it upward.
    // Setting NULL disables callbacks — driver will still receive but not forward.
    _rx_callback = callback;
    ESP_LOGI(TAG, "RX callback registered.");
}

// ============================================================
//  TX — Send functions
// ============================================================

zigbee_err_t zigbee_driver_send_lock_state(zigbee_lock_state_t state) {

    // Guard: must call init first
    if (!_initialized) {
        ESP_LOGE(TAG, "send_lock_state: not initialized");
        return ZIGBEE_ERR_NOT_INIT;
    }

    // Build plaintext payload.
    // Lock state fits in 1 byte: 0x00 = LOCKED, 0x01 = UNLOCKED.
    uint8_t plaintext[1];
    plaintext[0] = (uint8_t)state;

    // Delegate to _encrypt_and_send with the correct command ID.
    return _encrypt_and_send(ZIGBEE_CMD_LOCK_STATE_REPORT,
                              plaintext,
                              sizeof(plaintext));
}

zigbee_err_t zigbee_driver_send_door_state(zigbee_door_state_t state) {

    if (!_initialized) {
        ESP_LOGE(TAG, "send_door_state: not initialized");
        return ZIGBEE_ERR_NOT_INIT;
    }

    // Door state fits in 1 byte: 0x00 = CLOSED, 0x01 = OPEN.
    uint8_t plaintext[1];
    plaintext[0] = (uint8_t)state;

    return _encrypt_and_send(ZIGBEE_CMD_DOOR_STATE_REPORT,
                              plaintext,
                              sizeof(plaintext));
}

zigbee_err_t zigbee_driver_send_battery_status(const zigbee_battery_t* battery) {

    if (!_initialized) {
        ESP_LOGE(TAG, "send_battery_status: not initialized");
        return ZIGBEE_ERR_NOT_INIT;
    }

    // Validate pointer — battery struct must not be NULL
    if (battery == NULL) {
        ESP_LOGE(TAG, "send_battery_status: NULL param");
        return ZIGBEE_ERR_PARAM;
    }

    // Battery payload: 2 bytes
    //   [0] = percentage (0–100)
    //   [1] = is_low flag (0 or 1)
    uint8_t plaintext[2];
    plaintext[0] = battery->percentage;
    plaintext[1] = battery->is_low ? 0x01 : 0x00;

    return _encrypt_and_send(ZIGBEE_CMD_BATTERY_REPORT,
                              plaintext,
                              sizeof(plaintext));
}

zigbee_err_t zigbee_driver_send_password_changed(zigbee_password_type_t type) {

    if (!_initialized) {
        ESP_LOGE(TAG, "send_password_changed: not initialized");
        return ZIGBEE_ERR_NOT_INIT;
    }

    // Notify hub that a password changed.
    // We send only the TYPE (user or admin), NEVER the actual password value.
    // 1 byte: 0x01 = user password, 0x02 = admin password
    uint8_t plaintext[1];
    plaintext[0] = (uint8_t)type;

    return _encrypt_and_send(ZIGBEE_CMD_PASSWORD_CHANGED,
                              plaintext,
                              sizeof(plaintext));
}

zigbee_err_t zigbee_driver_send_ack(zigbee_cmd_t acked_cmd) {

    if (!_initialized) {
        ESP_LOGE(TAG, "send_ack: not initialized");
        return ZIGBEE_ERR_NOT_INIT;
    }

    // ACK payload carries the command ID being acknowledged.
    // 2 bytes: high byte + low byte of acked_cmd.
    // This lets the hub match the ACK to the command it sent.
    uint8_t plaintext[2];
    plaintext[0] = (uint8_t)((acked_cmd >> 8) & 0xFF); // high byte
    plaintext[1] = (uint8_t)( acked_cmd       & 0xFF); // low byte

    return _encrypt_and_send(ZIGBEE_CMD_ACK,
                              plaintext,
                              sizeof(plaintext));
}

// ============================================================
//  RX — Receive handler (called by zigbee_uart.c)
// ============================================================

void zigbee_driver_on_uart_receive(const uint8_t* buf, size_t len) {

    // This function is called from the UART RX task (low level layer)
    // every time a complete frame is detected in the byte stream.

    // ── Guard checks ─────────────────────────────────────────
    if (!_initialized) {
        ESP_LOGW(TAG, "on_uart_receive: not initialized, discarding");
        return;
    }
    if (buf == NULL || len == 0) {
        ESP_LOGW(TAG, "on_uart_receive: NULL or empty buffer");
        return;
    }

    // ── Step 1: Unpack raw bytes into a frame struct ─────────
    // zigbee_frame_unpack() checks SOF (0xFE) and verifies FCS.
    // If either fails, the frame is silently discarded — RF noise
    // or a misaligned UART stream produced invalid data.
    zigbee_frame_t encrypted_frame;
    frame_err_t ferr = zigbee_frame_unpack(buf, len, &encrypted_frame);
    if (ferr != FRAME_OK) {
        ESP_LOGW(TAG, "on_uart_receive: frame unpack failed (%d), discarding", ferr);
        return;
    }

    // ── Step 2: Split payload into IV and ciphertext ─────────
    // The TX side packed: payload = [IV (16B)] + [ciphertext (N B)]
    // We reverse that here.
    //
    // Minimum payload length check: must have at least IV + 1 block (16B)
    if (encrypted_frame.payload_len <= AES128_IV_SIZE) {
        ESP_LOGW(TAG, "on_uart_receive: payload too short to contain IV");
        return;
    }

    // IV = first 16 bytes of payload
    const uint8_t* iv         = &encrypted_frame.payload[0];

    // Ciphertext = everything after the IV
    const uint8_t* ciphertext = &encrypted_frame.payload[AES128_IV_SIZE];
    size_t cipher_len = encrypted_frame.payload_len - AES128_IV_SIZE;

    // ── Step 3: Decrypt ciphertext ───────────────────────────
    // zigbee_crypto_decrypt() uses the stored AES key and the IV
    // from the frame to recover the original plaintext.
    // If the wrong key is used, PKCS7 strip will fail → CRYPTO_ERR_PAD.
    uint8_t plaintext[CRYPTO_MAX_PLAINTEXT_SIZE] = {0};
    size_t  plaintext_len = 0;

    crypto_err_t cerr = zigbee_crypto_decrypt(iv,
                                               ciphertext,
                                               cipher_len,
                                               plaintext,
                                               &plaintext_len);
    if (cerr != CRYPTO_OK) {
        // Wrong key, corrupted ciphertext, or replay — discard silently.
        ESP_LOGW(TAG, "on_uart_receive: decrypt failed (%d), discarding", cerr);
        return;
    }

    // ── Step 4: Rebuild frame with decrypted plaintext ───────
    // Replace the encrypted payload with the decrypted plaintext
    // so the service layer receives clean, readable data.
    zigbee_frame_t decrypted_frame;
    memset(&decrypted_frame, 0, sizeof(zigbee_frame_t));

    decrypted_frame.cmd         = encrypted_frame.cmd;
    decrypted_frame.payload_len = (uint8_t)plaintext_len;
    memcpy(decrypted_frame.payload, plaintext, plaintext_len);

    // ── Step 5: Send ACK back to hub ─────────────────────────
    // Acknowledge every successfully received Hub→Door command.
    // The hub uses this to confirm delivery before marking the command done.
    // Only ACK commands that come from the hub (0x02xx range).
    if ((decrypted_frame.cmd & 0xFF00) == 0x0200) {
        zigbee_err_t ack_err = zigbee_driver_send_ack(decrypted_frame.cmd);
        if (ack_err != ZIGBEE_OK) {
            ESP_LOGW(TAG, "on_uart_receive: failed to send ACK (%d)", ack_err);
            // Non-fatal — continue processing even if ACK fails
        }
    }

    // ── Step 6: Deliver to service layer via callback ────────
    // If the service layer registered a callback, call it now
    // with the fully decrypted, parsed frame.
    // The service layer switch-cases on frame.cmd to decide what to do.
    if (_rx_callback != NULL) {
        _rx_callback(&decrypted_frame);
    } else {
        ESP_LOGW(TAG, "on_uart_receive: no callback registered, frame dropped");
    }

    ESP_LOGD(TAG, "RX processed: cmd=0x%04X plaintext_len=%d",
             decrypted_frame.cmd, decrypted_frame.payload_len);
}