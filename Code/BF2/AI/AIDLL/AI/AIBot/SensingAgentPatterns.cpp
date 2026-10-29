// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SensingAgentPatterns operations.

#include "SensingAgentPatterns.h"

#include "BotMain.h"                       // Bot
#include "SensingAgent.h"
#include "../AIBotActionPlan/BAPALookAtObject.h"
#include "../AIBotActionPlan/BAPALookInDir.h"
#include "../AIBotActionPlan/BAPAMergeSoldierInputs.h"
#include "../AIBotActionPlan/BAPCombinerParallel.h"
#include "../AIBotActionPlan/BAPCondition.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIControlInfo.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// The base owns the shared id field used by every pattern's getId/reset pair.
// Constructors and destructors perform no additional member initialization or cleanup.
ISensingAgentPattern::~ISensingAgentPattern()
{
}

SensingAgentTurretPattern::SensingAgentTurretPattern()
{
}

SensingAgentTurretPattern::~SensingAgentTurretPattern()
{
}

int SensingAgentTurretPattern::getId() const
{
    return m_id;
}

void SensingAgentTurretPattern::reset(int id)
{
    m_id = id;
}

SensingAgentMouseFreeLookPattern::SensingAgentMouseFreeLookPattern()
{
}

SensingAgentMouseFreeLookPattern::~SensingAgentMouseFreeLookPattern()
{
}

int SensingAgentMouseFreeLookPattern::getId() const
{
    return m_id;
}

void SensingAgentMouseFreeLookPattern::reset(int id)
{
    m_id = id;
}

SensingAgentSoldierPattern::SensingAgentSoldierPattern()
{
}

SensingAgentSoldierPattern::~SensingAgentSoldierPattern()
{
}

int SensingAgentSoldierPattern::getId() const
{
    return m_id;
}

void SensingAgentSoldierPattern::reset(int id)
{
    m_id = id;
}

SensingAgentNullPattern::SensingAgentNullPattern()
{
}

SensingAgentNullPattern::~SensingAgentNullPattern()
{
}

int SensingAgentNullPattern::getId() const
{
    return m_id;
}

void SensingAgentNullPattern::reset(int id)
{
    m_id = id;
}

// [FRONTIER] Pattern plan creation and soldier helpers require the look/aim plan runtime.
// [RETIRE-WHEN: aidll-02-skeleton] Replace each stop when its plan behavior is recovered.
BAPStatement* SensingAgentTurretPattern::createPlan(SensingAgent *, Bot *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentTurretPattern::createPlan");
}

BAPStatement* SensingAgentTurretPattern::createPlan(SensingAgent *, Bot *, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentTurretPattern::createPlan");
}

BAPStatement* SensingAgentMouseFreeLookPattern::createPlan(SensingAgent *, Bot *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentMouseFreeLookPattern::createPlan");
}

BAPStatement* SensingAgentMouseFreeLookPattern::createPlan(SensingAgent *, Bot *, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentMouseFreeLookPattern::createPlan");
}

// Look along the direction until told otherwise (BAPConFalse never holds).
BAPStatement* SensingAgentSoldierPattern::createPlan(SensingAgent* agent, Bot* bot,
                                                     const Vec3& direction)
{
    IPIControlInfo* control = 0;
    BotScript* plan = 0;
    if (!initPlan(agent, bot, control, plan))
        return static_cast<BAPStatement*>(plan);
    BAPALookInDir* const look = checkPointer(new BAPALookInDir(
        direction, 0.0f, checkPointer(new BAPConFalse()), control->getLookControls(), getId()));
    return endPlan(look, bot);
}

BAPStatement* SensingAgentSoldierPattern::createPlan(SensingAgent* agent, Bot* bot,
                                                     unsigned int target)
{
    IPIControlInfo* control = 0;
    BotScript* plan = 0;
    if (!initPlan(agent, bot, control, plan))
        return static_cast<BAPStatement*>(plan);
    BAPALookAtObject* const look = checkPointer(new BAPALookAtObject(
        bot, target, 0.0f, checkPointer(new BAPConFalse()), control->getLookControls(), getId(),
        0, 0.0f));
    return endPlan(look, bot);
}

// Both createPlan overloads pass a null plan, so the reuse arm is not reached from them.
bool SensingAgentSoldierPattern::initPlan(SensingAgent* agent, Bot* bot, IPIControlInfo*& control,
                                          BotScript*& plan)
{
    if (plan)
    {
        if (agent->getPlan()->m_planId == getId())
            return false;
        if (plan)
            delete plan;
        agent->setPlan(0);
    }
    agent->getBAPContext()->clear();

    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    if (!equipment)
    {
        plan = 0;
        return false;
    }
    control = equipment->getControlInfo();
    if (!control)
    {
        plan = 0;
        return false;
    }
    return true;
}

BAPStatement* SensingAgentSoldierPattern::endPlan(BAPALook* look, Bot*)
{
    BAPCombinerParallel* const plan = checkPointer(new BAPCombinerParallel(getId(), -1, false));
    BAPAMergeSoldierInputs* const merge = checkPointer(new BAPAMergeSoldierInputs(getId()));
    plan->addStatement(look);
    plan->addStatement(merge);
    plan->m_owner = this;
    return plan;
}

// Null-pattern planning still requires an implementation; these overloads stop visibly.
BAPStatement* SensingAgentNullPattern::createPlan(SensingAgent *, Bot *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentNullPattern::createPlan");
}

BAPStatement* SensingAgentNullPattern::createPlan(SensingAgent *, Bot *, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentNullPattern::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
