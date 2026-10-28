#include "BBChangeTeleport.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBChangeTeleport::BBChangeTeleport(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
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

BBChangeTeleport::~BBChangeTeleport()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBChangeTeleport::~BBChangeTeleport");
}

float BBChangeTeleport::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBChangeTeleport::calculateUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
