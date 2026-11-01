// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIStrategicArea operations.

#include "AIStrategicArea.h"

#include <algorithm>

#include <cmath>                                          // the two centre-to-corner distances

#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AISettingsInterfaces.h"                  // IAISettings::getNumberOfVehicleGroups
#include "../Main/CommonSingletonServer.h"
#include "../Main/IAIPathfinding.h"
#include "../Main/IBotManager.h"
#include "../Main/AIRandom.h"
#include "AIStrategicObjectManager.h"
#include "SAWayPoint.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/SquadManager.h"
#include "../AIBot/SquadMember.h"
#include "../AIBot/WPMoveTo.h"
#include "../AIBotBehaviour/CustomCurve.h"
#include "../AIStrategic/SAI.h"
#include "../AIStrategic/Strategy.h"                      // updateTemperature's modifiers
#include "../AIKnowledge/BFInformationEnvironment.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../../../AIInterface/BFKnowledge/BFGame.h"       // IAIGame - the control-point queries
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"    // IAITerrain::getTerrainAltitude
#include "../../../../Math/Vec3.h"                         // vec3ToVec2XZ / vec2ToVec3XZ

namespace dice {
namespace hfe {
namespace ai {

// Creates a control-point area with an unset-height sentinel and one record per pathfinding map.

AIStrategicArea::AIStrategicArea(const std::string& name, unsigned int controlPointId, float radius)
    : AIStrategicObject(radius, -666.0f)
    , m_list268()
    , m_centre()
    , m_mapData(checkPointer(new MapData[IAIPathfinding::instance->getNumberOfMaps()]))
    , m_neighbours()
    , m_neighbouringObjects()
    , m_name(name)
    , m_objectTypeFlags(0)
    , m_controlPointId(controlPointId)
    , m_landingZones()
    , m_landingZoneUsers(0)
    , m_b358(false)
{
    // The tail. Everything above this point lands before the three list members are constructed, so
    // it cannot be body code; everything here lands after them, so it cannot be an initialiser.
    m_layer = -1;
    m_b404 = true;
    m_statusChanged = false;
    m_i416 = 0;
    m_i420 = 0;
    m_i424 = 0;
    m_fromControlPoint      = true;    // the phase selector - see the header
    m_orderPosesInitialised = false;
}

AIStrategicArea::AIStrategicArea(const std::string& name, const Vec2& topLeft, const Vec2& centre,
                                 float height, float radius)
    : AIStrategicObject(radius, height)
    , m_list268()
    , m_centre(topLeft)
    , m_mapData(checkPointer(new MapData[IAIPathfinding::instance->getNumberOfMaps()]))
    , m_neighbours()
    , m_neighbouringObjects()
    , m_name(name)
    , m_objectTypeFlags(0)
    , m_controlPointId(static_cast<unsigned int>(-1))
    , m_landingZones()
    , m_landingZoneUsers(0)
    , m_b358(false)
{
    m_layer = -1;
    m_b404 = true;
    m_statusChanged = false;
    m_i416 = 0;
    m_i420 = 0;
    m_i424 = 0;
    m_fromControlPoint      = false;
    m_orderPosesInitialised = false;
    internalConstructorCall(topLeft, centre);
}

// Eleven hundred bytes: the per-map array, four lists, the name, and whatever the twenty-eight
// unwritten slots own. Not reachable before shutdown - `AIStrategicObjectManager`'s destructor is
// the only caller and it already carries a guarded seam for its area delete loop.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
AIStrategicArea::~AIStrategicArea()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicArea::~AIStrategicArea");
}

// Returns type code zero. [UNRESOLVED] Its meaning is not established as a StrategicObjectType.
int AIStrategicArea::getType() const
{
    return 0;
}

// - one load. The flag word an `.ai` script fills through
// `aiStrategicArea.addObjectTypeFlag`.
int AIStrategicArea::getObjectTypeFlags() const
{
    return m_objectTypeFlags;
}

void AIStrategicArea::addNeighbour(AIStrategicObject* neighbour)
{
    if (std::find(m_neighbours.begin(), m_neighbours.end(), neighbour) == m_neighbours.end())
        m_neighbours.push_back(neighbour);
}

void AIStrategicArea::addNeighbouringObject(AIStrategicObject* object)
{
    if (std::find(m_neighbouringObjects.begin(), m_neighbouringObjects.end(), object)
        == m_neighbouringObjects.end())
        m_neighbouringObjects.push_back(object);
}

bool AIStrategicArea::getClosestFlankingPositions(const Vec3& position, Vec3& first,
                                                  Vec3& second) const
{
    bool found = false;
    float best = 3.402823466e+38F;
    for (std::list<FlankRoute>::const_iterator it = m_flankRoutes.begin(); it != m_flankRoutes.end();
         ++it)
    {
        const float dx = it->m_first.x - position.x;
        const float dy = it->m_first.y - position.y;
        const float dz = it->m_first.z - position.z;
        const float distanceSq = dx * dx + dy * dy + dz * dz;
        if (distanceSq < best)
        {
            first = it->m_first;
            second = it->m_second;
            best = distanceSq;
            found = true;
        }
    }
    return found;
}

void AIStrategicArea::addObjectTypeFlag(int flag)
{
    m_objectTypeFlags |= 1 << (flag & 31);
}

bool AIStrategicArea::setOrderPos(int map, const Vec3& position)
{
    MapData& data = m_mapData[map];
    if (!m_orderPosesInitialised)
    {
        data.m_offset = position;
        return true;
    }

    Vec3 origin = {};
    vec2ToVec3XZ(m_position[0], origin);
    origin.y = m_f148;
    unsigned int polygon;
    unsigned int island;
    IAIPathfinding::instance->getPolygon(data.m_offset, map, polygon, island, 1);
    data.m_offset.x = position.x - origin.x;
    data.m_offset.y = position.y - origin.y;
    data.m_offset.z = position.z - origin.z;
    data.m_island = island;
    return true;
}

// 0x34f180 (Ghidra).
bool AIStrategicArea::isInside(const Vec2& point) const
{
    return insideCentreBox(point, m_centre, m_position[0]);
}

// 0x34f130 (Ghidra).
bool AIStrategicArea::isInside(const Vec3& point) const
{
    Vec2 flat;
    flat.x = 0.0f;
    flat.y = 0.0f;
    vec3ToVec2XZ(point, flat);
    return isInside(flat);
}

// Returns twice the center-to-side-corner distance along X or Z.
// Side indices are one-based and must be valid.
float AIStrategicArea::getXExtent(int side) const
{
    const float half = m_position[side - 1].x - m_centre.x;
    return half + half;
}

float AIStrategicArea::getZExtent(int side) const
{
    const float half = m_position[side - 1].y - m_centre.y;
    return half + half;
}

// The query's own result and
// its fourth (active) output are discarded; the local is seeded true only to be passed along.
// Any assigned id answers true, whatever the engine wrote; the singleton is not null-checked.
bool AIStrategicArea::getControlPointPosAndRadius(Vec3& position, float& radius)
{
    bool active = true;
    if (m_controlPointId == static_cast<unsigned int>(-1))
        return false;
    static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)
        ->getControlPointInfo(m_controlPointId, position, radius, active);
    return true;
}

// Resolves control-point position, height, and ownership after world objects are available.
void AIStrategicArea::postLoadInit()
{
    if (m_fromControlPoint)
    {
        IAIGame* const game = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame);

        Vec3 controlPointPosition;
        controlPointPosition.x = 0.0f;
        controlPointPosition.y = 0.0f;
        controlPointPosition.z = 0.0f;

        float radius = 0.0f;
        bool  active = false;

        game->getControlPointInfo(m_controlPointId, controlPointPosition, radius, active);

        Vec2 flat;
        flat.x = 0.0f;
        flat.y = 0.0f;
        vec3ToVec2XZ(controlPointPosition, flat);

        m_centre = flat;
        m_centre.x -= radius;
        m_centre.y -= radius;

        m_f148 = controlPointPosition.y;

        internalConstructorCall(m_centre, flat);
        init();
    }

