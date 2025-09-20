#include "isr.h"
// function prototype
void USART2_IRQHandler(void);
void DMA1_Stream6_IRQHandler(void);
void DMA1_Stream5_IRQHandler(void);
// void OTG_FS_IRQHandler(void);
// function definition
void USART2_IRQHandler(void) {
    // RX interrupt
    if(USART2_SR & USART_SR_RXNE) {
        uint8_t data = USART2_DR;
        uint16_t next_head = (uart_rx_head + 1) % UART_RX_BUFFER_SIZE;
        if(next_head != uart_rx_tail) {
            uart_rx_buffer[uart_rx_head] = data;
            uart_rx_head = next_head;
        }
    }
    
    // TX interrupt
    if((USART2_SR & USART_SR_TXE) && (USART2_CR1 & USART_CR1_TXEIE)) {
        if(uart_tx_head != uart_tx_tail) {
            USART2_DR = uart_tx_buffer[uart_tx_tail];
            uart_tx_tail = (uart_tx_tail + 1) % UART_TX_BUFFER_SIZE;
        } else {
            USART2_CR1 &= ~USART_CR1_TXEIE;
            USART2_CR1 |=  USART_CR1_TCIE;   
        }
    }

    // Transmission Complete
    if((USART2_SR & USART_SR_TC) && (USART2_CR1 & USART_CR1_TCIE)) {
        // USART2_SR &= ~USART_SR_TC;
        USART2_CR1 &= ~USART_CR1_TCIE;   
        uart_tx_busy = false;            
    }
}


void DMA1_Stream6_IRQHandler(void) {
    // TX Complete interrupt
    if(DMA1_HISR & (1 << 21)) {   // Check TCIF6
        DMA1_HIFCR |= (1 << 21);  // Clear TCIF6
        dma_tx_done = true;
    }
    
    // Error interrupts
    if(DMA1_HISR & (1 << 16)) {   // FEIF6 - FIFO Error
        DMA1_HIFCR |= (1 << 16);
    }
    
    if(DMA1_HISR & (1 << 18)) {   // DMEIF6 - Direct Mode Error  
        DMA1_HIFCR |= (1 << 18);
    }
    
    if(DMA1_HISR & (1 << 19)) {   // TEIF6 - Transfer Error
        DMA1_HIFCR |= (1 << 19);
    }
}

void DMA1_Stream5_IRQHandler(void) {
    // RX Half Transfer interrupt
    if(DMA1_LISR & (1 << 10)) {   // HTIF5
        DMA1_LIFCR |= (1 << 10);
    }
    
    // RX Transfer Complete interrupt 
    if(DMA1_LISR & (1 << 11)) {   // TCIF5
        DMA1_LIFCR |= (1 << 11);
    }
    
    // Error interrupts
    if(DMA1_LISR & (1 << 6)) {    // FEIF5
        DMA1_LIFCR |= (1 << 6);
    }
    
    if(DMA1_LISR & (1 << 8)) {    // DMEIF5
        DMA1_LIFCR |= (1 << 8);
    }
    
    if(DMA1_LISR & (1 << 9)) {    // TEIF5
        DMA1_LIFCR |= (1 << 9);
    }
}
// void OTG_FS_IRQHandler(void) {
//     uint32_t gintsts = OTG_FS_GINTSTS;

//     if (gintsts & (1 << 12)) { // USB reset
//         usb_device_state = USB_STATE_DEFAULT;
//         usb_device_address = 0;
//         USB_EP0_Init();
//         // Clear reset interrupt
//         OTG_FS_GINTSTS = (1 << 12);
//     }

//     if (gintsts & (1 << 13)) { // Enumeration done
//         // Configure speed
//         OTG_FS_DCFG &= ~0x3;
//         OTG_FS_DCFG |= 0x3;  // Full speed
//         // Clear enumeration done interrupt
//         OTG_FS_GINTSTS = (1 << 13);
//     }

//     if (gintsts & (1 << 4)) { // RX FIFO non-empty
//         uint32_t grxstsp = OTG_FS_GRXSTSP;
//         uint8_t epnum = grxstsp & 0xF;
//         uint8_t pktsts = (grxstsp >> 17) & 0xF;
        
//         if (pktsts == 0x06 && epnum == 0) { // Setup packet received
//             USB_EP0_SetupHandler();
//         }
//         // Clear RX FIFO interrupt
//         OTG_FS_GINTSTS = (1 << 4);
//     }

//     if (gintsts & (1 << 18)) { // IN endpoint interrupt
//         uint32_t diepint = OTG_FS_DIEPINT0;
//         if (diepint & (1 << 0)) { // Transfer completed
//             if (usb_device_address != 0 && usb_device_state == USB_STATE_ADDRESSED) {
//                 USB_ApplyAddress();
//             }
//         }
//         // Clear IN endpoint interrupt
//         OTG_FS_DIEPINT0 = diepint;
//         OTG_FS_GINTSTS = (1 << 18);
//     }

//     if (gintsts & (1 << 19)) { // OUT endpoint interrupt
//         uint32_t doepint = OTG_FS_DOEPINT0;
//         if (doepint & (1 << 3)) { // Setup phase done
//             // Re-enable EP0 OUT for next setup packet
//             OTG_FS_DOEPTSIZ0 = (3 << 29) | (1 << 19) | 64;
//             OTG_FS_DOEPCTL0 |= (1 << 31) | (1 << 26);
//         }
//         // Clear OUT endpoint interrupt
//         OTG_FS_DOEPINT0 = doepint;
//         OTG_FS_GINTSTS = (1 << 19);
//     }
// }
