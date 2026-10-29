#include "BBAvoid.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/BotMemory.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBot/CommunicationAgent.h"
#include "../AIBotBehaviourPattern/BBPAvoidCollision.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IBotManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../../Game/Objects/IPlayer.h"
#include "../../../../Math/Mat4.h"
#include <cmath>
#include <vector>

namespace dice {
namespace hfe {
namespace ai {

BBAvoid::BBAvoid(unsigned char behaviourId, unsigned char mode, BBPAvoidCollision* generator, UrgeCurve* urgeCurve, float* weights)
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
        m_targets[i] = -1;
    }
}

BBAvoid::~BBAvoid()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBAvoid::~BBAvoid");
}

namespace {

const int kC4WeaponType = 4;

// The centre of the information's collision sphere: its local offset turned by the
// transformation, plus the position.
Vec3 sphereCentre(Information* information, IPIPhysical* physical)
{
    const Mat4& m = information->getTransformation();
    const Vec3 offset = physical->getSphereLocalOffset();
    Vec3 turned;
    turned.x = m.m[8] * offset.z + m.m[0] * offset.x + m.m[4] * offset.y;
    turned.y = m.m[1] * offset.x + m.m[5] * offset.y + m.m[9] * offset.z;
    turned.z = offset.x * m.m[2] + offset.y * m.m[6] + offset.z * m.m[10];
    const Vec3& position = information->getPosition();
    Vec3 centre;
    centre.x = turned.x + position.x;
    centre.y = turned.y + position.y;
    centre.z = turned.z + position.z;
    return centre;
}

float lengthSquared(const Vec3& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

} // namespace

// 0x20bc40 (Ghidra). Soldiers only; anything else scores 0 (the binary still carries a speed and
// a heading gate for other kinds after that return, never reached). Over the navigation's
// obstacles that are not soldiers, the bot's own object, its change or repair targets: a wreck
// about to blow (health under the bot manager's bail value, or flag 0x2000000) while the bot
// holds no C4 scores 100 within its reach (its smallest radius plus 4, at most 10, at least
// the radius); anything else moving at 2 m/s or more scores its mass times the relative speed
// over the distance when the spheres (radii summed, times 0.6) meet within the look-ahead
// time (doubled in pose 3, times 1.5 in pose 2) and close at 3 m/s or more. The
// highest score takes the plan. Nothing to avoid: a grenade warning or tear gas scores 1000
// with an empty plan, else 0 and the plan let go. Otherwise the sum, times the memory's order
// factor when ordered, doubled within 5 s of being honked at, times the urgency handed in.
float BBAvoid::calculateUrgency(Bot* bot, bool full, float value)
{
    if (!full)
    {
        m_changedTarget[bot->getBotId()] = false;
        if (m_generator->isInitiated(bot))
            return m_urgency[bot->getBotId()];
        m_generator->unInit(bot);
        m_urgency[bot->getBotId()] = 0.0f;
        return 0.0f;
    }

    static float radiusTweak = 0.6f;
    BBPAvoidCollision* const generator = static_cast<BBPAvoidCollision*>(m_generator);
    Vec3 contact;
    contact.x = 0.0f;
    contact.y = 0.0f;
    contact.z = 0.0f;
    const int oldTarget = m_targets[bot->getBotId()];
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const unsigned int ownPrimary = equipment->isPrimary() ? bot->getCurrentEquipment()
                                                           : equipment->getPrimaryObject(bot->getSide());
    equipment->getName();                  // the shipped body asks for the name and drops it
    float best = 0.0f;
    float total = 0.0f;
    IPIMobile* const mobile = equipment->getMobile();
    const bool soldier = (equipment->getObjectTypeFlags() & 0x200000) != 0;
    if (!mobile || !soldier)
    {
        m_urgency[bot->getBotId()] = total;
        return total;
    }

    // no null guard, as shipped
    IPIPhysical* const physical = equipment->getPhysical();
    const Vec3 centre = sphereCentre(equipment, physical);
    const float ownRadius = physical->getSmallestRadius();
    const Vec3 velocity = mobile->getSpeed();
    float lookAhead = mobile->getAvoidCollisionLookAhead();
    if (equipment->getAIObject())
    {
        if (equipment->getSoldierPose() == 3)
            lookAhead = lookAhead + lookAhead;
        else if (equipment->getSoldierPose() == 2)
            lookAhead *= 1.5f;
    }
    const int kind = (equipment->getObjectTypeFlags() & 0x10) ? 3 : 2;
    unsigned int changeTarget = bot->getChangeTarget();
    Information* const change = informationManager->getInformation(changeTarget);
    if (change && !change->isPrimary())
        changeTarget = change->getPrimaryObject(bot->getSide());
    unsigned int repairTarget = bot->getRepairTarget();
    Information* const repair = informationManager->getInformation(repairTarget);
    if (repair && !repair->isPrimary())
        repairTarget = repair->getPrimaryObject(bot->getSide());

    const std::vector<unsigned int>& obstacles = bot->getBotNavigation()->getObstacles();
    const unsigned int count = obstacles.size();
    for (unsigned int i = 0; i < count; ++i)
    {
        const unsigned int handle = obstacles[i];
        Information* const obstacle = informationManager->getInformation(handle);
        if (!obstacle)
            continue;
        IPIMobile* const obstacleMobile = obstacle->getMobile();
        if (obstacle->getObjectSpawnerHolding()
            && obstacle->getObjectSpawner(bot->getSide()) == bot->getCurrentEquipment())
            continue;
        const unsigned int primary = obstacle->isPrimary() ? handle : obstacle->getPrimaryObject(bot->getSide());
        IPIPhysical* const obstaclePhysical = obstacle->getPhysical();
        if (!obstaclePhysical || ownPrimary == primary || handle == repairTarget || handle == changeTarget
            || handle == bot->getWantedRepairTarget() || (obstacle->getObjectTypeFlags() & 0x200000))
            continue;

        const Vec3 obstacleCentre = sphereCentre(obstacle, obstaclePhysical);
        Vec3 relativePosition;
        relativePosition.x = obstacleCentre.x - centre.x;
        relativePosition.y = obstacleCentre.y - centre.y;
        relativePosition.z = obstacleCentre.z - centre.z;
        Vec3 obstacleVelocity;
        obstacleVelocity.x = 0.0f;
        obstacleVelocity.y = 0.0f;
        obstacleVelocity.z = 0.0f;
        if (obstacleMobile)
            obstacleVelocity = obstacleMobile->getSpeed();
        Vec3 relativeVelocity;
        relativeVelocity.x = obstacleVelocity.x - velocity.x;
        relativeVelocity.y = obstacleVelocity.y - velocity.y;
        relativeVelocity.z = obstacleVelocity.z - velocity.z;
        const float relativeSpeedSquared = lengthSquared(relativeVelocity);
        const float radius = (obstaclePhysical->getSmallestRadius() + ownRadius) * radiusTweak;
        const float health = obstaclePhysical->getHealth();
        const float bailValue = IBotManager::instance->getVehicleBailValue();
        WeaponInfo* held = 0;
        IPIArmament* const armament = equipment->getArmament();
        if (armament && armament->getActiveWeapon() != -1)
            held = WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
        bool exploding = false;
        if ((bailValue > health || (obstacle->getObjectTypeFlags() & 0x2000000))
            && !(held && held->getWeaponType() == kC4WeaponType))
            exploding = true;
        if (!exploding && 4.0f > lengthSquared(obstacleVelocity))
            continue;

        const float distance = std::sqrt(lengthSquared(relativePosition));
        const float smallest = obstaclePhysical->getSmallestRadius();
        float reach = obstaclePhysical->getSmallestRadius() + 4.0f;
        if (reach > 10.0f)
            reach = 10.0f;
        if (reach <= smallest)
            reach = smallest;
        if (exploding)
        {
            if (reach > distance)
            {
                float scale = (reach - distance) / 5.0f + 0.5f;
                best = 100.0f;
                total += 100.0f;
                if (scale <= 1.0f)
                    scale = 1.0f;
                m_targets[bot->getBotId()] = static_cast<int>(handle);
                Vec3 noContact;
                noContact.x = 0.0f;
                noContact.y = 0.0f;
                noContact.z = 0.0f;
                Vec3 noVelocity = noContact;
                generator->init(bot, noContact, handle, noVelocity, scale);
            }
            continue;
        }

        float time = 0.0f;                 // written by any prediction that hits
        const float predicted = collisionPredicted(relativePosition, relativeVelocity, kind, lookAhead,
                                                   radius, contact, time);
        if (!(predicted > 0.0f) || !(relativeSpeedSquared >= 9.0f))
            continue;
        const float spread = 1.19209290e-07f > time ? 1.0f : std::sqrt(lengthSquared(relativePosition));
        const float mass = obstaclePhysical->getMass();
        const float urgency = mass * std::sqrt(relativeSpeedSquared) / spread * predicted;
        total += urgency;
        if (urgency > best)
        {
            best = urgency;
            m_targets[bot->getBotId()] = static_cast<int>(handle);
            generator->init(bot, contact, handle, relativeVelocity, time);
        }
    }

    Vec3 grenadePosition;
    grenadePosition.x = 0.0f;
    grenadePosition.y = 0.0f;
    grenadePosition.z = 0.0f;
    double grenadeTime = 0.0;
    bool flee = soldier && total == 0.0f
                && bot->getBotNavigation()->getGrenadeWarning(grenadePosition, grenadeTime);
    if (!flee)
    {
        world::IPlayer* const player = bot->getPlayer()->getPlayer();
        flee = player->getBlurValue() > 0.0f && player->getIsGassed();
    }
    if (flee)
    {
        Vec3 none;
        none.x = 0.0f;
        none.y = 0.0f;
        none.z = 0.0f;
        generator->init(bot, none, static_cast<unsigned int>(-1), none, 0.0f);
        m_urgency[bot->getBotId()] = 1000.0f;
        return 1000.0f;
    }

    if (total == 0.0f)
    {
        m_changedTarget[bot->getBotId()] = m_generator->isInitiated(bot);
        m_generator->unInit(bot);
        m_urgency[bot->getBotId()] = 0.0f;
        return 0.0f;
    }
    if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders())
        total *= bot->getMemory()->m_float8;
    if (soldier)
    {
        const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
        if (5.0 > now - bot->getCommunicationAgent()->getHonkedAtTime())
            total = total + total;
    }
    m_urgency[bot->getBotId()] = value * total;
    m_changedTarget[bot->getBotId()] = oldTarget != m_targets[bot->getBotId()];
    // no null guard, as shipped
    Information* const target = informationManager->getInformation(m_targets[bot->getBotId()]);
    bot->insertBehaviourInformation(0.5f, target->getPosition(), m_behaviourId);
    return m_urgency[bot->getBotId()];
}

