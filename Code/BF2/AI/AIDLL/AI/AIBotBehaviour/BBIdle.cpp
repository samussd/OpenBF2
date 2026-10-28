#include "BBIdle.h"

#include "../AIBotBehaviourPattern/BBPIdle.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBIdle::BBIdle(unsigned char behaviourId, unsigned char mode, BBPIdle* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_double24 = new double[numberOfBots];
    m_bool28 = new bool[numberOfBots];
    m_float2C = new float[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 1.0f;
        m_changedTarget[i] = false;
        m_double24[i] = 0.0;
        m_bool28[i] = false;
        m_float2C[i] = 0.0f;
    }
    m_targets = 0;
}

BBIdle::~BBIdle()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBIdle::~BBIdle");
}

float BBIdle::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBIdle::calculateUrgency");
}

void BBIdle::reset(Bot*)
{
    const int numberOfBots = getNumberOfBots();
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 1.0f;
        m_changedTarget[i] = false;
        m_double24[i] = 0.0;
        m_bool28[i] = false;
        m_float2C[i] = 0.0f;
    }
    m_targets = 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
