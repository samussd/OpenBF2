#include "BAPAMoveToFinding.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBot/SquadManager.h"
#include "../AIBot/SquadMember.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/IAIPathfinding.h"

#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

// 0x1e96e0 (Ghidra). The bot's equipment is asked for and dropped, as shipped.
BAPAMoveToFinding::BAPAMoveToFinding(Bot* bot, const Vec3& position, float maxSpeed, float maxArriveSpeed,
                                     BAPCondition* finishCondition, unsigned long long controls, int planId,
                                     float radius, float minRadius)
    : BAPAMoveTo(bot)
    , m_boolD4(false)
    , m_boolD5(false)
    , m_intD8(1)
    , m_bot(bot)
    , m_radius(radius)
    , m_minRadius(minRadius)
    , m_radiusSqr(radius * radius)
    , m_minRadiusSqr(minRadius * minRadius)
    , m_boolF8(false)
{
    for (int i = 0; i < 8; ++i)
        m_floatB4[i] = 0.0f;
    bot->getCurrentEquipment();
    m_planId = planId;
    m_coordinate = position;
    m_maxSpeed = maxSpeed;
    m_maxArriveSpeed = maxArriveSpeed;
    m_finishCondition = finishCondition;
    if (finishCondition)
        finishCondition->m_owner = this;
    m_float60 = 0.0f;
    m_hasPath = true;
    m_pathStarted = false;
    m_controls = controls;
    m_validPoint.x = 0.0f;
    m_validPoint.y = 0.0f;
}

BAPAMoveToFinding::~BAPAMoveToFinding()
{
}

// 0x1e9af0 (Ghidra). Runs only through an interpreter entry.
bool BAPAMoveToFinding::execute(Bot*, bool& ok, bool& done)
{
    ok = false;
    done = true;
    return false;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPAMoveToFinding::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAMoveToFinding::streamSelf");
}

// 0x1e9b10 (Ghidra), read in the asm. One step. The search starts again when it was not started,
// the target is not the navigation's, or the navigation's search is over; the found path is then
// followed. Within 1 m beyond an inner radius over 1 m the step heads for a valid point on the ring
// around the target instead; as shipped the ring is searched only once m_validPoint holds a point,
// and before that the step heads for (0, 0). b: no more steps (an airborne bot, one waiting for
// repairs, or a target in an artillery danger zone the bot is not in); a soldier in such a zone
// whose squad member avoids artillery sprints. The float is not used. True: point is this step's.
bool BAPAMoveToFinding::update(Vec2& point, bool& a, bool& b, float)
{
    // no null guard on the equipment, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (static_cast<BotMain*>(m_bot)->getShouldWaitForRepairs()
        || (equipment->getPhysical()->isInAir() && !equipment->getPhysical()->isInWater()))
    {
        a = false;
        b = true;
        return true;
    }
    bool restart = !m_pathStarted;
    if (!restart)
    {
        const Vec3& searched = m_bot->getBotNavigation()->getSearchTarget();
        const bool same = std::fabs(m_coordinate.x - searched.x) <= 1.1920929e-07f
            && std::fabs(m_coordinate.y - searched.y) <= 1.1920929e-07f
            && std::fabs(m_coordinate.z - searched.z) <= 1.1920929e-07f;
        restart = !same || !m_bot->getBotNavigation()->isSearchActive();
    }
    if (restart)
    {
        m_pathStarted = true;
        m_hasPath = m_bot->getBotNavigation()->initPathfinding(m_coordinate, 0.0f, m_radius);
    }
    bool stepped = false;
    if (m_hasPath && m_bot->getBotNavigation()->updatePath(m_currentPoint, m_boolF8, b, 0.0f))
    {
        stepped = true;
        point = m_currentPoint;
        a = m_boolF8;
    }
    const float distance = getSimpleTargetDistance(m_bot);
    if (m_minRadius + 1.0f > distance && m_minRadius > 1.0f)
    {
        if (1.1920929e-07f < std::fabs(m_validPoint.x - 0.0f) + std::fabs(m_validPoint.y - 0.0f))
        {
            IAIPathfinding* const pathfinding = IAIPathfinding::instance;
            IPIMobile* const mobile = equipment->getMobile();
            const int type = equipment->getUnit()->getEquipmentType();
            float angle = 0.0f;
            if (!(6.2831855f > angle))
                return false;
            bool valid;
            do
            {
                const float s = std::sin(angle);
                const float c = std::cos(angle);
                const float r = m_minRadius + 1.0f;
                Vec3 at;
                at.x = s * r + m_coordinate.x;
                at.y = 0.0f * r + m_coordinate.y;
                at.z = c * r + m_coordinate.z;
                valid = pathfinding->isValidPosition(mobile->getVehicleNumber(), type, at, true);
                if (valid)
                    vec3ToVec2XZ(at, m_validPoint);
                angle += 0.5f;
                if (!(6.2831855f > angle))
                {
                    if (!valid)
                        return false;
                    break;
                }
            } while (!valid);
        }
        point = m_validPoint;
    }
    else if (!b && isFinished(m_bot))
    {
        m_hasPath = true;
        a = true;
        m_boolF8 = true;
        point = m_currentPoint;
        stepped = true;
    }
    if (!b)
    {
        Vec3 target;
        target.x = point.x;
        target.y = equipment->getPosition().y;
        target.z = point.y;
        double since = 0.0;
        int bySide = 0;
        if ((equipment->getObjectTypeFlags() & 0x10) == 0
            && SquadManager::m_instance->isInArtilleryDangerZone(target, m_bot->getSide(), since, bySide)
            && !SquadManager::m_instance->isInArtilleryDangerZone(equipment->getPosition(), m_bot->getSide(),
                                                                  since, bySide))
            b = true;
        else if (equipment->getObjectTypeFlags() & 0x200000)
        {
            const int side = m_bot->getSide();
            if (SquadManager::m_instance->isInArtilleryDangerZone(equipment->getPosition(), side, since, bySide))
            {
                SquadMember* const member = m_bot->getSquadMember();
                if (member && member->isAvoidingArtillery())
                    setIsSprinting(m_bot, true, target, 0xFFFFFFFFu);
            }
        }
    }
    if (!stepped)
        m_pathStarted = false;
    return stepped;
}

// 0x1ea3b0 (Ghidra). Finishing clears the navigation's progress.
bool BAPAMoveToFinding::isFinished(Bot* bot)
{
    if ((!m_finishCondition || !m_finishCondition->evaluate(bot)) && !withinDistance())
        return false;
    static_cast<BotMain*>(bot)->getBotNavigation()->clearProgress();
    return true;
}

bool BAPAMoveToFinding::getSecondPoint(Vec2& point)
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getSecondPoint(point);
}

bool BAPAMoveToFinding::getLargestAngleDiff(float& angle)
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getLargestAngleDiff(angle);
}

float BAPAMoveToFinding::getTargetDistance()
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getClosestDistance();
}

bool BAPAMoveToFinding::usesPathfinding()
{
    return true;
}

// 0x1ea360 (Ghidra). Between the two radii, both included.
bool BAPAMoveToFinding::withinDistance()
{
    const float distanceSqr = getSimpleTargetDistanceSqr(m_bot);
    return distanceSqr <= m_radiusSqr && m_minRadiusSqr <= distanceSqr;
}

// [UNRESOLVED] The navigation's +192 flag, which BotNavigation.h names isPathSearchPending; read here
// as the stuck flag (setIsStuck writes it).
bool BAPAMoveToFinding::isStuck()
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->isPathSearchPending();
}

} // namespace ai
} // namespace hfe
} // namespace dice
