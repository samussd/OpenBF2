#include "BAPAMoveToObjectFinding.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBot/SensedObject.h"
#include "../AIBot/SquadManager.h"
#include "../AIBot/SquadMember.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IAIPathfinding.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../../Math/Mat4.h"

#include <cfloat>
#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

namespace {

// 0x1eb310 (Ghidra). A quarter turn about the vertical.
void getRight(const Vec3& in, Vec3& out)
{
    out.y = in.y;
    out.x = in.z;
    out.z = -in.x;
}

bool sameXYZ(const Vec3& a, const Vec3& b)
{
    return std::fabs(a.x - b.x) <= 1.1920929e-07f && std::fabs(a.y - b.y) <= 1.1920929e-07f
        && std::fabs(a.z - b.z) <= 1.1920929e-07f;
}

} // namespace

// 0x1eb030 (Ghidra). A move-away radius over the radius takes the radius.
BAPAMoveToObjectFinding::BAPAMoveToObjectFinding(Bot* bot, unsigned int object, float radius, float maxSpeed,
                                                 float maxArriveSpeed, BAPCondition* finishCondition,
                                                 unsigned long long controls, int planId,
                                                 BAPCondition* moveCondition, float moveAwayRadius,
                                                 int intParameter)
    : BAPAMoveToObject(bot, object, radius, maxSpeed, maxArriveSpeed, finishCondition, controls, planId)
    , m_searching(true)
    , m_boolE9(false)
    , m_intEC(1)
    , m_bot(bot)
    , m_moveCondition(moveCondition)
    , m_moveConditionHeld(true)
    , m_moveAwayRadius(moveAwayRadius)
    , m_moveAwayRadiusSqr(moveAwayRadius * moveAwayRadius)
    , m_arrived(false)
    , m_movingAway(false)
    , m_stepped(false)
    , m_followedSighting(false)
    , m_intParameter(intParameter)
    , m_searchTime(-1.0f)
{
    for (int i = 0; i < 8; ++i)
        m_floatC8[i] = 0.0f;
    if (m_radius < moveAwayRadius)
    {
        m_moveAwayRadius = m_radius;
        m_moveAwayRadiusSqr = m_radiusSqr;
    }
    if (moveCondition)
        moveCondition->m_owner = this;
}

// 0x1ec040 (Ghidra).
BAPAMoveToObjectFinding::~BAPAMoveToObjectFinding()
{
    delete m_moveCondition;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPAMoveToObjectFinding::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAMoveToObjectFinding::streamSelf");
}

