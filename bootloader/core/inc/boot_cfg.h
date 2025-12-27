#ifndef BOOT_CFG_H
#define BOOT_CFG_H
#include <stdint.h>
#include <stdio.h>
#include "sysclocks.h"
#include "gpio.h"
#include "nvic.h"
#include "uart.h"
#include "sysinit.h"
#include "tim2.h"

#define FW_FLASH_ADDR   (0x08020000UL) // (bank1 use for app)
#define FW_STAGING_ADDR (0x08010000UL) // (bank2 use for staging)
#define METADATA_ADDR   (0x08004000UL)
#define MAX_FW_SIZE     (128 * 1024) // 128KB
#define MAX_SIG_SIZE    (16 * 1024) // 16KB

#define APP_SECTION_NUMBER (1)
#define APP_BACKUP_SECTION_NUMBER (4)

#define APP_MSP         (*(volatile uint32_t *)(FW_FLASH_ADDR + 0x0))
#define APP_ENTRY       (*(volatile uint32_t *)(FW_FLASH_ADDR + 0x04))

typedef enum {
    SECURE_NONE = 0,
    SECURE_RSA2048,
    SECURE_ECDSA
} secure_mode_t;

typedef enum {
    COMM_UART = 0,
    COMM_USB,
    COMM_SPI,
    COMM_I2C
} comm_type_t;

typedef enum {
    BOOT_STATE_IDLE = 0,
    BOOT_STATE_LOAD_METADATA,
    BOOT_STATE_VERIFY_SIGNATURE,
    BOOT_STATE_VERIFY_HASH,
    BOOT_STATE_SWAP,
    BOOT_STATE_JUMP_TO_APP,
    BOOT_STATE_ERROR,
    BOOT_STATE_NORMAL_UPDATE,
    BOOT_STATE_FORCE_UPDATE,
    BOOT_STATE_RECEIVING_FW,
    BOOT_STATE_WRITE_FW,
} boot_state_t;

typedef struct {
    void *comm_cfg;
    void (*init)(void *cfg);
    uint16_t (*recv)(void *cfg, uint8_t *data, uint16_t len);
    void (*send)(void *cfg, const uint8_t *data, uint16_t len);
} comm_interface_t;

typedef struct {
    secure_mode_t secure_mode;
    comm_type_t  comm_type;
    comm_interface_t *comm_if; // abstract interface (UART/SPI/USB)
    boot_state_t state;
} boot_handle_t;

int8_t boot_set_state(boot_handle_t* boot_ctx, boot_state_t state);
int8_t boot_set_securemode(boot_handle_t *boot_ctx, secure_mode_t mode);
int8_t boot_set_comm_type(boot_handle_t *boot_ctx, comm_type_t type);
int8_t boot_config(boot_handle_t* boot_ctx);
int8_t boot_init(boot_handle_t* boot_ctx);
int8_t boot_deinit(boot_handle_t* boot_ctx);

#endif // BOOT_CFG_H