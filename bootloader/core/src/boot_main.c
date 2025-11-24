#include <string.h>
#include "boot_main.h"
#include "boot_fw_update.h"
#include "boot_verify_signature.h"
#include "boot_cfg.h"
#include "rsa2048.h"


extern uint32_t SystemCoreClock;
extern void SystemCoreClock_DeInit(void);
extern void NVIC_Disable_ISR(void);
extern int8_t boot_config(boot_handle_t* boot_ctx);
extern int8_t boot_init(boot_handle_t* boot_ctx);
extern fw_status_t firmware_update(boot_handle_t *boot_ctx, uint32_t fw_addr, uint32_t* fw_size );
extern rsa_verify_result_t verify_firmware(uint32_t flash_addr, uint32_t fw_size);
extern void enter_app(boot_handle_t *boot_ctx,uint32_t app_addr);
extern void send_message(boot_handle_t *ctx, const char *msg);
extern void print_verify_result(rsa_verify_result_t result);

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
void print_verify_result(rsa_verify_result_t result) {
    switch (result) {
        case RSA_VERIFY_OK:
            send_message(g_boot_ctx,"Signature is VALID.\n");
            break;
        case RSA_VERIFY_INVALID_SIGNATURE:
            send_message(g_boot_ctx,"Signature is INVALID\n");
            break;
        case RSA_VERIFY_PADDING_ERROR:
            send_message(g_boot_ctx,"Signature padding error\n");
            break;
        case RSA_VERIFY_INVALID_INPUT:
            send_message(g_boot_ctx,"Invalid input to verification function\n");
            break;
        case RSA_COVERT_SIGNATURE_ERROR:
            send_message(g_boot_ctx,"Error converting signature to big integer\n");
            break;
        case RSA_COVERT_MODULUS_ERROR:
            send_message(g_boot_ctx,"Error converting modulus to big integer\n");
            break;
        case RSA_COVERT_EXPONENT_ERROR:
            send_message(g_boot_ctx,"Error converting exponent to big integer\n");
            break;
        case RSA_KEY_OPERATION_ERROR:
            send_message(g_boot_ctx,"Error during RSA key operation\n");
            break;
        case RSA_CONVERT_KEY_OPERATION_ERROR:
            send_message(g_boot_ctx,"Error converting result of RSA operation to bytes\n");
            break;
        default:
            send_message(g_boot_ctx,"General RSA verification error\n");
            break;
    }
}

int boot_main(void) {
    boot_handle_t boot_ctx;
    g_boot_ctx = &boot_ctx;
    boot_config(&boot_ctx);
    boot_init(&boot_ctx);
    uint32_t fw_addr = FW_FLASH_ADDR;
    uint32_t fw_size = 0;
    int8_t is_update = 0;
    send_message(&boot_ctx,"Bootloader started...\r\n");

    fw_status_t fw_update_st = firmware_update(&boot_ctx,fw_addr,&fw_size);
        if (fw_update_st == FW_OK && fw_size > 0) {
            send_message(&boot_ctx, "Firmware update successful, ready to boot.\r\n");
            is_update = 1;
        } else {
            send_message(&boot_ctx, "No valid firmware update, checking stored image...\r\n");
            is_update = 0;
        }

        fw_size = read_fw_size_from_flash();
        rsa_verify_result_t vr = verify_firmware(fw_addr, fw_size);
        print_verify_result(vr);
        printf("Firmware size: %lu bytes\r\n", (unsigned long)fw_size);
        if (vr == RSA_VERIFY_OK) {
            send_message(&boot_ctx, "Entering application...\r\n");
            delay_ms(100);
            enter_app(&boot_ctx, fw_addr);
        } else {
            send_message(&boot_ctx, "Firmware verification failed.\r\n");
        }
        if(is_update == 1){
            send_message(&boot_ctx, "Boot failed after update.\r\n");
        } else {
            send_message(&boot_ctx, "No valid bootable image found.\r\n");
        }
    return 0;
}
