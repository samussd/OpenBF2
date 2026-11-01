#include "BBPIdle2d.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBot/CommunicationAgent.h"
#include "../AIBot/SquadMember.h"
#include "../AIBotActionPlan/BAPAResetControls.h"
#include "../AIBotActionPlan/BAPASendRadioMessage.h"
#include "../AIBotActionPlan/BAPAUseFlareOnMissileLock.h"
#include "../AIBotActionPlan/BAPCombinerParallel.h"
#include "../AIBotActionPlan/BAPCombinerSerial.h"
#include "../AIBotActionPlan/BAPCondition.h"
#include "../AIBotActionPlan/BAPFlowCWhile.h"
#include "../AIBotActionPlan/BAPIWIdle.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "../AIPlanInterpreter/BotScriptDebugger.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"
#include "../../../../IO/Input/Input.h"

namespace dice {
namespace hfe {
namespace ai {

BBPIdle2d::BBPIdle2d(int behaviourId)
    : BBPIdle(behaviourId)
{
    m_int0C = new int[getNumberOfBots()];
    // The bound is re-read from the settings on every pass.
    for (int i = 0; i < getNumberOfBots(); ++i)
        m_int0C[i] = -1;
}

BBPIdle2d::~BBPIdle2d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdle2d::~BBPIdle2d");
}

// 0x29cb10 (Ghidra).
void BBPIdle2d::init(Bot* bot, int target)
{
    m_initiated[bot->getBotId()] = true;
    m_int0C[bot->getBotId()] = target;
}

// Releases every control and stays put, watching for missile locks. The man-down state uses this
// plan too. Waiting for a passenger (a target set by init) repeats its radio answer.
BotScript* BBPIdle2d::createPlan(Bot* bot)
{
    const unsigned int id = bot->getBotId();
    if (!m_initiated[id])
    {
        if (bot->getDebugger())
            bot->getDebugger()->clear();
        bot->getScriptContext()->clear();
        if (bot->getPlan())
        {
            delete bot->getPlan();
            bot->setPlan(0);
        }
        return 0;
    }

    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    if (!bot->hasPlan())
    {
        if (bot->getPlan())
        {
            delete bot->getPlan();
            bot->setPlan(0);
        }
    }
    else if (static_cast<BAPStatement*>(bot->getPlan())->m_planId == m_behaviourId)
    {
        const int target = m_int0C[id];
        if (target >= 0 && (equipment->getObjectTypeFlags() & 0x200000u) == 0
            && bot->getCommunicationAgent()->confirmReceivedMessage(
                   static_cast<AIRadioMessageType>(2), target))
        {
            bot->getCommunicationAgent()->enqueueRadioMessage(
                static_cast<AIRadioMessageType>(6), static_cast<AIRadioChannelType>(6), 3, 1.0f, -1);
        }
        return bot->getPlan();
    }

    if (bot->getDebugger())
        bot->getDebugger()->clear();
    bot->getScriptContext()->clear();
    bot->getSquadMember()->clearRequestAction();

    BAPCombinerSerial* const serial = checkPointer(new BAPCombinerSerial(m_behaviourId, -1));
    if (m_int0C[id] >= 0)
    {
        Information* const primary =
            informationManager->getInformation(equipment->getPrimaryObject(bot->getSide()));
        if ((primary->getObjectTypeFlags() & 0x200000u) == 0)
            // [FRONTIER] [TEMP] [RETIRE-WHEN: aidll-02-skeleton] Needs BBPIdle::freePosition.
            BF2_AI_FRONTIER("dice::hfe::ai::BBPIdle2d::createPlan (waiting for a passenger)");
    }

    serial->addStatement(checkPointer(new BAPFlowCWhile(
        checkPointer(new BAPConTrue()),
        checkPointer(new BAPAResetControls(true, true, true, m_behaviourId)), m_behaviourId)));

    BAPCombinerParallel* const parallel =
        checkPointer(new BAPCombinerParallel(m_behaviourId, true, -1, false));
    parallel->addStatement(checkPointer(
        new BAPAUseFlareOnMissileLock(bot, 1ULL << io::PIFlareFire, m_behaviourId)));
    parallel->addStatement(serial);

    BAPIWIdle* const plan =
        checkPointer(new BAPIWIdle(equipment->getPosition(), parallel, m_behaviourId, bot));
    plan->m_owner = this;
    bot->getBotNavigation()->clearProgress();
    return plan;
}

int BBPIdle2d::getType()
{
    return 1;
}

} // namespace ai
} // namespace hfe
} // namespace dice
