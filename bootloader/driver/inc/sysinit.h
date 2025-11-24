#ifndef SYS_INIT_H
#define SYS_INIT_H
#include <stdint.h>
//  System control block 
#define SCB             (0xE000ED00UL)
#define SCB_VTOR        (*(volatile uint32_t *)(SCB + 0x08))
#define SCB_CPACR       (*(volatile uint32_t *)(SCB + 0x88))
#define SCB_AIRCR       (*(volatile uint32_t *)(SCB + 0x0C))

#define SCB_AIRCR_VECTKEY    (0x5FA << 16)
#define SCB_AIRCR_SYSRESETREQ (1 << 2)

#define VECT_TAB_BASE_ADDRESS   0x08000000UL
#define VECT_TAB_OFFSET         0x00000000UL

void System_Init(void);

static inline void NVIC_SystemReset(void) {
    __asm volatile("dsb"); 
    SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
    __asm volatile("dsb");

    while(1) { __asm volatile("nop"); } // wait for reset
}

#endif // SYS_INIT_H