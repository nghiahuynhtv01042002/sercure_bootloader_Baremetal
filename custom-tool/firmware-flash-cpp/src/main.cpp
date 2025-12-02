#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "serial.hpp"
#include "sender.hpp"

static void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main(int argc, char *argv[])
{
    std::cout << "========================================\n";
    std::cout << "  Firmware Upload Tool v1.0\n";
    std::cout << "========================================\n\n";

    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <COM_PORT> <FIRMWARE_FILE> <SIGNATURE>\n\n";
        std::cout << "Example:\n";
        std::cout << "  Windows: " << argv[0] << " COM3 firmware.bin signature.sig\n";
        return 1;
    }

    std::string com_port = argv[1];
    std::string fw_path  = argv[2];
    std::string sig_path = argv[3];

    serial ser;
    uint32_t baudrate = 115200;

    std::cout << "Opening serial port: " << com_port
              << " @ " << baudrate << " baud\n";

    if (!ser.open(com_port, baudrate)) {
        std::cout << "Error: Cannot open serial port " << com_port << "\n";
        return -1;
    }

    sleep_ms(100);

    Sender sender(ser);

    // 1. Send update signal
    if (!sender.sendUpdateSignal()) {
        std::cout << "Failed to send update signal\n";
        return -2;
    }

    // 2. Send firmware and signature
    if (!sender.sendFirmware(fw_path, sig_path)) {
        std::cout << "Firmware upload failed!\n";
        return -3;
    }

    std::cout << "\nFirmware upload successful!\n";
    return 0;
}
