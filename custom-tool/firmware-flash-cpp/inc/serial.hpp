#pragma once
#include <windows.h>
#include <cstdint>
#include <string>

class serial
{
private:
    HANDLE hSerial;

public:
    serial();
    ~serial();

    bool open(const std::string &com_port, uint32_t baudrate);
    int write(const uint8_t *data, uint32_t len);
    int read(uint8_t *data, uint32_t len);
    void close();
};