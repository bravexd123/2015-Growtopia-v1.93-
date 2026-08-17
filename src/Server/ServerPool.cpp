#include <Server/ServerPool.hpp>
#include <chrono>
#include <magic_enum.hpp>
#include <ENetWrapper/ENetServer.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <config.hpp>
#include <Logger/Logger.hpp>
#include <Event/EventPool.hpp>
#include <Event/TankEvents/TankDispatcher.hpp>
#include <Packet/PacketFactory.hpp>
#include <Utils/Utils.hpp>

ServerPool g_serverPool;
ServerPool* GetServerPool() {
    return &g_serverPool;
}

void ServerPool::Start() {
    std::thread t(&ServerPool::ServicePoll, this);
    this->m_serviceThread = std::move(t);
    this->running.store(true);
}
void ServerPool::Stop() {
    this->running.store(false);
}
bool ServerPool::IsRunning() const {
    return this->running.load();
}

std::shared_ptr<Server> ServerPool::StartInstance() {
    uint8_t instanceId = static_cast<uint8_t>(m_servers.size());
    auto tempPort = Configuration::GetBasePort() + this->serverOffset++;
    auto server{ std::make_shared<Server>() };

    if (!server->Init(Configuration::GetBaseHost(), tempPort, 0xFF)) {
        Logger::Print(EXCEPTION, "Failed to Initialize {}, port {} is busy...", fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "ENetServer"), fmt::format(fmt::emphasis::bold | fg(fmt::color::blue_violet), "{}", tempPort));
        return nullptr;
    }
    Logger::Print(INFO, "Starting up {} Instance", fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "ENetServer"));
    Logger::Print("InstanceId: {}, Server located at {}", fmt::format(fmt::emphasis::bold | fg(fmt::color::lavender), "{}", instanceId), fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "udp://{}:{}", Configuration::GetBaseHost(), tempPort));
    m_servers.insert_or_assign(instanceId, std::move(server));
    return m_servers[instanceId];
}
bool ServerPool::StopInstance(uint8_t instanceId) {
    if (this->m_servers.find(instanceId) == this->m_servers.end())
        return false;
    Logger::Print(INFO, "Shutting Down InstanceId {}", fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "{}", instanceId));
    return this->m_servers.erase(instanceId) > 0;
}

std::unordered_map<uint8_t, std::shared_ptr<Server>> ServerPool::GetServers() {
    return this->m_servers;
}
std::shared_ptr<Server> ServerPool::GetBalancedServer() {
    for (auto [instanceId, server] : m_servers) {
        if (server->GetActivePlayers() > 1)
            continue;
        return server;
    }
    return this->StartInstance();
}

size_t ServerPool::GetActiveServers() const {
    return this->m_servers.size();
}
size_t ServerPool::GetActivePlayers() const {
    size_t ret{ 0 };
    for (auto [instanceId, server] : m_servers)
        ret += server->GetActivePlayers();
    return ret;
}
size_t ServerPool::GetActiveWorlds() const {
    size_t ret{ 0 };
    for (auto [instanceId, server] : m_servers)
        ret += server->GetActiveWorlds();
    return ret;
}

void ServerPool::ScheduleDelayed(uint32_t delayMs, std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(m_delayedTasksMutex);
    m_delayedTasks.emplace_back(std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs), std::move(fn));
}

