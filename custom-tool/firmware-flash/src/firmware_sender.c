#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "serial_com.h"
#include "firmware_sender.h"


// Bootloader protocol commands (PC → MCU)


static void flush_serial(SerialHandle *serial, int timeout_ms) {
    uint8_t dummy[128];
    DWORD start = GetTickCount();
    while (GetTickCount() - start < (DWORD)timeout_ms) {
        int r = serial_read(serial, dummy, sizeof(dummy));
        if (r <= 0) break;
        sleep_ms(5);
    }
}

static int read_with_timeout(SerialHandle *serial, uint8_t *data, size_t len, int timeout_ms) {
    DWORD start = GetTickCount();
    int total = 0;

    while (total < (int)len) {
        if ((int)(GetTickCount() - start) >= timeout_ms)
            return total;

        int n = serial_read(serial, data + total, len - total);

        if (n > 0)
            total += n;
        else if (n == 0)
            sleep_ms(5);
        else
            return -1;
    }
    return total;
}

static int send_cmd_and_wait_ack(SerialHandle *serial, uint8_t cmd,
                                 uint8_t expected_ack, int timeout_ms) {
    if (serial_write(serial, &cmd, 1) != 1) {
        printf("Error: failed to send CMD 0x%02X\n", cmd);
        return -1;
    }
    uint8_t ack = 0;
    if (read_with_timeout(serial, &ack, 1, timeout_ms) != 1 || ack != expected_ack) {
        printf("Error: ACK mismatch (got 0x%02X expected 0x%02X)\n", ack, expected_ack);
        return -1;
    }

    return 0;
}

static int wait_ack(SerialHandle *serial, uint8_t expected_ack, int timeout_ms) {
    uint8_t ack = 0;

    int ret = read_with_timeout(serial, &ack, 1, timeout_ms);
    if (ret != 1) {
        printf("Error: timeout waiting for ACK 0x%02X\n", expected_ack);
        return -1;
    }

    if (ack != expected_ack) {
        printf("Error: ACK mismatch (got 0x%02X, expected 0x%02X)\n",
               ack, expected_ack);
        return -1;
    }

    return 0;
}
static int send_block_and_wait_ack(SerialHandle *serial, const uint8_t *buf, 
            size_t len, uint8_t expected_ack) {
    if (serial_write(serial, buf, len) != (int)len) {
        printf("Error: Failed writing block\n");
        return -1;
    }

    uint8_t ack = 0;
    if (read_with_timeout(serial, &ack, 1, ACK_TIMEOUT_MS) != 1 ||ack != expected_ack) {
        printf("Error: Block ACK mismatch (got 0x%02X)\n", ack);
        return -1;
    }

    return 0;
}


static int send_4bytes_size(SerialHandle *serial, uint32_t size, uint8_t expected_ack) {
    if (serial_write(serial, (uint8_t *)&size, 4) != 4) {
        printf("Error: Failed to send size\n");
        return -1;
    }

    uint8_t ack = 0;
    if (read_with_timeout(serial, &ack, 1, ACK_TIMEOUT_MS) != 1 || ack != expected_ack) {
        printf("Error: Wrong ACK for size (got 0x%02X expected 0x%02X)\n", ack, expected_ack);
        return -1;
    }

    return 0;
}


static int send_data_chunk(SerialHandle *serial, FILE *fw, uint32_t file_size) {
    uint8_t buffer[CHUNK_SIZE];
    uint32_t sent = 0;

    while (1) {
        size_t n = fread(buffer, 1, CHUNK_SIZE, fw);
        if (n == 0) break;

        if (send_block_and_wait_ack(serial, buffer, n, CHUNK_ACK) < 0) return -1;
        sent += n;
        int percent = (sent * 100) / file_size;
        printf("\r[DATA] %3d%% (%u/%u bytes)", percent, sent, file_size);
        fflush(stdout);
    }

    printf("\n[DATA] Completed\n");
    return 0;
}

int send_update_signal(SerialHandle *serial) {
    flush_serial(serial, 300);

    printf("Sending firmware update signal...\n");
    if (send_cmd_and_wait_ack(serial, UPDATE_FW_CMD, UPDATE_FW_ACK, ACK_TIMEOUT_MS) < 0) {
        printf("Error: Failed to send update signal\n");
        return -1;
    }
    printf("Update signal acknowledged by device.\n");
    return 0;
}

static int send_signature(SerialHandle *serial, const char *sig_path) {
    FILE *fs = fopen(sig_path, "rb");
    if (!fs) {
        printf("Warning: signature file not found. Skipped.\n");
        return 0; // not fatal
    }

    fseek(fs, 0, SEEK_END);
    uint32_t sig_size = ftell(fs);
    fseek(fs, 0, SEEK_SET);
    if (sig_size < 256) return -1;

    printf("\n[SIG] Sending signature cmd\n", sig_size);
    // notify MCU we will send signature
    if (send_cmd_and_wait_ack(serial, SIGNATURE_CMD, SIGNATURE_ACK, ACK_TIMEOUT_MS) < 0)
        return -1;
    // Send signature size
    printf("\n[SIG] Sending signature (%u bytes)\n", sig_size);

    if (send_4bytes_size(serial, sig_size,SIGNATURE_ACK) < 0)
        return -1;

    uint8_t *buf = malloc(sig_size);
    fread(buf, 1, sig_size, fs);
    fclose(fs);

    if (send_block_and_wait_ack(serial, buf, sig_size, SIGNATURE_ACK) < 0) {
        free(buf);
        return -1;
    }

    free(buf);
    printf("[SIG] Sending signature OK.\n");
    return 0;
}

int send_firmware(SerialHandle *serial, const char *com, const char *filepath, const char *sig_path) {
    flush_serial(serial, 300);

    FILE *fw = fopen(filepath, "rb");
    if (!fw) {
        printf("Error: cannot open %s\n", filepath);
        return -1;
    }

    fseek(fw, 0, SEEK_END);
    uint32_t file_size = ftell(fw);
    fseek(fw, 0, SEEK_SET);


    // Step 1: tell MCU to start FW update
    if (send_cmd_and_wait_ack(serial, START_CMD, START_ACK, ACK_TIMEOUT_MS) < 0)
        goto fail;
        
    printf("\nFirmware: %s (%u bytes)\n", filepath, file_size);
    // Step 2: send file size
    if (send_4bytes_size(serial, file_size,FW_SIZE_ACK) < 0)
        goto fail;

    // Step 3: erase flash
    if (send_cmd_and_wait_ack(serial, ERASE_CMD, ERASE_ACK, ACK_TIMEOUT_MS) < 0)
        goto fail;

    // Step 4: send the firmware in chunks
    if (send_data_chunk(serial, fw, file_size) < 0)
        goto fail;

    // Step 5: optional signature
    if(send_signature(serial, sig_path)< 0)
        goto fail;

    printf("\n[OK] Firmware upload complete.\n");

    fclose(fw);
    serial_close(serial);
    return 0;

fail:
    fclose(fw);
    serial_close(serial);
    return -1;
}
