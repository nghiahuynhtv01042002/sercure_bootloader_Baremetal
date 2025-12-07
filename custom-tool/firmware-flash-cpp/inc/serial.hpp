#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <thread>
#include <atomic>
class serial
{
private:
    HANDLE hSerial;
    std::thread reader;
    std::atomic<bool> running{false};

public:
    serial();
    ~serial();

    bool open(const std::string &com_port, uint32_t baudrate);
    int write(const uint8_t *data, uint32_t len);
    int read(uint8_t *data, uint32_t len);
    void close();

    void start_async_read();
    void stop_async_read();
};