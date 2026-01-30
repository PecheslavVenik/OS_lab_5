#include "database.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

Database::Database(const std::string& dbPath) : dbPath(dbPath), db(nullptr) {}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

bool Database::initialize() {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        std::cerr << "Ошибка открытия БД: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char* sql = 
        "CREATE TABLE IF NOT EXISTS measurements ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp INTEGER NOT NULL,"
        "temperature REAL NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_timestamp ON measurements(timestamp);";

    return executeQuery(sql);
}

bool Database::executeQuery(const std::string& query) {
    char* zErrMsg = 0;
    int rc = sqlite3_exec(db, query.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка SQL: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool Database::saveMeasurement(time_t timestamp, double temperature) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO measurements (timestamp, temperature) VALUES (?, ?);";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки SQL: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(timestamp));
    sqlite3_bind_double(stmt, 2, temperature);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

std::vector<TemperatureRecord> Database::getHistory(time_t start, time_t end) {
    std::vector<TemperatureRecord> result;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT timestamp, temperature FROM measurements WHERE timestamp BETWEEN ? AND ? ORDER BY timestamp ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return result;

    sqlite3_bind_int64(stmt, 1, start);
    sqlite3_bind_int64(stmt, 2, end);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TemperatureRecord rec;
        rec.timestamp = static_cast<time_t>(sqlite3_column_int64(stmt, 0));
        rec.temperature = sqlite3_column_double(stmt, 1);
        result.push_back(rec);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<TemperatureRecord> Database::getHourlyStats(time_t start, time_t end) {
    std::vector<TemperatureRecord> result;
    sqlite3_stmt* stmt;
    const char* sql = 
        "SELECT (timestamp / 3600) * 3600 as hour_ts, AVG(temperature) "
        "FROM measurements WHERE timestamp BETWEEN ? AND ? "
        "GROUP BY hour_ts ORDER BY hour_ts ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return result;

    sqlite3_bind_int64(stmt, 1, start);
    sqlite3_bind_int64(stmt, 2, end);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TemperatureRecord rec;
        rec.timestamp = static_cast<time_t>(sqlite3_column_int64(stmt, 0));
        rec.temperature = sqlite3_column_double(stmt, 1);
        result.push_back(rec);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<TemperatureRecord> Database::getDailyStats(time_t start, time_t end) {
    std::vector<TemperatureRecord> result;
    sqlite3_stmt* stmt;
    const char* sql = 
        "SELECT (timestamp / 86400) * 86400 as day_ts, AVG(temperature) "
        "FROM measurements WHERE timestamp BETWEEN ? AND ? "
        "GROUP BY day_ts ORDER BY day_ts ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return result;

    sqlite3_bind_int64(stmt, 1, start);
    sqlite3_bind_int64(stmt, 2, end);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TemperatureRecord rec;
        rec.timestamp = static_cast<time_t>(sqlite3_column_int64(stmt, 0));
        rec.temperature = sqlite3_column_double(stmt, 1);
        result.push_back(rec);
    }
    sqlite3_finalize(stmt);
    return result;
}

TemperatureRecord Database::getLastMeasurement() {
    TemperatureRecord rec = {0, 0.0};
    sqlite3_stmt* stmt;
    const char* sql = "SELECT timestamp, temperature FROM measurements ORDER BY timestamp DESC LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            rec.timestamp = static_cast<time_t>(sqlite3_column_int64(stmt, 0));
            rec.temperature = sqlite3_column_double(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }
    return rec;
}

void Database::cleanupOldRecords(time_t olderThan) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM measurements WHERE timestamp < ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, olderThan);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}
