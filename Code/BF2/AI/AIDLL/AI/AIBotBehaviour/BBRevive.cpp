#include "BBRevive.h"

#include "CustomCurve.h"
#include "../AIBotBehaviourPattern/BBPRevive.h"
#include "../Main/AIDllFrontier.h"
#include "../AIBot/BotMain.h"                  // Bot::getManDown
#include "../AIBot/BotMemory.h"
#include "../AIBot/SquadManager.h"
#include "../AIKnowledge/BFInformationEnvironment.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"
#include <algorithm>
#include <cmath>
#include <list>

namespace dice {
namespace hfe {
namespace ai {

BBRevive::BBRevive(unsigned char behaviourId, unsigned char mode, BBPRevive* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    const int strengths = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    m_toolIndex = new int[strengths];
    m_toolRangeSquared = new float[strengths];
    m_toolStrength = new int[strengths];
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
        m_changedTarget[i] = false;
    }
}

BBRevive::~BBRevive()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRevive::~BBRevive");
}

namespace {

const int kReviveToolWeaponType = 3;
const int kMedicKit = 5;

// Whether the unit behind the information is occupied by a player the factory knows who is not
// an AI player.
bool isHumanOccupied(Information* information)
{
    IPIUnit* const unit = information->getUnit();
    if (!unit || !unit->isOccupied())
        return false;
    const int playerId = unit->getOccupyingPlayerId(information->getSide());
    IAIPlayer* const player =
        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)->getPlayerFromId(playerId);
    return player && !player->getIsAIPlayer();
}

} // namespace

