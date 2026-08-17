#pragma once
#include <string>
#include <thread>
#include <atomic>

class HTTPServer {
public:
    bool Listen();
    void Stop();

    std::thread& GetThread();
    void ServicePoll();

public:
    HTTPServer() = default;
    ~HTTPServer() = default;

private:
    uintptr_t m_listenSocket = 0;
    std::thread m_serviceThread;
    std::atomic<bool> m_running{ false };
};

HTTPServer* GetHTTP();
