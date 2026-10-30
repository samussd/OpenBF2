// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BotNavigation operations.

#include "BotNavigation.h"

#include "BotMain.h"                       // getBotId / getCurrentEquipment - vtable +0xE8, +0xD8
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"   // IBFTimer::getGameTime
#include "../AIKnowledge/AIInformationGrid.h"       // the obstacle predicate's base
#include "../AIKnowledge/BFInformationEnvironment.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIPathFinding/NavMesh/QuadTree.h"
#include "../Main/IAIPathfinding.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Creates navigation paths and initializes movement state for the supplied bot.
BotNavigation::BotNavigation(BotMain* owner)
    : m_bot(owner)
    , m_float4(2.0f)
    , m_float8(2.0f)
    , m_int16(-1)
    , m_int20(-1)
    , m_int36(-1)
    , m_flag40(false)
    // the two paths, in the order the binary builds them: the fine one first.
    , m_polygonPath(checkPointer(new QTPath()))
    , m_polygonResource(0xFFFFFFFFu)
    , m_int116(0)
    , m_clusterPath(checkPointer(new ClusterNodePath()))
    , m_clusterResource(0xFFFFFFFFu)
    , m_double144(0.0)
    , m_double152(0.0)
    , m_double160(0.0)
    , m_double168(0.0)
    // 9,999,999.0f - a distance sentinel, not a time and not a magnitude. See above.
    , m_sentinel188(9999999.0f)
    , m_flag192(false)
    , m_flag193(false)
    , m_flag195(false)
    , m_int200(-1)
    , m_int204(-1)
    , m_int208(-1)
    , m_int212(-1)
    , m_int240(0)
    , m_grenadeTime(0.0)
    , m_grenadeFlag(false)
    // Independent sentinel, initialized to -1.0.
    , m_double300(-1.0)
{
    for (int i = 0; i < 3; ++i)
    {
        m_zero24[i]  = 0;
        m_zero44[i]  = 0;
        m_zero176[i] = 0;
        m_zero216[i] = 0;
    }
    m_grenadePosition.x = 0.0f;
    m_grenadePosition.y = 0.0f;
    m_grenadePosition.z = 0.0f;
    m_pathOwner = 0;                   // +60 - a pointer, see BotNavigation.h
    m_quadTree = 0;
    m_zero308[0] = 0;
    m_zero308[1] = 0;
    for (int i = 0; i < 3; ++i)
        m_zero280[i] = 0;
    m_sideSpace[0] = 0.0f;
    m_sideSpace[1] = 0.0f;
    for (int i = 0; i < 6; ++i)
        m_zero320[i] = 0;

    // the dead resize - see the block above. Written as what it is rather than dropped.
    m_points.resize(0);
}

// [FRONTIER] Remaining navigation operations stop until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

BotNavigation::~BotNavigation()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::~BotNavigation");
}

// 0x1a5280 (Ghidra). The shipped body also reads the game time and drops it.
void BotNavigation::setGrenadeWarning(const Vec3& position, double time, bool flag)
{
    m_grenadeTime = time;
    m_grenadePosition = position;
    m_grenadeFlag = flag;
}

// 0x1a5310 (Ghidra).
bool BotNavigation::getGrenadeWarning(Vec3& position, double& timeLeft)
{
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    position = m_grenadePosition;
    timeLeft = 2.8 - (now - m_grenadeTime);
    return timeLeft > 0.0;
}

// Resets navigation, movement flags and progress timing.
void BotNavigation::resetAll()
{
    reset();

    m_obstacleCount = 0;
    m_grenadeFlag = false;
    // Not the constructor's 0.0 - see the block above.
    m_grenadeTime = -9999.0;
    m_zero308[0] = 0;
    m_double300 = -1.0;

    clearProgress();
}

