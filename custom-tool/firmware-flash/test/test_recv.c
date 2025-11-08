#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include "serial_com.h"

#define CHUNK_SIZE 256

int receive_firmware(const char *com, const char *output_file) {
    SerialHandle serial;
    if (serial_open(&serial, com, 115200) != 0) {
        printf("Error: cannot open COM port: %s\n", com);
        return -1;
    }

    FILE *fw_copy = fopen(output_file, "wb");
    if (!fw_copy) {
        printf("Error: cannot create file: %s\n", output_file);
        serial_close(&serial);
        return -1;
    }

    printf("Waiting for start (0x55)...\n");
    uint8_t cmd = 0;
    while (serial_read(&serial, &cmd, 1) != 1 || cmd != 0x55) {
        Sleep(1);
    }
    printf(">> Start command received!\n");

    // send back ACK
    uint8_t ack = 0xAA;
    serial_write(&serial, &ack, 1);

    // recieve 4 bytes file size
    uint32_t file_size = 0;
    if (serial_read(&serial, (uint8_t*)&file_size, 4) != 4) {
        printf("Error: cannot read file size\n");
        fclose(fw_copy);
        serial_close(&serial);
        return -1;
    }
    printf("Expecting %u bytes...\n", file_size);

    uint8_t buffer[CHUNK_SIZE];
    uint32_t total = 0;

    while (total < file_size) {
        int to_read = (file_size - total > CHUNK_SIZE) ? CHUNK_SIZE : (file_size - total);
        int bytes = serial_read(&serial, buffer, to_read);
        if (bytes > 0) {
            fwrite(buffer, 1, bytes, fw_copy);
            total += bytes;
            uint8_t ok = 0xCC;
            serial_write(&serial, &ok, 1);
        }
    }

    printf("Done: received %u bytes, saved to %s\n", total, output_file);
    fclose(fw_copy);
    serial_close(&serial);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s COMx output.bin\n", argv[0]);
        return 0;
    }
    return receive_firmware(argv[1], argv[2]);
}
