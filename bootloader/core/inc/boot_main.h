#ifndef BOOT_MAIN_H
#define BOOT_MAIN_H
#include <stdint.h>
#include "sysclocks.h"
#include "gpio.h"
#include "nvic.h"
#include "uart.h"

//  System control block 
#define SCB             (0xE000ED00UL)
#define SCB_VTOR        (*(volatile uint32_t *)(SCB + 0x08))
#define SCB_CPACR       (*(volatile uint32_t *)(SCB + 0x88))


static inline void FPU_Enable(void) {
    // Set CP10 and CP11 full access
    SCB_CPACR |= (0xF << 20);  // bits 20-23 = CP10, CP11 full access
    __asm__ volatile ("dsb");   // Data Synchronization Barrier
    __asm__ volatile ("isb");   // Instruction Synchronization Barrier
}
#endif // BOOT_MAIN_H