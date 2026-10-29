#include "BBPIdleInfantery.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/SquadMember.h"
#include "../AIBotActionPlan/BAPAResetControls.h"
#include "../AIBotActionPlan/BAPASoldierPose.h"
#include "../AIBotActionPlan/BAPATrigger.h"
#include "../AIBotActionPlan/BAPBoleanOp.h"
#include "../AIBotActionPlan/BAPCombinerParallel.h"
#include "../AIBotActionPlan/BAPConWeaponIsZoomed.h"
#include "../AIBotActionPlan/BAPFlowCIf.h"
#include "../AIBotActionPlan/BAPFlowCWhile.h"
#include "../AIBotActionPlan/BAPIWIdle.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "../AIPlanInterpreter/BotScriptDebugger.h"
#include "../AIStrategicEntity/AIStrategicObject.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPIdleInfantery::BBPIdleInfantery(int behaviourId)
    : BBPIdle(behaviourId)
{
}

BBPIdleInfantery::~BBPIdleInfantery()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdleInfantery::~BBPIdleInfantery");
}

// The idle plan: release every control, then loop forever over a parallel step. Within 2 m of the
// rim of the closest strategic object of the bot's side, the step also zooms a scoped weapon back
// out (a trigger on its zoom input while it is zoomed in) and, for a soldier, holds a crouch.
// An idle plan of this generator already in place is kept. A bot this generator was not started
// for loses its plan.
BotScript* BBPIdleInfantery::createPlan(Bot* bot)
{
    if (!m_initiated[bot->getBotId()])
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

    if (bot->getDebugger())
        bot->getDebugger()->clear();
    bot->getScriptContext()->clear();
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    if (bot->hasPlan() && static_cast<BAPStatement*>(bot->getPlan())->m_planId == m_behaviourId)
        return bot->getPlan();
    if (bot->getPlan())
    {
        delete bot->getPlan();
        bot->setPlan(0);
    }
    bot->getSquadMember()->clearRequestAction();

    BAPCombinerParallel* const step =
        checkPointer(new BAPCombinerParallel(m_behaviourId, false, 99, false));
    step->addStatement(checkPointer(new BAPAResetControls(true, true, true, m_behaviourId)));

    // Retail does not guard the equipment.
    const Vec3& position = equipment->getPosition();
    const int side = bot->getSide();
    AIStrategicObject* const closest =
        AIStrategicObjectManager::getInstance()->getClosestObject(position, side, false);
    if (closest)
    {
        const float radius = closest->m_size[bot->getSide()];
        Vec3 centre = { 0.0f, 0.0f, 0.0f };
        vec2ToVec3XZ(closest->getPosition(bot->getSide()), centre);
        centre.y = position.y;
        const Vec3 offset = { centre.x - position.x, centre.y - position.y, centre.z - position.z };
        const float reach = radius + 2.0f;
        if (offset.x * offset.x + offset.y * offset.y + offset.z * offset.z < reach * reach)
        {
            IPIArmament* const armament = equipment->getArmament();
            if (armament && armament->getActiveWeapon() != -1)
            {
                WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(
                    armament->getWeapon(armament->getActiveWeapon()));
                if (weapon && weapon->getWeaponZoom() != io::PINone)
                {
                    // Retail shifts in 32 bits and sign-extends the mask.
                    const int zoomBit = 1 << (weapon->getWeaponZoom() & 31);
                    BAPATrigger* const zoomOut = checkPointer(new BAPATrigger(
                        static_cast<io::PlayerInputMap>(weapon->getWeaponZoom()),
                        checkPointer(new BAPConTrue()), checkPointer(new BAPConFalse()),
                        static_cast<unsigned long long>(static_cast<long long>(zoomBit)),
                        m_behaviourId, 0.0, false));
                    BAPConNot* const notZoomed =
                        checkPointer(new BAPConNot(checkPointer(new BAPConWeaponIsZoomed())));
                    step->addStatement(checkPointer(
                        new BAPFlowCIf(notZoomed, zoomOut, 0, m_behaviourId, true, false)));
                }
            }
            if (equipment->getObjectTypeFlags() & 0x200000u)
            {
                step->addStatement(checkPointer(new BAPASoldierPose(
                    SPCrouching, checkPointer(new BAPConTrue()), m_behaviourId, true)));
            }
        }
    }

    BAPFlowCWhile* const loop =
        checkPointer(new BAPFlowCWhile(checkPointer(new BAPConTrue()), step, m_behaviourId));
    BAPIWIdle* const plan =
        checkPointer(new BAPIWIdle(equipment->getPosition(), loop, m_behaviourId, bot));
    plan->m_owner = this;
    return plan;
}

int BBPIdleInfantery::getType()
{
    return 3;
}

void BBPIdleInfantery::init(Bot* bot)
{
    m_initiated[bot->getBotId()] = true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
