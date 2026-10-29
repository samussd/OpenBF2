#include "BBPRevive.h"

#include "../AIBot/BotMain.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPRevive::BBPRevive(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_target = new unsigned int[getNumberOfBots()];
    m_toolIndex = new int[getNumberOfBots()];
}

// 0x2a90b0 (Ghidra).
void BBPRevive::init(unsigned int target, int toolIndex, Bot* bot)
{
    m_initiated[bot->getBotId()] = true;
    m_target[bot->getBotId()] = target;
    m_toolIndex[bot->getBotId()] = toolIndex;
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
