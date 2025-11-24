#include "sysinit.h"
#pragma weak SystemInit_ExtMemCtl
void SystemInit_ExtMemCtl(void) {}

static inline void set_vector_table(uint32_t base_address, uint32_t offset) {
    SCB_VTOR = (base_address & 0xFFFFFE00U) | (offset & 0x1FFU);
}

static inline void FPU_Enable(void) {
    // Set CP10 and CP11 full access
    SCB_CPACR |= ((3UL << 10*2)|(3UL << 11*2)); 
    __asm__ volatile ("dsb");   // Data Synchronization Barrier
    __asm__ volatile ("isb");   // Instruction Synchronization Barrier
}

void System_Init(void) {
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    FPU_Enable();
  #endif

#if defined (DATA_IN_ExtSRAM) || defined (DATA_IN_ExtSDRAM)
  SystemInit_ExtMemCtl(); 
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */

  /* Configure the Vector Table location */
  SCB_VTOR = VECT_TAB_BASE_ADDRESS|VECT_TAB_OFFSET;

}