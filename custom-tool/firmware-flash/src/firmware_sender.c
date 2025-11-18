#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "serial_com.h"

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#endif

#define CHUNK_SIZE 256
#define MAX_RETRIES 3
#define ACK_TIMEOUT_MS 10000

// Flush serial buffer to clear debug messages
static void flush_serial(SerialHandle *serial, int timeout_ms) {
    uint8_t dummy[128];
    DWORD start_time = GetTickCount(); 
    DWORD current_time;
    while ( (current_time = GetTickCount()) - start_time < timeout_ms ) {
        if (serial_read(serial, dummy, sizeof(dummy)) <= 0) {
            break; 
        }
        sleep_ms(10); 
    }
}

static int read_with_timeout(SerialHandle *serial, uint8_t *data, size_t len, int timeout_ms) {
    DWORD start_time = GetTickCount(); 
    DWORD current_time;
    int total_read = 0;
    while (total_read < len) {
        current_time = GetTickCount();
        if ((int)(current_time - start_time) >= timeout_ms) {
            break; 
        }
        int bytes_to_read = len - total_read;
        int n = serial_read(serial, data + total_read, bytes_to_read);
        
        if (n > 0) {
            total_read += n;
            
        } else if (n == 0) {
            sleep_ms(10); 
            
        } else { 
            return -1; 
        }
    }
    return total_read; 
}

int send_firmware(SerialHandle *serial, const char *com, const char *filepath) {

    
    // Flush any existing data
    flush_serial(serial, 500);

    FILE *fw = fopen(filepath, "rb");
    if (!fw) {
        printf("Error: Cannot open firmware file: %s\n", filepath);
        serial_close(serial);
        return -1;
    }

    // Get file size
    fseek(fw, 0, SEEK_END);
    uint32_t file_size = ftell(fw);
    fseek(fw, 0, SEEK_SET);
    printf("Firmware file: %s\n", filepath);
    printf("Firmware size: %u bytes\n", file_size);

    // Step 1: Send start command (0x55)
    printf("\n[1/5] Sending start command...\n");
    uint8_t start_cmd = 0x55;
    if (serial_write(serial, &start_cmd, 1) != 1) {
        printf("Error: Failed to send start command\n");
        fclose(fw);
        serial_close(serial);
        return -1;
    }

    // Wait for ACK (0xAA)
    uint8_t ack = 0;
    printf("[1/5] Waiting for ACK...\n");
    if (read_with_timeout(serial, &ack, 1, ACK_TIMEOUT_MS) != 1 || ack != 0xAA) {
        printf("Error: No ACK received (got 0x%02X, expected 0xAA)\n", ack);
        fclose(fw);
        serial_close(serial);
        return -1;
    }
    printf("[1/5] start cmd: ACK received successfully 0x%02X\n",ack);

    // Step 2: Send firmware size
    printf("\n[2/5] Sending firmware size (%u bytes)...\n", file_size);
    sleep_ms(50); // Small delay before sending size
    
    if (serial_write(serial, (uint8_t *)&file_size, 4) != 4) {
        printf("Error: Failed to send firmware size\n");
        fclose(fw);
        serial_close(serial);
        return -1;
    }

    ack = 0;
    if (read_with_timeout(serial, &ack, 1, ACK_TIMEOUT_MS) != 1 || ack != 0xAA) {
        printf("Error: No ACK received (got 0x%02X, expected 0xAA)\n", ack);
        fclose(fw);
        serial_close(serial);
        return -1;
    }
    printf("[2/5] Sending firmware size: ACK received successfully: 0x%02X\n",ack);
  
    // Step 3: Send erase command (0xEC)
    printf("\n[3/5] Sending erase command...\n");
    uint8_t erase_cmd = 0xEC;
    if (serial_write(serial, &erase_cmd, 1) != 1) {
        printf("Error: Failed to send erase command\n");
        fclose(fw);
        serial_close(serial);
        return -1;
    }

    // Wait for ACK (0xAB)
    ack = 0;
    printf("[3/5] Waiting for ACK...\n");
    if (read_with_timeout(serial, &ack, 1, ACK_TIMEOUT_MS) != 1 || ack != 0xAB) {
        printf("Error: No ACK received (got 0x%02X, expected 0xAA)\n", ack);
        fclose(fw);
        serial_close(serial);
        return -1;
    }
    printf("[3/5] erase cmd: ACK received successfully 0x%02X\n",ack);

    // Step 3: Send firmware data in chunks
    printf("\n[4/5] Sending firmware data...\n");
    uint8_t buffer[CHUNK_SIZE];
    size_t bytes;
    uint32_t offset = 0;
    uint32_t chunk_count = 0;
    
    while ((bytes = fread(buffer, 1, CHUNK_SIZE, fw)) > 0) {
        // Send chunk
        if (serial_write(serial, buffer, bytes) != (int)bytes) {
            printf("\nError: Write failed at offset %u\n", offset);
            fclose(fw);
            serial_close(serial);
            return -1;
        }
        // Wait for chunk ACK (0xCC)
        ack = 0;
        if (read_with_timeout(serial, &ack, 1, ACK_TIMEOUT_MS) != 1 || ack != 0xCC) {
            printf("\nError: No chunk ACK at offset %u (got 0x%02X, expected 0xCC)\n", 
                   offset, ack);
            fclose(fw);
            serial_close(serial);
            return -1;
        }
        offset += bytes;
        chunk_count++;
        // Progress indicator
        int progress = (offset * 100) / file_size;
        printf("\r[4/5] Progress: %3d%% (%u / %u bytes, %u chunks)", 
               progress, offset, file_size, chunk_count);
        fflush(stdout);
    }
    printf("\n[4/5] All data sent successfully\n");

    // Step 4: Verify completion
    printf("\n[5/5] Firmware upload complete!\n");
    printf("Total bytes sent: %u\n", offset);
    printf("Total chunks: %u\n", chunk_count);
    
    fclose(fw);
    
    // Wait a bit to see final messages from device
    sleep_ms(500);
    
    // Read and display any final messages from bootloader
    uint8_t msg_buffer[256];
    int msg_len = serial_read(serial, msg_buffer, sizeof(msg_buffer) - 1);
    if (msg_len > 0) {
        msg_buffer[msg_len] = '\0';
        printf("\nDevice response:\n%s\n", msg_buffer);
    }
    
    serial_close(serial);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("  Firmware Upload Tool v1.0\n");
    printf("========================================\n\n");
    
    if (argc < 3) {
        printf("Usage: %s <COM_PORT> <FIRMWARE_FILE>\n", argv[0]);
        printf("\nExample:\n");
        printf("  Windows: %s COM3 firmware.bin\n", argv[0]);
        return 1;
    }
    const char *com = argv[1];
    SerialHandle serial;
    uint32_t baudrate = 115200;
    printf("Opening serial port: %s @ 115200 baud\n", com);
    if (serial_open(&serial, com, baudrate) != 0) {
        printf("Error: Cannot open serial port %s\n", com);
        return -1;
    }
    // Small delay for serial port to stabilize
    sleep_ms(100);

    int result = send_firmware(&serial, com, argv[2]);
    
    if (result == 0) {
        printf("\nFirmware upload successful!\n");
    } else {
        printf("\nFirmware upload failed!\n");
    }
    
    return result;
}