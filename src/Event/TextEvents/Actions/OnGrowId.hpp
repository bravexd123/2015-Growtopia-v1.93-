#pragma once
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Player/Player.hpp>
#include <Player/PlayerDialog/PlayerDialog.hpp>
#include <Utils/TextParse.hpp>

ACTION_EVENT("growid", OnGrowId) {
    pAvatar->PlayerDialog::Send(DIALOG_TYPE_REGISTRATION, TextParse());
}
