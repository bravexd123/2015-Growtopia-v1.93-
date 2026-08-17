#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <Server/Server.hpp>
#include <Server/ServerQueue.hpp>
#include <Packet/PacketManager.hpp>

class ServerPool : private PacketManager {
public:
    static ServerPool& Instance() { static ServerPool ret; return ret; }

public:
    void Start();
    void Stop();
    bool IsRunning() const;

    std::shared_ptr<Server> StartInstance();
    bool StopInstance(uint8_t instanceId);

public:
    std::unordered_map<uint8_t, std::shared_ptr<Server>> GetServers();
    std::shared_ptr<Server> GetBalancedServer();

    size_t GetActivePlayers() const;
    size_t GetActiveWorlds() const;
    size_t GetActiveServers() const;

    void ServicePoll();

    void ScheduleDelayed(uint32_t delayMs, std::function<void()> fn);

public:
    ServerPool() = default;
    ~ServerPool() = default;

private:
    std::unordered_map<uint8_t, std::shared_ptr<Server>> m_servers;
    std::array<std::deque<QueueContext>, QUEUE_MAXVAL> m_workerQueue;
    std::thread m_serviceThread;

    std::mutex m_delayedTasksMutex;
    std::vector<std::pair<std::chrono::steady_clock::time_point, std::function<void()>>> m_delayedTasks;

    uint16_t serverOffset       { 0 };
    std::atomic<bool> running   { false };
};

ServerPool* GetServerPool();
