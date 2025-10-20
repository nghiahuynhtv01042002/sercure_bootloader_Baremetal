#include "boot_main.h"

// #define APP_FLASH_ADDR  (0x08010000UL) (bank2 use for backup)
#define APP_FLASH_ADDR  (0x08008000UL)
#define APP_MSP         (*(volatile uint32_t *)(APP_FLASH_ADDR + 0x0))
#define APP_ENTRY       (*(volatile uint32_t *)(APP_FLASH_ADDR + 0x04))

extern uint32_t SystemCoreClock;
extern void SystemCoreClock_DeInit(void);
extern void NVIC_Disable_ISR(void);
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
}

void enter_app(boot_mode_config_t *boot_cfg) {
    boot_cfg->comm_if->send(boot_cfg->comm_if->comm_cfg,(const uint8_t *)"Booting ...\r\n",13);
    delay_ms(10);
    boot_cfg->comm_if->send(boot_cfg->comm_if->comm_cfg,(const uint8_t *)"Waiting firmware update signal in 3 seconds.\r\n",46);
    delay_ms(10);

    uint8_t count = 0;    
    while(count++ < 3) {
        delay_ms(1000);
    }

    boot_cfg->comm_if->send(boot_cfg->comm_if->comm_cfg,(const uint8_t *)"Jumping to application\r\n",35);
    delay_ms(10);

    // disable all interrupts
    NVIC_Disable_ISR(); 
    // set MSP
    __asm volatile("msr msp, %0" : : "r" (APP_MSP) : ); //base on reset handler set MSP or not
    // RCC deinit
    SystemCoreClock_DeInit();
    // Call reset hanlder of application
    func_ptr app_entry = (func_ptr)APP_ENTRY;
    app_entry();
}

int8_t boot_config(boot_mode_config_t* boot_cfg) {
    if (!boot_cfg) return -1;
    static UART_Config_t uart_cfg;
    uart_config(&uart_cfg);

    static comm_interface_t uart_driver = {
        .comm_cfg = &uart_cfg,
        .init = (void (*)(void *))UART_Init,
        .recv = (uint16_t (*)(void *, uint8_t *, uint16_t))UART_ReceiveData,
        .send = (void (*)(void *, const uint8_t *, uint16_t))UART_SendData
    };

    boot_cfg->secure_mode = SECURE_NONE;
    boot_cfg->comm_if = &uart_driver;
    return 0;
}

int8_t boot_init(boot_mode_config_t* boot_cfg) {
    if (!boot_cfg) return -1;
    boot_cfg->comm_if->init(boot_cfg->comm_if->comm_cfg);
    return 0;
}
int boot_main(void) {
    boot_mode_config_t boot_cfg;
    boot_config(&boot_cfg);
    boot_init(&boot_cfg);
    static uint8_t fw_chunk[11];
    // enter_app(&boot_cfg);
    uint16_t recv_length = 0;
    boot_cfg.comm_if->send(boot_cfg.comm_if->comm_cfg,(const uint8_t *)"Waiting firmware update signal in 3 seconds.\r\n", 46);
    while (1) {
        recv_length = boot_cfg.comm_if->recv(boot_cfg.comm_if->comm_cfg,fw_chunk,sizeof(fw_chunk));
        if (recv_length > 0) {
            boot_cfg.comm_if->send(boot_cfg.comm_if->comm_cfg,fw_chunk,recv_length);
        }
    }
    return 0;
}