// Resets navigation state and returns path resources to their owners.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] Incomplete resource paths stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Implement and validate path release and progress reset.
// [RETIRE-WHEN: botmain-bases] Remove when the corresponding implementation is complete and validated.
void BotNavigation::reset()
{
    m_flag316 = false;

    // giving the pathfinding resources back.
    if (m_polygonResource != 0xFFFFFFFFu)
    {
        m_pathOwner->releaseResource(m_polygonResource, m_bot->getBotId());
        m_polygonResource = 0xFFFFFFFFu;
    }
    if (m_clusterResource != 0xFFFFFFFFu)
    {
        m_pathOwner->releaseClusterResource(m_clusterResource, m_bot->getBotId());
        m_clusterResource = 0xFFFFFFFFu;
    }

    // Reaching through both path objects - see the block above.
    m_polygonPath->m_nodes.resize(0);
    m_polygonPath->m_i0 = 0;
    m_polygonPath->m_i20 = 0;

    m_clusterPath->m_nodes.resize(0);
    m_clusterPath->m_i0 = 0;

    m_int56 = 0;

    m_points.resize(0);

    m_int240 = 0;
    m_int116 = 0;
    m_double160 = 0.0;
    m_flag195 = false;
    m_flag193 = false;
    m_flag194 = false;
    m_lastActiveTime = 0.0;

    // Resolves the bot's current equipment handle; skips equipment work when it has no object.
    const unsigned int equipment = m_bot->getCurrentEquipment();
    Information* const info = informationManager->getPool()->get(equipment);
    // where the equipment last stood on the mesh: for its vehicle, then as infantry
    if (info && info->getMobile())
    {
        IPIMobile* const mobile = info->getMobile();
        m_vehicleNumber = mobile->getVehicleNumber();
        const IPIMobile::ValidPos& position = mobile->getLastValidPosition(m_vehicleNumber);
        m_int200 = position.polygon;
        m_int204 = position.quadTree;
        const IPIMobile::ValidPos& infantry = mobile->getLastValidInfantryPos();
        m_int208 = infantry.polygon;
        m_int212 = infantry.quadTree;
    }

    // The sentinel is the interesting one - see the block above.
    m_sentinel188 = 9999999.0f;
    m_sideSpace[0] = 0.0f;
    m_sideSpace[1] = 0.0f;
}

// Clears progress counters and restarts the progress timer at the current game time.
// Paused game time does not accumulate lack of progress.
void BotNavigation::clearProgress()
{
    m_zero176[0] = 0;                      // +176

    // the one non-zero store. See the block above.
    m_double144 =
        static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    m_double168 = 0.0;
    m_zero176[1] = 0;                      // +180
    m_zero176[2] = 0;                      // +184
    m_double152 = 0.0;
    m_flag192 = false;
    m_zero280[0] = 0;                      // +280
    m_zero280[1] = 0;                      // +284
    m_zero280[2] = 0;                      // +288
}

namespace {

// Moving primary objects (vehicles, soldiers) and anything flagged 0x2000000.
class MobileOrObstaclePredicate : public IAIInformationGridPredicate
{
public:
    bool includeInformation(Information* information) const override
    {
        return (information->isPrimary() && information->hasMobile())
            || (information->getObjectTypeFlags() & 0x2000000u);
    }
};

} // namespace

// Everything movable within 50 m of the bot, both the 1 and the 0 query of the environment.
void BotNavigation::updateObstacles()
{
    m_obstacles.resize(0);
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    const Vec3& position = equipment->getPosition();
    MobileOrObstaclePredicate predicate;
    IAIInformationEnvironment* const environment =
        static_cast<IAIInformationEnvironment*>(AISingletonServer::m_iaiInformationEnvironment);
    environment->getObjectsWithinRadius(m_obstacles, position, 50.0f, m_bot->getSide(), true,
                                        &predicate);
    environment->getObjectsWithinRadius(m_obstacles, position, 50.0f, m_bot->getSide(), false,
                                        &predicate);
    m_obstacleCount = static_cast<int>(m_obstacles.size());
}

// Runs the pending search. False without a pathfinder or in state 4.
bool BotNavigation::update(bool& a, bool* b)
{
    if (!m_pathOwner)
        return false;
    m_flag316 = false;
    if (m_int56 == 4)
        return false;

    a = false;
    if (b)
        *b = false;

    if (m_flag195)
    {
        m_points.resize(0);
        m_int116 = 0;
        m_flag68 = true;
        m_flag195 = false;
    }
    if (m_flag120)
        return doClusterSearch(a, b);
    if (m_flag68)
        return doPolygonSearch(a, b);
    return true;
}

