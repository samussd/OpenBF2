#include "BBPTriggerable.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPTriggerable::BBPTriggerable(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
    m_int14 = 0;
}

BBPTriggerable::~BBPTriggerable()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPTriggerable::~BBPTriggerable");
}

BotScript* BBPTriggerable::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPTriggerable::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
