#include "BBMedicAssist.h"

#include "CustomCurve.h"
#include "../AIBotBehaviourPattern/BBPMedicAssist.h"
#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"
#include "../AIBot/BotMain.h"                  // Bot::getManDown
#include "../AIBot/BotMemory.h"
#include "../AIBot/CommunicationAgent.h"
#include "../AIBot/SquadCoordinator.h"
#include "../AIBot/SquadManager.h"
#include "../AIBot/SquadMember.h"
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
#include "../Main/IBotManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"
#include "../../../../Math/Mat4.h"
#include <algorithm>
#include <cmath>
#include <list>

namespace dice {
namespace hfe {
namespace ai {

BBMedicAssist::BBMedicAssist(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    const int strengths = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    m_toolIndex = new int[strengths];
    m_toolRange = new float[strengths];
    m_toolStrength = new int[strengths];
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

BBMedicAssist::~BBMedicAssist()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMedicAssist::~BBMedicAssist");
}

namespace {

const int kRepairToolWeaponType = 1;   // the medic bag and the wrench alike
const int kSupportKit = 3;
const int kEngineerKit = 4;
const int kMedicKit = 5;

// Whether the unit behind the information is occupied, for the side, by a player the factory
// knows who is not an AI player.
bool isHumanOccupied(Information* information, int side)
{
    IPIUnit* const unit = information->getUnit();
    if (!unit || !unit->isOccupied())
        return false;
    IAIPlayer* const player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
                                  ->getPlayerFromId(unit->getOccupyingPlayerId(side));
    return player && !player->getIsAIPlayer();
}

float distanceBetween(const Vec3& a, const Vec3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

// 0x226980 (Ghidra).
bool BBMedicAssist::isHumanInTarget(Information* information, int side)
{
    if (!information)
        return false;
    Information* const primary = informationManager->getInformation(information->getPrimaryObject(side));
    if (!primary)
        return false;
    if (isHumanOccupied(primary, side))
        return true;
    int iterator;
    for (Information* secondary = informationManager->getInformation(primary->getFirstSecondaryObject(iterator, side));
         secondary;
         secondary = informationManager->getInformation(primary->getSecondaryObject(iterator, side)))
    {
        if (isHumanOccupied(secondary, side))
            return true;
    }
    return false;
}

// 0x227f10 (Ghidra). The secondary walk has no null guard on the information, as shipped.
bool BBMedicAssist::vehicleRequestsEngineer(Information* information, int side,
                                            CommunicationAgent* agent) const
{
    const AIRadioMessageType engineerRequest = static_cast<AIRadioMessageType>(4);
    if (information && agent->receivedMessageExists(engineerRequest, information->getPlayerId()))
        return true;
    int iterator;
    for (Information* secondary = informationManager->getInformation(information->getFirstSecondaryObject(iterator, side));
         secondary;
         secondary = informationManager->getInformation(information->getSecondaryObject(iterator, side)))
    {
        if (agent->receivedMessageExists(engineerRequest, secondary->getPlayerId()))
            return true;
    }
    return false;
}

float BBMedicAssist::calculateGiveAmmoUrgency(Bot*, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMedicAssist::calculateGiveAmmoUrgency");
}

// 0x226b40 (Ghidra). A support soldier (kit 3) hands out ammunition instead. Otherwise, among
// the friendly and neutral units within 75 m, the one best worth healing or repairing with the
// bot's tools of weapon type 1 (strength types a tool beats; not down; not itself for a medic
// firing or holding the tool; units flagged 0x80 only with a human inside; not tilted past 45
// degrees from the ground; not always-target soldiers; out of the tool's range only when the
// bot's last valid position is out of it too, or the bot cannot path there). A unit qualifies
// below 0.75 health (0.99 with a human inside), or when it asked over the radio for a medic or an
// engineer; an engineer close to a target its side does not own ignores the rest. Its score is
// its strength (times the memory's order factor for an ordered target) over the declining slope
// of the missing health times the distance; a request answered by the matching kit counts 1.5,
// 3 from a human. The best: 100 for a human within 50 m after a human's request, otherwise its
// declining slope times 10 with a human inside, else 2, times the urgency handed in; weighed by
// the squad's answer to the request (medic 1, engineer 2, who also records the repair target).
// Nothing found: 0 and the plan let go. A refresh keeps a target of the bot's side.
float BBMedicAssist::calculateUrgency(Bot* bot, bool full, float value)
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
    if (bot->getPlayer()->getSpawnKit() == kSupportKit)
        return calculateGiveAmmoUrgency(bot, value);

    const int strengths = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    int bestOccupant = -1;
    int bestTool = -1;
    Vec3 normal;
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = 0.0f;
    const int kit = bot->getPlayer()->getSpawnKit();
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const Vec3& position = equipment->getPosition();
    for (int i = 0; i < strengths; ++i)
    {
        m_toolIndex[i] = -1;
        m_toolStrength[i] = 0;
    }
    IPIArmament* const armament = equipment->getArmament();
    IPIMobile* const mobile = equipment->getMobile();
    int vehicleNumber = -2;
    if (mobile)
    {
        vehicleNumber = mobile->getVehicleNumber();
        equipment->getUnit()->getEquipmentType();   // asked and dropped, no null guard, as shipped
    }
    // no null guard, as shipped
    const std::vector<unsigned int> weapons = armament->getWeapons();
    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
        if (!weapon || weapon->getWeaponType() != kRepairToolWeaponType)
            continue;
        for (int s = 0; s < strengths; ++s)
        {
            if (weapon->getStrength(s) > m_toolStrength[s])
            {
                m_toolIndex[s] = static_cast<int>(i);
                m_toolStrength[s] = weapon->getStrength(s);
                m_toolRange[s] = weapon->getMaxRange();
            }
        }
    }