// 0x1eb360 (Ghidra). From `distance` at or over the move-away radius (or asked for directly) the
// search goes to the object. Closer, it goes to the end of the longest of up to three traces from
// the object, 0.98 of the way: along the bot's side of the object and to its sides, or for a soldier
// on the move across its way, all to the reach (the radii's gap * 0.2, at most 10, past the
// move-away radius). A trace that gets through ends the choice. No null guard on the object's mobile
// there, as shipped; the direction is not used.
void BAPAMoveToObjectFinding::initObjectFinding(float distance, const Vec3&, const Vec3& from,
                                                unsigned int quadTree, unsigned int polygon, const Vec3& target,
                                                bool direct)
{
    m_movingAway = false;
    m_arrived = false;
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(m_bot->getCurrentEquipment());
    if (!equipment)
    {
        m_searching = false;
        return;
    }
    Information* const object = informationManager->getInformation(m_object);
    if (!object)
    {
        m_searching = false;
        return;
    }
    IPIMobile* mobile = object->getMobile();
    if (!mobile && !object->isPrimary())
    {
        // [PLATFORM] ELF captures the manager before the side callback.
        InformationManager* const managerNow = informationManager;
        Information* const primary = managerNow->getInformation(object->getPrimaryObject(m_bot->getSide()));
        if (primary)
            mobile = primary->getMobile();
    }
    float objectReach = FLT_MAX;
    IPIPhysical* const physical = object->getPhysical();
    if (physical)
        objectReach = physical->getRadius() * 1.1f;
    const int type = equipment->getUnit()->getEquipmentType();
    if (m_moveAwayRadius <= distance || direct)
    {
        Vec3 zero;
        zero.x = 0.0f;
        zero.y = 0.0f;
        zero.z = 0.0f;
        m_movingAway = false;
        setIsSprinting(m_bot, false, zero, 0xFFFFFFFFu);
        m_coordinate = target;
    }
    else
    {
        Vec3 ways[3];
        for (int i = 0; i < 3; ++i)
        {
            ways[i].x = 0.0f;
            ways[i].y = 0.0f;
            ways[i].z = 0.0f;
        }
        const Vec3& speed = mobile->getSpeed();
        Vec3 flat;
        flat.x = speed.x;
        flat.y = 0.0f;
        flat.z = speed.z;
        const float flatSpeed = std::sqrt(flat.x * flat.x + 0.0f * 0.0f + flat.z * flat.z);
        const bool soldier = (equipment->getObjectTypeFlags() & 0x200000u) != 0;
        float reach = (m_radius - m_moveAwayRadius) * 0.2f;
        if (10.0f < reach)
            reach = 10.0f;
        reach = reach + m_moveAwayRadius;
        if (!soldier || flatSpeed <= 1.5f)
        {
            if (!soldier)
            {
                const Mat4& transformation = equipment->getTransformation();
                ways[0].x = transformation.m[8] * reach;
                ways[0].y = transformation.m[9] * reach;
                ways[0].z = reach * transformation.m[10];
            }
            else
            {
                Vec3 away;
                away.x = from.x - target.x;
                away.y = from.y - target.y;
                away.z = from.z - target.z;
                const float length2 = away.x * away.x + away.y * away.y + away.z * away.z;
                if (!(std::fabs(length2 - 1.0f) < 1.1920929e-07f))
                {
                    if (1.1920929e-07f <= std::fabs(length2))
                    {
                        const float inverse = 1.0f / std::sqrt(length2);
                        away.x = away.x * inverse;
                        away.y = away.y * inverse;
                        away.z = inverse * away.z;
                    }
                    else
                    {
                        away.x = 0.0f;
                        away.y = 0.0f;
                        away.z = 0.0f;
                    }
                }
                ways[0].x = away.x * reach;
                ways[0].y = away.y * reach;
                ways[0].z = reach * away.z;
            }
            getRight(ways[0], ways[1]);
            ways[2].x = -ways[1].x;
            ways[2].y = -ways[1].y;
            ways[2].z = -ways[1].z;
        }
        else
        {
            const float inverse = 1.0f / flatSpeed;
            flat.x = flat.x * inverse;
            flat.y = flat.y * inverse;
            flat.z = inverse * flat.z;
            Vec3 across;
            across.x = 0.0f;
            across.y = 0.0f;
            across.z = 0.0f;
            getRight(flat, across);
            const bool behind = (from.x - target.x) * across.x + (from.y - target.y) * across.y
                + (from.z - target.z) * across.z < 0.0f;
            if (behind)
            {
                across.x = -across.x;
                across.y = -across.y;
                across.z = -across.z;
            }
            ways[0].x = across.x * reach;
            ways[0].y = across.y * reach;
            ways[0].z = reach * across.z;
            ways[1].x = -ways[0].x;
            ways[1].y = -ways[0].y;
            ways[1].z = -ways[0].z;
            const Vec3 side = behind ? ways[1] : ways[0];
            getRight(side, ways[2]);
        }
        m_movingAway = true;
        setIsSprinting(m_bot, true, equipment->getPosition(), 0xFFFFFFFFu);
        BotNavigation* const navigation = static_cast<BotMain*>(m_bot)->getBotNavigation();
        float longest = 0.0f;
        unsigned int longestPolygon = 0xFFFFFFFFu;
        unsigned int last = 0;
        Vec3 hit;
        hit.x = 0.0f;
        hit.y = 0.0f;
        hit.z = 0.0f;
        Vec3 chosen = hit;
        bool through = false;
        for (int i = 0; i < 3 && !through; ++i)
        {
            Vec3 to;
            to.x = ways[i].x + target.x;
            to.y = ways[i].y + target.y;
            to.z = ways[i].z + target.z;
            through = IAIPathfinding::instance->trace(navigation, false, to, hit, last, type, -1, 0.0f, 0, 0);
            if (!through)
            {
                const float x = target.x - hit.x;
                const float y = target.y - hit.y;
                const float z = target.z - hit.z;
                const float length2 = x * x + y * y + z * z;
                if (longest < length2)
                {
                    chosen.x = target.x + (hit.x - target.x) * 0.98f;
                    chosen.y = (hit.y - target.y) * 0.98f + target.y;
                    chosen.z = (hit.z - target.z) * 0.98f + target.z;
                    longestPolygon = last;
                    longest = length2;
                }
            }
        }
        quadTree = navigation->getTraceIsland();
        if (!through)
        {
            m_coordinate = chosen;
            polygon = longestPolygon;
        }
        else
        {
            m_coordinate.x = target.x + (hit.x - target.x) * 0.98f;
            m_coordinate.y = target.y + (hit.y - target.y) * 0.98f;
            m_coordinate.z = target.z + (hit.z - target.z) * 0.98f;
            polygon = last;
        }
    }
    m_float60 = 0.0f;
    m_boolEA = false;
    if (m_movingAway)
        m_coordinate.y = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain)
                             ->getTerrainAltitude(m_coordinate.x, m_coordinate.z);
    BotNavigation* const navigation = static_cast<BotMain*>(m_bot)->getBotNavigation();
    m_searching = navigation->initPathfinding(quadTree, polygon, m_coordinate, objectReach, m_radius);
    m_searchTime = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
}

