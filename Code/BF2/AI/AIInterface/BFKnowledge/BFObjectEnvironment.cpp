// Engine object queries and information-grid updates used by the AI.
// [RETIRE-WHEN: aidll-01-adapters] Remove when the corresponding implementation is complete and validated.

#include "BFObjectEnvironment.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods
#include <cstring>
#include "../../../Scene/Object/PlayerManager.h"   // isValidPlayerId's one call
#include "../../../Core/Ids.h"                     // the four container CIDs, IID_IPlayerControlObject
#include "../../../Game/Objects/IObject.h"         // getTemplate, getAIComponent
#include "../../../Game/Objects/IObjectTemplate.h" // getClassID
#include "../../../Game/Objects/IPlayer.h"         // the per-player tests and getId
#include "../../../Game/Objects/Game/PlayerControlObject.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../../Game/IGameLogic.h"                                  // gameLogic: getPcoMap, giveDamage
#include "../../../Game/Objects/GameLogicComp/IArmorGLComp.h"          // getHitPoints / getMaxHitPoints
#include "../../../Game/Objects/GameLogicComp/ArmorGLCompTemplate.h"   // getCanBeDestroyed
#include "BFTimer.h"                                                   // IBFTimer::getGameTime
#include "BFTerrain.h"                                                 // collideLineWithWorld: intersectTerrain
#include "IAIObject.h"
#include "IAIObjectPlugIn.h"                                           // getUnit()->isOccupied
#include "../../../Scene/Object/ObjectManager.h"                       // intersectLine, intersectLineWithSpheres
#include "../../../Scene/Object/ObjectPredicator.h"
#include "../../../Physics/MaterialManager.h"                          // RayCastStopPredicate: see-through surfaces
#include "../../../Physics/Material.h"
#include "../../../Game/Objects/Game/Soldier.h"                        // getCameras
#include "../../../Game/Objects/ICameraObject.h"
#include "../../../Core/Random.h"                                       // pickRandomSensePosition: the draws
#include "../../../Physics/ICollisionMesh.h"
#include "../../../Game/Objects/ObjectLayoutAccess.h"                    // collisionMesh (+0x40)
#include <cfloat>
#include <set>

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IAIObjectEnvironment::IAIObjectEnvironment()
{
    if (!CommonSingletonServer::m_iaiObjectEnvironment)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IAIObjectEnvironment"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IAIObjectEnvironment::~IAIObjectEnvironment()
{
    if (CommonSingletonServer::m_iaiObjectEnvironment == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IAIObjectEnvironment"));
        BFSingletonServer::updateStaticMethods();
    }
}

// Zeroes the opaque vector storage, flag, and severe-damage timestamp separately.
// Leaves padding at +0x29..+0x2B untouched.
BFObjectEnvironment::BFObjectEnvironment()
{
    std::memset(m_opaque, 0, sizeof(m_opaque));  // +0x10 .. +0x27, the other two vectors
    m_byte28 = 0;                                // +0x28, a byte
    m_lastSevereDamageTime = 0.0;                // +0x2C, a double
}

BFObjectEnvironment::~BFObjectEnvironment()
{
}

} // namespace ai

namespace world { extern IBase* materialManager; }

