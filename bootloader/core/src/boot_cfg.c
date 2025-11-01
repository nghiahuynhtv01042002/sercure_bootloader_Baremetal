#include "boot_cfg.h"
extern void uart_config(UART_Config_t *uart_cfg) ;
int8_t boot_config(boot_handle_t* boot_ctx) {
    if (!boot_ctx) return -1;
    static UART_Config_t uart_cfg;
    uart_config(&uart_cfg);

    static comm_interface_t uart_driver = {
        .comm_cfg = &uart_cfg,
        .init = (void (*)(void *))UART_Init,
        .recv = (uint16_t (*)(void *, uint8_t *, uint16_t))UART_ReceiveData,
        .send = (void (*)(void *, const uint8_t *, uint16_t))UART_SendData
    };

    boot_ctx->secure_mode = SECURE_NONE;
    boot_ctx->comm_if = &uart_driver;
    return 0;
}

int8_t boot_init(boot_handle_t* boot_ctx) {
    if (!boot_ctx) return -1;
    boot_ctx->comm_if->init(boot_ctx->comm_if->comm_cfg);
    return 0;
}