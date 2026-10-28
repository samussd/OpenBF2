#include "BBRunOverSoldier.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBRunOverSoldier::BBRunOverSoldier(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
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

BBRunOverSoldier::~BBRunOverSoldier()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRunOverSoldier::~BBRunOverSoldier");
}

float BBRunOverSoldier::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRunOverSoldier::calculateUrgency");
}

bool BBRunOverSoldier::isApplicable(Information*, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRunOverSoldier::isApplicable");
}

} // namespace ai
} // namespace hfe
} // namespace dice
