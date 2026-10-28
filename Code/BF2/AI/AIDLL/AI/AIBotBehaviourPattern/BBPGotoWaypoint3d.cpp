#include "BBPGotoWaypoint3d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPGotoWaypoint3d::BBPGotoWaypoint3d(int behaviourId)
    : BBPGotoWaypoint()
{
    m_behaviourId = behaviourId;
}

BBPGotoWaypoint3d::~BBPGotoWaypoint3d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypoint3d::~BBPGotoWaypoint3d");
}

BotScript* BBPGotoWaypoint3d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypoint3d::createPlan");
}

void BBPGotoWaypoint3d::init(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypoint3d::init");
}

} // namespace ai
} // namespace hfe
} // namespace dice
