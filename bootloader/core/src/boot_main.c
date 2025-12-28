#include <string.h>
#include "boot_main.h"
#include "boot_fw_update.h"
#include "boot_verify_signature.h"
#include "boot_cfg.h"
#include "fw_metadata.h"
#include "rsa2048.h"


extern uint32_t SystemCoreClock;
extern void SystemCoreClock_DeInit(void);
extern void NVIC_Disable_ISR(void);
extern int8_t boot_config(boot_handle_t* boot_ctx);
extern int8_t boot_init(boot_handle_t* boot_ctx);
fw_status_t receive_fw_update_request(boot_handle_t *boot_ctx);
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

#if 0
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
#endif

#if 1
void print_firmware_status(fw_status_t status)
{
    switch (status) {
        case FW_OK:
            send_message(g_boot_ctx, "Firmware update OK\n");
            break;

        case FW_ERR_COMM:
            send_message(g_boot_ctx, "Firmware update error: communication failure\n");
            break;

        case FW_TIMEOUT_CMD:
            send_message(g_boot_ctx, "Firmware update error: command timeout\n");
            break;

        case FW_ERR_FLASH_ERASE:
            send_message(g_boot_ctx, "Firmware update error: flash erase failed\n");
            break;

        case FW_ERR_FLASH_WRITE:
            send_message(g_boot_ctx, "Firmware update error: flash write failed\n");
            break;

        case FW_ERR_INVALID_CMD:
            send_message(g_boot_ctx, "Firmware update error: invalid command\n");
            break;

        case FW_ERR_COPY_FW:
            send_message(g_boot_ctx, "Firmware update error: firmware copy failed\n");
            break;

        case FW_ERR_INVALID_SIGNATURE:
            send_message(g_boot_ctx, "Firmware update error: invalid firmware signature\n");
            break;

        default:
            send_message(g_boot_ctx, "Firmware update error: unknown status\n");
            break;
    }
}
#endif
int boot_main(void) {
    boot_handle_t boot_ctx;
    g_boot_ctx = &boot_ctx;
    if(boot_config(&boot_ctx) != 0) return -1;
    if(boot_init(&boot_ctx) != 0) return -1;
    uint32_t fw_addr = read_fw_addr_from_flash();
    uint32_t fw_size = read_fw_size_from_flash();

    // send acknowledgment after boot init
    boot_ctx.comm_if->send(boot_ctx.comm_if->comm_cfg, (const uint8_t[]){BOOT_FINISH_SIGNAL}, 1);
    send_message(&boot_ctx,"Bootloader is running...\r\n");

    fw_status_t fw_st = receive_fw_update_request(&boot_ctx);
    
    if( (fw_st == FW_TIMEOUT_CMD)) {
        send_message(&boot_ctx,"No update request received.\r\n");
    } else if(fw_st != FW_OK) {
        send_message(&boot_ctx,"firmware update request encounter problem.\r\n");
        return -1;
    }

    fw_st = handle_update_request(&boot_ctx, &fw_addr, &fw_size);
    if( fw_st != FW_OK) {
        send_message(&boot_ctx,"Handling update request failed.\r\n");
        print_firmware_status(fw_st);
        return -1;
    }
    fw_st = process_boot_state(&boot_ctx, &fw_addr, &fw_size);
    // handle errors here
    if (fw_st != FW_OK) {
        send_message(&boot_ctx,"Firmware update failed with error code: ");
        print_firmware_status(fw_st);
        return -1;
    }
    return 0;
}

