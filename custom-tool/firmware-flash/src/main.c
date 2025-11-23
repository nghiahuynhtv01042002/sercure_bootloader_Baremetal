#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "serial_com.h"
#include "firmware_sender.h"

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("  Firmware Upload Tool v1.0\n");
    printf("========================================\n\n");
    
    if (argc < 4) {
        printf("Usage: %s <COM_PORT> <FIRMWARE_FILE> <SIGNATURE>\n", argv[0]);
        printf("\nExample:\n");
        printf("  Windows: %s COM3 firmware.bin siganture.sig\n", argv[0]);
        return 1;
    }
    const char *com = argv[1];
    SerialHandle serial;
    uint32_t baudrate = 115200;
    printf("Opening serial port: %s @ %ld baud\n", com, baudrate);
    if (serial_open(&serial, com, baudrate) != 0) {
        printf("Error: Cannot open serial port %s\n", com);
        return -1;
    }
    // Small delay for serial port to stabilize
    sleep_ms(100);
    int result = send_update_signal(&serial);
    if(result != 0 ) {
        printf("\nFail to send signal update\n");
        return result;
    }
    result = send_firmware(&serial, com, argv[2],argv[3]);
    
    if (result == 0) {
        printf("\nFirmware upload successful!\n");
    } else {
        printf("\nFirmware upload failed!\n");
    }
    
    return result;
}