    initOrderPoses();

    if (hasObjectTypeFlag(ControlPoint))
    {
        if (m_controlPointId == 0xFFFFFFFFu)
        {
            IAIGame* const game = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame);

            const int found = game->findControlPoint(m_centre, m_position[0]);
            m_controlPointId = static_cast<unsigned int>(found);

            if (found != -1)
            {
                const int owner = game->getOwnerOfControlPoint(m_controlPointId);

                if (owner == 0)
                {
                    m_status[0] = StatusMap::Neutral;
                    m_status[1] = StatusMap::Neutral;
                }
                else if (owner == 1)
                {
                    m_status[0] = StatusMap::Owned;
                    m_status[1] = StatusMap::Hostile;
                }
                else
                {
                    m_status[0] = StatusMap::Hostile;
                    m_status[1] = StatusMap::Owned;
                }
            }
        }
    }
}

std::vector<AILandingZone*>& AIStrategicArea::getLandingZones()
{
    return m_landingZones;
}

const std::vector<AILandingZone*>& AIStrategicArea::getLandingZones() const
{
    return m_landingZones;
}

// 0x34bd30 (Ghidra).
bool AIStrategicArea::isLandingZoneUser(int index)
{
    return ((m_landingZoneUsers >> index) & 1) != 0;
}

// Initializes per-vehicle-group area arrays, with at least one element in each allocation.
// [FRONTIER] The destructor must release these arrays when its implementation is completed.
void AIStrategicArea::internalConstructorCall(const Vec2& topLeft, const Vec2& centre)
{
    // unused, and deliberately so - see the note above.
    (void)topLeft;

    m_landingZones.resize(0);

    m_position[0] = centre;
    m_position[1] = centre;

    m_i320 = 25;
    m_i324 = 25;
    m_sideEnabled[0] = true;
    m_sideEnabled[1] = true;

    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

    const int vehicleGroups = settings->getNumberOfVehicleGroups();
    const int groups = vehicleGroups ? vehicleGroups : 1;

    m_groupsDirty[0] = checkPointer(new bool[groups]);
    m_groupsDirty[1] = checkPointer(new bool[groups]);

    m_sortedDistances[0] = checkPointer(new std::list<std::pair<float, AIStrategicObject*> >[groups]);
    m_sortedDistances[1] = checkPointer(new std::list<std::pair<float, AIStrategicObject*> >[groups]);

    m_routes[0] = checkPointer(new std::map<AIStrategicObject*, std::list<AIStrategicObject*> >[groups]);
    m_routes[1] = checkPointer(new std::map<AIStrategicObject*, std::list<AIStrategicObject*> >[groups]);
    m_groupDistances[0] = checkPointer(new std::map<AIStrategicObject*, float>[groups]);
    m_groupDistances[1] = checkPointer(new std::map<AIStrategicObject*, float>[groups]);

    m_distancesDirty[0] = true;
    m_distancesDirty[1] = true;

    for (int i = 0; i < groups; ++i)
    {
        m_groupsDirty[0][i] = true;
        m_groupsDirty[1][i] = true;
    }

    // Stores equal center-to-corner distances for the initial positions.
    // Each side retains its own value because setPosition can later move them independently.
    const float dx0 = m_centre.x - m_position[0].x;
    const float dy0 = m_centre.y - m_position[0].y;
    m_size[1] = std::sqrt(dx0 * dx0 + dy0 * dy0);

    const float dx1 = m_centre.x - m_position[1].x;
    const float dy1 = m_centre.y - m_position[1].y;
    m_size[2] = std::sqrt(dx1 * dx1 + dy1 * dy1);

    m_enemyTakeover[0] = false;
    m_enemyTakeover[1] = false;
    m_f408 = 1.0f;
    m_f412 = 1.0f;

    m_vehicleSearchRadius = m_size[1] + m_size[1];
}

// Raises the stored area height to the terrain height when the terrain is higher.
void AIStrategicArea::init()
{
    const IAITerrain* const terrain =
        static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);

    const float altitude = terrain->getTerrainAltitude(m_position[0].x, m_position[0].y);

    if (altitude > m_f148)
        m_f148 = altitude;

    IAIPathfinding* const pathfinding = IAIPathfinding::instance;

    for (unsigned int i = 0; i < static_cast<unsigned int>(pathfinding->getNumberOfMaps()); ++i)
    {
        if (pathfinding->isMapUsed(static_cast<int>(i)))
        {
            const Vec3 orderPosition = getOrderPos(static_cast<int>(i), 1);

            unsigned int polygon = 0;
            unsigned int island  = 0;
            pathfinding->getPolygon(orderPosition, static_cast<int>(i), polygon, island, 1);
        }
    }
}

