#include "BBIdle.h"

#include "../AIBot/BotMain.h"
#include "../AIBotBehaviourPattern/BBPIdle.h"
#include "../AIBotBehaviourPattern/BBPIdle2d.h"
#include "../AIBotBehaviourPattern/BBPIdle3d.h"
#include "../AIBotBehaviourPattern/BBPIdleInfantery.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
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

// 0x224e90 (Ghidra). Idling is as urgent as the urgency handed in; the full-evaluation flag is
// not read. A primary vehicle that is not a soldier and has a mobile plug-in also waits for
// repairs and for passengers to be picked up, with its radio calls - that arm is a named stop.
// Any other bot drops its repair wait and pickup state, and the idle pattern starts for it.
float BBIdle::calculateUrgency(Bot* bot, bool, float value)
{
    BBPIdle* const generator = static_cast<BBPIdle*>(m_generator);
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    equipment->getPrimaryObject(bot->getSide());    // the result is discarded
    float urgency = value;
    const int target = -1;
    if (equipment->isPrimary() && !(equipment->getObjectTypeFlags() & 0x200000) && equipment->getMobile())
    {
        // [FRONTIER] [TEMP] vehicles (part 5): the repair wait, BBIdle::waitForPickup (0x2256c0) and
        // the pickup radio messages. [RETIRE-WHEN: part-5-vehicles]
        BF2_AI_FRONTIER("dice::hfe::ai::BBIdle::calculateUrgency (vehicle arm)");
    }

    BotMain* const main = static_cast<BotMain*>(bot);
    main->setIsRepaired(false);
    main->setShouldWaitForRepairs(false);
    m_double24[bot->getBotId()] = 0.0;
    m_bool28[bot->getBotId()] = false;
    m_float2C[bot->getBotId()] = 0.0f;
    urgency = value;

    m_urgency[bot->getBotId()] = urgency;
    if (generator->getType() == 1)
        static_cast<BBPIdle2d*>(generator)->init(bot, target);
    else if (generator->getType() == 2)
        static_cast<BBPIdle3d*>(generator)->init(50.0f, bot, target);
    else if (generator->getType() == 3)
        static_cast<BBPIdleInfantery*>(generator)->init(bot);
    return urgency;
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
