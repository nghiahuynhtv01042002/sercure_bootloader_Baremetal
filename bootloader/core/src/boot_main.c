#include "test_feat.h"
#ifndef TEST_FEAT_H
#include "boot_main.h"
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
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,(const uint8_t *)"Jumping to application\n",35);
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
void test_tmp(boot_handle_t *boot_ctx) {
    TIM2_Init();
    TIM2_SetTime(3000);
    
    static uint8_t fw_chunk[256];
    uint16_t total_recv = 0;  
    uint16_t recv_length = 0;
    int e_flag = 0;

    TIM2_Start();
    while (!TIM2_IsTimeElapsed()) {
        recv_length = boot_ctx->comm_if->recv(boot_ctx->comm_if->comm_cfg,fw_chunk + total_recv, sizeof(fw_chunk) - total_recv);
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
        boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, fw_chunk, total_recv);
        delay_ms(10);
        boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, (const uint8_t *)"Firmware update.\r\n", 18);
    } else {
        enter_app(boot_ctx,APP_FLASH_ADDR);
    }
}
#define START_CMD 0x55
#define START_ACK 0xAA
#define SIZE_ACK  0xAA
#define ERASE_CMD 0xEC
#define ERASE_ACK 0xAB
#define CHUNK_ACK 0xCC
#define CHUNK_SIZE 256

static int receive_new_firmware(boot_handle_t *boot_ctx, uint32_t flash_addr)
{
    uint8_t cmd;
    uint8_t ack;
    uint32_t fw_size = 0;
    uint32_t received = 0;
    uint8_t buffer[CHUNK_SIZE];

    // 1. Wait for start command (0x55)
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,
        (const uint8_t *)"Waiting for start...\r\n", 23);

    while (1) {
        if (boot_ctx->comm_if->recv(boot_ctx->comm_if->comm_cfg, &cmd, 1) == 1) {
            if (cmd == START_CMD) {
                ack = START_ACK;     // send 0xAA
                boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, &ack, 1);
                break;
            } else {
                boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,
                    (const uint8_t *)"Wrong start byte\r\n", 19);
            }
        }
    }

    // 2. Receive firmware size (4 bytes)
    uint8_t *p = (uint8_t *)&fw_size;
    uint32_t got = 0;

    while (got < 4) {
        int ret = boot_ctx->comm_if->recv(boot_ctx->comm_if->comm_cfg, p + got, 4 - got);
        if (ret > 0) got += ret;
    }

    // ACK size
    ack = SIZE_ACK; // 0xAA
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, &ack, 1);

    while (1) {
        if (boot_ctx->comm_if->recv(boot_ctx->comm_if->comm_cfg, &cmd, 1) == 1) {
            if (cmd == ERASE_CMD) {
                if (flash_erase_sector(APP_SECTION_NUMBER) != FLASH_OK) {
                    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,
                                            (const uint8_t *)"ERR: flash erase\r\n", 19);
                    return -1;
                }
                break;
            } else {
                boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,
                                        (const uint8_t *)"Wrong erase byte\r\n", 19);
            }
        }
    }    


    // ACK flash erase
    ack = ERASE_ACK; // 0xAB
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, &ack, 1);

    // 4. Receive firmware chunks
    while (received < fw_size) {
        uint32_t remain = fw_size - received;
        uint32_t chunk_size = (remain > CHUNK_SIZE) ? CHUNK_SIZE : remain;
        
        uint32_t chunk_received = 0;
        
        while (chunk_received < chunk_size) {
            int ret = boot_ctx->comm_if->recv(boot_ctx->comm_if->comm_cfg, 
                                              buffer + chunk_received, 
                                              chunk_size - chunk_received);
            if (ret > 0) {
                chunk_received += ret;
            } 
        }
        
        // Write chunk to flash
        if (flash_write_blk(flash_addr + received, buffer, chunk_received) != FLASH_OK) {
            boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, 
                                   (const uint8_t *)"ERR: flash write\r\n", 19);
            return -1;
        }
        
        received += chunk_received;
        
        // ACK chunk
        ack = CHUNK_ACK;
        boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg, &ack, 1);
    }
    
    // Done
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,
                           (const uint8_t *)"Firmware update done\r\n", 23);
    return 0;
}

int boot_main(void) {
    boot_handle_t boot_ctx;
    g_boot_ctx = &boot_ctx; // use for printf
    boot_config(&boot_ctx);
    boot_init(&boot_ctx);
    if(receive_new_firmware(&boot_ctx, APP_FLASH_ADDR) == 0 ) {
        enter_app(&boot_ctx, APP_FLASH_ADDR);
    }

    return 0;
}
#endif // TEST_FEAT_H