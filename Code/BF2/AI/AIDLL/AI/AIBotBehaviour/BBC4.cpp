#include "BBC4.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/SquadManager.h"
#include "../AIBotBehaviourPattern/BBPC4.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../AIStrategic/SAI.h"
#include "../AIStrategicEntity/AIStrategicObject.h"
#include "../Main/AIMain.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include <vector>
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBC4::BBC4(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
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

BBC4::~BBC4()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBC4::~BBC4");
}

namespace {

const int kC4WeaponType = 4;

} // namespace

// 0x20f290 (Ghidra). For a soldier (object type 0x200000) that has C4 to use. With a charge of
// placement type 0 down: 100, to detonate, while the enemy takes the object it was placed for
// and no friendly is within 15 m of it. Without a charge: 500, to place one, inside an owned
// control point (10 m). Otherwise 0 and the plan let go. A refresh without the full evaluation
// is always 0.
float BBC4::calculateUrgency(Bot* bot, bool full, float)
{
    if (!full)
    {
        m_urgency[bot->getBotId()] = 0.0f;
        return m_urgency[bot->getBotId()];
    }

    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    float urgency = 0.0f;
    bool detonate = false;
    AIStrategicObject* target = 0;
    SAI* const sai = IAIMain::m_instance->getSAI(bot->getSide());
    if (equipment && (equipment->getObjectTypeFlags() & 0x200000) && isApplicable(equipment, bot))
    {
        std::vector<unsigned int>& weapons = equipment->getArmament()->getWeapons();
        WeaponInfo* c4 = 0;
        for (unsigned int i = 0; i < weapons.size(); ++i)
        {
            c4 = WeaponInfoManager::m_instance->getInformation(weapons[i]);
            if (c4 && c4->getWeaponType() == kC4WeaponType)
                break;
        }
        const Bot::C4Info charge = bot->getC4();
        const Vec3 position = equipment->getPosition();
        if (charge.placed)
        {
            target = charge.target;
            if (charge.placementType == 0)
            {
                AIStrategicObject* taken = 0;
                Vec2 where;
                where.x = 0.0f;
                where.y = 0.0f;
                // no null guard on the object taken, as shipped
                if (sai->hostileTakeoveInProgress(taken, where) && charge.target
                    && taken->getIndex() == charge.target->getIndex()
                    && !SquadManager::m_instance->friendliesCloseTo(charge.position, 15.0f,
                                                                     bot->getSide()))
                {
                    urgency = 100.0f;
                    detonate = true;
                }
            }
        }
        else
        {
            detonate = false;
            // the last weapon looked at when none is C4; isApplicable rules that out
            if (c4->getCurrentAmmo() > 0 && sai->isPosInsideOwnedControlPoint(target, position, 10.0f))
                urgency = 500.0f;
        }
    }

    if (urgency < 0.001f)
        m_generator->unInit(bot);
    else
        static_cast<BBPC4*>(m_generator)->init(bot, detonate, target);
    m_urgency[bot->getBotId()] = urgency;
    return m_urgency[bot->getBotId()];
}

// 0x20f690 (Ghidra). A live, standing bot whose equipment carries C4 with ammo left.
bool BBC4::isApplicable(Information* information, Bot* bot)
{
    IPIArmament* const armament = information->getArmament();
    if (!bot->getPlayer()->getIsAlive() || bot->getManDown() || !armament)
        return false;
    std::vector<unsigned int>& weapons = armament->getWeapons();
    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
        if (weapon && weapon->getWeaponType() == kC4WeaponType && weapon->getCurrentAmmo() > 0)
            return true;
    }
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
