#include "http_server.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

HttpServer::HttpServer(Database& db, int port) : db(db), port(port), running(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    running = true;
    serverThread = std::thread(&HttpServer::run, this);
}

void HttpServer::stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void HttpServer::run() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка WSAStartup" << std::endl;
        return;
    }
#endif

    socket_t serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка bind" << std::endl;
        closesocket(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Ошибка listen" << std::endl;
        closesocket(serverSocket);
        return;
    }

    std::cout << "HTTP сервер слушает порт " << port << std::endl;
#ifdef _WIN32
    DWORD timeout = 1000;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#endif

    while (running) {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);
#else
        socklen_t clientAddrLen = sizeof(clientAddr);
#endif
        socket_t clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
        }
    }

    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[4096];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string request(buffer);
        std::string response = processRequest(request);
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    closesocket(clientSocket);
}

std::string HttpServer::processRequest(const std::string& request) {
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;

    if (method != "GET") {
        return "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
    }

    std::string content;
    std::string contentType = "application/json";

    std::string headers = 
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n";

    if (path == "/current") {
        content = getLastMeasurement();
    } else if (path.find("/stats") == 0) {
        std::string type = "history";
        time_t start = 0;
        time_t end = time(nullptr);

        size_t queryPos = path.find('?');
        if (queryPos != std::string::npos) {
            std::string query = path.substr(queryPos + 1);
            std::stringstream ss(query);
            std::string segment;
            while (std::getline(ss, segment, '&')) {
                size_t eqPos = segment.find('=');
                if (eqPos != std::string::npos) {
                    std::string key = segment.substr(0, eqPos);
                    std::string val = segment.substr(eqPos + 1);
                    if (key == "type") type = val;
                    if (key == "start") start = std::stoll(val);
                    if (key == "end") end = std::stoll(val);
                }
            }
        }
        content = getStats(type, start, end);
    } else {
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }

    headers += "Content-Type: " + contentType + "\r\n";
    headers += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    headers += "\r\n";

    return headers + content;
}

std::string HttpServer::getLastMeasurement() {
    TemperatureRecord rec = db.getLastMeasurement();
    std::stringstream ss;
    ss << "{\"timestamp\": " << rec.timestamp 
       << ", \"temperature\": " << std::fixed << std::setprecision(2) << rec.temperature << "}";
    return ss.str();
}

std::string HttpServer::getStats(const std::string& type, time_t start, time_t end) {
    std::vector<TemperatureRecord> records;
    if (type == "hourly") {
        records = db.getHourlyStats(start, end);
    } else if (type == "daily") {
        records = db.getDailyStats(start, end);
    } else {
        records = db.getHistory(start, end);
    }

    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < records.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"timestamp\": " << records[i].timestamp 
           << ", \"temperature\": " << std::fixed << std::setprecision(2) << records[i].temperature << "}";
    }
    ss << "]";
    return ss.str();
}
