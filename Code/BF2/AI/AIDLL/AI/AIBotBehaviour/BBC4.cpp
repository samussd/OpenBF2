#include "BBC4.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBC4::BBC4(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
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
}

BBC4::~BBC4()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBC4::~BBC4");
}

float BBC4::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBC4::calculateUrgency");
}

bool BBC4::isApplicable(Information*, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBC4::isApplicable");
}

} // namespace ai
} // namespace hfe
} // namespace dice
