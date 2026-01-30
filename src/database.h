#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <ctime>

struct TemperatureRecord {
    time_t timestamp;
    double temperature;
};

class Database {
public:
    Database(const std::string& dbPath);
    ~Database();

    bool initialize();
    bool saveMeasurement(time_t timestamp, double temperature);
    std::vector<TemperatureRecord> getHistory(time_t start, time_t end);
    std::vector<TemperatureRecord> getHourlyStats(time_t start, time_t end);
    std::vector<TemperatureRecord> getDailyStats(time_t start, time_t end);
    TemperatureRecord getLastMeasurement();
    void cleanupOldRecords(time_t olderThan);

private:
    std::string dbPath;
    struct sqlite3* db;
    bool executeQuery(const std::string& query);
};

#endif
