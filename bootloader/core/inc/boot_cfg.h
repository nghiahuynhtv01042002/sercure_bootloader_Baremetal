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
} boot_handle_t;
int8_t boot_set_securemode(boot_handle_t *boot_ctx, secure_mode_t mode);
int8_t boot_set_comm_type(boot_handle_t *boot_ctx, comm_type_t type);
int8_t boot_config(boot_handle_t* boot_ctx);
int8_t boot_init(boot_handle_t* boot_ctx);
#endif // BOOT_CFG_H