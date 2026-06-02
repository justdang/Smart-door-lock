#include <string.h>         // memcpy, memset
#include "esp_log.h"        // ESP_LOGI, ESP_LOGW, ESP_LOGE
#include "zigbee_frame.h"

// TAG is used by ESP_LOG macros to label log output in the console
// e.g. "I (123) ZIGBEE_FRAME: ..."
#define TAG "ZIGBEE_FRAME"

uint8_t zigbee_frame_calc_fcs(const uint8_t* buf, size_t len) {
    // FCS = XOR of every byte from 'length' through end of payload.
    // XOR is simple but effective: any single flipped bit changes the result.
    // This is the exact algorithm from the ZNP specification.
    uint8_t fcs = 0;               // start with 0 — XOR identity element
    for (size_t i = 0; i < len; i++) fcs ^= buf[i];             // XOR each byte into the accumulator
    return fcs;                    // final XOR value is the checksum
}

bool zigbee_frame_is_valid_cmd(zigbee_cmd_t cmd) {
    // Check if cmd matches any known command ID defined in zigbee_frame.h.
    // This prevents the driver from acting on garbage command IDs
    // that might appear if a corrupted frame slips past the FCS check.

    switch (cmd) {
        case ZIGBEE_CMD_LOCK_STATE_REPORT:
        case ZIGBEE_CMD_DOOR_STATE_REPORT:
        case ZIGBEE_CMD_BATTERY_REPORT:
        case ZIGBEE_CMD_PASSWORD_CHANGED:
        case ZIGBEE_CMD_ACK:
        case ZIGBEE_CMD_LOCK:
        case ZIGBEE_CMD_UNLOCK:
        case ZIGBEE_CMD_SET_USER_PASSWORD:
        case ZIGBEE_CMD_SET_ADMIN_PASSWORD:
        case ZIGBEE_CMD_REQUEST_STATUS:
            return true;           // recognized command

        default:
            return false;          // unknown command — reject
    }
}

// ============================================================
//  Pack — build a raw byte buffer from a zigbee_frame_t
// ============================================================

frame_err_t zigbee_frame_pack(const zigbee_frame_t* frame,
                               uint8_t* out_buf,
                               size_t*  out_len) {

    if (frame == NULL || out_buf == NULL || out_len == NULL) {
        ESP_LOGE(TAG, "pack: NULL param");
        return FRAME_ERR_PARAM;
    } //null ptr

    if (frame->payload_len > ZIGBEE_FRAME_MAX_PAYLOAD) {
        ESP_LOGE(TAG, "pack: payload_len %d exceeds max %d",
                 frame->payload_len, ZIGBEE_FRAME_MAX_PAYLOAD);
        return FRAME_ERR_OVERFLOW;
    }// báo lỗi tràn payload ->length quá lớn

    if (!zigbee_frame_is_valid_cmd(frame->cmd)) {
        ESP_LOGE(TAG, "pack: unknown cmd 0x%04X", frame->cmd);
        return FRAME_ERR_UNKNOWN_CMD;
    }// báo lỗi cmd không hợp lệ

    // SOF is always the first byte. Fixed value 0xFE per ZNP spec.
    // The receiver scans for this byte to find the start of a frame.
    uint8_t idx = 0;               // idx tracks our current write position
    out_buf[idx++] = ZIGBEE_FRAME_SOF;     // out_buf[0] = 0xFE

    // ── 5. Write LENGTH ──────────────────────────────────────
    // LENGTH = number of bytes in the payload field only.
    // Does NOT include SOF, LENGTH itself, CMD, or FCS.
    out_buf[idx++] = frame->payload_len;   // out_buf[1] = payload_len

    // ── 6. Write CMD (2 bytes, big-endian) ───────────────────
    // Big-endian: high byte first, then low byte.
    // e.g. cmd = 0x0211 → out_buf[2] = 0x02, out_buf[3] = 0x11
    out_buf[idx++] = (uint8_t)((frame->cmd >> 8) & 0xFF); // high byte
    out_buf[idx++] = (uint8_t)( frame->cmd       & 0xFF); // low byte

    // ── 7. Write PAYLOAD ─────────────────────────────────────
    // Copy the payload bytes (IV + ciphertext from zigbee_crypto)
    // into the frame buffer starting at current idx position.
    if (frame->payload_len > 0) {
        memcpy(&out_buf[idx], frame->payload, frame->payload_len);
        idx += frame->payload_len;         // advance idx past the payload
    }

    // ── 8. Calculate and write FCS ───────────────────────────
    // FCS is calculated over: LENGTH + CMD_HI + CMD_LO + PAYLOAD
    // That starts at out_buf[1] (skipping SOF at [0])
    // and covers (1 + 2 + payload_len) bytes total.
    //
    // fcs_start points to the LENGTH byte in out_buf.
    // fcs_len   = 1 (LENGTH) + 2 (CMD) + payload_len
    const uint8_t* fcs_start = &out_buf[1];
    size_t         fcs_len   = 1 + 2 + frame->payload_len;

    out_buf[idx++] = zigbee_frame_calc_fcs(fcs_start, fcs_len);

    // ── 9. Report total bytes written ────────────────────────
    *out_len = idx;  // total = 1(SOF) + 1(LEN) + 2(CMD) + payload_len + 1(FCS)

    ESP_LOGD(TAG, "Packed frame: cmd=0x%04X payload_len=%d total=%d",
             frame->cmd, frame->payload_len, idx);

    return FRAME_OK;
}

