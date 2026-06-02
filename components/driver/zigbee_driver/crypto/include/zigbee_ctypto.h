// Constants
#define AES128_KEY_SIZE   16
#define AES128_BLOCK_SIZE 16
#define AES128_IV_SIZE    16

#include <stdint.h>         
#include <stddef.h>         
#include <stdbool.h>
#include "zigbee_crypto.h"
#include "mbedtls/aes.h"    
#include "esp_random.h"     
#include <string.h>         
#include "esp_log.h"        

// Error code
typedef enum {
    CRYPTO_OK            =  0,
    CRYPTO_ERR_PARAM     = -1,
    CRYPTO_ERR_ENCRYPT   = -2,
    CRYPTO_ERR_DECRYPT   = -3,
    CRYPTO_ERR_PAD       = -4,  // invalid padding on decrypt
} crypto_err_t;

// API — only 3 functions needed
crypto_err_t zigbee_crypto_init(const uint8_t key[AES128_KEY_SIZE]);
crypto_err_t zigbee_crypto_encrypt(const uint8_t* plaintext, size_t plaintext_len, uint8_t* out_iv, uint8_t* out_ciphertext, size_t* out_len);
crypto_err_t zigbee_crypto_decrypt(const uint8_t* iv, const uint8_t* ciphertext, size_t ciphertext_len, uint8_t* out_plaintext, size_t* out_len);

//các tham số cho các hàm
// key
// plaintext: dữ liệu gốc cần mã hóa
// plaintext_len: độ dài của dữ liệu gốc
// out_ciphertext: buffer để lưu trữ dữ liệu đã mã hóa
// out_len: độ dài của dữ liệu đã mã hóa (được trả về sau khi mã hóa xong)
// iv: vector khởi tạo dùng cho mã hóa CBC (initialization vector)