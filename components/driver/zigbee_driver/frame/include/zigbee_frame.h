#ifndef ZIGBEE_FRAME_H
#define ZIGBEE_FRAME_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
// SOF (Start of Frame)
#define ZIGBEE_FRAME_SOF            0xFE
// số payload max trong 1 frame
#define ZIGBEE_FRAME_MAX_PAYLOAD    64
// SOF(1) + LEN(1) + CMD(2) + PAYLOAD(64) + FCS(1) = 69 bytes  
#define ZIGBEE_FRAME_MAX_SIZE       (1 + 1 + 2 + ZIGBEE_FRAME_MAX_PAYLOAD + 1)
typedef enum {
    // Door → Hub
    ZIGBEE_CMD_LOCK_STATE_REPORT    = 0x0101, // report current lock state
    ZIGBEE_CMD_DOOR_STATE_REPORT    = 0x0102, // report door open/closed
    //ZIGBEE_CMD_BATTERY_REPORT       = 0x0103, // report battery percentage
    ZIGBEE_CMD_PASSWORD_CHANGED     = 0x0104, // notify password was changed
    ZIGBEE_CMD_ACK                  = 0x0105, // acknowledge a received command

    // Hub → Door
    ZIGBEE_CMD_LOCK                 = 0x0210, // command: lock the door
    ZIGBEE_CMD_UNLOCK               = 0x0211, // command: unlock the door
    ZIGBEE_CMD_SET_USER_PASSWORD    = 0x0212, // command: set new user password
    ZIGBEE_CMD_SET_ADMIN_PASSWORD   = 0x0213, // command: set new admin password
    ZIGBEE_CMD_REQUEST_STATUS       = 0x0214, // command: request all status reports
} zigbee_cmd_t;
// __attribute__((packed)) tells the compiler:
// do NOT add any padding bytes between fields.
// Without this, the compiler might insert padding for alignment,
// making sizeof(zigbee_raw_frame_t) larger than expected.

typedef struct __attribute__((packed)) {
    uint8_t  sof;                               // always 0xFE
    uint8_t  length;                            // number of bytes in payload[]
    uint16_t cmd;                               // command ID (big-endian on wire)
    uint8_t  payload[ZIGBEE_FRAME_MAX_PAYLOAD]; // encrypted data (IV + ciphertext)
    uint8_t  fcs;                               // XOR checksum of length+cmd+payload
} zigbee_raw_frame_t;

// ============================================================
//  Parsed frame struct — what the driver works with internally
// ============================================================
typedef struct {
    zigbee_cmd_t cmd;                           // parsed command ID
    uint8_t      payload[ZIGBEE_FRAME_MAX_PAYLOAD]; // payload bytes
    uint8_t      payload_len;                   // actual number of payload bytes
} zigbee_frame_t;

// ============================================================
//  Error codes
// ============================================================
typedef enum {
    FRAME_OK                =  0,
    FRAME_ERR_PARAM         = -1,  // NULL pointer or invalid argument
    FRAME_ERR_NO_SOF        = -2,  // SOF byte 0xFE not found
    FRAME_ERR_FCS           = -3,  // FCS checksum mismatch — frame corrupted
    FRAME_ERR_OVERFLOW      = -4,  // payload too large for buffer
    FRAME_ERR_UNKNOWN_CMD   = -5,  // command ID not recognized
} frame_err_t;

// ============================================================
//  Public API
// ============================================================

/**
 * @brief Pack a zigbee_frame_t into a raw byte buffer ready to send over UART.
 *        Fills SOF, length, cmd, payload, and calculates FCS automatically.
 *
 * @param frame       Input frame with cmd, payload, payload_len filled.
 * @param out_buf     Output buffer to write raw bytes into.
 * @param out_len     Number of bytes written into out_buf.
 * @return FRAME_OK on success.
 */
frame_err_t zigbee_frame_pack(const zigbee_frame_t* frame,
                               uint8_t* out_buf,
                               size_t*  out_len);

/**
 * @brief Parse a raw byte buffer received from UART into a zigbee_frame_t.
 *        Verifies SOF and FCS before returning.
 *
 * @param buf         Raw bytes received from UART.
 * @param buf_len     Number of bytes in buf.
 * @param out_frame   Output parsed frame.
 * @return FRAME_OK on success, FRAME_ERR_FCS if checksum fails.
 */
frame_err_t zigbee_frame_unpack(const uint8_t*  buf,
                                 size_t          buf_len,
                                 zigbee_frame_t* out_frame);

/**
 * @brief Calculate FCS (XOR checksum) over length + cmd + payload bytes.
 *        FCS = length XOR cmd_hi XOR cmd_lo XOR payload[0] XOR ... XOR payload[n]
 *
 * @param buf     Pointer to first byte to include in FCS (the length byte).
 * @param len     Number of bytes to XOR together.
 * @return Calculated FCS byte.
 */
uint8_t zigbee_frame_calc_fcs(const uint8_t* buf, size_t len);

/**
 * @brief Check whether a command ID is valid / known.
 *
 * @param cmd   Command ID to check.
 * @return true if recognized, false otherwise.
 */
bool zigbee_frame_is_valid_cmd(zigbee_cmd_t cmd);

#endif /* ZIGBEE_FRAME_H */