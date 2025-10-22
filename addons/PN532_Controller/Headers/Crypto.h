#pragma once

#include "aes.h"
#include <cstring>

namespace crypto
{   
    /**
     * @brief AES-128 CBC encryption
     * 
     * @param key The encryption key (16 bytes)
     * @param iv The initialization vector (16 bytes)
     * @param in The input data
     * @param len The length of the input data
     * @param out The output buffer for the encrypted data
     */
    void aes128CbcEncrypt(const uint8_t *key, const uint8_t *iv,
                          const uint8_t *in, size_t len, uint8_t *out);

    /**
     * @brief AES-128 CBC decryption
     * 
     * @param key The decryption key (16 bytes)
     * @param iv The initialization vector (16 bytes)
     * @param in The input data
     * @param len The length of the input data
     * @param out The output buffer for the decrypted data
     */
    void aes128CbcDecrypt(const uint8_t *key, const uint8_t *iv,
                          const uint8_t *in, size_t len, uint8_t *out);
}