// 0x1ec1c0 (Ghidra), read in the asm. Keeps the path on the object's last valid place: a new search
// when the object's sighting moved it, when the navigation lost the target, or each second as the
// conditions ask; then one step along the path, and for a soldier on the way the same artillery
// checks and sprinting as a move to a position. No equipment or object, or a place off the mesh,
// ends the search; no bot mesh place also marks the bot stuck.
bool BAPAMoveToObjectFinding::update(Vec2& point, bool& a, bool& b, float c)
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(m_bot->getCurrentEquipment());
    if (!equipment || !m_searching)
    {
        m_searching = false;
        return false;
    }
    BotMain* const bot = static_cast<BotMain*>(m_bot);
    if (bot->getShouldWaitForRepairs()
        || (equipment->getPhysical()->isInAir() && !equipment->getPhysical()->isInWater()))
    {
        a = false;
        b = true;
        return true;
    }
    Information* const object = informationManager->getInformation(m_object);
    if (!object)
    {
        m_searching = false;
        return false;
    }
    IPIMobile* const mobile = equipment->getMobile();
    const IPIMobile::ValidPos& place = mobile->getLastValidPosition(mobile->getVehicleNumber());
    // [PLATFORM] ELF captures the manager before the side callback.
    InformationManager* const managerNow = informationManager;
    Information* const primary = managerNow->getInformation(object->getPrimaryObject(equipment->getSide()));
    if (!primary || !primary->getMobile())
    {
        m_searching = false;
        return false;
    }
    const IPIMobile::ValidPos& objectPlace =
        primary->getMobile()->getLastValidPosition(mobile->getVehicleNumber());
    BotNavigation* const navigation = bot->getBotNavigation();
    if (place.polygon == 0xFFFFFFFFu)
    {
        navigation->setIsStuck(true);
        m_searching = false;
        return false;
    }
    if (objectPlace.polygon == 0xFFFFFFFFu)
    {
        m_searching = false;
        return false;
    }
    Vec3 direction;
    direction.x = objectPlace.position.x - place.position.x;
    direction.y = objectPlace.position.y - place.position.y;
    direction.z = objectPlace.position.z - place.position.z;
    const float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y
                                     + direction.z * direction.z);
    if (9.9999997e-05f < distance)
    {
        const float inverse = 1.0f / distance;
        direction.x = direction.x * inverse;
        direction.y = direction.y * inverse;
        direction.z = inverse * direction.z;
    }
    SensedObject* const sensed = m_bot->getSensedObjectFromHandle(m_object);
    const bool sighted = !sensed || (sensed->m_flags & SensedObject::kFlag8) != 0;
    if (!sighted)
        m_followedSighting = false;
    const bool within = withinDistance();
    const bool arrived = m_arrived;
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    const bool late = 1.0f < now - m_searchTime;

    // Which search to start, if any.
    enum { None, ToObject, Reset } restart = None;
    if (sighted && !sameXYZ(m_coordinate, objectPlace.position) && !m_followedSighting
        && (!m_movingAway || arrived))
    {
        m_followedSighting = true;
        restart = ToObject;
    }
    else if (!sameXYZ(m_coordinate, navigation->getSearchTarget()))
        restart = ToObject;
    else
    {
        bool settled = !late;
        if (late)
        {
            if (!m_movingAway && distance < m_moveAwayRadius)
                restart = ToObject;
            else
            {
                if (m_movingAway)
                {
                    const float x = objectPlace.position.x - m_coordinate.x;
                    const float y = objectPlace.position.y - m_coordinate.y;
                    const float z = objectPlace.position.z - m_coordinate.z;
                    if (x * x + y * y + z * z < m_moveAwayRadius * m_moveAwayRadius && arrived)
                        restart = ToObject;
                }
                if (restart == None && arrived && !within && !sameXYZ(m_coordinate, objectPlace.position))
                    restart = ToObject;
                settled = restart == None;
            }
        }
        if (settled)
        {
            if (!m_stepped || !navigation->isSearchActive())
                restart = Reset;
            else if (late && m_moveCondition && m_moveCondition->evaluate(m_bot))
            {
                if (!m_moveConditionHeld || arrived)
                {
                    m_moveConditionHeld = true;
                    restart = ToObject;
                }
            }
            else
                m_moveConditionHeld = false;
        }
    }
    if (restart == ToObject)
        initObjectFinding(distance, direction, place.position, objectPlace.quadTree, objectPlace.polygon,
                          objectPlace.position, false);
    else if (restart == Reset)
    {
        Vec3 zero;
        zero.x = 0.0f;
        zero.y = 0.0f;
        zero.z = 0.0f;
        initObjectFinding(0.0f, zero, zero, objectPlace.quadTree, objectPlace.polygon, objectPlace.position, true);
    }

    m_stepped = true;
    if (!m_searching)
        return false;
    if (!m_movingAway)
        navigation->updateLastPoint(primary);
    const bool stepped = navigation->updatePath(m_currentPoint, m_arrived, b, c);
    Vec3 ahead;
    ahead.x = m_currentPoint.x;
    ahead.y = equipment->getPosition().y;
    ahead.z = m_currentPoint.y;
    double since = 0.0;
    int bySide = 0;
    if (stepped)
    {
        point = m_currentPoint;
        a = m_arrived;
        if ((equipment->getObjectTypeFlags() & 0x10) == 0)
        {
            const int side = m_bot->getSide();
            if (SquadManager::m_instance->isInArtilleryDangerZone(ahead, side, since, bySide)
                && !SquadManager::m_instance->isInArtilleryDangerZone(equipment->getPosition(), m_bot->getSide(),
                                                                      since, bySide))
                b = true;
        }
    }
    if (!b)
    {
        const bool soldier = (equipment->getObjectTypeFlags() & 0x200000) != 0;
        const Vec3 zero = { 0.0f, 0.0f, 0.0f };
        if (!m_movingAway && soldier
            && std::fabs(object->getPosition().y - equipment->getPosition().y) < 2.0f)
        {
            // Close on a soldier target the bot looks at it and sprints, unless it is firing a long
            // weapon that is not of type 4.
            const float closest = navigation->getClosestDistance();
            IPIPhysical* const physical = object->getPhysical();
            IPIArmament* const armament = equipment->getArmament();
            if (!armament || armament->getActiveWeapon() == -1)
            {
                m_searching = false;
                return false;
            }
            WeaponInfo* const weapon =
                WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
            if (!weapon)
            {
                m_searching = false;
                return false;
            }
            if ((!m_bot->isFiring() || weapon->getMaxRange() <= 5.0f || weapon->getWeaponType() == 4)
                && closest < physical->getRadius() + 25.0f)
                setIsSprinting(m_bot, true, zero, m_object);
            else
                setIsSprinting(m_bot, false, zero, 0xFFFFFFFFu);
        }
        else if (soldier)
        {
            const int side = m_bot->getSide();
            if (SquadManager::m_instance->isInArtilleryDangerZone(equipment->getPosition(), side, since, bySide))
            {
                SquadMember* const member = m_bot->getSquadMember();
                if (member && member->isAvoidingArtillery())
                    setIsSprinting(m_bot, true, ahead, 0xFFFFFFFFu);
            }
        }
        if (isFinished(m_bot))
        {
            m_searching = true;
            a = true;
            m_arrived = true;
            point = m_currentPoint;
            return true;
        }
    }
    if (!stepped)
        m_stepped = false;
    return stepped;
}