    float best = 0.0f;
    unsigned int orderedCount = 0;
    m_candidates.resize(0);
    static_cast<IAIInformationEnvironment*>(AISingletonServer::m_iaiInformationEnvironment)
        ->getFriendlyAndNeutralUnits(m_candidates, position, bot, 75.0f);
    unsigned int bestHandle = static_cast<unsigned int>(-1);
    CommunicationAgent* const agent = bot->getCommunicationAgent();
    const AIRadioMessageType medicRequestType = static_cast<AIRadioMessageType>(3);
    const AIRadioMessageType engineerRequestType = static_cast<AIRadioMessageType>(4);
    const std::list<AIRadioMessageInfo>& medicRequests = agent->getReceivedMessages(medicRequestType);
    const std::list<AIRadioMessageInfo>& engineerRequests = agent->getReceivedMessages(engineerRequestType);
    bool listenMedic = !medicRequests.empty();
    bool listenEngineer = !engineerRequests.empty();
    if (bot->isAirBorn())
    {
        listenMedic = false;
        listenEngineer = false;
    }
    const float factor = 1.0f;             // [UNRESOLVED] a constant factor on every score
    Vec3 squadPosition;
    squadPosition.x = 0.0f;
    squadPosition.y = 0.0f;
    squadPosition.z = 0.0f;
    // The squad's position, goal and leader are asked for and not used, as shipped.
    SquadCoordinator* const squad = bot->getSquadMember()->getSquad();
    squad->getSquadPosition(squadPosition);
    squad->getSquadGotoPos();
    squad->getSquadLeader();
    float bestDistance = 100.0f;
    bool bestHuman = false;
    bool humanRequest = false;

    for (std::vector<unsigned int>::iterator it = m_candidates.begin(); it != m_candidates.end(); ++it)
    {
        float threshold = 0.75f;
        bool human = false;
        Information* const candidate = informationManager->getInformation(*it);
        if (!candidate || !candidate->getPhysical())
            continue;
        if (candidate == equipment && kit == kMedicKit && bot->isFiring())
            continue;
        if ((candidate->getObjectTypeFlags() & 0x80) && !isHumanInTarget(candidate, bot->getSide()))
            continue;
        if (candidate == equipment && kit == kMedicKit && armament->getActiveWeapon() != -1)
        {
            WeaponInfo* const held =
                WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
            if (held && held->getWeaponType() == kRepairToolWeaponType)
                continue;
        }
        IPIPhysical* const physical = candidate->getPhysical();
        const int strengthType = physical->getStrengthType();
        if (physical->getManDown())
            continue;
        if (isHumanInTarget(candidate, bot->getSide()))
        {
            threshold = 0.99f;
            human = true;
        }
        if (m_toolIndex[strengthType] == -1)
            continue;
        const float health = physical->getHealth();
        int occupant = -1;
        if (candidate->getUnit())
            occupant = candidate->getUnit()->getOccupyingPlayerId(candidate->getSide());
        AIRadioMessageInfo medicRequest;
        bool medicHeard = false;
        if (listenMedic)
            medicHeard = agent->getReceivedMessage(medicRequest, medicRequestType, occupant);
        // Never filled: its sender stays -1, as shipped.
        AIRadioMessageInfo engineerRequest;
        bool engineerHeard = false;
        if (listenEngineer)
            engineerHeard = vehicleRequestsEngineer(candidate, candidate->getSide(), agent);
        if (!engineerHeard && kit == kEngineerKit && bot->getSquadMember()->isCloseToNonOwnedTarget())
            continue;
        if (!(threshold > health && health > 0.0f) && !medicHeard && !engineerHeard)
            continue;
        if (health > threshold)
            continue;

        const Vec3& at = candidate->getPosition();
        static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain)->getNormal(at.x, at.z, normal);
        const Mat4& transformation = candidate->getTransformation();
        if (0.7071f > normal.x * transformation.m[4] + normal.y * transformation.m[5]
                          + normal.z * transformation.m[6])
            continue;
        IPIUnit* const unit = candidate->getUnit();
        // no null guard, as shipped
        if (unit->getAlwaysTargetSoldier())
            continue;
        if (distanceBetween(at, position) > m_toolRange[strengthType])
        {
            if (!mobile)
                continue;
            if (!unit->useNoPathfindingToGetToObject() && candidate->getMobile())
            {
                const Vec3& valid = mobile->getLastValidPosition(vehicleNumber).position;
                const float dx = at.x - valid.x;
                const float dy = at.y - valid.y;
                const float dz = at.z - valid.z;
                const float range = m_toolRange[strengthType];
                if (range * range > dx * dx + dy * dy + dz * dz)
                    continue;
            }
        }

