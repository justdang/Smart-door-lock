#ifndef AES_SERVICE_H
#define AES_SERVICE_H
#include <stdint.h>
#include <stddef.h>
#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16
void aes_encrypt(
    const uint8_t *input,
    uint8_t *output,
    size_t length,
    const uint8_t *key,
    const uint8_t *iv
);

void aes_decrypt(
    const uint8_t *input,
    uint8_t *output,
    size_t length,
    const uint8_t *key,
    const uint8_t *iv
);
#endif