#include <stdio.h>
#include <stdint.h>
#include "serial_com.h"

#define CHUNK_SIZE 256
#if 0
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

    printf("Sending firmware...\n");

    // Optional: handshake
    uint8_t start_cmd = 0x55;
    serial_write(&serial, &start_cmd, 1);

    uint8_t ack = 0;
    if (serial_read(&serial, &ack, 1) != 1 || ack != 0xAA) {
        printf("Error: No ACK from MCU\n");
        fclose(fw);
        serial_close(&serial);
        return -1;
    }

    uint8_t buffer[CHUNK_SIZE];
    size_t bytes;
    uint32_t offset = 0;

    while ((bytes = fread(buffer, 1, CHUNK_SIZE, fw)) > 0) {
        if (serial_write(&serial, buffer, bytes) != (int)bytes) {
            printf("Error: write failed at offset %lu\n", offset);
            break;
        }

        if (serial_read(&serial, &ack, 1) != 1 || ack != 0xCC) {
            printf("MCU failed at offset %lu\n", offset);
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
        printf("Usage: %s COM3 firmware.bin\n", argv[0]);
        return 0;
    }
    return send_firmware(argv[1], argv[2]);
}
#endif

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s COMx\n", argv[0]);
        return -1;
    }

    SerialHandle serial;
    if (serial_open(&serial, argv[1], 115200) != 0) {
        printf("Error: cannot open port %s\n", argv[1]);
        return -1;
    }

    printf("Port %s opened. Sending test data...\n", argv[1]);

    const char *msg = "HELLO_FROM_PC\n";
    serial_write(&serial, (const uint8_t*)msg, strlen(msg));
#ifdef TEST_RECV
    uint8_t buffer[64];
    int r = serial_read(&serial, buffer, sizeof(buffer) - 1);
    if (r > 0) {
        buffer[r] = 0; // null-terminate
        printf("Received: %s\n", buffer);
    } else {
        printf("No data received.\n");
    }
#endif //  TEST_RECV
    serial_close(&serial);
    return 0;
}