        const float distance = candidate == equipment ? 1.0f : distanceBetween(at, position);
        float strength = candidate->getStrength();
        if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders())
        {
            std::list<unsigned int>& orders = *bot->getOrderList(m_behaviourId);
            if (orderedCount < orders.size() && std::find(orders.begin(), orders.end(), *it) != orders.end())
            {
                ++orderedCount;
                strength = candidate->getStrength() * bot->getMemory()->m_float8;
            }
        }
        float score = strength / (DecleiningSlopeCurve::calculate(1.0f - health) * distance);
        const bool answered =
            (medicHeard && medicRequest.m_senderPlayerId == occupant && kit == kMedicKit)
            || (engineerHeard && engineerRequest.m_senderPlayerId == occupant && kit == kEngineerKit);
        if (answered)
        {
            if (IBotManager::instance->getBotFromPlayerId(occupant))
            {
                score *= 1.5f;
            }
            else
            {
                score *= 3.0f;
                humanRequest = true;
            }
        }
        score *= factor;
        if (score > best)
        {
            bestHuman = human;
            bestDistance = distance;
            best = score;
            bestTool = m_toolIndex[strengthType];
            bestHandle = *it;
            bestOccupant = occupant;
        }
    }

    const unsigned int id = bot->getBotId();
    if (bestHandle == static_cast<unsigned int>(-1))
    {
        m_urgency[id] = 0.0f;
        m_changedTarget[id] = m_targets[id] != -1;
        m_targets[id] = -1;
        m_generator->unInit(bot);
        return 0.0f;
    }
    if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders()
        && bot->getOrderList(m_behaviourId)->empty())
        best *= bot->getMemory()->m_float8;
    if (bestHuman && humanRequest && 50.0f > bestDistance)
        m_urgency[id] = 100.0f;
    else if (bestHuman)
        m_urgency[id] = DecleiningSlopeCurve::calculate(best) * 10.0f * value;
    else
        m_urgency[id] = DecleiningSlopeCurve::calculate(best) * 2.0f * value;
    m_changedTarget[id] = static_cast<unsigned int>(m_targets[id]) != bestHandle;
    m_targets[id] = static_cast<int>(bestHandle);
    static_cast<BBPMedicAssist*>(m_generator)->init(static_cast<unsigned int>(m_targets[id]), bestTool,
                                                    bestOccupant, bot);
    if (kit == kMedicKit)
    {
        const int side = bot->getSide();
        const float response =
            SquadManager::m_instance->requestResponseModifier(bot->getSquadMember(), bestHandle, 1, side);
        m_urgency[id] = response * m_urgency[id];
    }
    else if (kit == kEngineerKit)
    {
        const int side = bot->getSide();
        const float response =
            SquadManager::m_instance->requestResponseModifier(bot->getSquadMember(), bestHandle, 2, side);
        m_urgency[id] = response * m_urgency[id];
        bot->setWantedRepairTarget(m_urgency[id] == 0.0f ? static_cast<unsigned int>(-1) : bestHandle);
    }
    return m_urgency[bot->getBotId()];
}

bool BBMedicAssist::isAplicable(Information* information, Bot* bot)
{
    IPIArmament* const armament = information->getArmament();
    if (bot->getManDown() || !armament)
        return false;
    const std::vector<unsigned int>& weapons = armament->getWeapons();
    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
        if (weapon && static_cast<unsigned int>(weapon->getWeaponType() - 1) < 2)
            return true;
    }
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