// Resolves order positions to the navmesh and converts them to area-relative offsets once.
void AIStrategicArea::initOrderPoses()
{
    Vec3 areaPosition;
    areaPosition.x = 0.0f;
    areaPosition.y = 0.0f;
    areaPosition.z = 0.0f;

    vec2ToVec3XZ(m_position[0], areaPosition);
    areaPosition.y = m_f148;

    IAIPathfinding* const pathfinding = IAIPathfinding::instance;

    for (int i = 0; i < pathfinding->getNumberOfMaps(); ++i)
    {
        MapData& data = m_mapData[i];

        unsigned int polygon = 0;
        unsigned int island  = 0;
        pathfinding->getPolygon(data.m_offset, i, polygon, island, 1);

        data.m_offset.x = data.m_offset.x - areaPosition.x;
        data.m_offset.y = data.m_offset.y - areaPosition.y;
        data.m_offset.z = data.m_offset.z - areaPosition.z;

        data.m_island = island;
    }

    m_orderPosesInitialised = true;
}

// Returns the area position plus the selected map's delta; map -1 omits the delta.
// The second parameter is unused; [UNRESOLVED] its original purpose is unknown.
Vec3 AIStrategicArea::getOrderPos(int map, int unused) const
{
    (void)unused;

    Vec3 position;
    position.x = m_position[0].x;
    position.y = m_f148;
    position.z = m_position[0].y;

    if (map != -1)
    {
        const MapData& data = m_mapData[map];
        position.x += data.m_offset.x;
        position.y += data.m_offset.y;
        position.z += data.m_offset.z;
    }

    return position;
}

// Tests a StrategicObjectType bit position in the object flags.
bool AIStrategicArea::hasObjectTypeFlag(StrategicObjectType type) const
{
    return ((m_objectTypeFlags >> type) & 1) != 0;
}

// Remaining virtual work keeps named stops. Live strategic ticks now reach the recovered update;
// later dirty/assignment/lifetime behavior remains separately bounded.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]

// 0x34a0f0 (Ghidra).
float AIStrategicArea::updateTemperature(const Strategy* strategy, int side)
{
    const float* const modifiers = strategy->getStrategicObjectsModifiers(m_status[side - 1]);
    float& temperature = m_f164[side - 1];
    float& withoutUnits = m_f164[side + 1];

    for (int type = 0; type < 21; ++type)
    {
        if (hasObjectTypeFlag(static_cast<StrategicObjectType>(type)))
        {
            temperature = modifiers[type] * temperature;
            withoutUnits = modifiers[type] * withoutUnits;
        }
    }

    StrategicObjectType position;
    switch (m_status[side - 1])
    {
    case StatusMap::Owned:
        position = hasNeighbourWithStatus(StatusMap::Hostile, side) ? Front : Safe;
        break;
    case StatusMap::Hostile:
        position = Enemy;
        break;
    case StatusMap::Neutral:
        position = NeutralType;
        break;
    default:
        position = static_cast<StrategicObjectType>(0);
        break;
    }
    if (position != 0)
    {
        temperature = modifiers[position] * temperature;
        withoutUnits = modifiers[position] * withoutUnits;
    }

    temperature = strategy->getSpecificStrategicObjectModifier(m_index) * temperature;
    withoutUnits = strategy->getSpecificStrategicObjectModifier(m_index) * withoutUnits;
    return temperature;
}

// Parameter types
// are mangled; neither body materializes a return. Names of local aggregates are inferred.
void AIStrategicArea::update(int side, ISAI* sai, bool& changed)
{
    IAIGame* const game = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame);
    m_assignedSquadStrength[side - 1] = 0;
    std::list<AIStrategicObject*> objects;
    m_assignedSquadStrength[side - 1] = SquadManager::m_instance->getSquadStrengths(
        side, m_index, static_cast<StrategicStrengthType>(m_attack[side - 1] ? 0 : 1));

    Vec2 upper;
    const double dx = static_cast<double>(m_position[side - 1].x) - m_centre.x;
#if defined(_MSC_VER)
    // [PLATFORM] PE spills only the Y difference before adding position again.
    const float dy = m_position[side - 1].y - m_centre.y;
#else
    const double dy = static_cast<double>(m_position[side - 1].y) - m_centre.y;
