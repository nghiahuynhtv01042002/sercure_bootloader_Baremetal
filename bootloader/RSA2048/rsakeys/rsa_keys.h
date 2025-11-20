#ifndef RSA_KEYS_H
#define RSA_KEYS_H

#include <stdint.h>

#define RSA_KEY_SIZE 256
#define SIGNATURE_SIZE 256

extern const uint8_t rsa_modulus[256];
extern const uint32_t rsa_exponent;
extern const uint8_t firmware_signature[256];

#endif // RSA_KEYS_H
