#include "BBPGotoWaypointSoldier.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/SquadMember.h"
#include "../AIBot/WPCloseTo.h"
#include "../AIBot/WaypointList.h"
#include "../AIBotActionPlan/BAPAMoveToFinding.h"
#include "../AIBotActionPlan/BAPAMoveToObjectFinding.h"
#include "../AIBotActionPlan/BAPASetSensingAgentFocus.h"
#include "../AIBotActionPlan/BAPCombinerSerial.h"
#include "../AIBotActionPlan/BAPConObjectMoved.h"
#include "../AIBotActionPlan/BAPIWMoveTo.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIControlInfo.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "../AIPlanInterpreter/BotScriptDebugger.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPGotoWaypointSoldier::BBPGotoWaypointSoldier(int behaviourId)
    : BBPGotoWaypoint()
{
    m_behaviourId = behaviourId;
}

BBPGotoWaypointSoldier::~BBPGotoWaypointSoldier()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPGotoWaypointSoldier::~BBPGotoWaypointSoldier");
}

namespace {

void dropPlan(Bot* bot)
{
    if (bot->getPlan())
    {
        delete bot->getPlan();
        bot->setPlan(0);
    }
}

void clearScript(Bot* bot)
{
    if (bot->getDebugger())
        bot->getDebugger()->clear();
    bot->getScriptContext()->clear();
}

// The plan both moves share: look towards the target, then move.
BotScript* wrapMove(BBPPattern* owner, int planId, Information* equipment, const Vec3& target, BAPStatement* move)
{
    BAPCombinerSerial* const serial = checkPointer(new BAPCombinerSerial(planId, -1));
    const Vec3& position = equipment->getPosition();
    Vec3 direction;
    direction.x = target.x - position.x;
    direction.y = target.y - position.y;
    direction.z = target.z - position.z;
    serial->addStatement(checkPointer(new BAPASetSensingAgentFocus(0.5f, direction, planId, false)));
    serial->addStatement(move);
    BAPIWMoveTo* const plan = checkPointer(new BAPIWMoveTo(target, serial, planId));
    plan->m_owner = owner;
    return plan;
}

} // namespace

// 0x294730 (Ghidra). A move along the path to the waypoint list's goal, or for a list on an object
// to that object (radius the list's and at least the reach, 4 times the equipment's radius, plus the
// object's). A plan of this generator stands until the list says the target changed and the target
// moved 4 seconds of walking; a position list without a goal leaves it, one object gone drops it.
BotScript* BBPGotoWaypointSoldier::createPlan(Bot* bot)
{
    if (!m_initiated[bot->getBotId()])
    {
        clearScript(bot);
        dropPlan(bot);
        return 0;
    }
    const float maxSpeed = bot->getSquadMember()->getMaxSpeed();
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    WaypointList* const order = bot->getWaypoints();
    bool replan = false;
    if (bot->hasPlan() && static_cast<BAPStatement*>(bot->getPlan())->m_planId == m_behaviourId)
    {
        if (!order->m_targetChanged || order->getWayPointError())
            return bot->getPlan();
        order->m_targetChanged = false;
        replan = static_cast<BAPInfoWrapper*>(bot->getPlan())->getInfoType() == 2;
    }
    if (!replan)
    {
        dropPlan(bot);
        clearScript(bot);
        if (order->getWayPointError())
            return 0;
    }
    const unsigned long long controls = equipment->getControlInfo()->getDriveControls();
    float reach = equipment->getPhysical()->getRadius() * 4.0f;
    if (!replan)
        bot->getSquadMember()->clearRequestAction();
    const float moved = maxSpeed * 4.0f * maxSpeed * 4.0f;
    const int type = order->getId();
    if (type == 1 || type == 2)
    {
        Vec3 goal;
        goal.x = 0.0f;
        goal.y = 0.0f;
        goal.z = 0.0f;
        if (!order->getGoalPoint(goal))
            return 0;
        if (replan && type == 2)
        {
            const Vec3& planned = static_cast<BAPIWMoveTo*>(bot->getPlan())->getPosition();
            const float x = planned.x - goal.x;
            const float z = planned.z - goal.z;
            if (!(moved < z * z + x * x))
                return bot->getPlan();
        }
        if (replan)
        {
            dropPlan(bot);
            clearScript(bot);
        }
        BAPAMoveToFinding* const move = checkPointer(new BAPAMoveToFinding(
            bot, goal, maxSpeed, maxSpeed, 0, controls, m_behaviourId, order->getWayPointRadius(), 0.0f));
        return wrapMove(this, m_behaviourId, equipment, goal, move);
    }
    if (type == 0)
    {
        const unsigned int handle = static_cast<WPCloseTo*>(order)->getObject();
        Information* const object = informationManager->getInformation(handle);
        if (!object)
        {
            if (replan)
                dropPlan(bot);
            return 0;
        }
        const Vec3 goal = object->getPosition();
        if (replan)
        {
            const Vec3& planned = static_cast<BAPIWMoveTo*>(bot->getPlan())->getPosition();
            const float x = goal.x - planned.x;
            const float y = goal.y - planned.y;
            const float z = goal.z - planned.z;
            if (!(moved < x * x + y * y + z * z))
                return bot->getPlan();
            dropPlan(bot);
            clearScript(bot);
        }
        IPIPhysical* const physical = object->getPhysical();
        if (physical)
            reach = physical->getRadius() + reach;
        BAPConObjectMoved* const hasMoved = checkPointer(new BAPConObjectMoved(handle, reach));
        const float listed = order->getWayPointRadius();
        BAPAMoveToObjectFinding* const move = checkPointer(new BAPAMoveToObjectFinding(
            bot, handle, listed <= reach ? reach : listed, maxSpeed, maxSpeed, 0, controls, m_behaviourId, hasMoved,
            0.0f, -1));
        return wrapMove(this, m_behaviourId, equipment, goal, move);
    }
    if (replan)
        dropPlan(bot);
    return 0;
}

// 0x294700 (Ghidra).
void BBPGotoWaypointSoldier::init(Bot* bot)
{
    m_initiated[bot->getBotId()] = true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