#endif
    upper.x = static_cast<float>(dx + m_position[side - 1].x);
    upper.y = static_cast<float>(static_cast<double>(dy) + m_position[side - 1].y);

    float total = 0.0f, withoutUnits = 0.0f;
    float friendly = 0.0f, enemy = 0.0f, ratio = 0.0f;
    int friendlyUnits = 0, enemyUnits = 0;
    SampleHelper battleSamples(6), strategicSamples(2);
    float battle[6], strategic[2];
    int battleCounts[6], strategicCounts[2];
    int secondary = -1;
    {
        std::vector<unsigned int> handles;
        battleSamples.clear(battle, battleCounts);
        strategicSamples.clear(strategic, strategicCounts);
        m_resourceNeed[side - 1] = 0;
        IAIInformationEnvironment* const environment =
            static_cast<IAIInformationEnvironment*>(AISingletonServer::m_iaiInformationEnvironment);
        environment->getObjectsWithinRectangle(handles, m_centre, upper, 0, true, nullptr);
        environment->getObjectsWithinRectangle(handles, m_centre, upper, 0, false, nullptr);
        environment->getObjectsWithinRectangle(handles, m_centre, upper, side, true, nullptr);
        environment->getObjectsWithinRectangle(handles, m_centre, upper, side, false, nullptr);

        // The two original record arms do the same sampling, except the root-only flag
        // suppression. Keep selected unit/physical/array receivers across their callbacks.
        const auto sampleInformation = [&](Information* information, bool root)
        {
            const double returnedSecurity = information->getSecurity();
            const float security = static_cast<float>(returnedSecurity);
#if defined(_MSC_VER)
            const double weighted = static_cast<double>(security) * information->getStrength();
#else
            const double weighted = returnedSecurity * information->getStrength();
#endif
            IPIUnit* const unit = information->getUnit();
            total = static_cast<float>(static_cast<double>(total) + weighted);
            if (!unit)
            {
                withoutUnits = static_cast<float>(weighted + withoutUnits);
                return;
            }

#if defined(_MSC_VER)
            // [PLATFORM] PE sums all six floats wide, then stores once. ELF stores each sum.
            const float* const values = unit->getBattleStrengths();
            const double sum = ((((static_cast<double>(values[0]) + values[1])
                + values[2]) + values[3]) + values[4]) + values[5];
            const float strength = static_cast<float>(sum);
#else
            float strength = 0.0f;
            for (int i = 0; i < 6; ++i)
                strength = unit->getBattleStrengths()[i] + strength;
#endif
            if (!unit->isOccupied())
                return;
            if (sai->isFriend(information->getSide()))
            {
                if (!root || (information->getObjectTypeFlags() & 0x10u) == 0)
                    ++friendlyUnits;
                friendly = static_cast<float>(static_cast<double>(security) * strength + friendly);
            }
            else if (sai->isEnemy(information->getSide()))
            {
                if (!root || (information->getObjectTypeFlags() & 0x10u) == 0)
                    ++enemyUnits;
                enemy = static_cast<float>(static_cast<double>(security) * strength + enemy);
                IPIPhysical* const physical = information->getPhysical();
#if defined(_MSC_VER)
                float* const input = unit->getBattleStrengths();
#endif
                const int type = physical->getStrengthType();
#if !defined(_MSC_VER)
                float* const input = unit->getBattleStrengths();
#endif
                battleSamples.addSample(input, type, battle, battleCounts);
                strategicSamples.addSample(unit->getStrategicStrengths(),
                    m_status[side - 1] != 0, strategic, strategicCounts);
                m_resourceNeed[side - 1] += static_cast<int>(
                    unit->getStrategicStrengths()[m_status[side - 1] == 0]);
            }
        };

#if defined(_MSC_VER)
        InformationManager* manager = handles.empty() ? nullptr : informationManager;
#endif
        auto end = handles.end();
        for (auto it = handles.begin(); it != end; ++it)
        {
#if !defined(_MSC_VER)
            InformationManager* manager = informationManager;
            Information* const root = manager->getInformation(*it);
#else
            // PE tests the handle's index before touching the captured manager's pool.
            Information* const root = (*it & 0xffffu) ? manager->getInformation(*it) : nullptr;
#endif
            if (root)
            {
                sampleInformation(root, true);
#if !defined(_MSC_VER)
                manager = informationManager;
#endif
                unsigned int handle = root->getFirstSecondaryObject(secondary, side);
                for (;;)
                {
#if defined(_MSC_VER)
                    // [PLATFORM] PE captures after secondary callbacks and carries this
                    // manager to the next root; ELF captures before each callback.
                    manager = informationManager;
                    Information* const current = (handle & 0xffffu)
                        ? manager->getInformation(handle) : nullptr;
#else
                    Information* const current = manager->getInformation(handle);
#endif
                    if (!current)
                        break;
                    sampleInformation(current, false);
#if !defined(_MSC_VER)
                    manager = informationManager;
#endif
                    handle = root->getSecondaryObject(secondary, side);
                }
                end = handles.end();
            }
#if defined(_MSC_VER)
            end = handles.end();
#endif
        }
        const double denominator = static_cast<double>(friendly) + enemy;
        if (denominator > 0.0)
            ratio = static_cast<float>(friendly / denominator);
    } // The query vector dies before the old-status capture and output stores.

    const StatusMap::Status oldStatus = m_status[side - 1];
    m_f164[side - 1] = total + m_size[0];
    m_f164[side + 1] = withoutUnits + m_size[0];
    m_friendlyRatio[side - 1] = ratio;
    m_enemyBattleStrength[side - 1] = enemy;
    m_friendlyBattleStrength[side - 1] = friendly;
    if (m_controlPointId != 0xffffffffu)
    {
        const int owner = game->getOwnerOfControlPoint(m_controlPointId);
        m_status[side - 1] = owner == side ? StatusMap::Owned
            : owner == 0 ? StatusMap::Neutral : StatusMap::Hostile;
    }
    else if (friendlyUnits > 0 || enemyUnits > 0)
    {
        if (m_sideEnabled[side - 1] && friendlyUnits > 0 && enemyUnits == 0)
            m_status[side - 1] = StatusMap::Owned;
        else if (m_sideEnabled[side == 1] && enemyUnits > 0 && friendlyUnits == 0)
            m_status[side - 1] = StatusMap::Hostile;
    }

    // Reread the ID/status after the owner callback. Owned->Neutral skips both the
    // takeover reset and getter; all other CP transitions query even when not owned.
    if (m_controlPointId != 0xffffffffu)
    {
        if (oldStatus == StatusMap::Owned && m_status[side - 1] == StatusMap::Neutral)
            changed = true;
        else
        {
            m_enemyTakeover[side - 1] = false;
            const double counter = getControlPointTakeOverCounter();
            if (m_status[side - 1] == StatusMap::Owned && counter < 0.9f)
            {
                m_enemyTakeover[side - 1] = true;
                m_status[side - 1] = StatusMap::Neutral;
            }
            changed = oldStatus == StatusMap::Owned
                ? m_status[side - 1] != StatusMap::Owned : m_status[side - 1] == StatusMap::Owned;
        }
    }
    else
    {
        changed = false;
        if ((oldStatus == StatusMap::Owned && m_status[side - 1] == StatusMap::Hostile)
            || (oldStatus == StatusMap::Hostile && m_status[side - 1] == StatusMap::Owned))
            changed = true;
    }
    m_statusChanged = changed;
    m_statistics[side - 1]->sample(battle, battleCounts);
    m_statistics[side + 1]->sample(strategic, strategicCounts);

    const float count = static_cast<float>(m_resourceNeed[side - 1]);
    const float floored = count > 1.0f ? count : 1.0f;
    const bool owned = m_status[side - 1] == StatusMap::Owned;
    const float input = owned ? m_needCurveInput[side - 1] : m_friendlyRatio[side - 1];
#if defined(_MSC_VER)
    double need = (2.0 - static_cast<double>(SCurve::calculate(input))) * floored;
    if (!owned)
        need *= 1.25;
    m_resourceNeed[side - 1] = static_cast<int>(need);
#else
    // [PLATFORM] ELF scales the base first and narrows the final product before cvttss2si.
    const float base = owned ? floored : floored * 1.25f;
    const float need = static_cast<float>((2.0 - static_cast<double>(SCurve::calculate(input))) * base);
    m_resourceNeed[side - 1] = static_cast<int>(need);
#endif
}

int AIStrategicArea::getChangedAttackDefence(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicArea::getChangedAttackDefence");
}

// Full pair : fresh game/id after the flag callback.
float AIStrategicArea::getControlPointTakeOverCounter()
{
    if (hasObjectTypeFlag(ControlPoint))
        return static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->
            getControlPointTakeOverCounter(m_controlPointId);
    return 1.0f;
}

