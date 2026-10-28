#include "BBPChange.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPChange::BBPChange(int behaviourId)
    : BBPPattern()
{
    const int numberOfBots = getNumberOfBots();
    m_vec10 = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec10[i].x = 0.0f;
        m_vec10[i].y = 0.0f;
        m_vec10[i].z = 0.0f;
    }
    m_bool14 = new bool[getNumberOfBots()];
    m_bool18 = new bool[getNumberOfBots()];
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
}

BBPChange::~BBPChange()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPChange::~BBPChange");
}

BotScript* BBPChange::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPChange::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
