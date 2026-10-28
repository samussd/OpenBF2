#include "BBMoveTo.h"

#include "../AIBotBehaviourPattern/BBPGotoWaypoint.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBMoveTo::BBMoveTo(unsigned char behaviourId, unsigned char mode, BBPGotoWaypoint* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_targets = 0;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
    }
}

BBMoveTo::~BBMoveTo()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMoveTo::~BBMoveTo");
}

float BBMoveTo::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMoveTo::calculateUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
