#include <stdio.h>
#include <stdint.h>
#include "serial_com.h"

#define CHUNK_SIZE 256

int send_firmware(const char *com, const char *filepath) {
    SerialHandle serial;
    if (serial_open(&serial, com, 115200) != 0) {
        return -1;
    }

    FILE *fw = fopen(filepath, "rb");
    if (!fw) {
        printf("Error: cannot open firmware file: %s\n", filepath);
        serial_close(&serial);
        return -1;
    }

    // Lấy kích thước file
    fseek(fw, 0, SEEK_END);
    uint32_t file_size = ftell(fw);
    fseek(fw, 0, SEEK_SET);
    printf("Sending firmware size = %u bytes\n", file_size);

    // Gửi start command
    uint8_t start_cmd = 0x55;
    serial_write(&serial, &start_cmd, 1);

    uint8_t ack = 0;
    if (serial_read(&serial, &ack, 1) != 1 || ack != 0xAA) {
        printf("Error: No ACK from receiver\n");
        fclose(fw);
        serial_close(&serial);
        return -1;
    }

    // Gửi file size (4 bytes, little endian)
    serial_write(&serial, (uint8_t *)&file_size, sizeof(file_size));

    uint8_t buffer[CHUNK_SIZE];
    size_t bytes;
    uint32_t offset = 0;
    printf("Sending firmware...\n");

    while ((bytes = fread(buffer, 1, CHUNK_SIZE, fw)) > 0) {
        if (serial_write(&serial, buffer, bytes) != (int)bytes) {
            printf("Error: write failed at offset %lu\n", offset);
            break;
        }
        if (serial_read(&serial, &ack, 1) != 1 || ack != 0xCC) {
            printf("Receiver failed at offset %lu\n", offset);
            break;
        }
        offset += bytes;
    }

    printf("Done, total %lu bytes sent\n", offset);
    fclose(fw);
    serial_close(&serial);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s COMx firmware.bin\n", argv[0]);
        return 0;
    }
    return send_firmware(argv[1], argv[2]);
}
