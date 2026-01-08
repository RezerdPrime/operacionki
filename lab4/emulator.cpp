#include "my_serial.hpp"
#include <iostream>
#include <random>
#include <cstdlib>
#include <thread>
#include <chrono>

void portable_sleep_ms(unsigned long ms) {
#if defined(_WIN32)
    Sleep(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <PORT>\n"; //
        return 1;
    }

    std::string port_name = argv[1];
    cplib::SerialPort port(port_name, cplib::SerialPort::BAUDRATE_9600);
    if (!port.IsOpen()) {
        std::cerr << "Can't open " << port_name << "\n";
        return 1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(20.0, 5.0);

    while (true) {
        double temp = d(gen);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f\n", temp);
        port.Write(buf);
        std::cout << "Sent: " << buf;
        portable_sleep_ms(5000);
    }

    return 0;
}