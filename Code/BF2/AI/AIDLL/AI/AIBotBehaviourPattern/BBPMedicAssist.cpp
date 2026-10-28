#include "BBPMedicAssist.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPMedicAssist::BBPMedicAssist(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
    m_int10 = new int[getNumberOfBots()];
    m_int14 = new int[getNumberOfBots()];
    for (int i = 0; i < getNumberOfBots(); ++i)
    {
        m_int0C[i] = -1;
        m_int10[i] = -1;
        m_int14[i] = -1;
    }
}

BBPMedicAssist::~BBPMedicAssist()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPMedicAssist::~BBPMedicAssist");
}

BotScript* BBPMedicAssist::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPMedicAssist::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
