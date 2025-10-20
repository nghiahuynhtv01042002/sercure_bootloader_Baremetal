#ifndef TIM2_H
#define TIM2_H

#include "sysclocks.h"
#include <stdint.h>
#include <stdbool.h>

// TIM2 base address
#define TIM2_BASE      (0x40000000UL)

// TIM2 register map
#define TIM2_CR1       (*(volatile uint32_t *)(TIM2_BASE + 0x00))
#define TIM2_CR2       (*(volatile uint32_t *)(TIM2_BASE + 0x04))
#define TIM2_SMCR      (*(volatile uint32_t *)(TIM2_BASE + 0x08))
#define TIM2_DIER      (*(volatile uint32_t *)(TIM2_BASE + 0x0C))
#define TIM2_SR        (*(volatile uint32_t *)(TIM2_BASE + 0x10))
#define TIM2_EGR       (*(volatile uint32_t *)(TIM2_BASE + 0x14))
#define TIM2_CCMR1     (*(volatile uint32_t *)(TIM2_BASE + 0x18))
#define TIM2_CCMR2     (*(volatile uint32_t *)(TIM2_BASE + 0x1C))
#define TIM2_CCER      (*(volatile uint32_t *)(TIM2_BASE + 0x20))
#define TIM2_CNT       (*(volatile uint32_t *)(TIM2_BASE + 0x24))
#define TIM2_PSC       (*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_ARR       (*(volatile uint32_t *)(TIM2_BASE + 0x2C))

// CR1 Bit definitions
#define TIM_CR1_CEN    (1U << 0)
#define TIM_CR1_UDIS   (1U << 1)
#define TIM_CR1_URS    (1U << 2)
#define TIM_CR1_OPM    (1U << 3)
#define TIM_CR1_DIR    (1U << 4)
#define TIM_CR1_ARPE   (1U << 7)

// DIER Bit definitions
#define TIM_DIER_UIE   (1U << 0)
#define TIM_DIER_TIE   (1U << 6)

// SR Bit definitions
#define TIM_SR_UIF     (1U << 0)
#define TIM_SR_TIF     (1U << 6)

// EGR Bit definitions
#define TIM_EGR_UG     (1U << 0)

// Timer2 IRQ
#define TIM2_IRQn      28

typedef struct {
    uint32_t time_ms;
    volatile uint8_t overflow;
} TIM2_Config_t;

extern volatile TIM2_Config_t tim2_config;

void TIM2_Init(void);
void TIM2_SetTime(uint32_t time_ms);
void TIM2_Start(void);
void TIM2_Stop(void);
void TIM2_Reset(void);
bool TIM2_IsTimeElapsed(void);
uint32_t TIM2_GetCounter(void);
void TIM2_ClearFlag(void);
void TIM2_IRQHandler(void);

#endif // TIM2_H