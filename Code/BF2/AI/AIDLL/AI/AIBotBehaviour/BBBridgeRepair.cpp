#include "BBBridgeRepair.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/SquadCoordinator.h"
#include "../AIBot/SquadManager.h"
#include "../AIBot/SquadMember.h"
#include "../AIBotBehaviourPattern/BBPBridgeRepair.h"
#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include <vector>
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

namespace {

const int kRepairToolWeaponType = 1;
const int kEngineerKit = 4;

// The squared distance between two points.
float distanceSquared(const Vec3& a, const Vec3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

} // namespace

// 0x20de20 (Ghidra). The shipped body also asks for the bot id and drops it.
bool BBBridgeRepair::getBridgeCoordinate(unsigned int handle, Vec3 direction, Bot* bot,
                                         Vec3 botPosition, Vec3& stand, Vec3& look)
{
    bot->getBotId();
    Vec3 back;
    back.x = -direction.x;
    back.y = -direction.y;
    back.z = -direction.z;
    // no null guard, as shipped
    const float radius = informationManager->getInformation(handle)->getPhysical()->getRadius();
    Vec3 ahead;
    ahead.x = direction.x * radius + stand.x;
    ahead.y = direction.y * radius + stand.y;
    ahead.z = direction.z * radius + stand.z;
    Vec3 behind;
    behind.x = back.x * radius + stand.x;
    behind.y = back.y * radius + stand.y;
    behind.z = back.z * radius + stand.z;
    const Vec3 aheadEdge = ahead;
    const Vec3 behindEdge = behind;
    SquadMember* const member = bot->getSquadMember();
    SquadCoordinator* const squad = member->getSquad();
    ahead.y += 1.5f;
    behind.y += 1.5f;

    bool aheadFound = false;
    for (int steps = 0; steps < 10 && !aheadFound;)
    {
        if (squad->isValidPosition(member, ahead))
        {
            aheadFound = true;
        }
        else
        {
            ++steps;
            ahead.x += direction.x;
            ahead.y += direction.y;
            ahead.z += direction.z;
        }
    }
    bool behindFound = false;
    for (int steps = 0; steps < 10 && !behindFound;)
    {
        if (squad->isValidPosition(member, behind))
        {
            behindFound = true;
        }
        else
        {
            ++steps;
            behind.x += back.x;
            behind.y += back.y;
            behind.z += back.z;
        }
    }

    if (aheadFound && !(behindFound
                        && distanceSquared(behind, botPosition) <= distanceSquared(ahead, botPosition)))
    {
        stand = ahead;
        look = aheadEdge;
        return true;
    }
    if (!behindFound)
        return false;
    stand = behind;
    look = behindEdge;
    return true;
}

// 0x20e2e0 (Ghidra). For an engineer (soldier or type 0x40 equipment) not swimming: the closest
// destroyed bridge segment it may repair, within 50 m (100 m near a squad repair request), with
// a place to stand: the urgency handed in times 20, times the squad's request response factor,
// doubled near a request. Otherwise 0 and the plan let go. A refresh without the full
// evaluation drops a target that no longer exists, else keeps the urgency while the plan runs.
float BBBridgeRepair::calculateUrgency(Bot* bot, bool full, float value)
{
    if (!full)
    {
        Information* const target = informationManager->getInformation(m_targets[bot->getBotId()]);
        if (!target)
        {
            m_changedTarget[bot->getBotId()] = m_urgency[bot->getBotId()] != 0.0f;
            m_urgency[bot->getBotId()] = 0.0f;
            m_generator->unInit(bot);
        }
        else
        {
            m_changedTarget[bot->getBotId()] = false;
            if (!m_generator->isInitiated(bot))
                m_urgency[bot->getBotId()] = 0.0f;
        }
        return m_urgency[bot->getBotId()];
    }

    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const unsigned int id = bot->getBotId();
    const Vec3 position = equipment->getPosition();
    int toolIndex = -1;
    static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    if (isApplicable(equipment, bot)
        && (equipment->getObjectTypeFlags() & 0x200000 || equipment->getObjectTypeFlags() & 0x40))
    {
        if ((equipment->getObjectTypeFlags() & 0x200000) && equipment->getSoldierPose() == 4)
        {
            m_changedTarget[id] = true;
            m_generator->unInit(bot);
            m_urgency[id] = 0.0f;
            return 0.0f;
        }

        IAIObject* const soldier = bot->getPlayer()->getSoldier();
        // no null guard, as shipped
        Information* const soldierInfo =
            informationManager->getInformation(soldier->getInformation(bot->getSide()));
        const std::vector<unsigned int> weapons = soldierInfo->getArmament()->getWeapons();
        for (unsigned int i = 0; i < weapons.size(); ++i)
        {
            WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
            if (weapon && weapon->getWeaponType() == kRepairToolWeaponType)
            {
                toolIndex = static_cast<int>(i);
                break;
            }
        }

        SquadMember* const member = bot->getSquadMember();
        SquadCoordinator* const squad = member->getSquad();
        const bool requested = squad->isRepairRequested();
        Vec3 direction;
        direction.x = 0.0f;
        direction.y = 0.0f;
        direction.z = 0.0f;
        Vec3 stand = direction;
        Vec3 look = direction;
        bool nearRequest = false;
        const int side = bot->getSide();
        const Vec3 here = equipment->getPosition();
        float distance;
        IAIObject* const segment = AIStrategicObjectManager::getInstance()->getClosestDestroyedBridgeSegment(
            here, requested, side, direction, distance);
        if (segment
            && direction.z * direction.z + direction.y * direction.y + direction.x * direction.x > 0.01f)
        {
            const unsigned int handle = segment->getHandle(1);
            // no null guard, as shipped
            const Vec3 segmentPosition = informationManager->getInformation(handle)->getPosition();
            if (requested && distanceSquared(squad->getRepairRequestPos(), segmentPosition) < 2500.0f)
                nearRequest = true;
            const float reach = nearRequest ? 10000.0f : 2500.0f;
            if (reach > distance && handle != static_cast<unsigned int>(-1))
            {
                stand = segmentPosition;
                if (getBridgeCoordinate(handle, direction, bot, position, stand, look))
                {
                    m_urgency[id] = value * 20.0f;
                    m_changedTarget[id] = static_cast<unsigned int>(m_targets[id]) != handle;
                    m_targets[id] = static_cast<int>(handle);
                    static_cast<BBPBridgeRepair*>(m_generator)->init(
                        static_cast<unsigned int>(m_targets[id]), toolIndex, direction, bot,
                        nearRequest, stand, look);
                    const int memberSide = bot->getSide();
                    const float response = SquadManager::m_instance->requestResponseModifier(
                        bot->getSquadMember(), handle, 2, memberSide);
                    m_urgency[id] = response * m_urgency[id];
                    if (nearRequest)
                        m_urgency[id] = m_urgency[id] + m_urgency[id];
                    return m_urgency[bot->getBotId()];
                }
            }
        }
    }

    m_urgency[id] = 0.0f;
    m_changedTarget[id] = m_targets[id] != -1;
    m_targets[id] = -1;
    m_generator->unInit(bot);
    return 0.0f;
}

// 0x20ecb0 (Ghidra). A live, standing engineer.
bool BBBridgeRepair::isApplicable(Information*, Bot* bot)
{
    if (!bot->getPlayer()->getIsAlive() || bot->getManDown())
        return false;
    return bot->getPlayer()->getSpawnKit() == kEngineerKit;
}

} // namespace ai
} // namespace hfe
} // namespace dice
