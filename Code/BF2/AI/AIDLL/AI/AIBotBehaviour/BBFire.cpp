#include "BBFire.h"

#include "CustomCurve.h"
#include "../AIBotBehaviourPattern/BBPFire.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBFire::BBFire(unsigned char behaviourId, unsigned char mode, BBPFire* generator,
               UrgeCurve* urgeCurve, float* weights)
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
    m_curve = new CustomCurve(CustomCurve::modifiedSoftStepFunc, 0.0f, 1.0f, 0.03f);
}

BBFire::~BBFire()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFire::~BBFire");
}

float BBFire::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFire::calculateUrgency");
}

void BBFire::initializeGenerator(BotMain*, int, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFire::initializeGenerator(BotMain*, int, bool, bool)");
}

void BBFire::initializeGenerator(unsigned int, const Vec3&, BotMain*, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFire::initializeGenerator(unsigned int, const Vec3&, BotMain*, int)");
}

} // namespace ai
} // namespace hfe
} // namespace dice
