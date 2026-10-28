#include "BBFire3d.h"

#include "../AIBotBehaviourPattern/BBPFire.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBFire3d::BBFire3d(unsigned char behaviourId, unsigned char mode, BBPFire* generator, UrgeCurve* urgeCurve,
                   float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
    , m_double28(0.0)
{
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

BBFire3d::~BBFire3d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFire3d::~BBFire3d");
}

float BBFire3d::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFire3d::calculateUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
