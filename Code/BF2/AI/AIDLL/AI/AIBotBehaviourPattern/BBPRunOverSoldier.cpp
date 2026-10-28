#include "BBPRunOverSoldier.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPRunOverSoldier::BBPRunOverSoldier(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
}

BBPRunOverSoldier::~BBPRunOverSoldier()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPRunOverSoldier::~BBPRunOverSoldier");
}

BotScript* BBPRunOverSoldier::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPRunOverSoldier::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
