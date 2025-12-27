#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "serial.hpp"
#include "sender.hpp"

static void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " -p <PORT> -i <FW_FILE> -s <SIG_FILE>\n"
              << "Options:\n"
              << "  -p : Serial port (e.g., COM3 or /dev/ttyUSB0)\n"
              << "  -i : Firmware binary file (.bin)\n"
              << "  -s : Signature file (.sig)\n";
}

int main(int argc, char *argv[])
{
    std::cout << "========================================\n";
    std::cout << "  Firmware Upload Tool v1.0\n";
    std::cout << "========================================\n\n";

    std::string com_port, fw_path, sig_path;

    // --- Parse Arguments ---
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) com_port = argv[++i];
        else if (arg == "-i" && i + 1 < argc) fw_path = argv[++i];
        else if (arg == "-s" && i + 1 < argc) sig_path = argv[++i];
    }

    if (com_port.empty() || fw_path.empty() || sig_path.empty()) {
        print_usage(argv[0]);
        return 1;
    }

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
    std::cout <<  "Wait signal from MCU...\n";
    if (sender.waitAck(0x88, 10000) == false) {
        std::cout << "Error: No signal from MCU\n";
        return -4;
    }
    uint8_t cmd;
    std::cout << "Sending update signal...\n"
          << "Enter command:\n"
          << "  y/Y - normal update\n"
          << "  f/F - force update\n"
          << "  s/S - skip update\n> ";
    std::cout << "Enter command: ";
    std::cin.get(reinterpret_cast<char&>(cmd));
    // if skip, just exit

    if (!sender.sendUpdateSignal(cmd)) {
        std::cout << "Failed to send update signal\n";
        return -2;
    }

    if (cmd == 's' || cmd == 'S') {
        std::cout << "Skipping firmware update.\n";
        std::cout << "\nMCU Log:\n";
        sender.printMcuLog(3000);
        ser.close();
        return 0;
    }

    if (!sender.sendFirmware(fw_path, sig_path)) {
        std::cout << "Firmware upload failed!\n";
        return -3;
    }

    std::cout << "Firmware upload successful!\n";

    std::cout << "\nMCU Log:\n";
    sender.printMcuLog(3000);
    ser.close();
    return 0;
}