// The flag update raises while the side's owned area is being captured. The side is unchecked.
bool AIStrategicArea::isEnemyTakingOverBase(int side)
{
    return m_enemyTakeover[side - 1];
}

// 0x348c10 (Ghidra).
float AIStrategicArea::calculateDefenseNeed(int side)
{
    if (m_status[side - 1] != StatusMap::Owned)
        return 0.0f;
    const float base = static_cast<float>(
        (static_cast<double>(m_enemyBattleStrength[side - 1]) + m_f164[side - 1]) *
        (2.0 - static_cast<double>(SCurve::calculate(m_needCurveInput[side - 1]))));
    if (hasNeighbourWithStatus(StatusMap::Hostile, side))
        return 1.5f * base;
    if (hasNeighbourWithStatus(StatusMap::Neutral, side))
        return 1.25f * base;
    return base;
}

// 0x348ce0 (Ghidra), from the disassembly; the decompiler loses the x87 result. The area's
// side-1 position is used whatever the side.
float AIStrategicArea::calculateAttackValue(int side)
{
    float value = 0.0f;
    if (m_status[side - 1] != StatusMap::Owned && hasNeighbourWithStatus(StatusMap::Owned, side))
    {
        value = 1.0f + m_needCurveInput[side - 1];
        value = value * m_f164[side - 1];
        if (!hasNeighbourWithStatus(StatusMap::Hostile, side))
            value = (hasNeighbourWithStatus(StatusMap::Neutral, side) ? 1.5f : 1.25f) * value;
        const IAITerrain* const terrain =
            static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
        Vec3 position;
        position.x = m_position[0].x;
        position.y = terrain->getTerrainAltitude(m_position[0].x, m_position[0].y);
        position.z = m_position[0].y;
        const float distance = SquadManager::m_instance->getDistanceToClosestSquad(position, side);
        if (1.0f < distance && distance < 100000.0f)
            value = static_cast<float>(1.0 / distance * 100.0 * value);
        else
            value = 0.0f;
    }
    return value - calculateAttackCost(side);
}

// 0x348e60 (Ghidra).
float AIStrategicArea::calculateAttackCost(int side)
{
    const EnemyStatistics* const statistics = m_statistics[side + 1];
    const float strength = 1 < statistics->m_size ? statistics->m_strength[1] : -1.0f;
    return static_cast<float>((1.0 + m_friendlyRatio[side - 1]) *
                              (1.5 - 0.5 * m_needCurveInput[side - 1]) * strength);
}

bool AIStrategicArea::isNeighbour(AIStrategicObject* object) const
{
    return std::find(m_neighbours.begin(), m_neighbours.end(), object) != m_neighbours.end();
}

// Native vector scan, captured end, no null/side guards; first matching status wins.
bool AIStrategicArea::hasNeighbourWithStatus(StatusMap::Status status, int side) const
{
    const auto end = m_neighbours.end();
    for (auto it = m_neighbours.begin(); it != end; ++it)
    {
        if ((*it)->getStatus(side) == status)
            return true;
    }
    return false;
}

bool AIStrategicArea::isNeighbouringObject(AIStrategicObject* object) const
{
    return std::find(m_neighbouringObjects.begin(), m_neighbouringObjects.end(), object)
        != m_neighbouringObjects.end();
}

// Applies the same scan to the second neighbor vector.
bool AIStrategicArea::hasNeighbouringObjectWithStatus(StatusMap::Status status, int side) const
{
    const auto end = m_neighbouringObjects.end();
    for (auto it = m_neighbouringObjects.begin(); it != end; ++it)
    {
        if ((*it)->getStatus(side) == status)
            return true;
    }
    return false;
}

bool AIStrategicArea::hasObjectTypeFlags(int flags) const
{
    return (m_objectTypeFlags & flags) != 0;
}

int AIStrategicArea::getTacticalStatus(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicArea::getTacticalStatus");
}

// Whether its control point can change hands now; areas without one never can.
bool AIStrategicArea::isAreaTakeable()
{
    if (m_controlPointId == static_cast<unsigned int>(-1))
        return false;
    Vec3 position = {};
    float radius;
    bool active;
    static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)
        ->getControlPointInfo(m_controlPointId, position, radius, active);
    return active;
}

// 0x34a380 (Ghidra).
bool AIStrategicArea::collectSquadResources(int side, ISAI* sai)
{
    if (m_assignedSquadStrength[side - 1] >= m_resourceNeed[side - 1])
        return true;
    Vec3 position;
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;
    vec2ToVec3XZ(getPosition(side), position);
    position.y = m_f148;
    validateDistances(side);

    int strength = 0;
    while (m_assignedSquadStrength[side - 1] <= m_resourceNeed[side - 1])
    {
        SquadCoordinator* const squad = SquadManager::m_instance->getClosestAvailableSquad(
            this, position, side, m_attack[side - 1], strength);
        if (!squad)
            return m_assignedSquadStrength[side - 1] >= m_resourceNeed[side - 1];
        const StatusMap::Status status = m_status[side - 1];
        if (status == StatusMap::Owned || (m_enemyTakeover[side - 1] && status == StatusMap::Neutral))
            sai->getCommanderInterface()->giveDefendOrder(position, squad);
        else
            sai->getCommanderInterface()->giveAttackOrder(position, squad);
        m_assignedSquadStrength[side - 1] += strength;
    }
    return true;
}

// 0x34a530 (Ghidra).
int AIStrategicArea::collectAdditionalSquadResources(int count, int side, ISAI* sai)
{
    Vec3 position;
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;
    vec2ToVec3XZ(getPosition(side), position);
    position.y = m_f148;
    validateDistances(side);

    int collected = 0;
    int strength = 0;
    for (; collected < count; ++collected)
    {
        SquadCoordinator* const squad = SquadManager::m_instance->getClosestAvailableSquad(
            this, position, side, m_attack[side - 1], strength);
        if (!squad)
            return collected;
        const StatusMap::Status status = m_status[side - 1];
        if (status == StatusMap::Owned || (m_enemyTakeover[side - 1] && status == StatusMap::Neutral))
            sai->getCommanderInterface()->giveDefendOrder(position, squad);
        else
            sai->getCommanderInterface()->giveAttackOrder(position, squad);
        m_assignedSquadStrength[side - 1] += strength;
    }
    return collected;
}

