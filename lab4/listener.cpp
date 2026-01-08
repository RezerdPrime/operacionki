#include "my_serial.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstdlib>


void portable_sleep_ms(unsigned long ms) {
#if defined(_WIN32)
    Sleep(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

bool parse_datetime(const std::string& line, std::tm& tm_out, double& temp) {
    if (line.size() < 20) return false;
    std::istringstream ss(line.substr(0, 19));
    ss >> std::get_time(&tm_out, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) return false;
    try {
        temp = std::stod(line.substr(20));
    } catch (...) {
        return false;
    }
    return true;
}

std::string current_datetime_str() {
    auto t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

time_t datetime_to_time_t(const std::tm& tm) {
    std::tm tmp = tm;
    return std::mktime(&tmp);
}

void trim_log_file(const std::string& filename, time_t cutoff_time) {
    std::vector<std::string> valid_lines;
    std::ifstream in(filename);
    std::string line;
    while (std::getline(in, line)) {
        std::tm tm{};
        double temp;
        if (parse_datetime(line, tm, temp)) {
            time_t t = datetime_to_time_t(tm);
            if (t >= cutoff_time) {
                valid_lines.push_back(line);
            }
        }
    }
    in.close();

    std::ofstream out(filename);
    for (const auto& l : valid_lines) {
        out << l << '\n';
    }
}

int main(int argc, char* argv[]) { // мб порт менять придется
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <PORT>\n";
        return 1;
    }

    std::string port_name = argv[1];
    cplib::SerialPort::Parameters params(cplib::SerialPort::BAUDRATE_9600);
    params.timeout = 1.0;

    cplib::SerialPort port;
    int err = port.Open(port_name, params);
    if (err != cplib::SerialPort::RE_OK) {
        std::cerr << "Failed to open port: " << err << "\n";
        return 1;
    }

    std::cout << "Listening on " << port_name << "...\n";

    std::vector<std::pair<time_t, double>> current_hour_data;
    std::vector<std::pair<time_t, double>> current_day_data;

    std::tm current_hour = {};
    std::tm current_day = {};

    auto t_now = std::time(nullptr);
    std::tm tm_now = *std::localtime(&t_now);
    current_hour = tm_now;
    current_hour.tm_min = 0;
    current_hour.tm_sec = 0;
    current_day = tm_now;
    current_day.tm_hour = 0;
    current_day.tm_min = 0;
    current_day.tm_sec = 0;

    while (true) {
        std::string data;
        size_t bytes_read = 0;
        char buffer[256] = {0};

        std::string line;
        while (true) {
            int err = port.Read(buffer, 1, &bytes_read);
            if (err != cplib::SerialPort::RE_OK || bytes_read == 0) break;
            if (buffer[0] == '\n') break;
            if (buffer[0] != '\r') line += buffer[0];
        }

        if (line.empty()) {
            portable_sleep_ms(100);
            continue;
        }

        double temp;
        try {
            temp = std::stod(line);
        } catch (...) {
            continue;
        }

        auto now_time = std::time(nullptr);
        std::tm now_tm = *std::localtime(&now_time);

        std::ofstream main_log("main.log", std::ios::app);
        main_log << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << " " << temp << "\n";
        main_log.close();

        auto cutoff_24h = now_time - 24 * 3600;
        trim_log_file("main.log", cutoff_24h);

        current_hour_data.emplace_back(now_time, temp);
        current_day_data.emplace_back(now_time, temp);

        std::tm hour_start = now_tm;
        hour_start.tm_min = 0;
        hour_start.tm_sec = 0;
        time_t hour_start_time = std::mktime(&hour_start);
        time_t current_hour_time = std::mktime(&current_hour);

        if (hour_start_time > current_hour_time) {

            if (!current_hour_data.empty()) {
                double sum = 0.0;
                for (const auto& p : current_hour_data) sum += p.second;
                double avg = sum / current_hour_data.size();

                std::ofstream hourly_log("hourly_avg.log", std::ios::app);
                hourly_log << "\n" << std::put_time(&current_hour, "%Y-%m-%d %H:00:00") << " " << avg;// << "\n";
                hourly_log.close();

                auto cutoff_month = now_time - 30 * 24 * 3600;
                trim_log_file("hourly_avg.log", cutoff_month);
            }
            current_hour = hour_start;
            current_hour_data.clear();
        }

        std::tm day_start = now_tm;
        day_start.tm_hour = 0;
        day_start.tm_min = 0;
        day_start.tm_sec = 0;
        time_t day_start_time = std::mktime(&day_start);
        time_t current_day_time = std::mktime(&current_day);

        if (day_start_time > current_day_time) {

            if (!current_day_data.empty()) {
                double sum = 0.0;
                for (const auto& p : current_day_data) sum += p.second;
                double avg = sum / current_day_data.size();

                std::ofstream daily_log("daily_avg.log", std::ios::app);
                daily_log << std::put_time(&current_day, "%Y-%m-%d 00:00:00") << " " << avg << "\n";
                daily_log.close();

                int current_year = now_tm.tm_year + 1900;
                std::vector<std::string> valid_lines;
                std::ifstream in("daily_avg.log");
                std::string l;
                while (std::getline(in, l)) {
                    if (l.size() >= 4) {
                        try {
                            int year = std::stoi(l.substr(0, 4));
                            if (year == current_year) {
                                valid_lines.push_back(l);
                            }
                        } catch (...) {}
                    }
                }
                in.close();
                std::ofstream out("daily_avg.log");
                for (const auto& vl : valid_lines) out << vl << '\n';
            }
            current_day = day_start;
            current_day_data.clear();
        }

        portable_sleep_ms(10);
    }

    port.Close();
    return 0;
}