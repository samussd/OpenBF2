#include "BBPMedicAssist.h"

#include "../AIBot/BotMain.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPMedicAssist::BBPMedicAssist(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_target = new unsigned int[getNumberOfBots()];
    m_toolIndex = new int[getNumberOfBots()];
    m_occupant = new int[getNumberOfBots()];
    for (int i = 0; i < getNumberOfBots(); ++i)
    {
        m_target[i] = static_cast<unsigned int>(-1);
        m_toolIndex[i] = -1;
        m_occupant[i] = -1;
    }
}

// 0x2a1320 (Ghidra).
void BBPMedicAssist::init(unsigned int target, int toolIndex, int occupant, Bot* bot)
{
    m_initiated[bot->getBotId()] = true;
    m_target[bot->getBotId()] = target;
    m_toolIndex[bot->getBotId()] = toolIndex;
    m_occupant[bot->getBotId()] = occupant;
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