// 0x34bd50 (Ghidra). The stale flag is left set, as shipped.
void AIStrategicArea::validateDistances(int side)
{
    if (!m_distancesDirty[side - 1])
        return;
    m_distances[side - 1].clear();
    const int groups =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNumberOfVehicleGroups();
    if (groups == 0)
    {
        validateDistances(side, 0);
        insertUnitDistances(side, 0);
        return;
    }
    for (int group = 0; group < groups; ++group)
    {
        validateDistances(side, group);
        insertUnitDistances(side, group);
    }
}

// 0x34be30 (Ghidra).
void AIStrategicArea::validateDistances(int side, int group)
{
    if (!m_groupsDirty[side - 1][group])
        return;
    m_groupsDirty[side - 1][group] = false;
    m_sortedDistances[side - 1][group].clear();
    m_routes[side - 1][group].clear();
    m_groupDistances[side - 1][group].clear();
    getDistances(side, m_sortedDistances[side - 1][group], m_routes[side - 1][group],
                 m_groupDistances[side - 1][group], 1 << group);
}

namespace {

// One search step: reach object from from, at the total distance.
struct ObjectSearchInfo
{
    AIStrategicObject* m_object;
    AIStrategicObject* m_from;
    float m_distance;
};

} // anonymous namespace

// 0x34aac0 (Ghidra). Steps are expanded first in, first out; a step that does not beat the best
// distance known for its object is dropped.
void AIStrategicArea::getDistances(int side, std::list<std::pair<float, AIStrategicObject*> >& sorted,
                                   std::map<AIStrategicObject*, std::list<AIStrategicObject*> >& routes,
                                   std::map<AIStrategicObject*, float>& distances, int groupMask)
{
    if (!allowsSomeVehicleGroups(groupMask))
        return;

    std::list<ObjectSearchInfo> search;
    const ObjectSearchInfo start = { this, this, 0.0f };
    search.push_back(start);
    routes.insert(std::make_pair(static_cast<AIStrategicObject*>(this),
                                 std::list<AIStrategicObject*>()));

    while (!search.empty())
    {
        const ObjectSearchInfo step = search.front();
        const std::map<AIStrategicObject*, float>::iterator known = distances.find(step.m_object);
        if ((known == distances.end() || step.m_distance < known->second) &&
            step.m_object->allowsSomeVehicleGroups(groupMask))
        {
            // Only areas carry neighbours, and only areas reach here.
            const std::vector<AIStrategicObject*> neighbours =
                static_cast<AIStrategicArea*>(step.m_object)->m_neighbours;
            for (std::vector<AIStrategicObject*>::const_iterator it = neighbours.begin();
                 it != neighbours.end(); ++it)
            {
                const Vec2& from = step.m_object->getPosition(side);
                const Vec2& to = (*it)->getPosition(side);
                const float dx = to.x - from.x;
                const float dz = to.y - from.y;
                float length = std::sqrt(dz * dz + dx * dx);
                if ((*it)->getStatus(side) == StatusMap::Neutral)
                    length *= 1.1f;
                else if ((*it)->getStatus(side) == StatusMap::Hostile)
                    length *= 5.0f;
                const ObjectSearchInfo next = { *it, step.m_object, length + step.m_distance };
                search.push_back(next);
            }

            if (known == distances.end())
                distances.insert(std::make_pair(step.m_object, step.m_distance));
            else
                known->second = step.m_distance;

            const std::map<AIStrategicObject*, std::list<AIStrategicObject*> >::iterator fromRoute =
                routes.find(step.m_from);
            std::map<AIStrategicObject*, std::list<AIStrategicObject*> >::iterator route =
                routes.find(step.m_object);
            if (route == routes.end())
                route = routes.insert(std::make_pair(step.m_object, fromRoute->second)).first;
            else
                route->second = fromRoute->second;
            route->second.push_front(step.m_object);
        }
        search.pop_front();
    }

    for (std::map<AIStrategicObject*, float>::const_iterator it = distances.begin();
         it != distances.end(); ++it)
        sorted.push_back(std::make_pair(it->second, it->first));
    sorted.sort();
}

// 0x34bf90 (Ghidra). The shipped scan reads the end node's distance before testing for the end;
// testing the end first gives the same result.
void AIStrategicArea::insertUnitDistances(int side, int group)
{
    const int mask = 1 << group;
    std::list<AreaDistanceInfo>& merged = m_distances[side - 1];
    const std::list<std::pair<float, AIStrategicObject*> >& sorted =
        m_sortedDistances[side - 1][group];
    std::list<AreaDistanceInfo>::iterator into = merged.begin();
    std::list<std::pair<float, AIStrategicObject*> >::const_iterator it = sorted.begin();
    for (; it != sorted.end(); ++it)
    {
        while (into != merged.end() && into->m_distance < it->first)
            ++into;
        if (into == merged.end())
            break;
        if (into->m_distance != it->first)
        {
            const AreaDistanceInfo info = { mask, it->second, it->first };
            merged.insert(into, info);
        }
        else
        {
            into->m_groupMask |= mask;
        }
    }
    for (; it != sorted.end(); ++it)
    {
        const AreaDistanceInfo info = { mask, it->second, it->first };
        merged.push_back(info);
    }
}

int AIStrategicArea::releaseSurplus(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicArea::releaseSurplus");
}

// Both delegations stay virtual;
// MSVC groups the Vec2/Vec3 overload slots in the opposite order to the ELF.
float AIStrategicArea::gravity(const Vec3& position, int side)
{
    Vec2 flat;
    flat.x = 0.0f;
    flat.y = 0.0f;
    vec3ToVec2XZ(position, flat);
    return gravity(flat, side);
}

// The position is read directly
// using side-1. x87 keeps the arithmetic wide until the float call argument.
float AIStrategicArea::gravity(const Vec2& position, int side)
{
    const double dx = static_cast<double>(position.x) - m_position[side - 1].x;
    const double dz = static_cast<double>(position.y) - m_position[side - 1].y;
    const double squaredDistance = dx * dx + dz * dz;
    double distance = std::sqrt(squaredDistance);
#if !defined(_MSC_VER)
    // [PLATFORM] ELF retries an unordered fsqrt through sqrtf, with its input
    // narrowed to float. PE uses the hardware result without this CRT fallback.
    if (distance != distance)
        distance = std::sqrt(static_cast<float>(squaredDistance));
#endif
    return gravity(static_cast<float>(distance), side);
}

