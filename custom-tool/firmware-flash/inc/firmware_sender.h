#ifndef FIMRWARE_SENDER_H
#define FIMRWARE_SENDER_H
#include <stdint.h>
#include "serial_com.h"

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#endif

#define START_CMD        (0x55)
#define START_ACK        (0xAA)


#define ERASE_CMD        (0xEC)
#define ERASE_ACK        (0x13)

#define FW_SIZE_ACK      (0xAA)   

#define CHUNK_ACK        (0xCC) 
#define CHUNK_SIZE       256

#define SIGNATURE_CMD    (0x53)
#define SIGNATURE_ACK    (0xAC)

#define UPDATE_FW_CMD    (0x70)
#define UPDATE_FW_ACK    (0x8F)

#define MAX_RETRIES      3
#define ACK_TIMEOUT_MS   10000

int send_firmware(SerialHandle *serial, const char *com, const char *filepath, const char *sig_path);
int send_update_signal(SerialHandle *serial);
static int send_signature(SerialHandle *serial, const char *sig_path);
#endif // FIMRWARE_SENDER_H