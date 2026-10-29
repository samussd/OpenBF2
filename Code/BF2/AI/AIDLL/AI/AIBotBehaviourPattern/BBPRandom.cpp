#include "BBPRandom.h"

#include "../AIBot/BotMain.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

BBPRandom::BBPRandom(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
}

// 0x2a77f0 (Ghidra).
void BBPRandom::init(Bot* bot)
{
    m_initiated[bot->getBotId()] = true;
    m_initTime = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
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
