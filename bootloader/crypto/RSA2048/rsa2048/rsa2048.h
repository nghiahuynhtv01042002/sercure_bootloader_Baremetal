#ifndef RSA_VERIFY_H
#define RSA_VERIFY_H

#include "bigint.h"
#include "sha256.h"
#include <stdint.h>
#include <stdbool.h>

// PKCS#1 v1.5 padding for SHA-256 (256 bytes for 2048-bit RSA)
#define RSA_PKCS1_SHA256_PREFIX_LEN 19
static const uint8_t RSA_PKCS1_SHA256_PREFIX[RSA_PKCS1_SHA256_PREFIX_LEN] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
    0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
    0x00, 0x04, 0x20
};

typedef enum {
    RSA_VERIFY_OK = 0,
    RSA_VERIFY_ERROR = -1,
    RSA_VERIFY_INVALID_SIGNATURE = -2,
    RSA_VERIFY_PADDING_ERROR = -3
} rsa_verify_result_t;

/**
 * Verify RSA signature using PKCS#1 v1.5 padding with SHA-256
 * 
 * @param message: Message data to verify
 * @param message_len: Length of message
 * @param signature: RSA signature bytes (big-endian)
 * @param sig_len: Signature length (should be same as modulus size)
 * @param modulus: RSA public key modulus (big-endian bytes)
 * @param mod_len: Modulus length in bytes
 * @param exponent: RSA public exponent (typically 65537)
 * @return RSA_VERIFY_OK if signature is valid, error code otherwise
 */
rsa_verify_result_t rsa_verify_signature(
    const uint8_t *message, size_t message_len,
    const uint8_t *signature, size_t sig_len,
    const uint8_t *modulus, size_t mod_len,
    uint32_t exponent
);
rsa_verify_result_t verify_firmware(const uint8_t *firmware_data, size_t firmware_size);
#endif // RSA_VERIFY_H