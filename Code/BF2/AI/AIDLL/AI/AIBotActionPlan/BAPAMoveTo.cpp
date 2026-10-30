#include "BAPAMoveTo.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBot/SensingAgent.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIPlanInterpreter/InfanteryControl.h"
#include "../Main/AIRandom.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

namespace {

// Unit length, unless already within an epsilon of it; zero for a vector of almost no length.
void normaliseOrZero(Vec2& v)
{
    const float length2 = v.x * v.x + v.y * v.y;
    if (std::fabs(length2 - 1.0f) < 1.1920929e-07f)
        return;
    if (std::fabs(length2) < 1.4210855e-14f)
    {
        v.x = 0.0f;
        v.y = 0.0f;
        return;
    }
    const float inverse = 1.0f / std::sqrt(length2);
    v.x = v.x * inverse;
    v.y = inverse * v.y;
}

} // namespace

// 0x1e5150 (Ghidra). The pose component: standing normally, lying under incoming fire stronger
// than a threshold drawn in [1, 15) (the component spreads it again), crouching in danger, 10 s.
BAPAMoveTo::BAPAMoveTo(Bot* bot)
    : BAPAction(0)
    , BAPAComponentSoldierPose(SPStanding, SPLying,
                               static_cast<float>((15.0 - 1.0)
                                                  * (static_cast<double>(AIRandom::m_instance->next())
                                                     * 2.3283064365386963e-10)
                                                  + 1.0),
                               SPCrouching, 0.0f, 0xFFFFFFFFu, 10.0f)
    , m_isSprinting(false)
    , m_float60(0.0f)
    , m_float64(1.0f)
    , m_float68(0.0f)
    , m_float74(0.0f)
    , m_float78(0.0f)
    , m_obstacle(0xFFFFFFFFu)
    , m_nextDirTime(-1000.0)
{
    m_coordinate.x = 0.0f;
    m_coordinate.y = 0.0f;
    m_coordinate.z = 0.0f;
    m_currentPoint.x = 0.0f;
    m_currentPoint.y = 0.0f;
    m_avoidPoint.x = 0.0f;
    m_avoidPoint.y = 0.0f;
    m_obstaclePosition.x = 0.0f;
    m_obstaclePosition.y = 0.0f;
    m_lastDir.x = 0.0f;
    m_lastDir.y = 0.0f;
    static_cast<BotMain*>(bot)->setMoveToStatus(0);
}

BAPAMoveTo::~BAPAMoveTo()
{
    if (m_finishCondition)
        delete m_finishCondition;
}

unsigned long long BAPAMoveTo::getControls()
{
    return m_controls;
}

BAPStatementType BAPAMoveTo::getId() const
{
    return BAPSTMoveTo;
}

bool BAPAMoveTo::hasDefault()
{
    return false;
}

bool BAPAMoveTo::getIsSprinting()
{
    return m_isSprinting;
}

bool BAPAMoveTo::getIsMovingAway()
{
    return false;
}

Vec2& BAPAMoveTo::getCurrentPoint()
{
    return m_currentPoint;
}

bool BAPAMoveTo::update(Vec2&, bool&, bool&, float)
{
    return false;
}

bool BAPAMoveTo::isFinished(Bot* bot)
{
    return m_finishCondition ? m_finishCondition->evaluate(bot) : false;
}

bool BAPAMoveTo::getSecondPoint(Vec2&)
{
    return false;
}

bool BAPAMoveTo::getLargestAngleDiff(float&)
{
    return false;
}

float BAPAMoveTo::getTargetDistance()
{
    return 0.0f;
}

unsigned int BAPAMoveTo::getObject()
{
    return 0xFFFFFFFFu;
}

// 0x1e5760 (Ghidra). No null guard on the equipment, as shipped.
float BAPAMoveTo::getSimpleTargetDistance(Bot* bot)
{
    return std::sqrt(getSimpleTargetDistanceSqr(bot));
}

float BAPAMoveTo::getSimpleTargetDistanceSqr(Bot* bot)
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    const Vec3& at = equipment->getPosition();
    const float dz = at.z - m_coordinate.z;
    const float dx = at.x - m_coordinate.x;
    const float dy = at.y - m_coordinate.y;
    return dz * dz + dx * dx + dy * dy;
}

bool BAPAMoveTo::isStuck()
{
    return false;
}

