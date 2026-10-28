#include "BBPRandom.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPRandom::BBPRandom(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
}

BBPRandom::~BBPRandom()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPRandom::~BBPRandom");
}

BotScript* BBPRandom::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPRandom::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
