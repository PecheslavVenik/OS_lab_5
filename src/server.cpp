#include "database.h"
#include "http_server.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdio>

class Server {
public:
    Server(const std::string& portFile, const std::string& dbPath) 
        : portFile(portFile), db(dbPath), httpServer(db, 8080), lastPosition(0) {}

    void run() {
        if (!db.initialize()) {
            std::cerr << "Ошибка инициализации БД" << std::endl;
            return;
        }

        httpServer.start();
        std::cout << "Сервер запущен" << std::endl;

        while (true) {
            readFromPort();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

private:
    std::string portFile;
    Database db;
    HttpServer httpServer;
    std::ifstream::pos_type lastPosition;

    bool parseTimestamp(const std::string& dateStr, const std::string& timeStr, time_t& outTime) {
        int day, month, year, hour, minute, second;
        if (sscanf(dateStr.c_str(), "%d.%d.%d", &day, &month, &year) != 3) return false;
        if (sscanf(timeStr.c_str(), "%d:%d:%d", &hour, &minute, &second) != 3) return false;
        struct tm tm_info = {};
        tm_info.tm_mday = day;
        tm_info.tm_mon = month - 1;
        tm_info.tm_year = year - 1900;
        tm_info.tm_hour = hour;
        tm_info.tm_min = minute;
        tm_info.tm_sec = second;
        tm_info.tm_isdst = -1;
        outTime = mktime(&tm_info);
        return outTime != -1;
    }

    void readFromPort() {
        std::ifstream portStream(portFile);
        if (!portStream.is_open()) return;

        portStream.seekg(0, std::ios::end);
        std::ifstream::pos_type endPos = portStream.tellg();
        if (endPos < lastPosition) lastPosition = 0;
        portStream.seekg(lastPosition);

        std::string line;
        while (std::getline(portStream, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string dateStr, timeStr;
            double temperature;
            if (iss >> dateStr >> timeStr >> temperature) {
                time_t ts;
                if (parseTimestamp(dateStr, timeStr, ts)) {
                    db.saveMeasurement(ts, temperature);
                    std::cout << "Saved: " << temperature << std::endl;
                }
            }
        }
        lastPosition = portStream.tellg();
        if (lastPosition == std::ifstream::pos_type(-1)) {
             portStream.clear();
             portStream.seekg(0, std::ios::end);
             lastPosition = portStream.tellg();
        }
    }
};

int main(int argc, char* argv[]) {
    std::string portFile = "virtual_port.txt";
    if (argc > 1) portFile = argv[1];
    Server server(portFile, "measurements.db");
    server.run();
    return 0;
}
