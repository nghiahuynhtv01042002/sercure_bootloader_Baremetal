#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#include <windows.h>
#include <stdint.h>

typedef struct {
    HANDLE hSerial;
} SerialHandle;

int serial_open(SerialHandle *h, const char *com_port, uint32_t baudrate);
int serial_write(SerialHandle *h, const uint8_t *data, uint32_t len);
int serial_read(SerialHandle *h, uint8_t *data, uint32_t len);
void serial_close(SerialHandle *h);

#endif
