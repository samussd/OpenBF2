#include "BBTriggerable.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
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

float BBTriggerable::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBTriggerable::calculateUrgency");
}

bool BBTriggerable::isApplicable(Information*, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBTriggerable::isApplicable");
}

} // namespace ai
} // namespace hfe
} // namespace dice
