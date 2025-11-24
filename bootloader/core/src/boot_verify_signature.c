#include "boot_verify_signature.h"
#include "rsa2048.h"
rsa_verify_result_t verify_firmware(uint32_t flash_addr, uint32_t fw_size) {
    const uint8_t *fw_ptr = (const uint8_t *)(uintptr_t)flash_addr;
    const uint8_t *sig_ptr = (const uint8_t *)(uintptr_t)(flash_addr + fw_size);

    return rsa_verify_signature(
        fw_ptr, (size_t)fw_size,
        sig_ptr, (size_t)SIGNATURE_SIZE,
        rsa_modulus, RSA_KEY_SIZE,
        rsa_exponent
    );
}