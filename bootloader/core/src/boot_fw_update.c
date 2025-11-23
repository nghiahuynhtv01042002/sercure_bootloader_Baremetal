#include "boot_fw_update.h"

extern uint32_t SystemCoreClock;
extern void SystemCoreClock_DeInit(void);
extern void NVIC_Disable_ISR(void);
 
// send acknowledgment
static void send_ack(boot_handle_t *ctx, uint8_t ack) {
    ctx->comm_if->send(ctx->comm_if->comm_cfg, &ack, 1);
}

void send_mesage(boot_handle_t *ctx, const char *msg) {
    ctx->comm_if->send(ctx->comm_if->comm_cfg, (const uint8_t *)msg, strlen(msg));
}

static fw_status_t wait_for_command(boot_handle_t *ctx, uint8_t expected, uint8_t ack) {
    uint8_t cmd;
    while (1) {
        if (ctx->comm_if->recv(ctx->comm_if->comm_cfg, &cmd, 1) == 1) {
            if (cmd == expected) {
                send_ack(ctx, ack);
                return FW_OK;
            } else {
                return FW_ERR_INVALID_CMD;
            }
        }
    }
}

// Receive data into buffer
static fw_status_t recv_data(boot_handle_t *ctx, uint8_t *buf, uint32_t size) {
    uint32_t received = 0;
    while (received < size) {
        int ret = ctx->comm_if->recv(ctx->comm_if->comm_cfg, buf + received, size - received);
        if (ret > 0) {
            received += ret;
        }
    }
    return FW_OK;
}

static fw_status_t recv_and_ack_size(boot_handle_t *ctx, uint32_t *out_size, uint8_t ack){
    if (recv_data(ctx, (uint8_t *)out_size, sizeof(uint32_t)) != FW_OK) return FW_ERR_COMM;

    send_ack(ctx, ack);
    return FW_OK;
}

static fw_status_t recv_and_write_chunks( boot_handle_t *ctx, uint32_t flash_addr,
                                        uint32_t total_size,    uint8_t ack_byte) {
    uint8_t buffer[CHUNK_SIZE];
    uint32_t received = 0;

    while (received < total_size) {
        uint32_t chunk_size =
            (total_size - received > CHUNK_SIZE) ? CHUNK_SIZE : (total_size - received);

        if (recv_data(ctx, buffer, chunk_size) != FW_OK)  return FW_ERR_COMM;

        if (flash_write_blk(flash_addr + received, buffer, chunk_size) != FLASH_OK)
            return FW_ERR_FLASH_WRITE;

        send_ack(ctx, ack_byte);
        received += chunk_size;
    }
    return FW_OK;
}

static fw_status_t wait_and_erase(boot_handle_t *ctx) {
    if (wait_for_command(ctx, ERASE_CMD, ERASE_ACK) != FW_OK) return FW_ERR_INVALID_CMD;

    if (flash_erase_sector(APP_SECTION_NUMBER) != FLASH_OK) return FW_ERR_FLASH_ERASE;

    return FW_OK;
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


fw_status_t receive_new_firmware(boot_handle_t *ctx, uint32_t flash_addr){
    // 1. Wait start
    if (wait_for_command(ctx, START_CMD, START_ACK) != FW_OK) return FW_ERR_INVALID_CMD;

    // 2. Receive firmware size + ACK
    uint32_t fw_size = 0;
    if (recv_and_ack_size(ctx, &fw_size, FW_SIZE_ACK) != FW_OK) return FW_ERR_COMM;

    // 3. Wait erase + erase
    if (wait_and_erase(ctx) != FW_OK) return FW_ERR_FLASH_ERASE;

    // 4. Receive FW data chunks
    if (recv_and_write_chunks(ctx, flash_addr, fw_size, CHUNK_ACK) != FW_OK)
        return FW_ERR_FLASH_WRITE;

    //  5. Wait for signature command + ACK
    if (wait_for_command(ctx, SIGNATURE_CMD, SIGNATURE_ACK) != FW_OK) return FW_ERR_INVALID_CMD;
    // 6. Receive signature size + ACK
    uint32_t sig_size = 0;
    uint32_t sig_addr = flash_addr + fw_size;

    if (recv_and_ack_size(ctx, &sig_size, SIGNATURE_ACK) != FW_OK)  return FW_ERR_COMM;

    // 6. Receive signature data
    if (recv_and_write_chunks(ctx, sig_addr, sig_size, SIGNATURE_ACK) != FW_OK) return FW_ERR_FLASH_WRITE;

    return FW_OK;
}

void firmware_update(boot_handle_t *boot_ctx) {
    TIM2_Init();
    TIM2_SetTime(10000);
    int received_flag = 0;
    send_mesage(boot_ctx,"Waiting for firmware update command...\r\n");
    TIM2_Start();
    while (!TIM2_IsTimeElapsed()) {
        if(wait_for_command(boot_ctx, UPDATE_FW_CMD, UPDATE_FW_ACK) == FW_OK) {
            received_flag = 1;
            send_ack(boot_ctx, UPDATE_FW_ACK);
            break;
        }
        delay_ms(1);
    }

    TIM2_Stop();
    TIM2_ClearFlag();
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_DisableIRQ(TIM2_IRQn);

    if (received_flag) {
        fw_status_t st = receive_new_firmware(boot_ctx, APP_FLASH_ADDR);
        if (st != FW_OK) {
            send_mesage(boot_ctx,"ERR: firmware update failed\r\n");
        } else {
            send_mesage(boot_ctx,"OK: firmware received\r\n");
        }
    } else {
        send_mesage(boot_ctx,"jumping to application...\r\n");

        // timeout, jump to app
        enter_app(boot_ctx, APP_FLASH_ADDR);
    }
}