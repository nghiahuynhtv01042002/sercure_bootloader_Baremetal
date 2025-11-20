
#include <string.h>
#include "rsa2048.h"
#include <stdio.h>
#include "rsa_keys.h"
rsa_verify_result_t rsa_verify_signature(
    const uint8_t *message, size_t message_len,
    const uint8_t *signature, size_t sig_len,
    const uint8_t *modulus, size_t mod_len,
    uint32_t exponent
) {
    bigInt_t sig_bigint, mod_bigint, exp_bigint, result_bigint;
    bigIntStatus_t status;
    // Validate inputs
    if (!message || !signature || !modulus || 
        message_len == 0 || sig_len != mod_len) {
        return RSA_VERIFY_ERROR;
    }

    // Convert signature to bigint (big-endian)
    status = bigint_from_bytes(&sig_bigint, signature, sig_len);
    if (status != BIGINT_OK) return RSA_VERIFY_ERROR;
    
    // Convert modulus to bigint (big-endian) 
    status = bigint_from_bytes(&mod_bigint, modulus, mod_len);
    if (status != BIGINT_OK) return RSA_VERIFY_ERROR;
    
    // Convert exponent to bigint
    status = bigint_from_uint32(&exp_bigint, exponent);
    if (status != BIGINT_OK) return RSA_VERIFY_ERROR;
    
    // Perform RSA public key operation: signature^exponent mod modulus
    status = bigint_mod_exp(&result_bigint, &sig_bigint, &exp_bigint, &mod_bigint);

    if (status != BIGINT_OK) return RSA_VERIFY_ERROR;
    
    
    // Convert result back to bytes with FIXED LENGTH
    uint8_t decrypted[256];
    status = bigint_to_bytes(&result_bigint, decrypted, mod_len);
    if (status != BIGINT_OK) return RSA_VERIFY_ERROR;
    
    // Now decrypted has exactly mod_len bytes with leading zeros if needed
    // Verify PKCS#1 v1.5 padding for SHA-256
    // Check padding structure: 0x00 0x01 FF...FF 0x00 DigestInfo Hash
    if (decrypted[0] != 0x00 || decrypted[1] != 0x01) {
        return RSA_VERIFY_PADDING_ERROR;
    }
    
    // Find separator (0x00) after padding bytes (0xFF)
    size_t separator_pos = 0;
    for (size_t i = 2; i < mod_len; i++) {
        if (decrypted[i] == 0x00) {
            separator_pos = i;
            break;
        } else if (decrypted[i] != 0xFF) {
            return RSA_VERIFY_PADDING_ERROR;
        }
    }
    
    if (separator_pos == 0 || separator_pos < 10) { // Need at least 8 padding bytes
        return RSA_VERIFY_PADDING_ERROR;
    }
    
    // Verify DigestInfo structure
    size_t digest_info_pos = separator_pos + 1;
    size_t expected_digest_info_len = RSA_PKCS1_SHA256_PREFIX_LEN + SHA256_DIGEST_SIZE;
    
    if (digest_info_pos + expected_digest_info_len != mod_len) {
        return RSA_VERIFY_PADDING_ERROR;
    }
    // Check DigestInfo prefix
    if (memcmp(decrypted + digest_info_pos, RSA_PKCS1_SHA256_PREFIX, 
               RSA_PKCS1_SHA256_PREFIX_LEN) != 0) {
        return RSA_VERIFY_PADDING_ERROR;
    }
    
    // Extract hash from decrypted signature
    uint8_t *sig_hash = decrypted + digest_info_pos + RSA_PKCS1_SHA256_PREFIX_LEN;

    // Compute hash of message
    uint8_t message_hash[SHA256_DIGEST_SIZE];
    sha256_hash(message, message_len, message_hash);
    
    // Compare hashes
    if (memcmp(sig_hash, message_hash, SHA256_DIGEST_SIZE) == 0) {
        return RSA_VERIFY_OK;
    } else {
        return RSA_VERIFY_INVALID_SIGNATURE;
    }
}

rsa_verify_result_t verify_firmware(const uint8_t *firmware_data, size_t firmware_size) {
    return rsa_verify_signature(
        firmware_data, firmware_size,
        firmware_signature, SIGNATURE_SIZE,
        rsa_modulus, RSA_KEY_SIZE,
        rsa_exponent
    );
}