// 0x20cdf0 (Ghidra).
float BBAvoid::collisionPredicted(Vec3& relativePosition, Vec3& relativeVelocity, int, float lookAhead,
                                  float radius, Vec3& contact, float& time)
{
    const float reachSquared = radius * radius * 1.5f;
    const float speedSquared = lengthSquared(relativeVelocity);
    if (0.001 * 0.001 > speedSquared)
        return 0.0f;
    const float speed = std::sqrt(speedSquared);
    const float inverse = 1.0f / speed;
    // the time of the closest approach
    const float closest = (-relativePosition.x * relativeVelocity.x * inverse
                           + relativeVelocity.y * inverse * -relativePosition.y
                           + inverse * relativeVelocity.z * -relativePosition.z) / speed;
    if (0.0f > closest)
        return 0.0f;
    Vec3 nearest;
    nearest.x = relativePosition.x + relativeVelocity.x * closest;
    nearest.y = relativePosition.y + relativeVelocity.y * closest;
    nearest.z = relativePosition.z + relativeVelocity.z * closest;
    const float missSquared = lengthSquared(nearest);
    if (missSquared >= reachSquared)
        return 0.0f;
    const float meet = closest - std::sqrt(std::fabs(missSquared - reachSquared)) / speed;
    time = meet;
    if (meet > lookAhead || 0.0f > meet)
        return 0.0f;
    contact.x = relativeVelocity.x * meet + relativePosition.x;
    contact.y = relativeVelocity.y * meet + relativePosition.y;
    contact.z = relativeVelocity.z * meet + relativePosition.z;
    return 1.0f;
}

} // namespace ai
} // namespace hfe
} // namespace dice
