// #include <stdint.h>
// #include "sysclocks.h"
// #include "usbfs.h"
// #include "gpio.h"
// #include "nvic.h"
// #include "uart.h"

// extern uint32_t SystemCoreClock;
// uint8_t tx_buf[UART_TX_BUFFER_SIZE];
// uint8_t rx_buf[UART_RX_BUFFER_SIZE];
// int main(void) {
//     USB_GPIO_Init();
//     USB_core_device_init();
//     USB_EP0_Init();
    
//     UART_Config_t uart_cfg;
//     uart_cfg.mode = UART_MODE_NORMAL;
//     uart_cfg.baudrate = 115200;
//     uart_cfg.tx_buffer = tx_buf;
//     uart_cfg.rx_buffer = rx_buf;
//     uart_cfg.tx_buffer_size = UART_TX_BUFFER_SIZE;
//     uart_cfg.rx_buffer_size = UART_RX_BUFFER_SIZE;
//     // enable NVIC OTG_FS_IRQn ở đây
//     UART_Init(&uart_cfg);
//     UART_SendData((const uint8_t *)"USB CDC Test Start\r\n",21);
//     while(1){
        
//         // loop
//     }
// }

