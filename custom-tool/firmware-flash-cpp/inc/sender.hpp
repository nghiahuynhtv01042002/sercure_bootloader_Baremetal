#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include "serial.hpp"

class Sender {
public:
    explicit Sender(serial &ser);

    bool sendUpdateSignal();
    bool sendFirmware(const std::string &filepath, const std::string &sig_path);

private:
    serial &ser;

    void flushSerial(int timeout_ms);
    int  readWithTimeout(uint8_t *data, size_t len, int timeout_ms);
    bool sendCmdAndWaitAck(uint8_t cmd, uint8_t expected_ack, int timeout_ms);
    bool waitAck(uint8_t expected_ack, int timeout_ms);
    bool sendBlockAndWaitAck(const uint8_t *buf, size_t len, uint8_t expected_ack);
    bool send4BytesSize(uint32_t size, uint8_t expected_ack);
    bool sendDataChunk(FILE *fw, uint32_t file_size);
    bool sendSignature(const std::string &sig_path);
};
