#include "boot_main.h"

#define APP_FLASH_ADDR  (0x08010000UL)
#define APP_MSP         (*(volatile uint32_t *)(APP_FLASH_ADDR + 0x0))
#define APP_ENTRY       (*(volatile uint32_t *)(APP_FLASH_ADDR + 0x04))

extern uint32_t SystemCoreClock;
extern void SystemCoreClock_DeInit(void);
static uint8_t tx_buf[UART_TX_BUFFER_SIZE];
static uint8_t rx_buf[UART_RX_BUFFER_SIZE];
typedef void(*func_ptr)(void);

void uart_config(UART_Config_t *uart_cfg) {
    uart_cfg->mode = UART_MODE_DMA;
    uart_cfg->baudrate = 115200;
    uart_cfg->tx_buffer = tx_buf;
    uart_cfg->rx_buffer = rx_buf;
    uart_cfg->tx_buffer_size = UART_TX_BUFFER_SIZE;
    uart_cfg->rx_buffer_size = UART_RX_BUFFER_SIZE;
    UART_Init(uart_cfg);

}

void jump_to_app() {
    UART_SendData((const uint8_t *)"Booting ...\r\n",13);
    delay_ms(10);
    UART_SendData((const uint8_t *)"Waiting firmware update signal in 3 seconds.\r\n",46);
    delay_ms(10);

    uint8_t count = 0;    
    while(count++ < 3) {
        delay_ms(1000);
    }

    UART_SendData((const uint8_t *)"Jumping to application\r\n",35);
    delay_ms(10);

    func_ptr app_entry = (func_ptr)APP_ENTRY;
    // Call reset hanlder of application
    SystemCoreClock_DeInit();
    app_entry();
}

int boot_main(void) {
    UART_Config_t uart_cfg;
    uart_config(&uart_cfg);
    jump_to_app();
    return 0;
}