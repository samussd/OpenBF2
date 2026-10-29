#include "BBTriggerable.h"

#include "../AIBot/BotMain.h"
#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBTriggerable::BBTriggerable(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
    }
    m_int24 = 0;
}

BBTriggerable::~BBTriggerable()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBTriggerable::~BBTriggerable");
}

// 0x234de0 (Ghidra). Never urgent on its own.
float BBTriggerable::calculateUrgency(Bot* bot, bool, float)
{
    m_urgency[bot->getBotId()] = 0.0f;
    return m_urgency[bot->getBotId()];
}

// 0x234e40 (Ghidra). Never applicable: the shipped body asks whether the player is alive and
// then whether the bot is down, and returns false either way.
bool BBTriggerable::isApplicable(Information*, Bot* bot)
{
    if (bot->getPlayer()->getIsAlive())
        bot->getManDown();
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
