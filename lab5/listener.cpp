#include "my_serial.hpp"
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <sqlite3.h>
#include <thread>
#include <chrono>
#include <string>

void portable_sleep_ms(unsigned long ms) {
#if defined(_WIN32)
    Sleep(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

static int exec_sql(sqlite3* db, const char* sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << "\n";
        sqlite3_free(errmsg);
    }
    return rc;
}

static void insert_measurement(sqlite3* db, double temp) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT INTO main (temperature) VALUES (%.3f);", temp);
    exec_sql(db, sql);
}

static void insert_hourly_avg(sqlite3* db, const std::tm& hour, double avg) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT INTO hourly_avg (hour_start, avg_temp) VALUES ('%04d-%02d-%02d %02d:00:00', %.3f);",
        1900 + hour.tm_year, hour.tm_mon + 1, hour.tm_mday, hour.tm_hour, avg);
    exec_sql(db, sql);
}

static void insert_daily_avg(sqlite3* db, const std::tm& day, double avg) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT INTO daily_avg (day_start, avg_temp) VALUES ('%04d-%02d-%02d 00:00:00', %.3f);",
        1900 + day.tm_year, day.tm_mon + 1, day.tm_mday, avg);
    exec_sql(db, sql);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <PORT>\n";
        return 1;
    }

    sqlite3* db;
    if (sqlite3_open("build\\temperature.db", &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        return 1;
    }

    exec_sql(db, R"(
        CREATE TABLE IF NOT EXISTS main (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            temperature REAL NOT NULL
        );
    )");

    exec_sql(db, R"(
        CREATE TABLE IF NOT EXISTS hourly_avg (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hour_start DATETIME NOT NULL,
            avg_temp REAL NOT NULL
        );
    )");

    exec_sql(db, R"(
        CREATE TABLE IF NOT EXISTS daily_avg (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            day_start DATETIME NOT NULL,
            avg_temp REAL NOT NULL
        );
    )");

    std::string port_name = argv[1];
    cplib::SerialPort::Parameters params(cplib::SerialPort::BAUDRATE_9600);
    params.timeout = 1.0;

    cplib::SerialPort port;
    int err = port.Open(port_name, params);
    if (err != cplib::SerialPort::RE_OK) {
        std::cerr << "Failed to open port: " << err << "\n";
        sqlite3_close(db);
        return 1;
    }

    std::cout << "Listening on " << port_name << "...\n";

    std::vector<double> current_hour_temps;
    std::vector<double> current_day_temps;

    auto now = std::time(nullptr);
    std::tm now_tm = *std::localtime(&now);
    std::tm current_hour = now_tm;
    current_hour.tm_min = 0;
    current_hour.tm_sec = 0;
    std::tm current_day = now_tm;
    current_day.tm_hour = 0;
    current_day.tm_min = 0;
    current_day.tm_sec = 0;

    std::string last_line;

    while (true) {
        char buffer[2] = {0};
        size_t bytes_read = 0;
        std::string line;

        while (true) {
            int read_err = port.Read(buffer, 1, &bytes_read);
            if (read_err != cplib::SerialPort::RE_OK || bytes_read == 0) break;
            if (buffer[0] == '\n') break;
            if (buffer[0] != '\r') line += buffer[0];
        }

        if (line.empty()) {
            portable_sleep_ms(100);
            continue;
        }

        double temp_candidate;
        try {
            temp_candidate = std::stod(line);
        } catch (...) {
            last_line.clear();
            continue;
        }

        if (!last_line.empty()) {
            if ((last_line == line)) {
                double temp = temp_candidate;

                insert_measurement(db, temp);

                now = std::time(nullptr);
                now_tm = *std::localtime(&now);

                current_hour_temps.push_back(temp);
                current_day_temps.push_back(temp);

                std::tm hour_check = now_tm;
                hour_check.tm_min = 0;
                hour_check.tm_sec = 0;

                time_t t_hour_check = std::mktime(&hour_check);
                time_t t_current_boundary = std::mktime(&current_hour);

                if (t_hour_check > t_current_boundary) {
                    if (!current_hour_temps.empty()) {
                        double sum = 0.0;
                        for (double t : current_hour_temps) sum += t;
                        double avg = sum / current_hour_temps.size();
                        insert_hourly_avg(db, current_hour, avg);
                        current_hour_temps.clear();
                    }
                    current_hour = hour_check;
                }


                std::tm day_check = now_tm;
                day_check.tm_hour = 0;
                day_check.tm_min = 0;
                day_check.tm_sec = 0;

                time_t t_day_check = std::mktime(&day_check);
                t_current_boundary = std::mktime(&current_day);

                if (t_day_check > t_current_boundary) {
                    if (!current_day_temps.empty()) {
                        double sum = 0.0;
                        for (double t : current_day_temps) sum += t;
                        double avg = sum / current_day_temps.size();
                        insert_daily_avg(db, current_day, avg);
                        current_day_temps.clear();
                    }
                    current_day = day_check;
                }
            }
            last_line.clear();
        } else {
            last_line = line;
        }

        portable_sleep_ms(10);
    }

    port.Close();
    sqlite3_close(db);
    return 0;
}