#include <string.h>
#include "mbedtls/aes.h" //tìm hiểu 
#include "esp_random.h"
#include "esp_log.h"
#include "zigbee_crypto.h"

#define TAG "ZIGBEE_CRYPTO"
static uint8_t _aes_key[AES128_KEY_SIZE];
static bool    _initialized = false;

/**
 * @brief Apply PKCS7 padding to buf in-place.
 *
 * Example: data_len = 4 ("LOCK")
 *   pad_val = 16 - (4 % 16) = 12 = 0x0C
 *   buf becomes: [L O C K 0C 0C 0C 0C 0C 0C 0C 0C 0C 0C 0C 0C]
 *   padded_len = 4 + 12 = 16
 */

 //hàm nội bộ 
 //Thực hiện padding PKCS7.
static void _pkcs7_pad(uint8_t* buf, size_t data_len, size_t* padded_len) {
    // pad_val is always 1–16, never 0
    uint8_t pad_val = (uint8_t)(AES128_BLOCK_SIZE - (data_len % AES128_BLOCK_SIZE));
    for (uint8_t i = 0; i < pad_val; i++) {
        buf[data_len + i] = pad_val;
    }
    *padded_len = data_len + pad_val;
}

/**
 * @brief Strip PKCS7 padding from decrypted buf in-place.
 *
 * Example: buf ends with [...0C 0C 0C 0C 0C 0C 0C 0C 0C 0C 0C 0C]
 *   last byte = 0x0C = 12
 *   verify last 12 bytes all equal 0x0C
 *   out_len = buf_len - 12
 *
 * @return CRYPTO_ERR_PAD if padding bytes are invalid.
 */
static crypto_err_t _pkcs7_strip(uint8_t* buf, size_t buf_len, size_t* out_len) {
    if (buf == NULL || buf_len == 0 || buf_len % AES128_BLOCK_SIZE != 0) {
        return CRYPTO_ERR_PAD;
    } // không tồn tại buf

    uint8_t pad_val = buf[buf_len - 1];

    // pad_val must be between 1 and AES128_BLOCK_SIZE (1–16)
    if (pad_val == 0 || pad_val > AES128_BLOCK_SIZE) {
        ESP_LOGW(TAG, "PKCS7 strip: invalid pad value %d", pad_val);
        return CRYPTO_ERR_PAD;
    }
    // Verify every padding byte equals pad_val
    for (uint8_t i = 1; i <= pad_val; i++) {
        if (buf[buf_len - i] != pad_val) {
            ESP_LOGW(TAG, "PKCS7 strip: padding byte mismatch at offset %d", i);
            return CRYPTO_ERR_PAD;
        }
    }

    *out_len = buf_len - pad_val;
    return CRYPTO_OK;
}

//  Public API
crypto_err_t zigbee_crypto_init(const uint8_t key[AES128_KEY_SIZE]) {
    if (key == NULL) {
        ESP_LOGE(TAG, "init: key is NULL");
        return CRYPTO_ERR_PARAM;
    }
    memcpy(_aes_key, key, AES128_KEY_SIZE);
    _initialized = true;
    ESP_LOGI(TAG, "Crypto initialized.");
    return CRYPTO_OK;
}

