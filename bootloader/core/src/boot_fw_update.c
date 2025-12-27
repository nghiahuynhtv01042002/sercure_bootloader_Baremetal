#include "boot_fw_update.h"
#include "fw_metadata.h"
#include "flash.h"
#include "boot_cfg.h"
#include "rsa2048.h"
#include "boot_verify_signature.h"
extern uint32_t SystemCoreClock;
extern void SystemCoreClock_DeInit(void);
extern void NVIC_Disable_ISR(void);


// send acknowledgment
static void send_ack(boot_handle_t *ctx, uint8_t ack) {
    ctx->comm_if->send(ctx->comm_if->comm_cfg, &ack, 1);
}

void send_message(boot_handle_t *ctx, const char *msg) {
    ctx->comm_if->send(ctx->comm_if->comm_cfg, (const uint8_t *)msg, strlen(msg));
}

static fw_status_t wait_for_command(boot_handle_t *ctx, uint8_t expected,
                                    uint8_t ack, uint32_t timeout_ms) {
    uint8_t cmd;

    if (timeout_ms == 0) {
        // blocking behavior
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

    // time out behavior
    TIM2_SetTime(timeout_ms);
    TIM2_Start();
    while (!TIM2_IsTimeElapsed()) {
        if (ctx->comm_if->recv(ctx->comm_if->comm_cfg, &cmd, 1) == 1) {
            TIM2_Stop();
            if (cmd == expected) {
                send_ack(ctx, ack);
                return FW_OK;
            } else {
                return FW_ERR_INVALID_CMD;
            }
        }
        delay_ms(1);
    }
    TIM2_Stop();
    return FW_ERR_TIMEOUT_CMD; /* timeout */
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

static fw_status_t wait_and_erase(boot_handle_t *ctx, uint8_t flash_sector_num) {
    if (wait_for_command(ctx, ERASE_CMD, ERASE_ACK,3000) != FW_OK) return FW_ERR_INVALID_CMD;

    if (flash_erase_sector(flash_sector_num) != FLASH_OK) return FW_ERR_FLASH_ERASE;

    return FW_OK;
}

void enter_app(boot_handle_t *boot_ctx,uint32_t app_addr) {
    uint32_t app_msp = *(volatile uint32_t *)(app_addr + 0x00ul);
    uint32_t app_pc_init = *(volatile uint32_t *)(app_addr + 0x04ul); // reset handler's app

    // disable all interrupts
    NVIC_Disable_ISR(); 
    // set MSP
    __asm volatile("msr msp, %0" : : "r" (app_msp) : ); //base on reset handler set MSP or not
    // RCC deinit
    SystemCoreClock_DeInit();
    // set VTOR for application
    // SCB_VTOR = app_addr;
    // Call reset hanlder of application
    func_ptr app_entry = (func_ptr)app_pc_init;
    app_entry();
}

static fw_status_t wait_for_update_signal(boot_handle_t *ctx, uint32_t timeout_ms) {
    uint8_t cmd;

    if (timeout_ms > 0) {
        TIM2_SetTime(timeout_ms);
        TIM2_Start();
    }

    while (1) {
        if (ctx->comm_if->recv(ctx->comm_if->comm_cfg, &cmd, 1) == 1) {
            if (timeout_ms > 0) TIM2_Stop();
            if (cmd == UPDATE_FW_CMD1 || cmd == UPDATE_FW_CMD2) {
                send_ack(ctx, UPDATE_FW_ACK);
                // change boot mode to update
                ctx->state = BOOT_STATE_NORMAL_UPDATE;
                return FW_OK;
            }
            else if ( cmd == FORCE_UPDATE_FW_CMD1 || cmd == FORCE_UPDATE_FW_CMD2) {
                send_ack(ctx, UPDATE_FW_ACK);
                // change boot mode to force update
                ctx->state = BOOT_STATE_FORCE_UPDATE;
                return FW_OK;
            }
            else if (cmd == SKIP_FW_UPDATE_CMD1 || cmd == SKIP_FW_UPDATE_CMD2) {
                send_ack(ctx, SKIP_FW_UPDATE_ACK);
                // change boot mode to normal
                ctx->state = BOOT_STATE_JUMP_TO_APP;
                return FW_OK;
            } 
            else {
                return FW_ERR_INVALID_CMD;
            }
        }
        if (timeout_ms > 0 && TIM2_IsTimeElapsed()) {
            TIM2_Stop();
            ctx->state = BOOT_STATE_JUMP_TO_APP;
            return FW_ERR_TIMEOUT_CMD;
        }
        if (timeout_ms == 0) continue;
        delay_ms(1);
    }
}

fw_status_t receive_new_firmware(boot_handle_t *ctx, uint32_t flash_addr, uint32_t* fw_size) {
    // 1. Wait start
    if (wait_for_command(ctx, START_CMD, START_ACK,3000) != FW_OK) return FW_ERR_INVALID_CMD;

    // 2. Receive firmware size + ACK
    *fw_size = 0;
    if (recv_and_ack_size(ctx, fw_size, FW_SIZE_ACK) != FW_OK) return FW_ERR_COMM;

    // 3. Wait erase + erase
    if (wait_and_erase(ctx,flash_get_sector(flash_addr)) != FW_OK) return FW_ERR_FLASH_ERASE;

    // 4. Receive FW data chunks
    if (recv_and_write_chunks(ctx, flash_addr, *fw_size, CHUNK_ACK) != FW_OK) return FW_ERR_FLASH_WRITE;

    // 5. Wait for signature command + ACK
    if (wait_for_command(ctx, SIGNATURE_CMD, SIGNATURE_ACK,3000) != FW_OK) return FW_ERR_INVALID_CMD;
    // 6. Receive signature size + ACK
    uint32_t sig_size = 0;
    uint32_t sig_addr = flash_addr + *fw_size;

    if (recv_and_ack_size(ctx, &sig_size, SIGNATURE_ACK) != FW_OK)  return FW_ERR_COMM;

    // 6. Receive signature data
    if (recv_and_write_chunks(ctx, sig_addr, sig_size, SIGNATURE_ACK) != FW_OK) return FW_ERR_FLASH_WRITE;

    // 7. Store and write metadata
    fw_metadata_t metadata;
    metadata.version = FW_VERSION;
    metadata.fw_addr = flash_addr;
    metadata.fw_size = *fw_size;
    metadata.sig_addr = sig_addr;
    metadata.sig_len = sig_size;
    metadata.flags = FW_FLAG_VALID;

    if (flash_erase_sector(flash_get_sector(METADATA_ADDR)) != FLASH_OK) return FW_ERR_FLASH_ERASE; 
    if (flash_write_blk( METADATA_ADDR, (uint8_t *)&metadata, sizeof(fw_metadata_t)) != FLASH_OK)   return FW_ERR_FLASH_WRITE;
    
    return FW_OK;
}

fw_status_t receive_fw_update_request(boot_handle_t *boot_ctx) {
    TIM2_Init();
    // send_message(boot_ctx,"Waiting for firmware update command...\r\n");
    fw_status_t status = FW_ERR_COMM;
    status = wait_for_update_signal(boot_ctx, 5000);
    TIM2_Stop();
    TIM2_ClearFlag();
    return status;
}

fw_status_t handle_update_request(boot_handle_t *boot_ctx, uint32_t* fw_addr, uint32_t* fw_size ) {
    fw_status_t fw_st;
    switch (boot_ctx->state)
    {
        case BOOT_STATE_NORMAL_UPDATE:
            // receive new firmware into staging bank (inactive bank )
            *fw_addr = FW_STAGING_ADDR;
            fw_st = receive_new_firmware(boot_ctx, *fw_addr, fw_size);
            boot_ctx->state = BOOT_STATE_VERIFY_SIGNATURE;
            return fw_st;
            break;
        case BOOT_STATE_FORCE_UPDATE:
            // flash new firmwaare dirvectly to active bank
            *fw_addr = FW_FLASH_ADDR;
            fw_st = receive_new_firmware(boot_ctx, *fw_addr, fw_size);
            boot_ctx->state = BOOT_STATE_JUMP_TO_APP;
            return fw_st;
            break;
        case BOOT_STATE_JUMP_TO_APP:
            *fw_addr = read_fw_addr_from_flash();
            return FW_OK;
            break;
        default:
            return FW_ERR_INVALID_CMD;
            break;
    }
}

fw_status_t process_boot_state(boot_handle_t *boot_ctx, uint32_t* fw_addr, uint32_t *fw_size) {
    rsa_verify_result_t vr;
    int jump_to_app = 0;
    switch (boot_ctx->state)
    {
        case BOOT_STATE_VERIFY_SIGNATURE:
            *fw_size = read_fw_size_from_flash();
            vr = verify_firmware(*fw_addr, *fw_size);
            if (vr == RSA_VERIFY_OK) {
                // write firmware to active bank
                if(flash_copy_firmware(FW_STAGING_ADDR,FW_FLASH_ADDR,*fw_size)!= FLASH_OK) {
                    return FW_ERR_COPY_FW;
                }
                else {
                    *fw_addr = FW_FLASH_ADDR;
                    jump_to_app = 1;

                }
            } else {
                jump_to_app = 0;
            }
            break;
        case BOOT_STATE_FORCE_UPDATE:
            vr = RSA_VERIFY_OK;
            jump_to_app = 1;
            break;
        case BOOT_STATE_JUMP_TO_APP:
            *fw_size = read_fw_size_from_flash();
            vr = verify_firmware(*fw_addr, *fw_size);
            if (vr == RSA_VERIFY_OK) {
                jump_to_app = 1;
            } else {
                jump_to_app = 0;
            }
            break;
        default:
            return FW_ERR_INVALID_CMD;
            break;
    }
    if(jump_to_app) {
        enter_app(boot_ctx, *fw_addr);
        return FW_OK;
    } else {
        send_message(boot_ctx,"Signature is INVALID. \r\n");
        return FW_ERR_INVALID_SIGNATURE;
    }
}
