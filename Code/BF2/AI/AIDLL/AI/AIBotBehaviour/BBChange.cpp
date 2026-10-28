#include "BBChange.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBChange::BBChange(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    for (int i = 0; i < 8; ++i)
        m_words24[i] = 0;
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
        m_targets[i] = -1;
    }
}

BBChange::~BBChange()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBChange::~BBChange");
}

float BBChange::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBChange::calculateUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
