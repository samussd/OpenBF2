#include "BBPChangeTeleport.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPChangeTeleport::BBPChangeTeleport(int behaviourId)
    : BBPPattern()
{
    m_bool10 = new bool[getNumberOfBots()];
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
}

BBPChangeTeleport::~BBPChangeTeleport()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPChangeTeleport::~BBPChangeTeleport");
}

BotScript* BBPChangeTeleport::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPChangeTeleport::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
