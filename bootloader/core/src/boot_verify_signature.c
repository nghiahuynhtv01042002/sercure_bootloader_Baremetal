#include "boot_verify_signature.h"
#include "rsa2048.h"
#include "boot_cfg.h"
#include "fw_metadata.h"
rsa_verify_result_t verify_firmware(uint32_t flash_addr, uint32_t fw_size) {
    if (flash_addr == 0 || fw_size == 0) {
        return RSA_VERIFY_INVALID_INPUT;
    }   
    const uint8_t *fw_ptr = (const uint8_t *)(uintptr_t)flash_addr;
    const uint8_t *sig_ptr = NULL;
    if( flash_addr != FW_STAGING_ADDR ) {
        // get signature location from metatdata
        sig_ptr = (const uint8_t *)(uintptr_t)read_sig_addr_from_flash();
    } else {
        // get signature attached after firmware in flash
        sig_ptr = (const uint8_t *)(uintptr_t)(flash_addr + fw_size);
    }

    return rsa_verify_signature(
        fw_ptr, (size_t)fw_size,
        sig_ptr, (size_t)SIGNATURE_SIZE,
        rsa_modulus, RSA_KEY_SIZE,
        rsa_exponent
    );
}