// 0x1e5660 (Ghidra). A position target leaves the sensing agent's move target handle as it was.
void BAPAMoveTo::setIsSprinting(Bot* bot, bool sprint, const Vec3& target, unsigned int object)
{
    m_isSprinting = sprint;
    if (!sprint)
        return;
    SensingAgent* const sensing = bot->getSensingAgent();
    if (object == 0xFFFFFFFFu)
        sensing->setMoveTargetPosition(target);
    else
    {
        sensing->setMoveTarget(object);
        Vec3 zero;
        zero.x = 0.0f;
        zero.y = 0.0f;
        zero.z = 0.0f;
        sensing->setMoveTargetPosition(zero);
    }
}

Vec3& BAPAMoveTo::getCoordinate()
{
    return m_coordinate;
}

float BAPAMoveTo::getMaxSpeed()
{
    return m_maxSpeed;
}

float BAPAMoveTo::getMaxArriveSpeed()
{
    return m_maxArriveSpeed;
}

// 0x1e58b0 (Ghidra), read in the asm. Toward the target, unit length ((1, 0) on the spot). With
// path finding, an obstacle near the path 10 m ahead is looked for when none is avoided or the
// avoided one needs a new look; true then means updateInfantryMoveToPosWithObstacles took the step.
// An avoided obstacle is dropped when gone, when the bot is clear of it (radius + 1), when it moved
// (0.5) or when the bot is within 0.3 of its target. While avoiding it, the bot keeps its last
// direction (true) when the goal itself lies inside the obstacle's room, heads for the avoidance
// point instead of the target unless it is the bot's change or repair target, and bends the
// direction onto a circle of m_passDistance around the obstacle while it is in the way. The
// target is a copy; moving it to the avoidance point does not reach the caller.
bool BAPAMoveTo::getTargetDir(Bot* bot, Vec2 target, Vec2& direction, Information* information, bool flag)
{
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (m_nextDirTime > now)
    {
        direction = m_lastDir;
        return m_lastDirResult;
    }
    m_lastDirResult = true;
    m_nextDirTime = now + 0.0f;
    // no null guard on the equipment, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    Vec2 position;
    position.x = 0.0f;
    position.y = 0.0f;
    vec3ToVec2XZ(equipment->getPosition(), position);
    if (usesPathfinding()
        && (m_obstacle == 0xFFFFFFFFu || needsUpdate(m_avoidPoint, m_obstacle, m_obstacleRadius)))
    {
        Vec2 ahead;
        ahead.x = 0.0f;
        ahead.y = 0.0f;
        static_cast<BotMain*>(bot)->getBotNavigation()->getFirstPointBeyondDist(position, 10.0f, ahead);
        if (updateInfantryMoveToPosWithObstacles(bot, ahead, m_coordinate, getObject(), m_avoidPoint,
                                                 m_obstacle, m_passDistance, m_obstaclePosition,
                                                 m_obstacleRadius, m_obstacleFlag, information, flag))
            return true;
    }

    direction.x = target.x - position.x;
    direction.y = target.y - position.y;
    const float targetDistance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    float distance = targetDistance;
    bool keepTarget = false;
    Information* const obstacle = informationManager->getInformation(m_obstacle);
    if (!obstacle)
        m_obstacle = 0xFFFFFFFFu;
    else
    {
        Vec2 toObstacle;
        toObstacle.x = m_obstaclePosition.x - position.x;
        toObstacle.y = m_obstaclePosition.y - position.y;
        const float fromX = position.x - m_obstaclePosition.x;
        const float fromY = position.y - m_obstaclePosition.y;
        const float obstacleDistanceSqr = fromX * fromX + fromY * fromY;
        if ((1.0f + m_obstacleRadius) * (1.0f + m_obstacleRadius) < obstacleDistanceSqr)
            m_obstacle = 0xFFFFFFFFu;
        bool ownTarget = false;
        if (m_obstacle != 0xFFFFFFFFu)
        {
            unsigned int change = bot->getChangeTarget();
            Information* info = informationManager->getInformation(change);
            if (info && !info->isPrimary())
                change = info->getPrimaryObject(bot->getSide());
            unsigned int repair = bot->getRepairTarget();
            info = informationManager->getInformation(repair);
            if (info && !info->isPrimary())
                repair = info->getPrimaryObject(bot->getSide());
            ownTarget = m_obstacle == repair || m_obstacle == change;
        }
        keepTarget = ownTarget;
        if (m_obstacle != 0xFFFFFFFFu)
        {
            if (!ownTarget && !m_obstacleFlag)
            {
                keepTarget = 0.0f > toObstacle.x * direction.x + toObstacle.y * direction.y;
                if (!keepTarget && information
                    && !(toObstacle.x * toObstacle.x + toObstacle.y * toObstacle.y
                         < targetDistance * targetDistance))
                    keepTarget = true;
            }
            Vec2 obstacleAt;
            obstacleAt.x = 0.0f;
            obstacleAt.y = 0.0f;
            vec3ToVec2XZ(obstacle->getPosition(), obstacleAt);
            const float movedX = m_obstaclePosition.x - obstacleAt.x;
            const float movedY = m_obstaclePosition.y - obstacleAt.y;
            if (!(0.25f > movedX * movedX + movedY * movedY))
                m_obstacle = 0xFFFFFFFFu;
            else
            {
                Vec2 goal;
                goal.x = 0.0f;
                goal.y = 0.0f;
                vec3ToVec2XZ(m_coordinate, goal);
                const float goalX = goal.x - position.x;
                const float goalY = goal.y - position.y;
                const float goalSqr = goalX * goalX + goalY * goalY;
                const float obstacleGoalX = obstacleAt.x - goal.x;
                const float obstacleGoalY = obstacleAt.y - goal.y;
                const float obstacleToGoal = std::sqrt(obstacleGoalX * obstacleGoalX
                                                       + obstacleGoalY * obstacleGoalY);
                const float radius = m_obstacleRadius;
                const float room = radius - obstacleToGoal + 0.8f;
                const bool goalInside = room * room > goalSqr;
                bool keepDirection = m_obstacleFlag && radius >= obstacleToGoal
                    && (goalInside
                        || (radius > m_passDistance
                            && (radius + 1.0f) * (radius + 1.0f) >= obstacleDistanceSqr));
                if (!keepDirection && (!keepTarget || ownTarget) && radius >= obstacleToGoal && goalInside)
                    keepDirection = true;
                if (keepDirection)
                {
                    m_lastDir = direction;
                    return true;
                }
                target = m_avoidPoint;
                direction.x = m_avoidPoint.x - position.x;
                direction.y = m_avoidPoint.y - position.y;
                distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            }
        }
    }

    if (getIsSprinting() && 1.1920929e-07f < std::fabs(target.x - 0.0f) + std::fabs(target.y - 0.0f))
    {
        Vec3 look;
        look.x = target.x;
        look.y = 0.0f;
        look.z = target.y;
        setIsSprinting(bot, true, look, 0xFFFFFFFFu);
    }

    if (m_obstacle != 0xFFFFFFFFu)
    {
        if (0.3f > distance)
            m_obstacle = 0xFFFFFFFFu;
        if (m_obstacle != 0xFFFFFFFFu && !keepTarget && distance > 0.5f)
        {
            Vec2 away;
            away.x = m_avoidPoint.x - position.x;
            away.y = m_avoidPoint.y - position.y;
            normaliseOrZero(away);
            Vec2 toObstacle;
            toObstacle.x = m_obstaclePosition.x - position.x;
            toObstacle.y = m_obstaclePosition.y - position.y;
            normaliseOrZero(toObstacle);
            if (away.y * toObstacle.y + away.x * toObstacle.x > 0.05f)
            {
                Vec2 out;
                out.x = position.x + away.x - m_obstaclePosition.x;
                out.y = position.y + away.y - m_obstaclePosition.y;
                normaliseOrZero(out);
                direction.x = out.x * m_passDistance + m_obstaclePosition.x - position.x;
                direction.y = m_passDistance * out.y + m_obstaclePosition.y - position.y;
                normaliseOrZero(direction);
                m_lastDir = direction;
                m_lastDirResult = false;
                return false;
            }
        }
    }
    if (distance > 0.001f)
    {
        const float inverse = 1.0f / distance;
        direction.x = direction.x * inverse;
        direction.y = inverse * direction.y;
    }
    else
    {
        direction.x = 1.0f;
        direction.y = 0.0f;
    }
    m_lastDir = direction;
    m_lastDirResult = false;
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
