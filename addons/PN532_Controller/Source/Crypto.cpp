// #include "aes.h"
#include <cstring>

// namespace crypto
// {
//     void aes128CbcEncrypt(const uint8_t *key, const uint8_t *iv,
//                           const uint8_t *in, size_t len, uint8_t *out)
//     {
//         AES_ctx ctx;
//         uint8_t buffer[32]; // or allocate len bytes; CBC expects 16-byte blocks
//         memcpy(out, in, len);
//         AES_init_ctx_iv(&ctx, key, iv);
//         AES_CBC_encrypt_buffer(&ctx, out, static_cast<uint32_t>(len));
//     }

//     void aes128CbcDecrypt(const uint8_t *key, const uint8_t *iv,
//                           const uint8_t *in, size_t len, uint8_t *out)
//     {
//         AES_ctx ctx;
//         memcpy(out, in, len);
//         AES_init_ctx_iv(&ctx, key, iv);
//         AES_CBC_decrypt_buffer(&ctx, out, static_cast<uint32_t>(len));
//     }
// }
