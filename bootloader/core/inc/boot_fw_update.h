#ifndef BOOT_FW_UPDATE_H
#define BOOT_FW_UPDATE_H
#include <stdint.h>
#include "boot_cfg.h"

#define START_CMD (0x55)
#define START_ACK (0xAA)
#define SIZE_ACK  (0xAA)
#define ERASE_CMD (0xEC)
#define ERASE_ACK (0xAB)
#define CHUNK_ACK (0xCC)
#define CHUNK_SIZE (256)

typedef enum {
    FW_OK = 0,
    FW_ERR_COMM,
    FW_ERR_FLASH_ERASE,
    FW_ERR_FLASH_WRITE,
    FW_ERR_INVALID_CMD,
} fw_status_t;
fw_status_t receive_new_firmware(boot_handle_t *ctx, uint32_t flash_addr);

#endif // BOOT_FW_UPDATE_H