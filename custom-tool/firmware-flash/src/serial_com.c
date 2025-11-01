#include "serial_com.h"
#include <stdio.h>
#include <windows.h>

int serial_open(SerialHandle *h, const char *com_port, uint32_t baudrate) {
    if (!h || !com_port) return -1;
    char full_port[32];
    if (strncmp(com_port, "\\\\.\\", 4) != 0) {
        snprintf(full_port, sizeof(full_port), "\\\\.\\%s", com_port);
        com_port = full_port;
    }
    h->hSerial = CreateFileA(
        com_port,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (h->hSerial == INVALID_HANDLE_VALUE) {
        printf("Error: cannot open %s\n", com_port);
        return -1;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(h->hSerial, &dcb)) {
        printf("Error: cannot get COM state\n");
        serial_close(h);
        return -1;
    }

    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity   = NOPARITY;

    if (!SetCommState(h->hSerial, &dcb)) {
        printf("Error: cannot set COM state\n");
        serial_close(h);
        return -1;
    }

    COMMTIMEOUTS to = {0};
    to.ReadIntervalTimeout         = 50;
    to.ReadTotalTimeoutConstant    = 50;
    to.ReadTotalTimeoutMultiplier  = 10;
    to.WriteTotalTimeoutConstant   = 50;
    to.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(h->hSerial, &to)) {
        printf("Error: cannot set timeouts\n");
        serial_close(h);
        return -1;
    }

    return 0;
}

int serial_write(SerialHandle *h, const uint8_t *data, uint32_t len) {
    if (!h || h->hSerial == INVALID_HANDLE_VALUE) return -1;

    DWORD written = 0;
    if (!WriteFile(h->hSerial, data, len, &written, NULL)) {
        return -1;
    }
    return (int)written;
}

int serial_read(SerialHandle *h, uint8_t *data, uint32_t len) {
    if (!h || h->hSerial == INVALID_HANDLE_VALUE) return -1;

    DWORD bytes = 0;
    if (!ReadFile(h->hSerial, data, len, &bytes, NULL)) {
        return -1;
    }
    return (int)bytes;
}

void serial_close(SerialHandle *h) {
    if (h && h->hSerial && h->hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(h->hSerial);
        h->hSerial = INVALID_HANDLE_VALUE;
    }
}
