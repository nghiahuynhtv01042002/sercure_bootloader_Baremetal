#ifndef BOOT_VERIFY_SIGNATURE_H
#define BOOT_VERIFY_SIGNATURE_H
#include <stdint.h>
#include "rsa2048.h"
#include "rsa_keys.h"
#include "sha256.h"

rsa_verify_result_t verify_firmware(uint32_t flash_addr, uint32_t fw_size);
#endif // BOOT_VERIFY_SIGNATURE_H