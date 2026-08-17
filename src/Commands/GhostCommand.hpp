#pragma once
#include <Commands/CommandType.hpp>
#include <World/World.hpp>
#include <Packet/GameUpdatePacket.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/VariantFunction.hpp>
#include <Manager/Item/ItemEffects.hpp>
#include <ENetWrapper/ENetWrapper.hpp>

COMMAND_EVENT_ROLE("/ghost", GhostCommand, PlayerRole::Moderator) {
    pAvatar->SetGhostEnabled(!pAvatar->IsGhostEnabled());

    TankPacketData state{};
    state.m_type = NET_GAME_PACKET_SET_CHARACTER_STATE;
    state.m_netId = pAvatar->GetNetId();
    state.m_jumpCount = 128;

    state.m_punchIndex = ItemEffects::GetActivePunchEffect(pAvatar->GetItems());
    state.m_animationType = 128;
    state.m_floatVariable = 150.0f;
    state.m_vectorX = 1000.0f;
    state.m_vectorY = 350.0f;
    state.m_vectorX2 = 200.0f;
    state.m_vectorY2 = 1000.0f;
    state.m_effectFlags = static_cast<int32_t>(ItemEffects::ComputeCharacterStateFlags(pAvatar));

    auto pWorld = pAvatar->GetWorld();
    if (pWorld) {
        for (auto* pOther : pWorld->GetPlayers()) {
            STankPacket statePacket(state);
            ENetWrapper::SendPacket(pOther->Get(), statePacket);
        }
    } else {
        STankPacket statePacket(state);
        ENetWrapper::SendPacket(pAvatar->Get(), statePacket);
    }

    VarList::OnConsoleMessage(pAvatar->Get(), pAvatar->IsGhostEnabled() ? "`2Ghost mode enabled.``" : "`4Ghost mode disabled.``");
}
