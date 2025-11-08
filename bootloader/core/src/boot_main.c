#include "boot_main.h"
#include <string.h>
#define APP_BACKUP_ADDR  (0x08010000UL) //(bank2 use for backup)
#define APP_FLASH_ADDR  (0x08008000UL)
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

void enter_app(boot_handle_t *boot_ctx) {
    boot_ctx->comm_if->send(boot_ctx->comm_if->comm_cfg,(const uint8_t *)"Jumping to application\r\n",35);
    delay_ms(10);

    // disable all interrupts
    NVIC_Disable_ISR(); 
    // set MSP
    __asm volatile("msr msp, %0" : : "r" (APP_MSP) : ); //base on reset handler set MSP or not
    // RCC deinit
    SystemCoreClock_DeInit();
    // Call reset hanlder of application
    func_ptr app_entry = (func_ptr)APP_ENTRY;
    app_entry();
}
#define TEST_UART_TIM_ENTRYAPP
// #define TEST_FLASH
#define TEST_SECTOR_NUM (5)
#define TEST_SECTOR_ADDR (0x08020000UL)

void test_flash(boot_handle_t *boot_ctx) {
    uint32_t test_data = 0x12345678;
    uint32_t read_data;
    printf("========== TESTING FLASH WORD==========\n");
    // test earse function
    printf("[1] Earse sector %d\n",TEST_SECTOR_NUM);
    if(!flash_erase_sector(TEST_SECTOR_NUM)) {
        printf("Earse OK\n");
    } else {
        printf("Earse FAIL\n");
        printf("========== TESTING FLASH FAIL!!!!!!!!==========\n");
        return;
    }
    // test write flash
    printf("adr: %016lx : 0x%08lx\n",TEST_SECTOR_ADDR,*(volatile uint32_t *)TEST_SECTOR_ADDR);
    printf("[2] Write word 0x%08lx a t0x%08lx\n",test_data,TEST_SECTOR_ADDR);
    if (!flash_write_word(TEST_SECTOR_ADDR, test_data)) {
        printf("Program OK\n");
    } else {
        printf("Program FAIL\n");
        printf("=== FLASH TEST FAILED ===\n");
        return;
    }
    // read data back
    printf("[3] Reading back...\n");
    read_data = flash_read_word(TEST_SECTOR_ADDR);
    printf("read_data: 0x%08lx\n",read_data);
    printf("adr: %08lx : 0x%08lx\n",TEST_SECTOR_ADDR,*(volatile uint32_t *)TEST_SECTOR_ADDR);

    // Compare
    printf("[4] Verifying...\n");
    if (read_data == test_data) {
        printf("Test PASSED\n");
    } else {
        printf("Test FAILED\n");
    }

    // test for array 
    printf("========== TESTING FLASH ARRAY==========\n");
    printf("[1] Earse sector %d\n",TEST_SECTOR_NUM);
    if(!flash_erase_sector(TEST_SECTOR_NUM)) {
        printf("Earse OK\n");
    } else {
        printf("Earse FAIL\n");
        printf("========== TESTING FLASH FAIL!!!!!!!!==========\n");
        return;
    }

    uint8_t arr[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 ,0x07, 0x08, 0x09, 0x0a, 0x0b};
    uint8_t read_arr[12];
    uint32_t write_byte = 0;
    printf("[2] Write arr\n");
    for(uint32_t i = 0; i < sizeof(arr); i+=4) {
        write_byte = ((uint32_t)arr[i])|
                        ((uint32_t)arr[i+1] << 8)|
                        ((uint32_t)arr[i+2] << 16)|
                        ((uint32_t)arr[i+3] << 24);
        if (flash_write_word(TEST_SECTOR_ADDR + i, write_byte)) {
            printf("flash write error at:%08ld",TEST_SECTOR_ADDR +i);
            return;
        }
    }
    printf("Write OK");
    for (uint32_t i = 0; i < sizeof(read_arr); i++) {
        read_arr[i] = *(volatile uint8_t *)(TEST_SECTOR_ADDR + i);
    }
    printf("[3] Verification\n");
    uint32_t mismatch = 0;
    for (uint32_t i = 0; i < sizeof(read_arr); i++) {
        if (arr[i] != read_arr[i]) {
            printf("Mismatch at byte %lu: write=0x%02X read=0x%02X\n", i, arr[i], read_arr[i]);
            mismatch = 1;
        }
    }
    if(mismatch) {
        printf("Flash array Fail\n");
    } else {
        printf("Flash array OK\n");
    }
    printf("=== FLASH TEST END ===\n");
    return;
}

int boot_main(void) {
    boot_handle_t boot_ctx;
    g_boot_ctx = &boot_ctx; // use for printf
    boot_config(&boot_ctx);
    boot_init(&boot_ctx);

    #ifdef TEST_FLASH
        test_flash(&boot_ctx);
    while (1);
    #elif defined(TEST_UART_TIM_ENTRYAPP)
    TIM2_Init();
    TIM2_SetTime(3000);
    
    static uint8_t fw_chunk[256];
    uint16_t total_recv = 0;  
    uint16_t recv_length = 0;
    int e_flag = 0;

    boot_ctx.comm_if->send(boot_ctx.comm_if->comm_cfg,(const uint8_t *)"Waiting firmware update signal in 3 seconds.\r\n", 47);
    TIM2_Start();
    while (!TIM2_IsTimeElapsed()) {
        recv_length = boot_ctx.comm_if->recv(boot_ctx.comm_if->comm_cfg,fw_chunk + total_recv, sizeof(fw_chunk) - total_recv);
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
        boot_ctx.comm_if->send(boot_ctx.comm_if->comm_cfg, fw_chunk, total_recv);
        delay_ms(10);
        boot_ctx.comm_if->send(boot_ctx.comm_if->comm_cfg, (const uint8_t *)"Firmware update.\r\n", 18);
    } else {
        enter_app(&boot_ctx);
    }
    #endif

    return 0;
}
