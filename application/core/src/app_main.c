#include "app_main.h"

static uint8_t tx_buf[UART_TX_BUFFER_SIZE];
static uint8_t rx_buf[UART_RX_BUFFER_SIZE];

void uart_config(UART_Config_t *uart_cfg) {
    uart_cfg->mode = UART_MODE_DMA;
    uart_cfg->baudrate = 115200;
    uart_cfg->tx_buffer = tx_buf;
    uart_cfg->rx_buffer = rx_buf;
    uart_cfg->tx_buffer_size = UART_TX_BUFFER_SIZE;
    uart_cfg->rx_buffer_size = UART_RX_BUFFER_SIZE;
    UART_Init(uart_cfg);
    
}
int app_main(void) {
    UART_Config_t uart_cfg;
    uart_config(&uart_cfg);
    printf("this is a test printf for app\n");
    delay_ms(1000);
    printf("===end====\n");

    return 0;
}