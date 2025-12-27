#ifndef BOOT_FW_UPDATE_H
#define BOOT_FW_UPDATE_H
#include <stdint.h>
#include "boot_cfg.h"

// boot finish signal
#define BOOT_FINISH_SIGNAL   0x88
// Start sequence
#define START_CMD       0x55
#define START_ACK       0xAA

// Flash erase
#define ERASE_CMD       0xEC
#define ERASE_ACK       0x13

// Firmware chunk
#define CHUNK_SIZE      256
#define FW_SIZE_ACK     0xAA
#define CHUNK_ACK       0xCC

// Signature
#define SIGNATURE_CMD   0x53
#define SIGNATURE_ACK   0xAC

// Update firmware commands
#define UPDATE_FW_CMD1  'y'
#define UPDATE_FW_CMD2  'Y'
#define FORCE_UPDATE_FW_CMD1 'f'
#define FORCE_UPDATE_FW_CMD2 'F'
#define SKIP_FW_UPDATE_CMD1  's'
#define SKIP_FW_UPDATE_CMD2  'S'

// ACK for all firmware update commands
#define UPDATE_FW_ACK   0x8F
#define SKIP_FW_UPDATE_ACK UPDATE_FW_ACK

typedef enum {
    FW_OK = 0,
    FW_ERR_COMM,
    FW_ERR_TIMEOUT_CMD,
    FW_ERR_FLASH_ERASE,
    FW_ERR_FLASH_WRITE,
    FW_ERR_INVALID_CMD,
    FW_ERR_COPY_FW,
    FW_ERR_INVALID_SIGNATURE, 
} fw_status_t;

typedef void(*func_ptr)(void);

fw_status_t receive_new_firmware(boot_handle_t *ctx, uint32_t flash_addr, uint32_t* fw_size);
fw_status_t receive_fw_update_request(boot_handle_t *boot_ctx);
fw_status_t handle_update_request(boot_handle_t *boot_ctx, uint32_t *fw_addr, uint32_t* fw_size );
fw_status_t process_boot_state(boot_handle_t *boot_ctx, uint32_t *fw_addr, uint32_t *fw_size);
void send_message(boot_handle_t *ctx, const char *msg);
void enter_app(boot_handle_t *boot_ctx, uint32_t app_addr);
#endif // BOOT_FW_UPDATE_H