namespace ai {
namespace {

// bf2 0x08400bc0. The object's AI component, or for a ladder, parachute, zipline or grappling
// rope container the AI component of what it carries.
IAIObject* getIAIObject(world::IObject* object)
{
    IAIObject* found = static_cast<IAIObject*>(object->getAIComponent());
    if (found)
        return found;
    const unsigned int classId = object->getTemplate()->getClassID();
    if (classId == world::CID_LadderContainerTemplate || classId == world::CID_ZiplineContainerTemplate
        || classId == world::CID_GrapplingHookRopeContainerTemplate)
    {
        world::IObject* const occupying =
            static_cast<world::PlayerControlObject*>(object)->getOccupingObject();
        if (occupying)
            found = static_cast<IAIObject*>(occupying->getAIComponent());
    }
    else if (classId == world::CID_ParachuteTemplate)
    {
        world::IObject* const occupying = static_cast<world::IPlayerControlObject*>(
            object->queryInterface(world::IID_IPlayerControlObject))->getOccupingObject();
        if (occupying)
            found = static_cast<IAIObject*>(occupying->getAIComponent());
    }
    return found;
}

// bf2 vtable 0x088d75b8. Lets the line through the objects to ignore and, when asked, through
// see-through surfaces, keeping the squared distance from the start to the nearest such surface.
class RayCastStopPredicate : public world::ObjectPredicator
{
public:
    RayCastStopPredicate(const std::vector<IAIObject*>& ignore, bool seeThrough, const Vec3& from)
        : m_ignore(&ignore), m_seeThrough(seeThrough), m_object(0), m_nearest(FLT_MAX), m_from(from)
    {
    }
    // 0x0840b310
    virtual bool predicate(world::IObject* object)
    {
        IAIObject* const ai = getIAIObject(object);
        if (ai)
        {
            for (std::vector<IAIObject*>::const_iterator it = m_ignore->begin(); it != m_ignore->end(); ++it)
                if (ai == *it)
                    return false;
        }
        m_object = object;
        return true;
    }
    virtual unsigned int getFlags() const { return 0x1000000; }
    virtual bool performCulling(world::IObject*) { return false; }
    // 0x0840b360
    virtual bool predicateSurface(int surface)
    {
        world::Material* const material =
            static_cast<world::MaterialManager*>(world::materialManager)->getMaterialPtr(surface);
        if (!m_seeThrough || !material->getIsSeeThrough())
            return true;
        const Vec3& at = m_object->getAbsolutePosition();
        const float dx = at.x - m_from.x;
        const float dy = at.y - m_from.y;
        const float dz = at.z - m_from.z;
        const float distance = dx * dx + dy * dy + dz * dz;
        if (distance < m_nearest)
            m_nearest = distance;
        return false;
    }

    /* +0x04 */ const std::vector<IAIObject*>* m_ignore;
    /* +0x08 */ bool m_seeThrough;
    /* +0x0C */ world::IObject* m_object;
    /* +0x10 */ float m_nearest;
    /* +0x14 */ Vec3 m_from;
};

// bf2 vtable 0x088d75d0. Lets the sphere test through the objects whose primary object is to be
// ignored. [UNRESOLVED] the flag at +8 is stored and not read.
class SphereExcludeFilter : public world::ObjectPredicator
{
public:
    SphereExcludeFilter(const std::vector<IAIObject*>& ignore, bool flag) : m_ignore(&ignore), m_flag(flag) {}
    // 0x0840b280
    virtual bool predicate(world::IObject* object)
    {
        IAIObject* const ai = getIAIObject(object);
        if (!ai)
            return true;
        for (std::vector<IAIObject*>::const_iterator it = m_ignore->begin(); it != m_ignore->end(); ++it)
            if (*it && ai == (*it)->getPrimaryObject())
                return false;
        return true;
    }
    virtual unsigned int getFlags() const { return 0x1000000; }
    virtual bool performCulling(world::IObject*) { return true; }

    /* +0x04 */ const std::vector<IAIObject*>* m_ignore;
    /* +0x08 */ bool m_flag;
};

// bf2 0x0840b1d0 / 0x0840b450. A part counts when it has collision at the first level of detail.
// getAllChildren walks the first-child chain of each part again, as shipped, so a deep chain is
// listed more than once.
struct CollisionPredicate
{
    bool predicate(world::IObject* object) const { return object->hasCollisionLod(0); }
};

void getAllChildren(world::IObject* object, std::vector<world::IObject*>& out, const CollisionPredicate* filter)
{
    if (!filter || filter->predicate(object))
        out.push_back(object);
    for (world::IObject* child = object->getChild(); child; child = child->getChild())
        getAllChildren(child, out, filter);
}

// A random index below count: the draw on the engine's random state, scaled and rounded
// (the x87 store runs with the chop control word, so the 0.5 rounds).
unsigned int randomIndex(unsigned int count)
{
    const float draw = static_cast<float>(static_cast<int>(((g_random.getRaw() >> 7) | 1u) + 1u)) * 5.9604645e-08f;
    return static_cast<unsigned int>(static_cast<double>(draw) * (static_cast<double>(count) - 1.0) + 0.0 + 0.5);
}

} // namespace

void BFObjectEnvironment::getWorldFrustumObjects(std::vector<IAIObject*>& out, const Frustum& frustum, AIObjectPredicate* filter) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getWorldFrustumObjects");
}

