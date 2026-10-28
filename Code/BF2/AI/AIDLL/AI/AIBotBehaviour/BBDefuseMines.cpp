#include "BBDefuseMines.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

BBDefuseMines::BBDefuseMines(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    for (int i = 0; i < 3; ++i)
        m_words24[i] = 0;
    const int strengths = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    m_int30 = new int[strengths];
    m_int34 = new int[getNumberOfBots()];
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
        m_int34[i] = -1;
    }
}

BBDefuseMines::~BBDefuseMines()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBDefuseMines::~BBDefuseMines");
}

float BBDefuseMines::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBDefuseMines::calculateUrgency");
}

bool BBDefuseMines::isApplicable(Information*, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBDefuseMines::isApplicable");
}

} // namespace ai
} // namespace hfe
} // namespace dice
