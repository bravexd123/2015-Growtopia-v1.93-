#include <Network/HTTPServer.hpp>
#include <config.hpp>
#include <Logger/Logger.hpp>
#include <Utils/TextParse.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

HTTPServer g_httpServer;
HTTPServer* GetHTTP() {
    return &g_httpServer;
}

static bool ReadHeaders(SOCKET client, std::string& outHeaders, std::string& outLeftover) {
    char buf[4096];
    std::string data;
    while (true) {
        int n = recv(client, buf, sizeof(buf), 0);
        if (n <= 0)
            return false;
        data.append(buf, n);

        auto posCRLF = data.find("\r\n\r\n");
        auto posLF = data.find("\n\n");
        std::size_t headerEnd = std::string::npos;
        std::size_t sepLen = 0;

        if (posCRLF != std::string::npos) {
            headerEnd = posCRLF;
            sepLen = 4;
        }
        else if (posLF != std::string::npos) {
            headerEnd = posLF;
            sepLen = 2;
        }

        if (headerEnd != std::string::npos) {
            outHeaders = data.substr(0, headerEnd);
            outLeftover = data.substr(headerEnd + sepLen);
            return true;
        }

        if (data.size() > 1024 * 64)
            return false;
    }
}

static int GetContentLength(const std::string& headers) {

    auto pos = headers.find("Content-Length:");
    if (pos == std::string::npos)
        pos = headers.find("content-length:");
    if (pos == std::string::npos)
        return 0;

    pos += std::string("Content-Length:").size();
    while (pos < headers.size() && headers[pos] == ' ')
        pos++;

    int value = 0;
    while (pos < headers.size() && headers[pos] >= '0' && headers[pos] <= '9') {
        value = value * 10 + (headers[pos] - '0');
        pos++;
    }
    return value;
}

static void HandleClient(SOCKET client) {
    std::string headers, leftover;
    if (!ReadHeaders(client, headers, leftover)) {
        closesocket(client);
        return;
    }

    int contentLength = GetContentLength(headers);
    std::string body = leftover;
    while (static_cast<int>(body.size()) < contentLength) {
        char buf[4096];
        int n = recv(client, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        body.append(buf, n);
    }

    Logger::Print(INFO, "A request from loopback client | {}", headers.substr(0, headers.find('\n')));

    TextParse parser{};
    parser.Add("server", Configuration::GetBaseHost());
    parser.Add<uint16_t>("port", Configuration::GetBasePort());
    parser.Add<bool>("type", true);
    parser.Add("beta_server", Configuration::GetBaseHost());
    parser.Add<uint16_t>("beta_port", Configuration::GetBasePort());
    parser.Add<bool>("beta_type", true);
    parser.Add("meta", "0DA34F801AE23F00");
    parser.Add("RTENDMARKERBS1001", "", "");

    std::string content = parser.GetAsString();
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(content.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + content;

    send(client, response.c_str(), static_cast<int>(response.size()), 0);
    closesocket(client);
}

bool HTTPServer::Listen() {
    Logger::Print(INFO, "Starting {}, binding port to server", fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "HTTP Server"));

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::Print(EXCEPTION, "WSAStartup failed.");
        return false;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        Logger::Print(EXCEPTION, "Failed to create HTTP listen socket.");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(80);

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Logger::Print(EXCEPTION, "Failed to bind port 80 to HTTP Server.");
        closesocket(listenSocket);
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        Logger::Print(EXCEPTION, "Failed to listen on port 80.");
        closesocket(listenSocket);
        return false;
    }

    m_listenSocket = static_cast<uintptr_t>(listenSocket);
    m_running.store(true);

    std::thread t(&HTTPServer::ServicePoll, this);
    this->m_serviceThread = std::move(t);

    Logger::Print("{} Initialized, Listening to {}", fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "HTTP Server"), fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "http://0.0.0.0:80"));
    return true;
}

void HTTPServer::Stop() {
    m_running.store(false);
    if (m_listenSocket) {
        closesocket(static_cast<SOCKET>(m_listenSocket));
        m_listenSocket = 0;
    }
    if (m_serviceThread.joinable())
        m_serviceThread.join();
}

std::thread& HTTPServer::GetThread() {
    return this->m_serviceThread;
}

void HTTPServer::ServicePoll() {
    SOCKET listenSocket = static_cast<SOCKET>(m_listenSocket);
    while (m_running.load()) {
        sockaddr_in clientAddr{};
        int clientAddrLen = sizeof(clientAddr);
        SOCKET client = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (client == INVALID_SOCKET) {
            if (!m_running.load())
                break;
            continue;
        }
        HandleClient(client);
    }
}
