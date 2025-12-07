#include "serial.hpp"
#include  <string>
#include "serial.hpp"
#include <iostream>
#include <string>

serial::serial()
    : hSerial(INVALID_HANDLE_VALUE)
{
}

serial::~serial()
{
    close();
}

bool serial::open(const std::string &com_port, uint32_t baudrate)
{
    if (com_port.empty())
        return false;

    std::string full_port;
    if (com_port.rfind("\\\\.\\", 0) == std::string::npos) {
        full_port = "\\\\.\\" + com_port;
    } else {
        full_port = com_port;
    }

    hSerial = CreateFileA(
        full_port.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cout << "Error: cannot open " << full_port << "\n";
        return false;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(hSerial, &dcb)) {
        std::cout << "Error: cannot get COM state\n";
        close();
        return false;
    }

    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity   = NOPARITY;

    if (!SetCommState(hSerial, &dcb)) {
        std::cout << "Error: cannot set COM state\n";
        close();
        return false;
    }

    COMMTIMEOUTS to = {0};
    to.ReadIntervalTimeout         = 50;
    to.ReadTotalTimeoutConstant    = 50;
    to.ReadTotalTimeoutMultiplier  = 10;
    to.WriteTotalTimeoutConstant   = 50;
    to.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &to)) {
        std::cout << "Error: cannot set timeouts\n";
        close();
        return false;
    }

    return true;
}

int serial::write(const uint8_t *data, uint32_t len)
{
    if (hSerial == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    if (!WriteFile(hSerial, data, len, &written, nullptr))
        return -1;

    return (int)written;
}

int serial::read(uint8_t *data, uint32_t len)
{
    if (hSerial == INVALID_HANDLE_VALUE)
        return false;

    DWORD read_bytes = 0;
    if (!ReadFile(hSerial, data, len, &read_bytes, nullptr))
        return -1;

    return (int)read_bytes ;
}

void serial::close()
{
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
}

void serial::start_async_read() {
    if (running) return;
    running = true;

    reader = std::thread([this]() {
        uint8_t buf[128];

        while (running) {
            int n = read(buf, sizeof(buf));
            if (n > 0) {
                // filter ACK bytes (0x00..0x1F except '\r','\n')
                bool printable = true;
                for (int i = 0; i < n; i++) {
                    if (buf[i] < 0x20 && buf[i] != '\r' && buf[i] != '\n') {
                        printable = false;
                    }
                }

                if (printable) {
                    std::string s((char*)buf, (char*)buf + n);
                    std::cout << "[MCU] " << s;
                    std::cout.flush();
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });
}

void serial::stop_async_read() {
    running = false;
    if (reader.joinable())
        reader.join();
}
// void serial::start_user_input() {
//     std::thread([this]() {
//         while (true) {
//             std::string line;

//             if (!std::getline(std::cin, line)) {
//                 break;
//             }

//             line.push_back('\n');
//             this->write(reinterpret_cast<const uint8_t*>(line.c_str()),
//                         (uint32_t)line.size());
//         }
//     }).detach();
// }
