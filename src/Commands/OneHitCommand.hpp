#pragma once
#include <string>
#include <Commands/CommandType.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/1hit", OneHitCommand, PlayerRole::Developer) {
    bool enabled = !pAvatar->IsOneHitEnabled();
    pAvatar->SetOneHitEnabled(enabled);
    VarList::OnConsoleMessage(pAvatar->Get(), enabled ? "`21-Hit Break enabled.``" : "`41-Hit Break disabled.``");
}
