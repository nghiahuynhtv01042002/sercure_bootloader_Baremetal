#include "boot_fw_update.h"
// wait for a specific command
// send acknowledgment

static void send_ack(boot_handle_t *ctx, uint8_t ack) {
    ctx->comm_if->send(ctx->comm_if->comm_cfg, &ack, 1);
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

fw_status_t receive_new_firmware(boot_handle_t *ctx, uint32_t flash_addr) {
    uint32_t fw_size = 0;
    uint8_t buffer[CHUNK_SIZE];
    
    // 1. Wait start command
    if (wait_for_command(ctx, START_CMD, START_ACK) != FW_OK) return FW_ERR_INVALID_CMD;

    // 2. Receive firmware size
    if (recv_data(ctx, (uint8_t *)&fw_size, sizeof(fw_size)) != FW_OK) return FW_ERR_COMM;
    uint8_t ack = SIZE_ACK;
    ctx->comm_if->send(ctx->comm_if->comm_cfg, &ack, 1);

    // 3. Wait erase command
    if (wait_for_command(ctx, ERASE_CMD, ERASE_ACK) != FW_OK) return FW_ERR_INVALID_CMD;
    if (flash_erase_sector(APP_SECTION_NUMBER) != FLASH_OK) return FW_ERR_FLASH_ERASE;

    // 4. Receive firmware chunks
    uint32_t received = 0;
    while (received < fw_size) {
        uint32_t chunk_size = (fw_size - received > CHUNK_SIZE) ? CHUNK_SIZE : (fw_size - received);
        if (recv_data(ctx, buffer, chunk_size) != FW_OK) return FW_ERR_COMM;
        if (flash_write_blk(flash_addr + received, buffer, chunk_size) != FW_OK) return FW_ERR_FLASH_WRITE;

        // ACK chunk
        send_ack(ctx, CHUNK_ACK);

        received += chunk_size;
    }

    return FW_OK;
}