bool BotNavigation::isActive()
{
    return 3.0 > static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime()
                     - m_lastActiveTime;
}

void BotNavigation::updateOccationally()
{
    updateNavPoints();
    updateClosestDistance();
    if (m_int56 == 2)
    {
        if (hasIllegalPath())
            reset();
        else if (!checkProgress())
            m_flag192 = true;
    }
    if (m_flag236)
        updateSpaceLeftAndRight();
}

// A path point on a missing polygon, or on one whose material is 0.
bool BotNavigation::hasIllegalPath()
{
    for (unsigned int i = 0; i < m_points.size(); ++i)
    {
        const Polygon* const polygon = m_quadTree->getPolygon(m_points[i].m_index36);
        if (!polygon || !polygon->m_material)
            return true;
    }
    return false;
}

// [FRONTIER] [TEMP] [RETIRE-WHEN: aidll-02-skeleton] The search and path-following half of the
// navigation; next in the tick order once the bots get a move order.
void BotNavigation::updateNavPoints()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::updateNavPoints");
}

void BotNavigation::updateClosestDistance()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::updateClosestDistance");
}

bool BotNavigation::checkProgress()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::checkProgress");
}

void BotNavigation::updateSpaceLeftAndRight()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::updateSpaceLeftAndRight");
}

bool BotNavigation::doPolygonSearch(bool&, bool*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::doPolygonSearch");
}

bool BotNavigation::doClusterSearch(bool&, bool*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::doClusterSearch");
}

// 0x1a4cf0 (Ghidra). Whether the point lies within the smallest radius of an obstacle that is
// not a soldier.
bool BotNavigation::pointWithinObstacleRadius(const Vec3& point)
{
    for (unsigned int i = 0; i < m_obstacles.size(); ++i)
    {
        Information* const obstacle = informationManager->getInformation(m_obstacles[i]);
        if (!obstacle || (obstacle->getObjectTypeFlags() & 0x200000) != 0)
            continue;
        IPIPhysical* const physical = obstacle->getPhysical();
        if (!physical)
            continue;
        const Vec3& at = obstacle->getPosition();
        const float dx = at.x - point.x;
        const float dy = at.y - point.y;
        const float dz = at.z - point.z;
        const float radius = physical->getSmallestRadius();
        if (radius * radius > dx * dx + dy * dy + dz * dz)
            return true;
    }
    return false;
}

// Always returns true, leaving BotManager's final pathfinding sweep gated by isSearchActive.
bool BotNavigation::isRunning()
{
    return true;
}

// 0x1a4e50 (Ghidra). From the current point on, the first navigation point farther than the
// distance from `from` (XZ). out keeps the last point looked at, and is left as it was when no
// point is left.
void BotNavigation::getFirstPointBeyondDist(const Vec2& from, float distance, Vec2& out)
{
    for (unsigned int i = static_cast<unsigned int>(m_int116); i < m_points.size(); ++i)
    {
        const Vec3& at = m_points[i].m_position;
        out.x = at.x;
        out.y = at.z;
        const float dx = from.x - at.x;
        const float dz = from.y - at.z;
        if (distance * distance < dz * dz + dx * dx)
            return;
    }
}

// [FRONTIER] 0x1a01b0 (Ghidra), 248 B: starts a search toward a position.
bool BotNavigation::initPathfinding(const Vec3&, float, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::initPathfinding");
}

// [FRONTIER] 0x1a2200 (Ghidra), 1.4 KB: follows the found path one step.
bool BotNavigation::updatePath(Vec2&, bool&, bool&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::updatePath");
}

// [FRONTIER] 0x1a40c0 (Ghidra), 94 B.
bool BotNavigation::getSecondPoint(Vec2&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::getSecondPoint");
}

// [FRONTIER] 0x1a4120 (Ghidra), 1.4 KB.
bool BotNavigation::getLargestAngleDiff(float&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::getLargestAngleDiff");
}

} // namespace ai
} // namespace hfe
} // namespace dice
