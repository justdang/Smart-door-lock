#ifndef ZIGBEE_DRIVER_H
#define ZIGBEE_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "zigbee_frame.h"   // zigbee_frame_t, zigbee_cmd_t
#include "zigbee_crypto.h"  // AES128_KEY_SIZE

#define ZIGBEE_AES_KEY_SIZE         AES128_KEY_SIZE
#define ZIGBEE_ACK_TIMEOUT_MS       2000
// Số lượt retry tối đa khi gửi một lệnh mà không nhận được ACK từ hub.
#define ZIGBEE_MAX_RETRY            3

typedef enum {
    ZIGBEE_LOCK_STATE_LOCKED   = 0x00,
    ZIGBEE_LOCK_STATE_UNLOCKED = 0x01,
} zigbee_lock_state_t;
//trạng thái đóng mở cửa
typedef enum {
    ZIGBEE_DOOR_STATE_CLOSED = 0x00,
    ZIGBEE_DOOR_STATE_OPEN   = 0x01,
} zigbee_door_state_t;
//trạng thái pin
// typedef struct {
//     uint8_t percentage;  // 0–100
//     bool    is_low;      // true if below warning threshold
// } zigbee_battery_t;
//phân biệt mật khẩu admin và user
typedef enum {
    ZIGBEE_PASSWORD_USER  = 0x01,
    ZIGBEE_PASSWORD_ADMIN = 0x02,
} zigbee_password_type_t;
//mã lỗi
typedef enum {
    ZIGBEE_OK                =  0,
    ZIGBEE_ERR_NOT_INIT      = -1,  // zigbee_driver_init() not called
    ZIGBEE_ERR_PARAM         = -2,  // NULL or invalid argument
    ZIGBEE_ERR_ENCRYPT       = -3,  // AES encryption failed
    ZIGBEE_ERR_DECRYPT       = -4,  // AES decryption failed
    ZIGBEE_ERR_FRAME_PACK    = -5,  // frame packing failed
    ZIGBEE_ERR_FRAME_UNPACK  = -6,  // frame unpacking or FCS failed
    ZIGBEE_ERR_SEND          = -7,  // UART send failed
    ZIGBEE_ERR_ACK_TIMEOUT   = -8,  // no ACK received within timeout
} zigbee_err_t;

// ============================================================
//  RX Callback
//  Registered by the service layer.
//  Called by the driver whenever a valid, decrypted frame arrives.
// ============================================================

// The service layer defines a function matching this signature
// and registers it via zigbee_driver_register_rx_callback().
// The driver calls it automatically on every valid received frame.
typedef void (*zigbee_rx_callback_t)(const zigbee_frame_t* frame);
/**
 * @brief Initialize the Zigbee driver.
 *        Must be called once at startup before any other function.
 *        Internally calls zigbee_crypto_init() with the provided key.
 *
 * @param aes_key   16-byte AES-128 key shared between door and hub.
 * @return ZIGBEE_OK on success.
 *
 * How it works:
 *   1. Validates aes_key is not NULL.
 *   2. Calls zigbee_crypto_init(aes_key) to store the key in crypto module.
 *   3. Sets internal _initialized flag to true.
 */
zigbee_err_t zigbee_driver_init(const uint8_t aes_key[ZIGBEE_AES_KEY_SIZE]);
/**
 * @brief Register the RX callback function.
 *        The service layer calls this once at startup to tell the driver
 *        where to deliver received frames.
 *
 * @param callback  Function pointer to the service layer handler.
 *
 * How it works:
 *   Stores the callback pointer internally.
 *   When the driver receives and validates a frame, it calls callback(frame).
 */
void zigbee_driver_register_rx_callback(zigbee_rx_callback_t callback);

// ============================================================
//  TX — Send functions
//  Each function:
//    1. Builds a zigbee_frame_t with the appropriate cmd and payload
//    2. Encrypts the payload with AES-128 CBC via zigbee_crypto
//    3. Packs the encrypted frame into raw bytes via zigbee_frame_pack
//    4. Sends the raw bytes over UART via zigbee_uart
// ============================================================
/**
 * @brief Send lock state to the hub.
 *        Called by service layer after lock/unlock action completes.
 *
 * @param state   ZIGBEE_LOCK_STATE_LOCKED or ZIGBEE_LOCK_STATE_UNLOCKED.
 * @return ZIGBEE_OK on success.
 *
 * Payload layout (before encryption):
 *   [0] = state (1 byte)
 */
zigbee_err_t zigbee_driver_send_lock_state(zigbee_lock_state_t state);
/**
 * @brief Send door open/closed state to the hub.
 *        Called when the door sensor changes state.
 *
 * @param state   ZIGBEE_DOOR_STATE_OPEN or ZIGBEE_DOOR_STATE_CLOSED.
 * @return ZIGBEE_OK on success.
 *
 * Payload layout (before encryption):
 *   [0] = state (1 byte)
 */
zigbee_err_t zigbee_driver_send_door_state(zigbee_door_state_t state);
// zigbee_err_t zigbee_driver_send_battery_status(const zigbee_battery_t* battery);

/**
 * @brief Notify the hub that a password was changed.
 *        Only sends the type (user/admin), NOT the actual password value.
 *
 * @param type   ZIGBEE_PASSWORD_USER or ZIGBEE_PASSWORD_ADMIN.
 * @return ZIGBEE_OK on success.
 *
 * Payload layout (before encryption):
 *   [0] = type (1 byte)
 */
zigbee_err_t zigbee_driver_send_password_changed(zigbee_password_type_t type);

/**
 * @brief Send an ACK to the hub confirming a command was received.
 *        Called internally after successfully processing any Hub→Door command.
 *
 * @param acked_cmd   The command ID being acknowledged.
 * @return ZIGBEE_OK on success.
 *
 * Payload layout (before encryption):
 *   [0] = acked_cmd high byte
 *   [1] = acked_cmd low byte
 */
zigbee_err_t zigbee_driver_send_ack(zigbee_cmd_t acked_cmd);

// ============================================================
//  RX — Receive handler
//  Called by the UART low-level layer when bytes arrive.
//  Not called directly by service layer.
// ============================================================

/**
 * @brief Process raw bytes received from UART (from CC2530).
 *        Called by zigbee_uart.c when a complete frame arrives.
 *
 * @param buf     Raw byte buffer received over UART.
 * @param len     Number of bytes in buf.
 *
 * How it works:
 *   1. zigbee_frame_unpack()  → parse + verify FCS
 *   2. Split payload into IV (first 16 bytes) + ciphertext (rest)
 *   3. zigbee_crypto_decrypt() → decrypt payload
 *   4. Rebuild frame with decrypted payload
 *   5. zigbee_driver_send_ack() → send ACK back to hub
 *   6. Call registered rx_callback(frame) → delivers to service layer
 */
void zigbee_driver_on_uart_receive(const uint8_t* buf, size_t len);

#endif /* ZIGBEE_DRIVER_H */