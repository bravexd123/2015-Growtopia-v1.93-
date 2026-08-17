#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <Player/Player.hpp>
#include <Server/Server.hpp>
#include <Server/ServerPool.hpp>
#include <Player/PlayerPool.hpp>
#include <World/World.hpp>
#include <Event/EventPool.hpp>
#include <Utils/TextParse.hpp>

namespace CommandUtils {

    inline std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    inline bool IsAllDigits(const std::string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
    }

    inline Player* FindOnlinePlayer(const std::string& query) {
        if (query.empty())
            return nullptr;

        bool queryIsId = IsAllDigits(query);
        uint32_t queryId = queryIsId ? static_cast<uint32_t>(std::stoul(query)) : 0;
        std::string lowerQuery = ToLower(query);

        Player* partialMatch = nullptr;
        for (auto& [serverId, pServer] : GetServerPool()->GetServers()) {
            if (!pServer || !pServer->GetPlayerPool())
                continue;
            for (auto& [connectId, pOther] : pServer->GetPlayerPool()->GetPlayers()) {
                if (!pOther || !pOther->GetDetail().IsFlagOn(CLIENTFLAG_LOGGED_ON))
                    continue;
                if (queryIsId && pOther->GetUserId() == queryId)
                    return pOther;
                std::string otherLower = ToLower(pOther->GetRawName());
                if (otherLower == lowerQuery)
                    return pOther;
                if (!partialMatch && !otherLower.empty() && otherLower.find(lowerQuery) != std::string::npos)
                    partialMatch = pOther;
            }
        }
        return partialMatch;
    }

    inline std::shared_ptr<Server> FindServerForPlayer(Player* pAvatar) {
        if (!pAvatar)
            return nullptr;
        for (auto& [serverId, pServer] : GetServerPool()->GetServers()) {
            if (!pServer || !pServer->GetPlayerPool())
                continue;
            if (pServer->GetPlayerPool()->GetPlayerByPeer(pAvatar->Get()) == pAvatar)
                return pServer;
        }
        return nullptr;
    }

    inline Player* FindPlayerByPeer(ENetPeer* peer) {
        for (auto& [serverId, pServer] : GetServerPool()->GetServers()) {
            if (!pServer || !pServer->GetPlayerPool())
                continue;
            if (Player* pFound = pServer->GetPlayerPool()->GetPlayerByPeer(peer))
                return pFound;
        }
        return nullptr;
    }

    inline void SendToWorld(Player* pAvatar, const std::string& worldName) {
        if (!pAvatar || worldName.empty())
            return;
        if (pAvatar->GetWorld() && pAvatar->GetWorld()->GetName() == worldName)
            return;

        auto pServer = FindServerForPlayer(pAvatar);
        if (!pServer)
            return;

        if (auto pCurrentWorld = pAvatar->GetWorld()) {
            pCurrentWorld->BroadcastPlayerLeft(pAvatar);
            pCurrentWorld->ReleaseNetId(pAvatar->GetNetId());
            pCurrentWorld->RemovePlayer(pAvatar);
            pAvatar->SetWorld(nullptr);
            pAvatar->SetNetId(-1);
            pAvatar->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);
        }
        if (auto* pJoinEvent = GetEventPool()->ActionManager::GetEventIfExists("join_request")) {
            TextParse joinParser("action|join_request\nname|" + worldName + "\n");
            pJoinEvent->sig_function(pAvatar, pServer, std::string(), joinParser, nullptr);
        }
    }

}
