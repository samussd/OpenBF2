#include "BBPGotoWaypoint2d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPGotoWaypoint2d::BBPGotoWaypoint2d(int behaviourId)
    : BBPGotoWaypoint()
{
    m_behaviourId = behaviourId;
}

BBPGotoWaypoint2d::~BBPGotoWaypoint2d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypoint2d::~BBPGotoWaypoint2d");
}

BotScript* BBPGotoWaypoint2d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypoint2d::createPlan");
}

void BBPGotoWaypoint2d::init(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypoint2d::init");
}

} // namespace ai
} // namespace hfe
} // namespace dice
