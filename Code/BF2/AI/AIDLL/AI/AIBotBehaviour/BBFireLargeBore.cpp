#include "BBFireLargeBore.h"

#include "../AIBotBehaviourPattern/BBPFire.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBFireLargeBore::BBFireLargeBore(unsigned char behaviourId, unsigned char mode, BBPFire* generator, UrgeCurve* urgeCurve,
                                 float* weights, bool flag)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
    , m_double28(0.0)
{
    m_flag3C = flag;
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    m_int24 = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
        m_int24[i] = -1;
        m_targets[i] = -1;
    }
    generator->setUrgencyGenerator(this);
}

BBFireLargeBore::~BBFireLargeBore()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFireLargeBore::~BBFireLargeBore");
}

float BBFireLargeBore::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFireLargeBore::calculateUrgency");
}

void BBFireLargeBore::initializeGenerator(BotMain*, int, bool, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFireLargeBore::initializeGenerator");
}

} // namespace ai
} // namespace hfe
} // namespace dice