void BFObjectEnvironment::getWorldSphereObjects(std::vector<IAIObject*>& out, const Vec3& centre, float radius, AIObjectPredicate* filter) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getWorldSphereObjects");
}

void BFObjectEnvironment::getAllWorldObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllWorldObjects");
}

void BFObjectEnvironment::getAllDestroyableObjects(std::vector<DestroyableObject>& out) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllDestroyableObjects");
}

void BFObjectEnvironment::getWorldPlayerIds(std::vector<int>& out) const
{
    const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();
    for (std::list<world::IPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
        out.push_back((*it)->getId());
}

// bf2 0x08401700. The objects living players stand or ride in, as in the version with ids, but each
// vehicle once: a primary object goes in as it is found; a seat's primary object goes in at the
// end, unless its unit counts as occupied (its own occupant has put it in already).
void BFObjectEnvironment::getWorldPlayerOccupiedObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const
{
    const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();
    std::set<IAIObject*> primaries;
    for (std::list<world::IPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        world::IPlayer* const player = *it;
        if (!player || !player->getIsAlive() || player->getIsManDown() || !player->getVehicle())
            continue;
        IAIObject* aiObject = static_cast<IAIObject*>(player->getVehicle()->getAIComponent());
        if (!aiObject)
        {
            const unsigned int classId = player->getVehicle()->getTemplate()->getClassID();
            if (classId == world::CID_LadderContainerTemplate || classId == world::CID_ZiplineContainerTemplate
                || classId == world::CID_GrapplingHookRopeContainerTemplate)
            {
                world::IObject* const occupying =
                    static_cast<world::PlayerControlObject*>(player->getVehicle())->getOccupingObject();
                if (occupying)
                    aiObject = static_cast<IAIObject*>(occupying->getAIComponent());
            }
            else if (classId == world::CID_ParachuteTemplate)
            {
                world::IPlayerControlObject* const control = static_cast<world::IPlayerControlObject*>(
                    player->getVehicle()->queryInterface(world::IID_IPlayerControlObject));
                aiObject = static_cast<IAIObject*>(control->getOccupingObject()->getAIComponent());
            }
            if (!aiObject)
                continue;
        }
        if (!aiObject->isPrimary())
            primaries.insert(aiObject->getPrimaryObject());
        else if (!filter || filter->predicate(aiObject))
            out.push_back(aiObject);
    }
    for (std::set<IAIObject*>::const_iterator it = primaries.begin(); it != primaries.end(); ++it)
    {
        if ((*it)->getUnit()->isOccupied())
            continue;
        if (!filter || filter->predicate(*it))
            out.push_back(*it);
    }
}

// Collects AI objects matching the supplied predicate within the requested region.
void BFObjectEnvironment::getWorldPlayerOccupiedObjectsWithIds(std::vector<std::pair<int, IAIObject*> >& out, AIObjectPredicate* filter) const
{
    const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();

    for (std::list<world::IPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        world::IPlayer* player = *it;

        if (!player || !player->getIsAlive() || player->getIsManDown() || !player->getVehicle())
            continue;

        IAIObject* aiObject = static_cast<IAIObject*>(player->getVehicle()->getAIComponent());

        if (!aiObject)
        {
            if ((*it)->getVehicle()->getTemplate()->getClassID() == world::CID_LadderContainerTemplate)
            {
                world::IObject* occupying =
                    static_cast<world::PlayerControlObject*>(player->getVehicle())->getOccupingObject();
                if (occupying)
                    aiObject = static_cast<IAIObject*>(occupying->getAIComponent());
            }
            else if ((*it)->getVehicle()->getTemplate()->getClassID() == world::CID_ParachuteTemplate)
            {
                world::IPlayerControlObject* control = static_cast<world::IPlayerControlObject*>(
                    player->getVehicle()->queryInterface(world::IID_IPlayerControlObject));
                aiObject = static_cast<IAIObject*>(control->getOccupingObject()->getAIComponent());
            }
            else if ((*it)->getVehicle()->getTemplate()->getClassID() == world::CID_ZiplineContainerTemplate ||
                     (*it)->getVehicle()->getTemplate()->getClassID() == world::CID_GrapplingHookRopeContainerTemplate)
            {
                world::IObject* occupying =
                    static_cast<world::PlayerControlObject*>(player->getVehicle())->getOccupingObject();
                if (occupying)
                    aiObject = static_cast<IAIObject*>(occupying->getAIComponent());
            }

            if (!aiObject)
                continue;
        }

        if (filter)
        {
            if (filter->predicate(aiObject))
                out.push_back(std::make_pair((*it)->getId(), aiObject));
        }
        else
        {
            out.push_back(std::make_pair((*it)->getId(), aiObject));
        }
    }
}

IAIObject* BFObjectEnvironment::getLocalPlayerObject() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerObject");
}

