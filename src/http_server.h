#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "database.h"
#include <atomic>
#include <thread>

class HttpServer {
public:
    HttpServer(Database& db, int port);
    ~HttpServer();

    void start();
    void stop();

private:
    Database& db;
    int port;
    std::atomic<bool> running;
    std::thread serverThread;

    void run();
    void handleClient(int clientSocket);
    std::string processRequest(const std::string& request);
    std::string getLastMeasurement();
    std::string getStats(const std::string& type, time_t start, time_t end);
};

#endif
