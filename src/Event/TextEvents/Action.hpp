#pragma once
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Packet/TextFunction.hpp>
#include <Logger/Logger.hpp>

EVENT("action", OnAction) {
    if (!eventParser.Contain("action"))
        return;

    std::string eventName = eventParser.Get("action", 1);
    auto* eventFunction = GetEventPool()->ActionManager::GetEventIfExists(eventName);

    if (!eventFunction) {

        Logger::Print(WARNING, "Unhandled action: {}", eventName);
        return;
    }
    eventFunction->sig_function(pAvatar, pServer, eventData, eventParser, pTankData);
}