void ServerPool::ServicePoll() {

    auto lastPingTime = std::chrono::steady_clock::now();
    constexpr auto pingInterval = std::chrono::seconds(5);

    while (this->IsRunning()) {
        auto now = std::chrono::steady_clock::now();
        if (now - lastPingTime >= pingInterval) {
            lastPingTime = now;
            for (auto& [instanceId, pServer] : this->GetServers()) {
                if (!pServer || !pServer->GetPlayerPool())
                    continue;
                for (auto& [connectId, pAvatar] : pServer->GetPlayerPool()->GetPlayers()) {
                    TankPacketData t{};
                    t.m_type = NET_GAME_PACKET_PING_REQUEST;
                    t.m_netId = 0;
                    STankPacket packet(t);
                    ENetWrapper::SendPacket(pAvatar->Get(), packet, false);
                }
            }
        }

        {
            std::vector<std::function<void()>> due;
            {
                std::lock_guard<std::mutex> lock(m_delayedTasksMutex);
                auto now = std::chrono::steady_clock::now();
                for (auto it = m_delayedTasks.begin(); it != m_delayedTasks.end();) {
                    if (it->first <= now) {
                        due.push_back(std::move(it->second));
                        it = m_delayedTasks.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            for (auto& fn : due)
                fn();
        }

        for (auto& [instanceId, pServer] : this->GetServers()) {
            if (!pServer || !pServer->GetHost()) {
                Logger::Print(WARNING, "ServicePoll: InstanceId {} has a null server/host!", instanceId);
                continue;
            }

            int serviceResult;
            {
                std::lock_guard<std::mutex> lock(ENetWrapper::g_enetMutex);
                serviceResult = enet_host_service(pServer->GetHost(), pServer->GetEvent(), 0);
            }
            if (serviceResult < 0) {
                Logger::Print(WARNING, "enet_host_service returned error {} for InstanceId {}", serviceResult, instanceId);
                continue;
            }
            if (serviceResult == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            auto event = *pServer->GetEvent();
            auto playerPool = pServer->GetPlayerPool();
            if (!event.peer || !playerPool)
                continue;

            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                Logger::Print(INFO, "ENET_EVENT_TYPE_CONNECT received from peer, connectId {}", event.peer->connectID);
                if (playerPool->GetPlayer(event.peer->connectID))
                    break;
                Player* pAvatar = playerPool->NewPlayer(event.peer);
                if (pAvatar) {
                    pAvatar->OnConnect();
                }
            } break;
            case ENET_EVENT_TYPE_DISCONNECT: {

                Player* pAvatar = playerPool->GetPlayerByPeer(event.peer);
                Logger::Print(INFO, "ENET_EVENT_TYPE_DISCONNECT received from peer, connectId {}, matched player={}", event.peer->connectID, pAvatar != nullptr);
                if (!pAvatar)
                    break;
                pAvatar->OnDisconnect();
                playerPool->RemovePlayerByPeer(event.peer);
            } break;
            case ENET_EVENT_TYPE_RECEIVE: {
                Player* pAvatar = playerPool->GetPlayer(event.peer->connectID);
                Logger::Print(INFO, "ENET_EVENT_TYPE_RECEIVE: dataLength={}, hasAvatar={}", event.packet->dataLength, pAvatar != nullptr);

                if (event.packet->dataLength < sizeof(IPacketType::m_packetType) + 1 || event.packet->dataLength > 0x400 || !pAvatar) {
                    Logger::Print(WARNING, "RECEIVE rejected: dataLength out of bounds or no avatar");
                    enet_packet_destroy(event.packet);
                    break;
                }
                auto messageType = *(int32_t*)event.packet->data;
                Logger::Print(INFO, "RECEIVE messageType={}", messageType);

                switch (messageType) {
                case NET_MESSAGE_GENERIC_TEXT:
                case NET_MESSAGE_GAME_MESSAGE: {
                    auto data = this->DataToString(event.packet->data + sizeof(IPacketType::m_packetType), event.packet->dataLength - sizeof(IPacketType::m_packetType));
                    Logger::Print(INFO, "RECEIVE data: {}", data);

                    size_t ridPos = data.find("rid|");
                    if (ridPos != std::string::npos) {
                        size_t ridValStart = ridPos + 4;
                        size_t platPos = data.find("platformID", ridValStart);
                        if (platPos != std::string::npos && platPos > ridValStart) {
                            if (data[platPos - 1] != '\n' && data[platPos - 1] != '|') {
                                data.insert(platPos, "\n");
                            }
                        }
                    }

                    GetEventPool()->AddQueue(data.substr(0, data.find('|')), pAvatar, pServer, data, TextParse(data), nullptr);
                } break;
                case NET_MESSAGE_GAME_PACKET: {
                    auto* pTankData = this->DataToTankData(event.packet->data + sizeof(IPacketType::m_packetType), event.packet->dataLength - sizeof(IPacketType::m_packetType));
                    if (!pTankData) {
                        Logger::Print(WARNING, "RECEIVE rejected: malformed tank packet");
                        break;
                    }
                    HandleTankPacket(pAvatar, pServer, pTankData);
                } break;
                default:
                    Logger::Print(WARNING, "RECEIVE unknown messageType {}", messageType);
                    break;
                }

                enet_packet_destroy(event.packet);
            } break;
            default:
                break;
            }
        }
    }
}
