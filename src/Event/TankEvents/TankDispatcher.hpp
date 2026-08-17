#pragma once
#include <memory>
#include <Packet/GameUpdatePacket.hpp>

class Player;
class Server;

void HandleTankPacket(Player* pAvatar, std::shared_ptr<Server> pServer, TankPacketData* pTankData);
