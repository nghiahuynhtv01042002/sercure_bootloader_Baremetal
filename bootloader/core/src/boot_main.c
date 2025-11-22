#include "test_feat.h"
#ifndef TEST_FEAT_H
#include "boot_main.h"
#include "boot_fw_update.h"
#include <string.h>
#define APP_BACKUP_ADDR  (0x08010000UL) //(bank2 use for backup)
#define APP_FLASH_ADDR  (0x08004000UL)
#define APP_SECTION_NUMBER (1)
#define APP_BACKUP_SECTION_NUMBER (4)

#define APP_MSP         (*(volatile uint32_t *)(APP_FLASH_ADDR + 0x0))
#define APP_ENTRY       (*(volatile uint32_t *)(APP_FLASH_ADDR + 0x04))

extern uint32_t SystemCoreClock;
extern void SystemCoreClock_DeInit(void);
extern void NVIC_Disable_ISR(void);
extern int8_t boot_config(boot_handle_t* boot_ctx);
extern int8_t boot_init(boot_handle_t* boot_ctx);


typedef void(*func_ptr)(void);
// Global boot context pointer for __io_putchar / printf
static boot_handle_t *g_boot_ctx = NULL;
/* for printf */
int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    if (g_boot_ctx && g_boot_ctx->comm_if && g_boot_ctx->comm_if->comm_cfg) {
        // use comm interface send
        g_boot_ctx->comm_if->send(g_boot_ctx->comm_if->comm_cfg, &c, 1);
    } 
    return ch;
}

void enter_app(boot_handle_t *boot_ctx,uint32_t app_addr) {
    const char* msg = "Jumping to application\n";
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,(const uint8_t *)msg,strlen(msg));
    delay_ms(10);
    uint32_t app_msp = *(volatile uint32_t *)(app_addr + 0x00ul);
    uint32_t app_pc_init = *(volatile uint32_t *)(app_addr + 0x04ul); // reset handler's app

    // disable all interrupts
    NVIC_Disable_ISR(); 
    // set MSP
    __asm volatile("msr msp, %0" : : "r" (app_msp) : ); //base on reset handler set MSP or not
    // RCC deinit
    SystemCoreClock_DeInit();
    // Call reset hanlder of application
    func_ptr app_entry = (func_ptr)app_pc_init;
    app_entry();
}

void firmware_update(boot_handle_t *boot_ctx) {
    TIM2_Init();
    TIM2_SetTime(10000);
    uint8_t recv_byte = 0;
    int received_flag = 0;

    TIM2_Start();
    while (!TIM2_IsTimeElapsed()) {
        if (boot_ctx->comm_if->recv(boot_ctx->comm_if->comm_cfg, &recv_byte, 1) == 1) {
            if (recv_byte == 0x70) {        // check start byte
                uint8_t ack = 0x71;          // send ACK
                boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, &ack, 1);
                received_flag = 1;
                break;
            }
        }
        delay_ms(1);
    }

    TIM2_Stop();
    TIM2_ClearFlag();
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_DisableIRQ(TIM2_IRQn);

    if (received_flag) {
        // use centralized receive_new_firmware from boot_fw_update.c
        fw_status_t st = receive_new_firmware(boot_ctx, APP_FLASH_ADDR);
        if (st != FW_OK) {
            const char *err = "ERR: firmware update failed\r\n";
            boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, (const uint8_t *)err, strlen(err));
        } else {
            const char *ok = "OK: firmware received\r\n";
            boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, (const uint8_t *)ok, strlen(ok));
        }
    } else {
        // timeout, jump to app
        enter_app(boot_ctx, APP_FLASH_ADDR);
    }
}

int boot_main(void) {
    boot_handle_t boot_ctx;
    g_boot_ctx = &boot_ctx; // use for printf
    boot_config(&boot_ctx);
    boot_init(&boot_ctx);
    firmware_update(&boot_ctx);
    return 0;
}
#endif // TEST_FEAT_H