bool BFObjectEnvironment::getLocalPlayerPosition(Vec3& position) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerPosition");
}

bool BFObjectEnvironment::getLocalPlayerCameraTransformation(Mat4& transform) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerCameraTransformation");
}

int BFObjectEnvironment::getLocalPlayerId() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerId");
}

const std::vector<int>& BFObjectEnvironment::getActivePlayerIds() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getActivePlayerIds");
}

// Returns whether PlayerManager resolves the id to a player.
bool BFObjectEnvironment::isValidPlayerId(int id) const
{
    return world::playerManager->getPlayerFromId(id) != 0;
}

int BFObjectEnvironment::getNextPlayerId(int id) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getNextPlayerId");
}

// bf2 0x08401dd0. Whether the terrain or an object (collideLineWithObjects) stands between the two.
bool BFObjectEnvironment::collideLineWithWorld(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool a, float b, bool c, bool& hit)
{
    Vec3 point;
    point.x = 0.0f;
    point.y = 0.0f;
    point.z = 0.0f;
    Vec3 normal = point;
    float distance;
    if (static_cast<IAITerrain*>(CommonSingletonServer::m_iaiTerrain)->intersectTerrain(from, to, point, distance, normal))
        return true;
    return collideLineWithObjects(from, to, ignore, a, b, c, hit) != 0;
}

// bf2 0x08402130. Whether an object other than the ones to ignore stands between the two; with
// spheres the cheaper sphere test (radius b) is used. hit: a see-through surface (when c asks
// for them) lay nearer than the object hit.
int BFObjectEnvironment::collideLineWithObjects(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool spheres, float radius, bool c, bool& hit)
{
    Vec3 point;
    point.x = 0.0f;
    point.y = 0.0f;
    point.z = 0.0f;
    Vec3 normal = point;
    world::IObject* object = 0;
    int part = 0;
    RayCastStopPredicate stop(ignore, c, from);
    Vec3 direction;
    direction.x = to.x - from.x;
    direction.y = to.y - from.y;
    direction.z = to.z - from.z;
    bool found;
    if (spheres)
    {
        SphereExcludeFilter exclude(ignore, c);
        found = world::objectManager->intersectLineWithSpheres(object, point, normal, part, from, direction,
                                                               &exclude, &stop, radius);
    }
    else
        found = world::objectManager->intersectLine(object, point, normal, part, from, direction, &stop,
                                                    true, false, false, true, 0);
    const float dx = point.x - from.x;
    const float dy = point.y - from.y;
    const float dz = point.z - from.z;
    hit = stop.m_nearest < dx * dx + dy * dy + dz * dz;
    return found;
}

bool BFObjectEnvironment::collideLineWithWorld(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, Vec3* hit)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::collideLineWithWorld");
}

bool BFObjectEnvironment::getMaxHeight(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, float& height)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getMaxHeight");
}

bool BFObjectEnvironment::hasIndirectLineOfFire(IWeapon* weapon, const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::hasIndirectLineOfFire");
}

