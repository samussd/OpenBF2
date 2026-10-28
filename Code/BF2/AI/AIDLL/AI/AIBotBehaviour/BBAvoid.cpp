#include "BBAvoid.h"

#include "../AIBotBehaviourPattern/BBPAvoidCollision.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBAvoid::BBAvoid(unsigned char behaviourId, unsigned char mode, BBPAvoidCollision* generator, UrgeCurve* urgeCurve, float* weights)
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
        m_targets[i] = -1;
    }
}

BBAvoid::~BBAvoid()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBAvoid::~BBAvoid");
}

float BBAvoid::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBAvoid::calculateUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
