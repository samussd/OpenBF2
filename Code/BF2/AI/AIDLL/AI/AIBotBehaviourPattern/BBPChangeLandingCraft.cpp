#include "BBPChangeLandingCraft.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPChangeLandingCraft::BBPChangeLandingCraft(int behaviourId)
    : BBPPattern()
{
    m_bool10 = new bool[getNumberOfBots()];
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
}

BBPChangeLandingCraft::~BBPChangeLandingCraft()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPChangeLandingCraft::~BBPChangeLandingCraft");
}

BotScript* BBPChangeLandingCraft::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPChangeLandingCraft::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