// bf2 0x084024b0. A random point on an object that is not a soldier: a random part with collision,
// then a random point on a random face of its collision mesh, or with no faces a random vertex.
// position: the point in the world; normal: the same point in the object's own frame (BotSenses
// keeps it to look at again). No part with collision, or a mesh with neither: the object's
// position and a zero point. The hint is not used.
void BFObjectEnvironment::pickRandomSensePosition(Vec3& position, Vec3& normal, Vec3, IAIObject* object)
{
    world::IObject* const root = object->getObject();
    m_senseParts.clear();
    const CollisionPredicate collides;
    if (collides.predicate(root))
        m_senseParts.push_back(root);
    for (world::IObject* child = root->getChild(); child; child = child->getChild())
        getAllChildren(child, m_senseParts, &collides);
    world::ICollisionMesh* mesh = 0;
    world::IObject* part = 0;
    Mat4 toLocal;
    if (!m_senseParts.empty())
    {
        toLocal.setInverse(root->getAbsoluteTransformation());
        do
        {
            const unsigned int count = static_cast<unsigned int>(m_senseParts.size());
            if (count < 2)
            {
                part = m_senseParts[0];
                if (part->hasCollisionLod(0))
                    mesh = static_cast<world::ICollisionMesh*>(world::object_detail::LayoutAccess::collisionMesh(part));
                break;
            }
            part = m_senseParts[randomIndex(count)];
            if (part->hasCollisionLod(0))
                mesh = static_cast<world::ICollisionMesh*>(world::object_detail::LayoutAccess::collisionMesh(part));
        } while (!mesh);
    }
    // no null guard on a single part's mesh, as shipped
    const int faces = m_senseParts.empty() ? 0 : mesh->getFaceCount(0);
    const unsigned int vertices = m_senseParts.empty() ? 0 : mesh->getVertexCount(0);
    if (m_senseParts.empty() || (faces == 0 && vertices == 0))
    {
        object->getBoundingRadius();   // asked and dropped, as shipped
        normal.x = 0.0f;
        normal.y = 0.0f;
        normal.z = 0.0f;
        position = object->getPosition();
        return;
    }
    Vec3 local;
    if (faces == 0)
        local = static_cast<const Vec3*>(mesh->getVertexVector(0))[randomIndex(vertices)];
    else
    {
        const unsigned int face = randomIndex(static_cast<unsigned int>(faces));
        float u = static_cast<float>(static_cast<int>(((g_random.getRaw() >> 7) | 1u) + 1u)) * 5.9604645e-08f;
        float v = static_cast<float>(static_cast<int>(((g_random.getRaw() >> 7) | 1u) + 1u)) * 5.9604645e-08f;
        if (1.0f < u + v)
        {
            u = 1.0f - u;
            v = 1.0f - v;
        }
        const unsigned short* const corner = static_cast<const unsigned short*>(mesh->getFaceVector(0)) + face * 4;
        const Vec3* const vertex = static_cast<const Vec3*>(mesh->getVertexVector(0));
        const Vec3& a = vertex[corner[0]];
        const Vec3& b = vertex[corner[1]];
        const Vec3& c = vertex[corner[2]];
        local.x = (b.x - a.x) * u + a.x + (c.x - a.x) * v;
        local.y = (b.y - a.y) * u + a.y + (c.y - a.y) * v;
        local.z = (b.z - a.z) * u + a.z + (c.z - a.z) * v;
    }
    const Mat4& frame = part->getAbsoluteTransformation();
    Vec3 atWorld;
    atWorld.x = frame.m[8] * local.z + frame.m[0] * local.x + frame.m[4] * local.y + frame.m[12];
    atWorld.y = frame.m[1] * local.x + frame.m[5] * local.y + frame.m[9] * local.z + frame.m[13];
    atWorld.z = local.x * frame.m[2] + local.y * frame.m[6] + local.z * frame.m[10] + frame.m[14];
    position = atWorld;
    normal.x = toLocal.m[8] * atWorld.z + toLocal.m[0] * atWorld.x + toLocal.m[4] * atWorld.y + toLocal.m[12];
    normal.y = toLocal.m[1] * atWorld.x + toLocal.m[5] * atWorld.y + toLocal.m[9] * atWorld.z + toLocal.m[13];
    normal.z = atWorld.x * toLocal.m[2] + atWorld.y * toLocal.m[6] + atWorld.z * toLocal.m[10] + toLocal.m[14];
}

// bf2 0x08403010; getSoldierSensePosition is the same body. A soldier's sense position is its first
// camera: where it is and its offset. The part is not used.
bool BFObjectEnvironment::pickSoldierRandomSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int&)
{
    return soldierSensePosition(position, normal, object);
}

// bf2 0x08403100.
bool BFObjectEnvironment::getSoldierSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int)
{
    return soldierSensePosition(position, normal, object);
}

