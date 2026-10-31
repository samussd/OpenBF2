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
#include "../AIKnowledge/IPIUnit.h"
#include "../AIPathFinding/NavMesh/QuadTree.h"
#include "../Main/IAIPathfinding.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AIPathfindingSearchTypes.h"
#include "../../../../Core/Debug.h"
#include "../../../../IO/Input/PlayerInput.h"

#include <cmath>

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
    , m_targetMaterial(0)
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
        m_zero176[i] = 0;
        m_zero216[i] = 0;
    }
    m_startPosition.x = 0.0f;
    m_startPosition.y = 0.0f;
    m_startPosition.z = 0.0f;
    m_searchTarget = m_startPosition;
    m_grenadePosition.x = 0.0f;
    m_grenadePosition.y = 0.0f;
    m_grenadePosition.z = 0.0f;
    m_pathOwner = 0;                   // +60 - a pointer, see BotNavigation.h
    m_quadTree = 0;
    m_zero308 = 0;
    m_pathLength = 0.0f;
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
    m_zero308 = 0;
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

// 0x1a46a0 (Ghidra). What is left of the way: to the current point, along the points (m_pathLength)
// and on along the portals of the cluster path. As shipped the portals are read from the start of
// the cluster path, however far it has been walked, and a missing portal counts as the last one.
void BotNavigation::updateClosestDistance()
{
    if (m_points.empty())
    {
        m_sentinel188 = 0.0f;
        return;
    }
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    const Vec3& at = equipment->getPosition();
    const Vec3& current = m_points[m_int116].m_position;
    float dx = at.x - current.x;
    float dy = at.y - current.y;
    float dz = at.z - current.z;
    float length = std::sqrt(dz * dz + dx * dx + dy * dy);
    unsigned int point = static_cast<unsigned int>(m_int116) + 1;
    for (; point < m_points.size(); ++point)
    {
        const Vec3& from = m_points[point - 1].m_position;
        const Vec3& to = m_points[point].m_position;
        dx = from.x - to.x;
        dy = from.y - to.y;
        dz = from.z - to.z;
        length = std::sqrt(dz * dz + dx * dx + dy * dy) + length;
    }
    m_pathLength = length;
    const ClusterNodePath* const path = m_clusterPath;
    const unsigned int nodes = static_cast<unsigned int>(path->m_nodes.size());
    if (static_cast<unsigned int>(path->m_i0) < nodes)
    {
        const unsigned int left = nodes - static_cast<unsigned int>(path->m_i0);
        Vec3 last = m_points[point - 1].m_position;
        Vec3 portal;
        portal.x = 0.0f;
        portal.y = 0.0f;
        portal.z = 0.0f;
        for (unsigned int node = 1; node < left; ++node)
        {
            unsigned int unused;
            path->getPoint(portal, unused, node);
            dx = last.x - portal.x;
            dy = last.y - portal.y;
            dz = last.z - portal.z;
            length = std::sqrt(dx * dx + dy * dy + dz * dz) + length;
            last = portal;
        }
    }
    m_sentinel188 = length;
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

// 0x1a01b0 (Ghidra). A target off the mesh, or no equipment, starts nothing.
bool BotNavigation::initPathfinding(const Vec3& to, float a, float b)
{
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (equipment)
    {
        const int type = equipment->getUnit()->getEquipmentType();
        const int vehicle = equipment->getMobile()->getVehicleNumber();
        unsigned int polygon;
        unsigned int quadTree;
        if (IAIPathfinding::instance->getPolygon(to, vehicle, polygon, quadTree, type))
            return initPathfinding(quadTree, polygon, to, a, b);
    }
    m_flag193 = false;
    return false;
}

// 0x1a02b0 (Ghidra), read in the asm. The same target for the same vehicle while a search is on
// keeps it: true at once with under two stages, or when the bot is on the current point's polygon
// or still on the path; otherwise more than one point is retraced. Anything else starts over from
// the bot's last valid place, which must be within 10 m of it and on the target's quadtree. True:
// the search is on (the direct path may already have answered it).
bool BotNavigation::initPathfinding(unsigned int quadTree, unsigned int polygon, const Vec3& to, float a, float b)
{
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (!equipment)
    {
        m_flag193 = false;
        return false;
    }
    m_flag236 = (equipment->getObjectTypeFlags() & 0x200000) != 0;
    if (!m_pathOwner)
        m_pathOwner = IAIPathfinding::instance;
    IPIMobile* const mobile = equipment->getMobile();
    const IPIMobile::ValidPos& last = mobile->getLastValidPosition();
    const bool sameTarget = std::fabs(to.x - m_searchTarget.x) <= 1.1920929e-07f
        && std::fabs(to.y - m_searchTarget.y) <= 1.1920929e-07f
        && std::fabs(to.z - m_searchTarget.z) <= 1.1920929e-07f;
    if (!sameTarget)
        clearProgress();
    if (m_flag193 && sameTarget && m_vehicleNumber == mobile->getVehicleNumber()
        && m_equipmentType == equipment->getUnit()->getEquipmentType())
    {
        if (static_cast<unsigned int>(m_int56) < 2)
            return true;
        if (!m_points.empty() && static_cast<unsigned int>(m_points[m_int116].m_index36) == last.polygon)
            return true;
        if (!m_points.empty() && stillOnPath())
            return true;
        if (m_points.size() > 1)
        {
            retrace();
            return true;
        }
    }
    if (quadTree != 0xFFFFFFFFu && polygon != 0xFFFFFFFFu)
    {
        m_int36 = static_cast<int>(polygon);
        m_searchTarget = to;
        if (last.quadTree != 0xFFFFFFFFu && last.polygon != 0xFFFFFFFFu)
        {
            const Vec3& at = equipment->getPosition();
            const float dx = at.x - last.position.x;
            const float dy = at.y - last.position.y;
            const float dz = at.z - last.position.z;
            if (!(dx * dx + dy * dy + dz * dz > 100.0f))
            {
                if (quadTree != last.quadTree)
                {
                    Debug(DebugTypeError, "AI/AIDLL/AI/AIBot/BotNavigation.cpp", 0xc4, "")
                        << "Bot " << m_bot->getName()
                        << " is ordered to go to an island that it is not on (" << quadTree << " != "
                        << last.quadTree << ")";
                    m_flag193 = false;
                    return false;
                }
                m_flag68 = true;
                m_flag120 = true;
                m_flag192 = false;
                m_int16 = static_cast<int>(last.quadTree);
                m_startQuadTree = last.quadTree;
                m_int20 = static_cast<int>(last.polygon);
                m_startPosition = last.position;
                m_float4 = a;
                m_float8 = b;
                m_float8Sqr = b * b;
                m_vehicleNumber = mobile->getVehicleNumber();
                const IPIMobile::ValidPos& vehiclePosition = mobile->getLastValidPosition(m_vehicleNumber);
                m_int204 = static_cast<int>(vehiclePosition.quadTree);
                m_int200 = static_cast<int>(vehiclePosition.polygon);
                const IPIMobile::ValidPos& infantry = mobile->getLastValidInfantryPos();
                m_int212 = static_cast<int>(infantry.quadTree);
                m_int208 = static_cast<int>(infantry.polygon);
                m_equipmentType = equipment->getUnit()->getEquipmentType();
                m_flag195 = false;
                m_int240 = 0;
                reset();
                m_flag193 = true;
                m_smallestRadius = equipment->getPhysical()->getSmallestRadius();
                m_pathOwner->getMaterial(m_vehicleNumber, quadTree, static_cast<unsigned int>(m_int36),
                                         m_targetMaterial);
                testDirectPath();
                m_quadTree = static_cast<QuadTree*>(
                    m_pathOwner->getQuadTree(m_vehicleNumber, static_cast<unsigned int>(m_int16)));
                m_flag316 = true;
                return true;
            }
        }
    }
    m_flag193 = false;
    m_flag192 = true;
    return false;
}

// 0x1a0da0 (Ghidra). A target within 200 m of the start that a straight trace reaches needs no
// search: it becomes the last navigation point and the two stages are done.
void BotNavigation::testDirectPath()
{
    static const float minDistSqr = 40000.0f;
    const float dx = m_searchTarget.x - m_startPosition.x;
    const float dy = m_searchTarget.y - m_startPosition.y;
    const float dz = m_searchTarget.z - m_startPosition.z;
    if (!(dx * dx + dy * dy + dz * dz < minDistSqr))
        return;
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (!equipment || !equipment->getMobile())
        return;
    Vec3 hit;
    hit.x = 0.0f;
    hit.y = 0.0f;
    hit.z = 0.0f;
    unsigned int polygon;
    unsigned char material;
    if (!m_pathOwner->trace(static_cast<unsigned int>(m_int20), m_startQuadTree, m_vehicleNumber,
                            m_startPosition, m_searchTarget, hit, polygon, m_equipmentType, -1, 0.0f, 5,
                            &material))
        return;
    const std::size_t index = m_points.size();
    m_points.resize(index + 1);
    NavigationPoint& point = m_points[index];
    point.m_position = m_searchTarget;
    point.m_index36 = static_cast<int>(polygon);
    point.m_point0 = m_searchTarget;
    point.m_point12 = m_searchTarget;
    m_int36 = static_cast<int>(polygon);
    m_targetMaterial = material;
    m_flag68 = false;
    m_flag120 = false;
    m_int116 = 0;
    m_int56 = 2;
    updateClosestDistance();
}

// 0x1a10c0 (Ghidra). The bot left its points: they go, and the walk of the cluster path restarts at
// the cluster the bot is in, or at a cluster next to it. Without either, or with under two nodes,
// the search is off. No null guard on the equipment, as shipped.
void BotNavigation::retrace()
{
    ClusterNodePath* const path = m_clusterPath;
    const unsigned int nodes = static_cast<unsigned int>(path->m_nodes.size());
    if (nodes >= 2)
    {
        path->m_i0 = 0;
        m_points.clear();
        Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
        const IPIMobile::ValidPos& last = equipment->getMobile()->getLastValidPosition();
        m_int16 = static_cast<int>(last.quadTree);
        m_int20 = static_cast<int>(last.polygon);
        m_startPosition = last.position;
        m_flag195 = true;
        const unsigned short cluster = static_cast<unsigned short>(m_pathOwner->getClusterOfPolygon(
            m_vehicleNumber, static_cast<unsigned int>(m_int16), last.polygon));
        for (unsigned int i = 0; i + 1 < nodes; ++i)
        {
            unsigned short node = 0xFFFF;
            if (path->m_i0 < static_cast<int>(path->m_nodes.size()))
                node = path->m_nodes[path->m_i0].first;
            if (cluster == node)
                return;
            ++path->m_i0;
        }
        path->m_i0 = 0;
        for (unsigned int i = 0; i + 1 < nodes; ++i)
        {
            unsigned short node = 0xFFFF;
            if (path->m_i0 < static_cast<int>(path->m_nodes.size()))
                node = path->m_nodes[path->m_i0].first;
            if (m_pathOwner->areNeighbouringClusters(m_vehicleNumber, static_cast<unsigned int>(m_int16),
                                                     cluster, node))
                return;
            ++path->m_i0;
        }
    }
    m_flag195 = false;
    reset();
}

// 0x1a3ec0 (Ghidra). The bot's polygon is the current point's or the one before it, an earlier
// point's, or one of the next 20 (the bot then skips ahead to it). No null guard on the equipment,
// as shipped.
bool BotNavigation::stillOnPath()
{
    if (m_points.size() < 2)
        return false;
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    const int polygon = static_cast<int>(equipment->getMobile()->getLastValidPosition().polygon);
    if (polygon == -1)
        return false;
    const unsigned int current = static_cast<unsigned int>(m_int116);
    const unsigned int size = static_cast<unsigned int>(m_points.size());
    if (current >= size)
        return false;
    const int previous = current != 0 ? m_points[current - 1].m_index36 : -1;
    if (polygon == previous || polygon == m_points[current].m_index36)
        return true;
    for (int i = static_cast<int>(current); i >= 0; --i)
    {
        if (m_points[i].m_index36 == polygon)
            return true;
    }
    const unsigned int left = size - current;
    const unsigned int end = current + (left < 20 ? left : 20);
    for (unsigned int i = current; i < end; ++i)
    {
        if (m_points[i].m_index36 == polygon)
        {
            m_int116 = static_cast<int>(i);
            const int rest = m_int240 - static_cast<int>(i - current);
            m_int240 = rest > 0 ? rest : 0;
            return true;
        }
    }
    return false;
}

// 0x1a2200 (Ghidra), read in the asm. A soldier firing at an enemy waits on a polygon of material 4.
// A progress time of -666 presses the action input once. With 30 m or more left and over ten
// points to go, the points beyond the next ten are dropped only while the current polygon's
// cluster is the walk's; as shipped that also compares the cluster with the walk's index. Off the
// path for longer than 2 s (0.5 s for a non-soldier) the bot retraces, after which retail reads
// the current point from a list that retrace may have emptied.
bool BotNavigation::updatePath(Vec2& point, bool& arrived, bool& waiting, float distance)
{
    if (m_flag192)
        return false;
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    m_lastActiveTime = now;
    if (!m_flag193)
        return false;
    arrived = false;
    waiting = false;
    if (static_cast<unsigned int>(m_int56) < 2)
        m_lastActiveTime = 0.0;
    if (m_flag120 || m_flag195
        || (m_flag68 && static_cast<unsigned int>(m_int116) >= m_points.size()))
    {
        waiting = true;
        return true;
    }
    Information* const target = informationManager->getInformation(m_bot->getFiringTarget());
    if (!m_points.empty() && m_flag236 && m_bot->isFiring() && target
        && target->getSide() != m_bot->getSide())
    {
        const int polygon = m_points.size() < 2 ? m_int36 : m_points[m_int116].m_index36;
        unsigned char material;
        if (m_pathOwner->getMaterial(m_vehicleNumber, m_startQuadTree, static_cast<unsigned int>(polygon),
                                     material)
            && material == 4)
        {
            waiting = true;
            m_lastActiveTime = 0.0;
            return true;
        }
    }
    if (m_double168 == -666.0f)
    {
        m_bot->getPlayerInput()->setInput(io::PIAction, 1.0f);
        m_double168 = now;
    }
    if (checkArrivedAtPoint(distance))
    {
        if (m_int116 == static_cast<int>(m_points.size()) - 1)
        {
            m_lastActiveTime = 0.0;
            m_sentinel188 = 100000000.0f;
            clearProgress();
            arrived = true;
        }
        else
        {
            ++m_int116;
            if (m_int240 > 0)
                --m_int240;
        }
    }
    bool sameCluster = true;
    if (!(30.0f > m_pathLength) && m_points.size() > static_cast<unsigned int>(m_int116) + 10)
    {
        const int cluster = m_quadTree->getPolygon(m_points[m_int116].m_index36)->m_cluster;
        if (cluster != m_clusterPath->m_i0)
        {
            unsigned short node = 0xFFFF;
            if (m_clusterPath->m_i0 < static_cast<int>(m_clusterPath->m_nodes.size()))
                node = m_clusterPath->m_nodes[m_clusterPath->m_i0].first;
            sameCluster = static_cast<unsigned short>(cluster) == node;
        }
    }
    if (sameCluster && static_cast<unsigned int>(m_clusterPath->m_i0) + 1 <= m_clusterPath->m_nodes.size()
        && !m_flag68)
    {
        m_flag68 = true;
        const std::size_t keep = static_cast<std::size_t>(m_int116) + 10;
        if (m_points.size() > keep)
            m_points.resize(keep);
    }
    if (!m_flag195)
    {
        if (m_int56 != 2 && (!m_flag68 || static_cast<unsigned int>(m_int56) > 1))
            return false;
        stillOnPath();
        if (!m_flag194)
            m_double160 = now;
        else if (now - m_double160 > (m_flag236 ? 2.0f : 0.5f))
        {
            m_flag194 = false;
            // A local static of the binary with no writer: always false.
            static const bool useSkipTrace = false;
            m_int240 = (useSkipTrace && m_flag236) ? 1 : 0;
            retrace();
        }
    }
    // [PLATFORM] data(), not operator[]: see above.
    vec3ToVec2XZ(m_points.data()[m_int116].m_position, point);
    return true;
}

// 0x1a49e0 (Ghidra). Within 3 m in height, and in the plane within 0.5 m for a soldier; otherwise
// within max(m_float4, 1), or at the last point the equipment's smallest radius less 1 if that is
// more. No null guard on the equipment, as shipped. The distance is not used.
bool BotNavigation::checkArrivedAtPoint(float)
{
    static const float soldierMinDist = 0.5f;
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    const Vec3& at = equipment->getPosition();
    if (m_points.empty())
        return false;
    const Vec3& point = m_points[m_int116].m_position;
    if (std::fabs(at.y - point.y) > 3.0f)
        return false;
    float radius = soldierMinDist;
    if (!m_flag236)
    {
        radius = m_float4 <= 1.0f ? 1.0f : m_float4;
        if (m_int116 == static_cast<int>(m_points.size()) - 1)
        {
            const float room = m_smallestRadius - 1.0f;
            radius = room <= radius ? radius : room;
        }
    }
    const float dx = at.x - point.x;
    const float dz = at.z - point.z;
    return radius * radius > dx * dx + dz * dz;
}

// 0x1a40c0 (Ghidra). The point after the current one.
bool BotNavigation::getSecondPoint(Vec2& out)
{
    const std::size_t next = static_cast<std::size_t>(m_int116) + 1;
    if (next >= m_points.size())
        return false;
    vec3ToVec2XZ(m_points[next].m_position, out);
    return true;
}

// [FRONTIER] 0x1a4120 (Ghidra), 1.4 KB.
bool BotNavigation::getLargestAngleDiff(float&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::getLargestAngleDiff");
}

} // namespace ai
} // namespace hfe
} // namespace dice
