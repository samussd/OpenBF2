#include "BBPRevive.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPRevive::BBPRevive(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
    m_int10 = new int[getNumberOfBots()];
}

BBPRevive::~BBPRevive()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPRevive::~BBPRevive");
}

BotScript* BBPRevive::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPRevive::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