bool BFObjectEnvironment::soldierSensePosition(Vec3& position, Vec3& normal, IAIObject* object)
{
    if (!object->getObject() || !object || !object->isOfType(static_cast<InformationType>(0x15)))
        return false;
    world::IObject* const soldier = object->getObject();
    if (!const_cast<world::IObjectTemplate*>(soldier->getTemplate())->queryImplementation(world::CID_SoldierTemplate))
        return false;
    // no empty-map guard, as shipped
    world::ICameraObject* const camera = static_cast<world::Soldier*>(soldier)->getCameras()->begin()->second;
    world::IObject* eye = 0;
    if (camera)
        eye = static_cast<world::IObject*>(camera->queryInterface(world::IID_IObject));
    position = eye->getAbsolutePosition();
    normal = eye->getRelativePosition();
    return true;
}

int BFObjectEnvironment::getHeightAndNormalForObject(IAIObject* object, const Vec3& position, float& height, Vec3& normal, float& slope)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getHeightAndNormalForObject");
}

void BFObjectEnvironment::getVehicleSpawnerPositions(std::vector<Vec3>& out, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getVehicleSpawnerPositions");
}

IAIObject* BFObjectEnvironment::getObjectFromName(const std::string& name)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getObjectFromName");
}

int BFObjectEnvironment::getAllStaticCollisionMeshes(std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllStaticCollisionMeshes");
}

int BFObjectEnvironment::getAllStaticCollisionMeshesWithinBoundary(const Vec2& min, const Vec2& max, std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllStaticCollisionMeshesWithinBoundary");
}

bool BFObjectEnvironment::isAsleep(unsigned int id) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::isAsleep");
}

int BFObjectEnvironment::getTargetObjects(std::vector<TargetData>& out, int team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getTargetObjects");
}

// ELF, 10 bytes. Empty in the dedicated image.
void BFObjectEnvironment::loadOvergrowthCollision(const std::string& path) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::loadOvergrowthCollision");
}

bool BFObjectEnvironment::initAiCombatAreas() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::initAiCombatAreas");
}

bool BFObjectEnvironment::isAiPositionInsideCombatArea(const Vec3& position) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::isAiPositionInsideCombatArea");
}

int BFObjectEnvironment::getAiCombatAreaLayer() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAiCombatAreaLayer");
}

int BFObjectEnvironment::getAICombatArea(std::list<Vec2>& out) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAICombatArea");
}

// Updates the AI object environment during the simulation tick.
void BFObjectEnvironment::damageSeverelyDamagedVehicles()
{
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    if (m_lastSevereDamageTime > now)
        m_lastSevereDamageTime = 0.0;

    if (!(now - m_lastSevereDamageTime > 4.0))
        return;

    m_lastSevereDamageTime = now;

    std::map<unsigned int, world::IObject*>* const pcos = gameLogic->getPcoMap();   // +0x198

    for (std::map<unsigned int, world::IObject*>::iterator it = pcos->begin(); it != pcos->end(); ++it)
    {
        world::IObject* const object = it->second;

        if (object->getTemplate()->getClassID() == world::CID_SoldierTemplate)
            continue;

        world::IPlayerControlObject* pco = 0;
        for (world::IObject* o = object; o; o = o->getParent())
        {
            pco = static_cast<world::IPlayerControlObject*>(
                o->queryInterface(world::IID_IPlayerControlObject));
            if (pco)
                break;
        }

        if (pco->isOccupied())                                                        // +0x18
            continue;

        world::IArmorGLComp* const armor =
            static_cast<world::IArmorGLComp*>(object->getArmorComponent());          // +0x3C
        world::ArmorGLCompTemplate* const armorTemplate = static_cast<world::ArmorGLCompTemplate*>(
            object->getTemplate()->getComponentFromClassID(world::CID_ArmorGLCompTemplate));

        if (!armor || !armorTemplate)
            continue;

        const float maxHitPoints = armor->getMaxHitPoints();                         // +0x84

        if (!armorTemplate->getCanBeDestroyed())                                      // +0x154
            continue;

        if (armor->getHitPoints() / maxHitPoints < 0.5f)                              // +0x3C
        {
            Vec3 position;  position.x = 0.0f;  position.y = 0.0f; position.z = 0.0f;
            Vec3 direction; direction.x = 0.0f; direction.y = 1.0f; direction.z = 0.0f;

            gameLogic->giveDamage(object, maxHitPoints * 0.1f, 0, 0, 0, -1, 0, 0,     // +0xE4
                                  position, direction, -1, false, true, false);
        }
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
