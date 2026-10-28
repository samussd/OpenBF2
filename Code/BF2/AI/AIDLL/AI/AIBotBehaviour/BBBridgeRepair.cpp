#include "BBBridgeRepair.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

BBBridgeRepair::BBBridgeRepair(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    for (int i = 0; i < 3; ++i)
        m_words24[i] = 0;
    const int strengths = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    m_int30 = new int[strengths];
    m_int34 = new int[strengths];
    m_int38 = new int[strengths];
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

BBBridgeRepair::~BBBridgeRepair()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBBridgeRepair::~BBBridgeRepair");
}

float BBBridgeRepair::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBBridgeRepair::calculateUrgency");
}

bool BBBridgeRepair::isApplicable(Information*, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBBridgeRepair::isApplicable");
}

} // namespace ai
} // namespace hfe
} // namespace dice
