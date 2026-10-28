#include "BBPGotoWaypointSoldier.h"

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

void BBPGotoWaypointSoldier::init(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypointSoldier::init");
}

} // namespace ai
} // namespace hfe
} // namespace dice
