
#include "tim2.h"
#include "nvic.h"

volatile TIM2_Config_t tim2_config = {0, 0};

void TIM2_Init(void) {
    // Enable TIM2 clock
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    // Reset TIM2 peripheral
    RCC_APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC_APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
    
    // Stop the timer
    TIM2_CR1 &= ~TIM_CR1_CEN;
    
    // Clear all interrupt flags
    TIM2_SR = 0;
    
    TIM2_PSC = 95;
    TIM2_ARR = 999;
    
    // Generate update event
    TIM2_EGR |= TIM_EGR_UG;
    
    // Enable update interrupt
    TIM2_DIER |= TIM_DIER_UIE;
    
    // Enable NVIC interrupt for TIM2
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 0);
    
    tim2_config.overflow = 0;
}

void TIM2_SetTime(uint32_t time_ms) {
    if (time_ms == 0 || time_ms > 60000) {
        time_ms = 1000;
    }
    
    tim2_config.time_ms = time_ms;
    
    TIM2_CR1 &= ~TIM_CR1_CEN;
    TIM2_CNT = 0;
    TIM2_ARR = (time_ms * 1000) - 1;
    TIM2_EGR |= TIM_EGR_UG;
    TIM2_SR = 0;
    tim2_config.overflow = 0;
}

void TIM2_Start(void) {
    TIM2_CNT = 0;
    TIM2_SR = 0;
    tim2_config.overflow = 0;
    TIM2_CR1 |= TIM_CR1_CEN;
}

void TIM2_Stop(void) {
    TIM2_CR1 &= ~TIM_CR1_CEN;
}

void TIM2_Reset(void) {
    TIM2_CNT = 0;
    TIM2_SR = 0;
    tim2_config.overflow = 0;
}

bool TIM2_IsTimeElapsed(void) {
    return (tim2_config.overflow != 0);
}

uint32_t TIM2_GetCounter(void) {
    return TIM2_CNT;
}

void TIM2_ClearFlag(void) {
    tim2_config.overflow = 0;
    TIM2_SR &= ~TIM_SR_UIF;
}

void TIM2_IRQHandler(void) {
    if (TIM2_SR & TIM_SR_UIF) {
        TIM2_SR &= ~TIM_SR_UIF;
        tim2_config.overflow = 1;
        TIM2_CR1 &= ~TIM_CR1_CEN;
    }
}