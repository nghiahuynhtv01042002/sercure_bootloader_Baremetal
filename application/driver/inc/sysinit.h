#ifndef SYS_INIT_H
#define SYS_INIT_H
#include <stdint.h>
//  System control block 
#define SCB             (0xE000ED00UL)
#define SCB_VTOR        (*(volatile uint32_t *)(SCB + 0x08))
#define SCB_CPACR       (*(volatile uint32_t *)(SCB + 0x88))

#define VECT_TAB_BASE_ADDRESS   0x08000000UL
#define VECT_TAB_OFFSET         0x00008000UL

void System_Init(void);
#endif // SYS_INIT_H