// 0x34efa0 (Ghidra). As shipped it orders nothing: a mobile unit is simply retained.
bool AIStrategicArea::retainBot(int, int botId, ISAI*)
{
    Bot* const bot = IBotManager::instance->getBot(botId);
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    return equipment && equipment->getMobile() && equipment->getUnit();
}

// Marks the side's distances and every vehicle group stale. Without groups only the one flag
// the constructor allocated is set. The side is not checked.
void AIStrategicArea::setAsDirty(int side)
{
    m_distancesDirty[side - 1] = true;
    const int groups = static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings)
        ->getNumberOfVehicleGroups();
    if (groups == 0)
    {
        m_groupsDirty[side - 1][0] = true;
    }
    else
    {
        for (int i = 0; i < groups; ++i)
            m_groupsDirty[side - 1][i] = true;
    }
}

// 0x34c340 (Ghidra).
int AIStrategicArea::orderBot(int side, const std::list<AIStrategicObject*>& route, int botId, ISAI* sai)
{
    Bot* const bot = IBotManager::instance->getBot(botId);
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    if ((equipment->getObjectTypeFlags() & 0x10) == 0)
        orderNormalBot(side, route, botId, sai);
    else
        orderAirBot(side, route, botId, sai);
    return 0;
}

// The size uses side, unlike the
// position's side-1 index above. Unordered distance takes the division arm.
float AIStrategicArea::gravity(float distance, int side)
{
    if (distance <= 1.0f)
        return m_size[side];
    const double wideDistance = distance;
    return static_cast<float>(m_size[side] / (wideDistance * wideDistance));
}

// 0x34a2f0 (Ghidra).
bool AIStrategicArea::getRouteList(int side, AIStrategicObject* from, std::list<AIStrategicObject*>& route) const
{
    const std::map<AIStrategicObject*, std::list<AIStrategicObject*> >& routes = *m_routes[side - 1];
    const std::map<AIStrategicObject*, std::list<AIStrategicObject*> >::const_iterator it = routes.find(from);
    if (it == routes.end())
        return false;
    route = it->second;
    return true;
}

// [FRONTIER] 0x34c430 (Ghidra), 1.3 KB.
void AIStrategicArea::orderAirBot(int, const std::list<AIStrategicObject*>&, int, ISAI*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicArea::orderAirBot");
}

// 0x349ab0 (Ghidra). With the control point active, within 0.8 of its radius; otherwise within twice
// the distance from the area's centre to its corner. Twenty random tries around the point, then the
// object's order position.
void AIStrategicArea::getControlpointPos(AIStrategicObject* object, int side, int map, int type,
                                         const Vec3& position, Vec3& out)
{
    IAIPathfinding* const pathfinding = IAIPathfinding::instance;
    bool active = true;
    float radius = 1.0f;
    Vec3 point;
    point.x = 0.0f;
    point.y = 0.0f;
    point.z = 0.0f;
    static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getControlPointInfo(m_controlPointId, point, radius, active);
    float reach;
    float fraction;
    float within;
    if (!active)
    {
        const Vec2& flat = getPosition(side);
        const float x = flat.x - m_centre.x;
        const float y = flat.y - m_centre.y;
        const float length = std::sqrt(y * y + x * x);
        reach = length + length;
        fraction = 1.0f;
        within = reach;
    }
    else
    {
        reach = radius;
        fraction = 0.8f;
        within = 0.8f * radius;
    }
    const float dx = point.x - position.x;
    const float dy = point.y - position.y;
    const float dz = point.z - position.z;
    const unsigned int island = m_mapData[map].m_island;
    if (std::sqrt(dx * dx + dy * dy + dz * dz) < within && pathfinding->isValidPosition(map, type, position, island, true))
    {
        out = position;
        return;
    }
    out.x = point.x;
    out.y = m_f148;
    out.z = point.z;
    object->randomizeControlpointPos(fraction, reach, out);
    for (int i = 0; i < 20; ++i)
    {
        if (pathfinding->isValidPosition(map, type, out, island, true))
            return;
        out.x = point.x;
        out.y = m_f148;
        out.z = point.z;
        object->randomizeControlpointPos(fraction, reach, out);
    }
    out = object->getOrderPos(map, side);
}

