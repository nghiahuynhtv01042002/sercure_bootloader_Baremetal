#include "boot_cfg.h"
extern void uart_config(UART_Config_t *uart_cfg) ;

static UART_Config_t g_uart_cfg;
static const comm_interface_t uart_driver = {
    .comm_cfg = &g_uart_cfg,
    .init     = (void (*)(void *))UART_Init,
    .recv     = (uint16_t (*)(void *, uint8_t *, uint16_t))UART_ReceiveData,
    .send     = (void (*)(void *, const uint8_t *, uint16_t))UART_SendData
};


int8_t boot_set_state(boot_handle_t* boot_ctx, boot_state_t state) {
    if (!boot_ctx) return -1;
    boot_ctx->state = state;
    return 0;
}

static int8_t  boot_select_comm(boot_handle_t *boot_ctx, comm_type_t type) {
    if (!boot_ctx) return -1;
    switch (type) {
        case COMM_UART:
            uart_config(&g_uart_cfg);
            boot_ctx->comm_if = (comm_interface_t *)&uart_driver;
            break;
        case COMM_USB:
            // stil not implement
            return -1;
            break;
        case COMM_SPI:
            // stil not implement
            return -1;
            break;
        case COMM_I2C:
            // stil not implement
            return -1;
            break;
        default:
            return -1;
    }
    return 0;
}

int8_t boot_set_securemode(boot_handle_t *boot_ctx, secure_mode_t mode) {
    if (!boot_ctx) return -1;
    boot_ctx->secure_mode = mode;
    return 0;
}

int8_t boot_set_comm_type(boot_handle_t *boot_ctx, comm_type_t type) {
    if (!boot_ctx) return -1;
    boot_ctx->comm_type = type;
    return 0;
}

int8_t boot_config(boot_handle_t* boot_ctx) {
    if (!boot_ctx) return -1;
    boot_ctx->secure_mode = SECURE_NONE;
    boot_ctx->comm_type = COMM_UART;
    return boot_select_comm(boot_ctx,  boot_ctx->comm_type);
}

int8_t boot_init(boot_handle_t* boot_ctx) {
    if (!boot_ctx) return -1;
    boot_ctx->state = BOOT_STATE_IDLE;
    boot_ctx->comm_if->init(boot_ctx->comm_if->comm_cfg);
    return 0;
}

int8_t boot_deinit(boot_handle_t* boot_ctx) {
    if (!boot_ctx) return -1;
    return 0;
}

