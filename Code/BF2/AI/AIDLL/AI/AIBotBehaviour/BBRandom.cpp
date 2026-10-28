#include "BBRandom.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBRandom::BBRandom(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    m_vec38.x = 0.0f;
    m_vec38.y = 0.0f;
    m_vec38.z = 0.0f;
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
    }
    m_flag24 = false;
    m_double28 = 0.0;
    m_double30 = 0.0;
}

BBRandom::~BBRandom()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRandom::~BBRandom");
}

float BBRandom::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRandom::calculateUrgency");
}

bool BBRandom::isApplicable(Information*, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRandom::isApplicable");
}

} // namespace ai
} // namespace hfe
} // namespace dice