namespace {

// In front of the bot on the way to the goal and closer to it than the bot.
bool onTheWay(const Vec3& point, const Vec3& position, const Vec3& goal, float distanceSqr)
{
    const float x = point.x - position.x;
    const float y = point.y - position.y;
    const float z = point.z - position.z;
    if (!(0.0f < y * (goal.y - position.y) + x * (goal.x - position.x) + z * (goal.z - position.z)))
        return false;
    const float gx = point.x - goal.x;
    const float gy = point.y - goal.y;
    const float gz = point.z - goal.z;
    return gz * gz + gx * gx + gy * gy < distanceSqr;
}

// A place in the object: a valid random point in 80 % of it, in twenty tries, or its order position.
Vec3 pickPlace(AIStrategicObject* object, int side, int map, int type, unsigned int island)
{
    Vec3 pick;
    pick.x = 0.0f;
    pick.y = 0.0f;
    pick.z = 0.0f;
    object->randomizePos(side, 0.8f, pick);
    for (int i = 0; i < 20; ++i)
    {
        if (IAIPathfinding::instance->isValidPosition(map, type, pick, island, true))
            return pick;
        object->randomizePos(side, 0.8f, pick);
    }
    return object->getOrderPos(map, side);
}

// The waypoint of the bot's route between two objects: the one it already has if that still leads
// on, otherwise a random one in up to three tries. No waypoints forget the assigned areas.
bool pickWaypoint(BotMain* bot, const std::vector<SAWayPoint*>& waypoints, const Vec3& position, const Vec3& goal,
                  float distanceSqr, int map, bool frontOnly, bool firstLeg, std::list<std::pair<Vec3, int> >& points)
{
    bool picked = false;
    if (firstLeg && bot->hasSameAssignedAreas(map))
    {
        const Vec3 current = bot->getCurrentWaypoint();
        if (onTheWay(current, position, goal, distanceSqr))
        {
            bot->setCurrentWaypoint(current, map);
            points.push_back(std::make_pair(bot->getCurrentWaypoint(), -1));
            picked = true;
        }
    }
    if (firstLeg)
        bot->copyAssignedAreas();
    if (waypoints.empty())
    {
        bot->resetAssignedAreas();
        return picked;
    }
    if (picked)
        return true;
    for (int tries = 0; tries < 3; ++tries)
    {
        const int index = static_cast<int>(AIRandom::m_instance->nextFloat() * static_cast<float>(waypoints.size()));
        const SAWayPoint* const waypoint = waypoints[index];
        const Vec3& at = waypoint->getPosition();
        if (frontOnly)
        {
            const float x = at.x - position.x;
            const float y = at.y - position.y;
            const float z = at.z - position.z;
            if (!(0.0f < x * (goal.x - position.x) + y * (goal.y - position.y) + z * (goal.z - position.z)))
                continue;
        }
        const float gx = at.x - goal.x;
        const float gy = at.y - goal.y;
        const float gz = at.z - goal.z;
        if (waypoint->getPolygon() == 0xFFFFFFFFu || distanceSqr <= gx * gx + gy * gy + gz * gz)
            continue;
        points.push_back(std::make_pair(at, -1));
        if (firstLeg)
            bot->setCurrentWaypoint(at, map);
        return true;
    }
    if (firstLeg)
        bot->resetAssignedAreas();
    return false;
}

// With one object the bot either keeps the waypoint it has, while that leads on, or draws one that
// does; the assigned areas are copied last.
void pickOnlyWaypoint(BotMain* bot, const std::vector<SAWayPoint*>& waypoints, const Vec3& position, const Vec3& goal,
                      float distanceSqr, int map, std::list<std::pair<Vec3, int> >& points)
{
    if (bot->hasSameAssignedAreas(map))
    {
        const Vec3 current = bot->getCurrentWaypoint();
        if (onTheWay(current, position, goal, distanceSqr))
        {
            bot->setCurrentWaypoint(current, map);
            points.push_back(std::make_pair(bot->getCurrentWaypoint(), -1));
        }
        bot->copyAssignedAreas();
        return;
    }
    bool picked = false;
    for (int tries = 0; tries < 3 && !waypoints.empty(); ++tries)
    {
        const int index = static_cast<int>(AIRandom::m_instance->nextFloat() * static_cast<float>(waypoints.size()));
        const SAWayPoint* const waypoint = waypoints[index];
        if (onTheWay(waypoint->getPosition(), position, goal, distanceSqr) && waypoint->getPolygon() != 0xFFFFFFFFu)
        {
            points.push_back(std::make_pair(waypoint->getPosition(), -1));
            bot->setCurrentWaypoint(waypoint->getPosition(), map);
            picked = true;
            break;
        }
    }
    if (!picked)
        bot->resetAssignedAreas();
    bot->copyAssignedAreas();
}

} // namespace

// 0x34c940 (Ghidra), read in the asm for the calls the decompiler dropped. The route's objects in
// order, each reached through a waypoint of the leg to it and a place in it; the last object's
// radius ends the move, 0 at a control point. A control point of this area, not held by the side,
// is the place when the area has flag 0x10. An empty route: this area, the bot staying where it is
// when it is already inside and on the mesh. A route equal to the bot's stays.
void AIStrategicArea::orderNormalBot(int side, const std::list<AIStrategicObject*>& route, int botId, ISAI* sai)
{
    BotMain* const bot = static_cast<BotMain*>(IBotManager::instance->getBot(botId));
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    IAIPathfinding* const pathfinding = IAIPathfinding::instance;
    const float reach = equipment->getPhysical()->getRadius();
    const float wide = reach + reach;
    const int map = equipment->getMobile()->getVehicleNumber();
    const int type = equipment->getUnit()->getEquipmentType();
    const Vec3& position = equipment->getPosition();
    Vec2 here;
    here.x = 0.0f;
    here.y = 0.0f;
    vec3ToVec2XZ(position, here);
    const unsigned int island = m_mapData[map].m_island;
    const bool attackPoint = hasObjectTypeFlags(0x10) && static_cast<int>(m_controlPointId) != -1 && getStatus(side) != 0;
    std::list<std::pair<Vec3, int> > points;
    float radius = 0.0f;
    if (route.empty())
    {
        if (attackPoint)
        {
            Vec3 place;
            getControlpointPos(this, side, map, type, position, place);
            points.push_back(std::make_pair(place, m_index));
        }
        else
        {
            Vec3 place;
            if (isInside(here) && pathfinding->isValidPosition(map, type, equipment->getPosition(), island, true))
                place = equipment->getPosition();
            else
                place = pickPlace(this, side, map, type, island);
            points.push_back(std::make_pair(place, m_index));
            radius = 0.25f * m_size[side] + wide;
        }
    }
    else
    {
        int from = sai->getBotCurrentPos(botId);
        const bool single = route.size() == 1;
        bool firstLeg = true;
        bool legDone = false;
        AIStrategicObjectManager* const objects = AIStrategicObjectManager::getInstance();
        for (std::list<AIStrategicObject*>::const_iterator it = route.begin(); it != route.end(); ++it)
        {
            AIStrategicObject* const object = *it;
            const int index = object->getIndex();
            if (from != -1 && from != index)
            {
                Vec3 goal;
                goal.x = 0.0f;
                goal.y = 0.0f;
                goal.z = 0.0f;
                vec2ToVec3XZ(single ? getPosition(side) : object->getPosition(side), goal);
                goal.y = single ? m_f148 : object->m_f148;
                const std::vector<SAWayPoint*>& waypoints = objects->getWaypoints(map, from, index);
                const float x = goal.x - position.x;
                const float y = goal.y - position.y;
                const float z = goal.z - position.z;
                const float distanceSqr = y * y + x * x + z * z;
                if (!legDone)
                    bot->setNewAssignedAreas(from, index);
                if (single)
                    pickOnlyWaypoint(bot, waypoints, position, goal, distanceSqr, map, points);
                else
                    pickWaypoint(bot, waypoints, position, goal, distanceSqr, map, firstLeg, !legDone, points);
                legDone = true;
            }
            firstLeg = false;
            if (object == this && attackPoint)
            {
                Vec3 place;
                getControlpointPos(this, side, map, type, position, place);
                points.push_back(std::make_pair(place, m_index));
                radius = 0.0f;
            }
            else
            {
                points.push_back(std::make_pair(pickPlace(object, side, map, type, island), single ? m_index : index));
                radius = object->m_size[side] * 0.25f + wide;
            }
            from = index;
        }
    }
    WPMoveTo* const move = checkPointer(new WPMoveTo(radius, 60.0f, 2, points));
    if (bot->getSquadMember()->isSameWaypointList(bot->getWaypoints(), move))
        delete move;
    else
        bot->setWaypoints(move);
}

} // namespace ai
} // namespace hfe
} // namespace dice