crypto_err_t zigbee_crypto_encrypt(const uint8_t* plaintext, size_t plaintext_len, uint8_t out_iv[AES128_IV_SIZE], uint8_t* out_ciphertext, size_t* out_len) {
    if (!_initialized) {
        ESP_LOGE(TAG, "encrypt: not initialized");
        return CRYPTO_ERR_NOT_INIT;
    } // chưa khởi tạo
    if (plaintext == NULL || out_iv == NULL ||
        out_ciphertext == NULL || out_len == NULL) {
        ESP_LOGE(TAG, "encrypt: NULL param");
        return CRYPTO_ERR_PARAM;
    } // tham số null
    if (plaintext_len == 0 || plaintext_len > CRYPTO_MAX_PLAINTEXT_SIZE) {
        ESP_LOGE(TAG, "encrypt: invalid plaintext_len %d", plaintext_len);
        return CRYPTO_ERR_PARAM;
    } //lỗi plaintext_len

    esp_fill_random(out_iv, AES128_IV_SIZE);

    uint8_t padded_buf[CRYPTO_MAX_CIPHERTEXT_SIZE] = {0};
    memcpy(padded_buf, plaintext, plaintext_len);//(dest, src, size)

    size_t padded_len = 0;
    _pkcs7_pad(padded_buf, plaintext_len, &padded_len); 

    uint8_t iv_copy[AES128_IV_SIZE];
    memcpy(iv_copy, out_iv, AES128_IV_SIZE);

    mbedtls_aes_context ctx; //
    mbedtls_aes_init(&ctx);

    int ret = mbedtls_aes_setkey_enc(&ctx, _aes_key, AES128_KEY_SIZE * 8);
    if (ret != 0) {
        ESP_LOGE(TAG, "encrypt: setkey_enc failed (%d)", ret);
        mbedtls_aes_free(&ctx);
        return CRYPTO_ERR_ENCRYPT;
    }

    ret = mbedtls_aes_crypt_cbc(&ctx,
                                  MBEDTLS_AES_ENCRYPT,
                                  padded_len,       // must be multiple of 16
                                  iv_copy,          // local copy — gets consumed
                                  padded_buf,       // input
                                  out_ciphertext);  // output
    mbedtls_aes_free(&ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "encrypt: crypt_cbc failed (%d)", ret);
        return CRYPTO_ERR_ENCRYPT;
    }

    *out_len = padded_len;
    ESP_LOGD(TAG, "Encrypted %d → %d bytes", plaintext_len, padded_len);
    return CRYPTO_OK;
}

crypto_err_t zigbee_crypto_decrypt(const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len, uint8_t* out_plaintext, size_t* out_len) {
    if (!_initialized) {
        ESP_LOGE(TAG, "decrypt: not initialized");
        return CRYPTO_ERR_NOT_INIT;
    } //chưa khởi tạo
    if (iv == NULL || ciphertext == NULL ||
        out_plaintext == NULL || out_len == NULL) {
        ESP_LOGE(TAG, "decrypt: NULL param");
        return CRYPTO_ERR_PARAM;
    } // tham số null
    // ciphertext length must always be a multiple of 16
    if (ciphertext_len == 0 || ciphertext_len % AES128_BLOCK_SIZE != 0) {
        ESP_LOGE(TAG, "decrypt: ciphertext_len %d is not multiple of 16",
                 ciphertext_len);
        return CRYPTO_ERR_PARAM;
    } //không tồn tại, độ dài không hợp lệ

    uint8_t iv_copy[AES128_IV_SIZE];
    memcpy(iv_copy, iv, AES128_IV_SIZE);

    mbedtls_aes_context ctx; //một struct để lưu trạng thái AES
    mbedtls_aes_init(&ctx); //khởi tạo ctx

    int ret = mbedtls_aes_setkey_dec(&ctx, _aes_key, AES128_KEY_SIZE * 8); //thiết lập khóa
    if (ret != 0) {
        ESP_LOGE(TAG, "decrypt: setkey_dec failed (%d)", ret);
        mbedtls_aes_free(&ctx);
        return CRYPTO_ERR_DECRYPT;
    }

    ret = mbedtls_aes_crypt_cbc(&ctx,
                                  MBEDTLS_AES_DECRYPT,
                                  ciphertext_len,   // must be multiple of 16
                                  iv_copy,          // local copy — gets consumed
                                  ciphertext,       // input
                                  out_plaintext);   // output
    mbedtls_aes_free(&ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "decrypt: crypt_cbc failed (%d)", ret);
        return CRYPTO_ERR_DECRYPT;
    }

    // ── 4. Strip PKCS7 padding ───────────────────────────────
    crypto_err_t pad_err = _pkcs7_strip(out_plaintext, ciphertext_len, out_len);
    if (pad_err != CRYPTO_OK) {
        ESP_LOGE(TAG, "decrypt: PKCS7 strip failed — wrong key or corrupted frame");
        return pad_err;
    }

    ESP_LOGD(TAG, "Decrypted %d → %d bytes", ciphertext_len, *out_len);
    return CRYPTO_OK;
}