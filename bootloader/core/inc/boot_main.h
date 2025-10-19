#ifndef BOOT_MAIN_H
#define BOOT_MAIN_H
#include <stdint.h>
#include <stdio.h>
#include "sysclocks.h"
#include "gpio.h"
#include "nvic.h"
#include "uart.h"
#include "sysinit.h"

typedef enum {
    SECURE_NONE = 0,
    SECURE_RSA2048,
    SECURE_ECDSA
} secure_mode_t;

typedef enum {
    COMM_UART = 0,
    COMM_SPI,
    COMM_USB
} comm_mode_t;

typedef struct {
    void *comm_cfg;
    void (*init)(void *cfg);
    uint16_t (*recv)(void *cfg, uint8_t *data, uint16_t len);
    void (*send)(void *cfg, const uint8_t *data, uint16_t len);
} comm_interface_t;

typedef struct {
    secure_mode_t secure_mode;
    comm_interface_t *comm_if; // abstract interface (UART/SPI/USB)
} boot_mode_config_t;

#endif // BOOT_MAIN_H