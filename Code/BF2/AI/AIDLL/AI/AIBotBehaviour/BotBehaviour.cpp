#include "BotBehaviour.h"

#include "../AIBot/BotMain.h"                           // Bot::getBotId
#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

BotBehaviour::BotBehaviour(float* weights, UrgeCurve* urgeCurve, int numberOfBots)
    : m_targets(0)
    , m_generator(0)
{
    m_urgency = new float[numberOfBots];
    m_weights = weights;
    m_changedTarget = new bool[numberOfBots];
    m_activeUrgency = new float[numberOfBots];
    m_urgeCurve = urgeCurve;
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_activeUrgency[i] = 1e-05f;
        m_changedTarget[i] = true;
    }
}

BotBehaviour::~BotBehaviour()
{
    delete[] m_urgency;
    delete[] m_changedTarget;
    delete[] m_activeUrgency;
    delete m_generator;
}

float BotBehaviour::getUrgency(Bot* bot)
{
    return m_urgency[bot->getBotId()];
}

float BotBehaviour::getActiveUrgency(Bot* bot)
{
    return m_activeUrgency[bot->getBotId()];
}

float BotBehaviour::updateActiveUrgency(Bot* bot)
{
    m_activeUrgency[bot->getBotId()] = m_urgency[bot->getBotId()];
    return m_activeUrgency[bot->getBotId()];
}

bool BotBehaviour::hasChangedTarget(Bot* bot)
{
    return m_changedTarget[bot->getBotId()];
}

// True when the urgency has left [low, high] relative to the active one. With no active
// urgency, any positive urgency counts; a zero urgency always counts.
bool BotBehaviour::quotientChanged(Bot* bot, float low, float high)
{
    if (m_activeUrgency[bot->getBotId()] == 0.0f)
        return 0.0f < m_urgency[bot->getBotId()];
    if (m_urgency[bot->getBotId()] == 0.0f)
        return true;
    const float quotient = m_urgency[bot->getBotId()] / m_activeUrgency[bot->getBotId()];
    return quotient < low || high < quotient;
}

bool BotBehaviour::hasChanged(Bot* bot, float low, float high)
{
    return hasChangedTarget(bot) && quotientChanged(bot, low, high);
}

UrgencyGenerator* BotBehaviour::getChosenGenerator()
{
    return this;
}

void BotBehaviour::reset(Bot* bot)
{
    if (m_generator)
        m_generator->unInit(bot);
    m_urgency[bot->getBotId()] = 0.0f;
    m_activeUrgency[bot->getBotId()] = 1e-04f;
    m_changedTarget[bot->getBotId()] = true;
    if (m_targets)
        m_targets[bot->getBotId()] = -1;
}

BotScript* BotBehaviour::generatePlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotBehaviour::generatePlan");
}

bool BotBehaviour::isAplicable(Information*, Bot*)
{
    return true;
}

int BotBehaviour::getNumberOfBots()
{
    return static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings)->getMaxNBots();
}

} // namespace ai
} // namespace hfe
} // namespace dice