// 0x22b9d0 (Ghidra). Picks, among the friendly units within 40 m, the downed soldier best worth
// reviving with the bot's revive tools (weapon type 3): not lying (pose 4), not under water, a
// strength type a tool beats and revive time left. A soldier's weight is 1 less the share of
// that time the run to it at full speed takes, times the memory's order factor for an ordered
// target, times 10 for a human. A human target within 50 m scores 100; otherwise the declining
// slope of the weight times 5 for a human, else 2, times the urgency handed in. A medic's score
// is then weighed by its squad's answer to the revive request. Nothing found: 0 and the plan
// let go. A refresh without the full evaluation keeps a target of the bot's side.
float BBRevive::calculateUrgency(Bot* bot, bool full, float value)
{
    if (!full)
    {
        Information* const target = informationManager->getInformation(m_targets[bot->getBotId()]);
        if (target && target->getSide() == bot->getSide())
        {
            m_changedTarget[bot->getBotId()] = false;
            if (!m_generator->isInitiated(bot))
                m_urgency[bot->getBotId()] = 0.0f;
        }
        else
        {
            m_changedTarget[bot->getBotId()] = m_urgency[bot->getBotId()] != 0.0f;
            m_urgency[bot->getBotId()] = 0.0f;
            m_generator->unInit(bot);
        }
        return m_urgency[bot->getBotId()];
    }

    const int strengths = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    for (int i = 0; i < strengths; ++i)
    {
        m_toolIndex[i] = -1;
        m_toolStrength[i] = 0;
    }
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    IPIArmament* const armament = equipment->getArmament();
    const Vec3& position = equipment->getPosition();
    m_candidates.resize(0);
    static_cast<IAIInformationEnvironment*>(AISingletonServer::m_iaiInformationEnvironment)
        ->getFriendlyUnits(m_candidates, position, bot, 40.0f);
    const unsigned int id = bot->getBotId();
    if (m_candidates.empty())
    {
        m_changedTarget[id] = m_urgency[id] != 0.0f;
        m_urgency[id] = 0.0f;
        m_generator->unInit(bot);
        return 0.0f;
    }

    // no null guard, as shipped
    const std::vector<unsigned int> weapons = armament->getWeapons();
    const int kit = bot->getPlayer()->getSpawnKit();
    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
        if (!weapon || weapon->getWeaponType() != kReviveToolWeaponType)
            continue;
        for (int s = 0; s < strengths; ++s)
        {
            if (weapon->getStrength(s) > m_toolStrength[s])
            {
                m_toolIndex[s] = static_cast<int>(i);
                m_toolStrength[s] = weapon->getStrength(s);
                const float range = weapon->getMaxRange();
                m_toolRangeSquared[s] = range * range;
            }
        }
    }

    float bestWeight = 0.0f;
    unsigned int orderedCount = 0;
    int bestTool = -1;
    unsigned int bestHandle = static_cast<unsigned int>(-1);
    // no null guard, as shipped
    const float maxSpeed = equipment->getMobile()->getMaxSpeed();
    float bestDistance = 100.0f;
    for (std::vector<unsigned int>::iterator it = m_candidates.begin(); it != m_candidates.end(); ++it)
    {
        Information* const candidate = informationManager->getInformation(*it);
        if (!candidate)
            continue;
        IPIPhysical* const physical = candidate->getPhysical();
        if (!physical || candidate == equipment || !candidate->getMobile()
            || !(candidate->getObjectTypeFlags() & 0x200000))
            continue;
        if (!physical->getManDown() || candidate->getSoldierPose() == 4)
            continue;
        const Vec3& at = candidate->getPosition();
        const IAITerrain* const terrain = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
        const float water = terrain->getWaterLevel(at.x, at.z);
        const float ground = terrain->getHeight(at.x, at.z);
        if (water + 0.5f > at.y && water - ground > 0.75f)
            continue;
        const int strengthType = physical->getStrengthType();
        if (m_toolIndex[strengthType] == -1)
            continue;
        const float timeLeft = physical->getManDownTimeLeft();
        if (0.001f > timeLeft)
            continue;

        const float dx = at.x - position.x;
        const float dy = at.y - position.y;
        const float dz = at.z - position.z;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        float weight = distance / maxSpeed / timeLeft;
        if (weight > 1.0f)
            weight = 1.0f;
        weight = 1.0f - weight;
        if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders())
        {
            std::list<unsigned int>& orders = *bot->getOrderList(m_behaviourId);
            if (orderedCount < orders.size() && std::find(orders.begin(), orders.end(), *it) != orders.end())
            {
                ++orderedCount;
                weight *= bot->getMemory()->m_float8;
            }
        }
        if (isHumanOccupied(candidate))
            weight *= 10.0f;
        if (weight > bestWeight)
        {
            bestDistance = distance;
            bestWeight = weight;
            bestTool = m_toolIndex[strengthType];
            bestHandle = *it;
        }
    }

    if (bestHandle == static_cast<unsigned int>(-1))
    {
        m_urgency[id] = 0.0f;
        m_changedTarget[id] = m_targets[id] != -1;
        m_targets[id] = -1;
        m_generator->unInit(bot);
        return 0.0f;
    }

    Information* const best = informationManager->getInformation(bestHandle);
    const bool human = best && isHumanOccupied(best);
    if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders()
        && bot->getOrderList(m_behaviourId)->empty())
        bestWeight *= bot->getMemory()->m_float8;
    float urgency;
    if (human && 50.0f > bestDistance)
        urgency = 100.0f;
    else if (human)
        urgency = DecleiningSlopeCurve::calculate(bestWeight) * 5.0f * value;
    else
        urgency = DecleiningSlopeCurve::calculate(bestWeight) * 2.0f * value;
    m_urgency[id] = urgency;
    m_changedTarget[id] = static_cast<unsigned int>(m_targets[id]) != bestHandle;
    m_targets[id] = static_cast<int>(bestHandle);
    static_cast<BBPRevive*>(m_generator)->init(static_cast<unsigned int>(m_targets[id]), bestTool, bot);
    if (kit == kMedicKit)
    {
        const int side = bot->getSide();
        const float response =
            SquadManager::m_instance->requestResponseModifier(bot->getSquadMember(), bestHandle, 1, side);
        m_urgency[id] = response * m_urgency[id];
    }
    return m_urgency[bot->getBotId()];
}

bool BBRevive::isAplicable(Information* information, Bot*)
{
    IPIArmament* const armament = information->getArmament();
    if (!armament)
        return false;
    const std::vector<unsigned int>& weapons = armament->getWeapons();
    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
        if (weapon && weapon->getWeaponType() == 3)
            return true;
    }
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
