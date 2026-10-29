#include "BBPC4.h"

#include "../AIBot/BotMain.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPC4::BBPC4(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
}

// 0x2425e0 (Ghidra).
void BBPC4::init(Bot* bot, bool detonate, AIStrategicObject* target)
{
    m_initiated[bot->getBotId()] = true;
    m_detonate = detonate;
    m_target = target;
}

BBPC4::~BBPC4()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPC4::~BBPC4");
}

BotScript* BBPC4::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPC4::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
