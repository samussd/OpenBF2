#include "BBPGotoWaypointSoldier.h"

#include "../AIBot/BotMain.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPGotoWaypointSoldier::BBPGotoWaypointSoldier(int behaviourId)
    : BBPGotoWaypoint()
{
    m_behaviourId = behaviourId;
}

BBPGotoWaypointSoldier::~BBPGotoWaypointSoldier()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypointSoldier::~BBPGotoWaypointSoldier");
}

BotScript* BBPGotoWaypointSoldier::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypointSoldier::createPlan");
}

// 0x294700 (Ghidra).
void BBPGotoWaypointSoldier::init(Bot* bot)
{
    m_initiated[bot->getBotId()] = true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
