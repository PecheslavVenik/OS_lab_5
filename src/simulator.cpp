#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <iomanip>
#include <string>
#include <cstring>

class TemperatureDevice {
private:
    double baseTemperature;
    std::ofstream logStream;

public:
    TemperatureDevice(const std::string& portFile) : baseTemperature(20.0) {
        logStream.open(portFile, std::ios::out | std::ios::trunc);
        if (!logStream.is_open()) {
            throw std::runtime_error("Не удалось открыть файл порта");
        }
    }

    ~TemperatureDevice() {
        if (logStream.is_open()) {
            logStream.close();
        }
    }

    std::string formatTime(time_t timestamp) {
        char buffer[80];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", timeinfo);
        return std::string(buffer);
    }

    double generateTemperature() {
        double variation = ((rand() % 100) - 50) / 10.0;
        baseTemperature += variation * 0.1;

        if (baseTemperature < -10.0) baseTemperature = -10.0;
        if (baseTemperature > 40.0) baseTemperature = 40.0;

        return baseTemperature;
    }

    void sendTemperature() {
        double temp = generateTemperature();
        time_t now = time(nullptr);

        logStream << formatTime(now) << " " << std::fixed << std::setprecision(2)
                  << temp << std::endl;
        logStream.flush();

        std::cout << "Отправлено: " << temp << "°C" << std::endl;
    }

    void run(int intervalSeconds) {
        std::cout << "Симулятор устройства запущен. Интервал: "
                  << intervalSeconds << " сек" << std::endl;

        while (true) {
            sendTemperature();
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        }
    }
};

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));

    std::string portFile = "virtual_port.txt";
    int interval = 60;

    if (argc > 1) {
        portFile = argv[1];
    }
    if (argc > 2) {
        interval = std::atoi(argv[2]);
    }

    try {
        TemperatureDevice device(portFile);
        device.run(interval);
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
