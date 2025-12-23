#include "sender.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>

// ====== Define bootloader constants (add your own) ======



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
// ========================================================

static void sleep_ms(int ms) {
    Sleep(ms);
}

Sender::Sender(serial &s)
    : ser(s) {}

void Sender::flushSerial(int timeout_ms)
{
    uint8_t dummy[128];
    DWORD start = GetTickCount();

    while (GetTickCount() - start < (DWORD)timeout_ms) {
        if (!ser.read(dummy, sizeof(dummy)))
            break;
        sleep_ms(5);
    }
}

int Sender::readWithTimeout(uint8_t *data, size_t len, int timeout_ms)
{
    DWORD start = GetTickCount();
    int total = 0;

    while (total < (int)len) {

        if (GetTickCount() - start >= (DWORD)timeout_ms)
            return total;

        int n = ser.read(data + total, len - total);
        if (n > 0)
            total += n;
        else if (n == 0)
            sleep_ms(5);
        else
            return -1;
    }
    return total;
}

bool Sender::sendCmdAndWaitAck(uint8_t cmd, uint8_t expected_ack, int timeout_ms)
{
    if (ser.write(&cmd, 1) != 1) {
        std::cerr << "Failed sending CMD 0x" << std::hex << (int)cmd << "\n";
        return false;
    }

    uint8_t ack = 0;
    if (readWithTimeout(&ack, 1, timeout_ms) != 1 || ack != expected_ack) {
        std::cerr << "ACK mismatch. Got=0x" << std::hex << (int)ack
                  << " expected=0x" << (int)expected_ack << "\n";
        return false;
    }
    return true;
}

bool Sender::waitAck(uint8_t expected_ack, int timeout_ms)
{
    uint8_t ack = 0;
    if (readWithTimeout(&ack, 1, timeout_ms) != 1 || ack != expected_ack) {
        std::cerr << "ACK mismatch. Got=0x" << std::hex << (int)ack
                  << " expected=0x" << (int)expected_ack << "\n";
        return false;
    }
    return true;
}

bool Sender::sendBlockAndWaitAck(const uint8_t *buf, size_t len, uint8_t expected_ack)
{
    if (!ser.write(buf, len)) {
        std::cerr << "Failed to write block\n";
        return false;
    }

    uint8_t ack = 0;
    if (readWithTimeout(&ack, 1, ACK_TIMEOUT_MS) != 1 || ack != expected_ack) {
        std::cerr << "Block ACK mismatch. Got=0x" << std::hex << (int)ack << "\n";
        return false;
    }

    return true;
}

bool Sender::send4BytesSize(uint32_t size, uint8_t expected_ack)
{
    if (!ser.write(reinterpret_cast<uint8_t*>(&size), 4)) {
        std::cerr << "Failed sending size\n";
        return false;
    }

    uint8_t ack = 0;
    if (readWithTimeout(&ack, 1, ACK_TIMEOUT_MS) != 1 || ack != expected_ack) {
        std::cerr << "ACK mismatch for size\n";
        return false;
    }
    return true;
}

bool Sender::sendDataChunk(FILE *fw, uint32_t file_size)
{
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    uint32_t sent = 0;

    while (true) {
        size_t n = fread(buffer.data(), 1, CHUNK_SIZE, fw);
        if (n == 0)
            break;

        if (!sendBlockAndWaitAck(buffer.data(), n, CHUNK_ACK))
            return false;

        sent += n;
        int percent = (sent * 100) / file_size;

        std::cout << "\r[DATA] " << percent << "% (" << sent << "/" << file_size << " bytes)";
        std::cout.flush();
    }

    std::cout << "\n[DATA] Completed\n";
    return true;
}

bool Sender::sendSignature(const std::string &sig_path)
{
    FILE *fs = fopen(sig_path.c_str(), "rb");
    if (!fs) {
        std::cout << "[SIG] Signature not found, skipping\n";
        return true;
    }

    fseek(fs, 0, SEEK_END);
    uint32_t sig_size = ftell(fs);
    fseek(fs, 0, SEEK_SET);

    if (sig_size < 256) {
        std::cerr << "Signature too small\n";
        fclose(fs);
        return false;
    }

    std::cout << "[SIG] Sending signature command\n";
    if (!sendCmdAndWaitAck(SIGNATURE_CMD, SIGNATURE_ACK, ACK_TIMEOUT_MS)) {
        fclose(fs);
        return false;
    }

    std::cout << "[SIG] Sending signature size (" << sig_size << " bytes)\n";
    if (!send4BytesSize(sig_size, SIGNATURE_ACK)) {
        fclose(fs);
        return false;
    }

    std::vector<uint8_t> buf(sig_size);
    fread(buf.data(), 1, sig_size, fs);
    fclose(fs);

    if (!sendBlockAndWaitAck(buf.data(), sig_size, SIGNATURE_ACK))
        return false;

    std::cout << "[SIG] OK\n";
    return true;
}

bool Sender::sendUpdateSignal(uint8_t cmd)
{
    flushSerial(300);

    std::cout << "Sending update signal...\n";
    if (!sendCmdAndWaitAck(cmd, UPDATE_FW_ACK, ACK_TIMEOUT_MS)) {
        std::cerr << "Update signal failed\n";
        return false;
    }

    std::cout << "Device acknowledged update\n";
    return true;
}

bool Sender::sendFirmware(const std::string &filepath,
                                  const std::string &sig_path)
{
    flushSerial(300);

    FILE *fw = fopen(filepath.c_str(), "rb");
    if (!fw) {
        std::cerr << "Cannot open file " << filepath << "\n";
        return false;
    }

    fseek(fw, 0, SEEK_END);
    uint32_t file_size = ftell(fw);
    fseek(fw, 0, SEEK_SET);

    // 1. Start CMD
    if (!sendCmdAndWaitAck(START_CMD, START_ACK, ACK_TIMEOUT_MS))
        goto fail;

    std::cout << "Firmware: " << filepath << " (" << file_size << " bytes)\n";

    // 2. Send size
    if (!send4BytesSize(file_size, FW_SIZE_ACK))
        goto fail;

    // 3. Erase
    if (!sendCmdAndWaitAck(ERASE_CMD, ERASE_ACK, ACK_TIMEOUT_MS))
        goto fail;

    // 4. Send chunks
    if (!sendDataChunk(fw, file_size))
        goto fail;

    // 5. Send signature
    if (!sig_path.empty())
        if (!sendSignature(sig_path))
            goto fail;

    std::cout << "[OK] Firmware upload complete.\n";

    fclose(fw);
    ser.close();
    return true;

fail:
    fclose(fw);
    ser.close();
    return false;
}
