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

void enter_app(boot_handle_t *boot_ctx) {
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,(const uint8_t *)"Jumping to application\r\n",35);
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
int boot_main(void) {
    boot_handle_t boot_ctx;
    boot_config(&boot_ctx);
    boot_init(&boot_ctx);
    TIM2_Init();
    TIM2_SetTime(3000);
    
    static uint8_t fw_chunk[256];
    uint16_t total_recv = 0;  
    uint16_t recv_length = 0;
    int e_flag = 0;
    
    boot_ctx.comm_if->send(boot_ctx.comm_if->comm_cfg,(const uint8_t *)"Waiting firmware update signal in 3 seconds.\r\n", 47);
    TIM2_Start();
    
    while (!TIM2_IsTimeElapsed()) {
        recv_length = boot_ctx.comm_if->recv(boot_ctx.comm_if->comm_cfg,fw_chunk + total_recv, sizeof(fw_chunk) - total_recv);
        if (recv_length > 0) {
            total_recv += recv_length;
            // this is temp logic
            if (fw_chunk[total_recv - 1] == '\n') {
                e_flag = 1;
                TIM2_Stop();
                TIM2_ClearFlag();
                NVIC_ClearPendingIRQ(TIM2_IRQn);
                NVIC_DisableIRQ(TIM2_IRQn);
                break;
            }
        }
        delay_ms(1);  
    }
    
    if(e_flag == 1) {
        boot_ctx.comm_if->send(boot_ctx.comm_if->comm_cfg, fw_chunk, total_recv);
        delay_ms(10);
        boot_ctx.comm_if->send(boot_ctx.comm_if->comm_cfg, (const uint8_t *)"Firmware update.\r\n", 18);
    } else {
        enter_app(&boot_ctx);
    }
    return 0;
}