// 0x1ed340 (Ghidra). Moving away, within the distance counts only once at the end of the path.
// Finishing clears the navigation's progress.
bool BAPAMoveToObjectFinding::isFinished(Bot* bot)
{
    const bool condition = m_finishCondition && m_finishCondition->evaluate(bot);
    const bool finished = m_movingAway ? condition || (m_arrived && withinDistance())
                                       : condition || withinDistance();
    if (finished)
        static_cast<BotMain*>(bot)->getBotNavigation()->clearProgress();
    return finished;
}

bool BAPAMoveToObjectFinding::getIsMovingAway()
{
    return m_movingAway;
}

bool BAPAMoveToObjectFinding::getLargestAngleDiff(float& angle)
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getLargestAngleDiff(angle);
}

float BAPAMoveToObjectFinding::getTargetDistance()
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getClosestDistance();
}

bool BAPAMoveToObjectFinding::usesPathfinding()
{
    return true;
}

// 0x1ed250 (Ghidra). Between the two radii, both included.
bool BAPAMoveToObjectFinding::withinDistance()
{
    const float distanceSqr = getSimpleTargetDistanceSqr(m_bot);
    return distanceSqr <= m_radiusSqr && m_moveAwayRadiusSqr <= distanceSqr;
}

} // namespace ai
} // namespace hfe
} // namespace dice