// ============================================================
//  Unpack — parse a raw byte buffer into a zigbee_frame_t
// ============================================================

frame_err_t zigbee_frame_unpack(const uint8_t*  buf,
                                 size_t          buf_len,
                                 zigbee_frame_t* out_frame) {

    // ── 1. Validate input pointers ───────────────────────────
    if (buf == NULL || out_frame == NULL) {
        ESP_LOGE(TAG, "unpack: NULL param");
        return FRAME_ERR_PARAM;
    }

    // ── 2. Minimum frame size check ──────────────────────────
    // A frame with zero payload bytes still needs:
    // SOF(1) + LEN(1) + CMD(2) + FCS(1) = 5 bytes minimum
    if (buf_len < 5) {
        ESP_LOGW(TAG, "unpack: buf_len %d too small", buf_len);
        return FRAME_ERR_PARAM;
    }

    // ── 3. Check SOF ─────────────────────────────────────────
    // First byte must be 0xFE. If not, the buffer is misaligned —
    // the UART task should resync by scanning for the next 0xFE.
    if (buf[0] != ZIGBEE_FRAME_SOF) {
        ESP_LOGW(TAG, "unpack: SOF mismatch, got 0x%02X expected 0x%02X",
                 buf[0], ZIGBEE_FRAME_SOF);
        return FRAME_ERR_NO_SOF;
    }

    // ── 4. Read LENGTH ───────────────────────────────────────
    // buf[1] = number of payload bytes that follow the CMD field.
    uint8_t payload_len = buf[1];

    // Sanity check — payload_len must not exceed our buffer limit
    if (payload_len > ZIGBEE_FRAME_MAX_PAYLOAD) {
        ESP_LOGW(TAG, "unpack: payload_len %d exceeds max", payload_len);
        return FRAME_ERR_OVERFLOW;
    }

    // ── 5. Check total frame length ──────────────────────────
    // Expected total = 1(SOF) + 1(LEN) + 2(CMD) + payload_len + 1(FCS)
    size_t expected_len = 1 + 1 + 2 + payload_len + 1;
    if (buf_len < expected_len) {
        ESP_LOGW(TAG, "unpack: buf_len %d < expected %d", buf_len, expected_len);
        return FRAME_ERR_PARAM;
    }

    // ── 6. Verify FCS ────────────────────────────────────────
    // Recalculate FCS over the received bytes and compare to the
    // FCS byte appended at the end of the frame.
    //
    // FCS covers: LENGTH + CMD_HI + CMD_LO + PAYLOAD
    // That is buf[1] through buf[1 + 2 + payload_len] (exclusive)
    // fcs_len = 1(LEN) + 2(CMD) + payload_len
    size_t  fcs_len      = 1 + 2 + payload_len;
    uint8_t calc_fcs     = zigbee_frame_calc_fcs(&buf[1], fcs_len);
    uint8_t received_fcs = buf[1 + fcs_len];  // last byte of frame

    if (calc_fcs != received_fcs) {
        // Mismatch means data was corrupted in transit (RF noise, UART error)
        ESP_LOGW(TAG, "unpack: FCS mismatch calc=0x%02X recv=0x%02X",
                 calc_fcs, received_fcs);
        return FRAME_ERR_FCS;
    }

    // ── 7. Parse CMD (2 bytes, big-endian) ───────────────────
    // buf[2] = high byte, buf[3] = low byte
    // Shift high byte left by 8 bits and OR with low byte to reconstruct uint16
    uint16_t raw_cmd = ((uint16_t)buf[2] << 8) | (uint16_t)buf[3];

    // Cast to zigbee_cmd_t and validate it is a known command
    zigbee_cmd_t cmd = (zigbee_cmd_t)raw_cmd;
    if (!zigbee_frame_is_valid_cmd(cmd)) {
        ESP_LOGW(TAG, "unpack: unknown cmd 0x%04X", raw_cmd);
        return FRAME_ERR_UNKNOWN_CMD;
    }

    // ── 8. Fill output frame struct ──────────────────────────
    // Zero out the struct first to avoid leftover garbage in payload[]
    memset(out_frame, 0, sizeof(zigbee_frame_t));

    out_frame->cmd         = cmd;
    out_frame->payload_len = payload_len;

    // Copy payload bytes from buf[4] onward into out_frame->payload
    // buf[4] is the first payload byte (after SOF, LEN, CMD_HI, CMD_LO)
    if (payload_len > 0) {
        memcpy(out_frame->payload, &buf[4], payload_len);
    }

    ESP_LOGD(TAG, "Unpacked frame: cmd=0x%04X payload_len=%d",
             out_frame->cmd, out_frame->payload_len);

    return FRAME_OK;
}