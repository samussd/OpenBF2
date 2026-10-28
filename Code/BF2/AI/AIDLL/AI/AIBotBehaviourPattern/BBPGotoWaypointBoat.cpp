#include "BBPGotoWaypointBoat.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPGotoWaypointBoat::BBPGotoWaypointBoat(int behaviourId)
    : BBPGotoWaypoint()
{
    m_behaviourId = behaviourId;
}

BBPGotoWaypointBoat::~BBPGotoWaypointBoat()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypointBoat::~BBPGotoWaypointBoat");
}

BotScript* BBPGotoWaypointBoat::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypointBoat::createPlan");
}

void BBPGotoWaypointBoat::init(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypointBoat::init");
}

} // namespace ai
} // namespace hfe
} // namespace dice
