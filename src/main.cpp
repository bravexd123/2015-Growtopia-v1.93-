#include <thread>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <config.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <Logger/Logger.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Database/Database.hpp>
#include <Network/HTTPServer.hpp>
#include <Event/EventPool.hpp>
#include <Server/ServerPool.hpp>
#include <Utils/Utils.hpp>

int main(int argc, char* argv[]) {
    std::string commandArguments{};
    for (int i = 1; i < argc; i++) commandArguments.append(std::string(argv[i]).append(" "));

    Logger::Init();
    Logger::Print(INFO, "Starting {} V{}", Configuration::GetName(), Configuration::GetVersion());

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (commandArguments.find("--nohttp") == std::string::npos) {
        if (!GetHTTP()->Listen())
            Logger::Print(WARNING, "Failed to starting {}, please run an external service.", "HTTP Server");
    }

    if (!GetDatabase()->Connect()) {
        Logger::Print(WARNING, "Failed to initialize {}, please check filesystem permissions for the db/ directory.", "Database");
        GetHTTP()->Stop();
        return EXIT_FAILURE;
    }
    if (!GetItemManager()->Serialize()) {
        Logger::Print(EXCEPTION, "{} >> failed to load items.dat, please make sure the file is on /cache", "ItemManager");
        GetHTTP()->Stop();
        return EXIT_FAILURE;
    }

    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        std::filesystem::path clientCacheDir = std::filesystem::path(localAppData) / "Growtopia" / "cache";
        std::filesystem::path clientItemsDat = clientCacheDir / "items.dat";
        std::error_code ec;
        if (!std::filesystem::exists(clientItemsDat, ec)) {
            std::filesystem::create_directories(clientCacheDir, ec);
            std::filesystem::copy_file("cache/items.dat", clientItemsDat, std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec)
                Logger::Print(INFO, "Seeded local Growtopia client cache with items.dat ('{}')", clientItemsDat.string());
        }
    }

    Logger::Print(INFO, "Initializing {} and registering events...", "Event Pool");
    GetEventPool()->RegisterEvents();

    Logger::Print(INFO, "Initializing {} and starting up {}...", "Server Pool", "Master Server");
    if (enet_initialize() != 0) {
        Logger::Print(EXCEPTION, "Failed to Initialize ENet, Terminating the Server...");
        GetHTTP()->Stop();
        return EXIT_FAILURE;
    };
    if (!GetServerPool()->StartInstance()) {
        Logger::Print(EXCEPTION, "Couldn't Startup {} Correctly, Terminating the Server...", "Master Server");
        GetHTTP()->Stop();
        return EXIT_FAILURE;
    };
    GetServerPool()->Start();

    std::vector<std::thread> workerThread{};
    workerThread.push_back(
        std::thread([&]() {
            std::string consoleCommand{};
            while (std::cin >> consoleCommand) {
                switch (Hash::Fnv1A(consoleCommand)) {
                case "shutdown"_fnv: {
                    for (auto [instanceId, server] : GetServerPool()->GetServers()) {
                        if (!GetServerPool()->StopInstance(instanceId))
                            Logger::Print(WARNING, "Failed to shutdown InstanceId {}, this instance doesn't found on {}", instanceId, "Server Pool");
                    }
                    GetHTTP()->Stop();
                    GetServerPool()->Stop();
                } break;
                case "svinfo"_fnv: {
                    std::string data{};
                    data.append("Server Information\n")
                        .append(fmt::format("{}: {}\n", "Active Servers", fmt::format("{}/4", GetServerPool()->GetActiveServers())))
                        .append(fmt::format("{}: {}\n", "Active Players (All Servers)", GetServerPool()->GetActivePlayers()))
                        .append(fmt::format("{}: {}\n", "Active Worlds (All Servers)", GetServerPool()->GetActiveWorlds()));
                    data.append("Servers Stats\n");
                    for (auto [instanceId, server] : GetServerPool()->GetServers())
                        data.append(fmt::format("S{} | Uptime: {} [Players: {}, Worlds: {}] | In/Out PPS: {}/{}\n", instanceId, server->GetUptime(), server->GetActivePlayers(), server->GetActiveWorlds(), server->GetHost()->incomingBandwidth, server->GetHost()->outgoingBandwidth));

                    data.resize(data.length() - 1);
                    Logger::Print(INFO, data);
                } break;
                case "worldtest"_fnv: {
                    auto servers = GetServerPool()->GetServers();
                    if (servers.empty()) {
                        Logger::Print(WARNING, "worldtest: no server instance running");
                        break;
                    }
                    auto pWorld = servers.begin()->second->GetWorldPool()->NewWorld("START");
                    auto* pTile = pWorld->GetTile(0, 0);
                    Logger::Print(INFO, "worldtest: world '{}' {}x{}, tile(0,0) foreground={}, background={}",
                        pWorld->GetName(), pWorld->GetWidth(), pWorld->GetHeight(),
                        pTile ? pTile->m_foregroundId : 0, pTile ? pTile->m_backgroundId : 0);

                    Tile modified{};
                    modified.m_foregroundId = 999;
                    modified.m_backgroundId = 888;
                    pWorld->SetTile(5, 5, modified);

                    bool saved = GetDatabase()->GetWorldTable()->Save(*pWorld);
                    World freshWorld;
                    WorldSnapshot snap;
                    bool loaded = GetDatabase()->GetWorldTable()->Load(pWorld->GetName(), snap)
                        && freshWorld.LoadSnapshot(snap.name, snap.width, snap.height, snap.tiles);
                    auto* pRoundTripTile = loaded ? freshWorld.GetTile(5, 5) : nullptr;
                    Logger::Print(INFO, "worldtest: saved={} loaded={} roundtrip tile(5,5) foreground={} background={} (expect 999/888)",
                        saved, loaded,
                        pRoundTripTile ? pRoundTripTile->m_foregroundId : -1,
                        pRoundTripTile ? pRoundTripTile->m_backgroundId : -1);
                } break;
                case "playertest"_fnv: {

                    Player fakePlayer(nullptr);
                    fakePlayer.GetDetail().SetTankIDName("planningtest");
                    fakePlayer.GetDetail().SetTankIDPass("hunter2222");
                    fakePlayer.SetRawName("PlanningTest");
                    fakePlayer.SetDisplayName("PlanningTest");
                    fakePlayer.SetEmailAddress("test@example.com");
                    fakePlayer.SetDiscordClientId(123456789);
                    fakePlayer.GetItems()->SetGems(500);
                    fakePlayer.GetItems()->AddItem(2, 5);

                    uint32_t newId = GetDatabase()->GetPlayerTable()->Insert(&fakePlayer);
                    bool exists = GetDatabase()->GetPlayerTable()->IsAccountExist("planningtest");

                    Player loadedPlayer(nullptr);
                    bool loaded = GetDatabase()->GetPlayerTable()->Load("planningtest", &loadedPlayer);

                    Logger::Print(INFO, "playertest: insertedId={} exists={} loaded={} gems={} (expect 500) rawName={} (expect PlanningTest) discordId={} (expect 123456789) dirtCount={} (expect 5)",
                        newId, exists, loaded, loadedPlayer.GetItems()->GetGems(), loadedPlayer.GetRawName(),
                        loadedPlayer.GetDiscordClientId(),
                        loadedPlayer.GetItems()->m_bpItems.count(2) ? (int)loadedPlayer.GetItems()->m_bpItems[2] : -1);
                } break;
                case "hwinfo"_fnv: {
                    std::string
                        username = Console::Execute("whoami"),
                        threadsUsage = Console::Execute(fmt::format("ps huH p {} | wc -l", ::getpid()));
                    username.resize(username.length() - 1);
                    threadsUsage.resize(threadsUsage.length() - 1);

                    std::string data{ "Hardware Information\n" };
                    data.append(fmt::format("{}: {}\n", "Hostname", Console::GetHostname()))
                        .append(fmt::format("{}: {}\n", "Username", username))
                        .append(fmt::format("{}: {}\n", "Process Id", ::getpid()))
                        .append(fmt::format("{}: {}\n", "Thread Concurrency", std::thread::hardware_concurrency()))
                        .append(fmt::format("{}: {}\n", "Threads Used", threadsUsage))
                        .append(fmt::format("{}: {}\n", "CPU Load", Console::GetLoadAverage()));
                    data.resize(data.length() - 1);
                    Logger::Print(INFO, data);
                } break;
                default:
                    break;
                }
            }
            }));
    for (auto& thread : workerThread)
        thread.detach();
    while (GetServerPool()->IsRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
