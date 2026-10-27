// Portal registration and sector transitions. Directory is attested; filename [INFERRED].

#include "PortalManager.h"

#include "../../../Core/Ids.h"       // IID_IBase, world::IID_IPortalManager - the two ids
                                     // queryInterface compares against, both loaded from globals in
                                     // the ELF (0x8328616 / 0x8328625), not from immediates.
#include "../../../Game/Objects/IObject.h"
#include "../../../Math/Mat4.h"
#include "../ObjectManager.h"                // getObjectsFromGrid, the update's sector sweep
#include "../ObjectPredicator.h"
#include "../ObjectUtil.h"                   // getPortalMesh, geom::IPortalCollider
#include "PortalPassing.h"
#include "Sector.h"

#include <cmath>
#include <new>

namespace dice {
namespace hfe {
namespace world {

PortalEvent::PortalEvent(PortalEventType type, IObject* passingObject, IObject* portalObject,
                         unsigned int portalId)
    : m_type(type)
    , m_passingObject(passingObject)
    , m_portalObject(portalObject)
    , m_portalId(portalId)
{
}

PortalEvent::~PortalEvent()
{
}

PortalEventType PortalEvent::getType() const
{
    return m_type;
}

IObject* PortalEvent::getPassingObject() const
{
    return m_passingObject;
}

IObject* PortalEvent::getPortalObject() const
{
    return m_portalObject;
}

unsigned int PortalEvent::getPortalId() const
{
    return m_portalId;
}

namespace {

// The grid sweep's filter: objects carrying a sector component. The type name is attested.
class PortalObjectsPred : public ObjectPredicator
{
public:
    virtual bool predicate(IObject* object)
    {
        return object->queryComponent(IID_ISector, IID_ISector) != 0;
    }
    virtual unsigned int getFlags() const { return 0x1000000; }   // [UNRESOLVED] flag name
    virtual bool performCulling(IObject*) { return true; }
};

// Orders collisions farthest first, as both ports sort them. The type name is attested.
struct PortalCollisionPred
{
    bool operator()(const PortalManager::PortalCollision* a,
                    const PortalManager::PortalCollision* b) const
    {
        return a->distance > b->distance;
    }
};

// The engine's in-place normalise: a unit vector is left alone, a near-zero one becomes zero.
void normalise(Vec3& v)
{
    const float epsilon = 1.19209290e-7f;    // FLT_EPSILON
    const float lengthSquared = v.x * v.x + v.y * v.y + v.z * v.z;
    if (std::fabs(lengthSquared - 1.0f) < epsilon)
        return;
    if (std::fabs(lengthSquared) < epsilon)
    {
        v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
        return;
    }
    const float inverse = 1.0f / std::sqrt(lengthSquared);
    v.x *= inverse; v.y *= inverse; v.z *= inverse;
}

// Row-vector point transform: the translation is the matrix's last row.
Vec3 transformPoint(const Vec3& p, const Mat4& m)
{
    const Vec3 out = { p.x * m.m[0] + p.y * m.m[4] + p.z * m.m[8]  + m.m[12],
                       p.x * m.m[1] + p.y * m.m[5] + p.z * m.m[9]  + m.m[13],
                       p.x * m.m[2] + p.y * m.m[6] + p.z * m.m[10] + m.m[14] };
    return out;
}

// One centimetre past a portal hit along the direction of travel, so the next test misses it.
Vec3 stepPast(const Vec3& hit, const Vec3& direction)
{
    const float step = 0.01f;
    const Vec3 out = { direction.x * step + hit.x,
                       direction.y * step + hit.y,
                       direction.z * step + hit.z };
    return out;
}

} // namespace

// Starts with one reference and an empty callback set for each of the two portal event types.
PortalManager::PortalManager()
    : m_refCount(1)                                 // +0x04
    , m_passingObjects()                            // +0x08  allocate(0x18) at 0x8325F8D
    , m_sectorObjects()                             // +0x14  allocate(0x14) at 0x8325FC9
    , m_globalCallbacks()                           // +0x20  0x8325FF4/FB/0x8326002
    , m_showStats(false)                            // +0x2C
    , m_debugLevel(0)                               // +0x30
{
    // One callback set per event type. `operator new(0xCu)` at 0x8326022 is the 12-byte
    // std::set<IPortalEventCallback*>; its own header node is the allocate(0x14) at 0x8326036.
    for (int type = 0; type < NUM_PORTAL_EVENT_TYPES; ++type)
        m_globalCallbacks.push_back(new std::set<IPortalEventCallback*>());
}

// Deletes PassingObjectData records and callback sets, but not PortalPassing objects or callbacks.
// Callers must unregister live passing objects before manager destruction to release them.
PortalManager::~PortalManager()
{
    // The per-object data blocks. Loop 0x8326280-0x832629E; the delete itself is the hoisted block
    // at 0x83263F0. The map is emptied by its own destructor afterwards (_M_erase at 0x8326350,
    // deallocate(header, 24) at 0x8326330).
    for (std::map<IObject*, PassingObjectData*>::iterator it = m_passingObjects.begin();
         it != m_passingObjects.end(); ++it)
    {
        delete it->second;                          // 0x83263F3, bare operator delete - POD
    }

    // The two callback sets. Loop 0x83262B0-0x83262E6.
    for (std::vector<std::set<IPortalEventCallback*>*>::iterator it = m_globalCallbacks.begin();
         it != m_globalCallbacks.end(); ++it)
    {
        delete *it;                                 // 0x83262D7, set dtor + operator delete
    }

    // m_sectorObjects is emptied by its own destructor: _M_erase at 0x8326381, deallocate(header,
    // 20) at 0x8326315. The vector's buffer goes at 0x83263B2. All three are compiler output.
}

// The IBase quartet. 0x83285B0 / 0x83285C0 / 0x83285D0 / 0x8328610.

// Byte for byte the same four functions as world::BasicPhysicsSystem's - same sizes (15/11/57/32),
// same shapes. release() calls vtable +0x14, the deleting destructor (`call dword ptr [eax+14h]` at
// 0x83285F0), and returns the new count (`mov eax, ebx` at 0x8328600).
void PortalManager::addRef()
{
    ++m_refCount;
}

int PortalManager::getRef()
{
    return m_refCount;
}

int PortalManager::release()
{
    int remaining = --m_refCount;
    if (remaining == 0)
        delete this;                                // 0x83285F0, call [vtable+0x14] = D0
    return remaining;
}

// Returns IBase or IPortalManager for the matching global id, otherwise null.
// Factory registration must request IID_IPortalManager, not zero.
void* PortalManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<PortalManager*>(this);
    if (iid == IID_IPortalManager)
        return const_cast<PortalManager*>(this);
    return 0;
}

// Adds a global callback for event type 0 or 1. The caller must keep type within
// the two-entry vector; registration does not check bounds.
void PortalManager::subscribeGlobalEvents(PortalEventType type, IPortalEventCallback* callback)
{
    m_globalCallbacks[type]->insert(callback);      // 0x83271AC index, 0x83271C7 insert_unique
}

// unSubscribeGlobalEvents - 0x83271E0, 41 bytes. The mirror image; `erase(const key&)`, whose return
// (a size_type) is discarded.
void PortalManager::unSubscribeGlobalEvents(PortalEventType type, IPortalEventCallback* callback)
{
    m_globalCallbacks[type]->erase(callback);       // 0x83271EC index, 0x8327200 erase
}

// Intentionally empty statistics rendering in the dedicated image.
void PortalManager::renderStats()
{
}

// Reads or writes the statistics flag and integer debug level.
void PortalManager::setShowStats(bool on)
{
    m_showStats = on;
}

bool PortalManager::getShowStats() const
{
    return m_showStats;                             // 0x8327497, movzx
}

void PortalManager::setDebugLevel(int level)
{
    m_debugLevel = level;
}

int PortalManager::getDebugLevel() const
{
    return m_debugLevel;
}

// Gives the object a fresh portal-passing component outside every sector and tracks it from its
// current position. A repeat registration replaces the record without freeing the old one.
void PortalManager::registerPortalPassingObject(IObject* object, const Vec3& portalPassingCoordinate)
{
    PortalPassing* const passing = new PortalPassing(portalPassingCoordinate, 0);
    object->setComponent(IID_IPortalPassing, passing);
    PassingObjectData* const data = new PassingObjectData;
    data->lastPosition = object->getAbsolutePosition();
    data->portalPassing = passing;
    m_passingObjects[object] = data;
}

// Removes a portal registration by detaching the component before releasing it, then deletes data
// and erases the entry. A null record, left by a failed subscription, is not checked, as shipped.
void PortalManager::unRegisterPortalPassingObject(IObject* object)
{
    std::map<IObject*, PassingObjectData*>::iterator it =
        m_passingObjects.find(object);
    if (it == m_passingObjects.end())
        return;

    object->setComponent(IID_IPortalPassing, 0);
    PassingObjectData* data = it->second;
    data->portalPassing->release();
    ::operator delete(data);
    m_passingObjects.erase(it);
}

// Gives the object a fresh sector component and records it.
void PortalManager::registerSectorObject(IObject* object)
{
    Sector* const sector = new Sector();
    object->setComponent(IID_ISector, sector);
    m_sectorObjects.insert(object);
}

// Queries the sector with IID_ISector in both arguments, detaches it, releases it and erases membership.
// The doubled-ID interpretation remains [UNRESOLVED]; preserve the observed dispatch.
void PortalManager::unRegisterSectorObject(IObject* object)
{
    IBase* sector = object->queryComponent(IID_ISector, IID_ISector);
    if (!sector)
        return;

    object->setComponent(IID_ISector, 0);
    sector->release();
    m_sectorObjects.erase(object);
}

// An unregistered object fails, but the lookup leaves a null record for it in the registry.
bool PortalManager::subscribePortalPassingObjectEvents(PortalEventType type,
                                                       IObject* object,
                                                       IPortalEventCallback* callback)
{
    PassingObjectData* const data = m_passingObjects[object];
    if (data)
        data->portalPassing->subscribeEvents(type, callback);
    return data != 0;
}

bool PortalManager::unSubscribePortalPassingObjectEvents(PortalEventType type,
                                                         IObject* object,
                                                         IPortalEventCallback* callback)
{
    PassingObjectData* const data = m_passingObjects[object];
    if (data)
        data->portalPassing->unSubscribeEvents(type, callback);
    return data != 0;
}

// Any object with a sector component qualifies; registration with this manager is not required.
bool PortalManager::subscribeSectorEvents(PortalEventType type,
                                          IObject* object,
                                          IPortalEventCallback* callback)
{
    Sector* const sector = static_cast<Sector*>(object->queryComponent(IID_ISector, IID_ISector));
    if (sector)
        sector->subscribeEvents(type, callback);
    return sector != 0;
}

bool PortalManager::unSubscribeSectorEvents(PortalEventType type,
                                            IObject* object,
                                            IPortalEventCallback* callback)
{
    Sector* const sector = static_cast<Sector*>(object->queryComponent(IID_ISector, IID_ISector));
    if (sector)
        sector->unSubscribeEvents(type, callback);
    return sector != 0;
}

// The event type indexes the global callback sets unchecked.
void PortalManager::broadcastEvent(PortalEvent* event) const
{
    const std::set<IPortalEventCallback*>& callbacks = *m_globalCallbacks[event->getType()];
    for (std::set<IPortalEventCallback*>::const_iterator it = callbacks.begin();
         it != callbacks.end(); ++it)
    {
        (*it)->handleEvent(event);
    }
}

// Tests the segment against the object's portals and appends a hit. The results pass through
// function statics on both ports, so the test is not reentrant. The collider is not checked.
PortalManager::PortalCollision* PortalManager::checkAddPortalCollision(
        std::list<PortalCollision*>& collisions,
        IObject* object,
        const Vec3& from,
        const Vec3& to,
        bool twoSided) const
{
    static Vec3  intersectionNormal   = { 0.0f, 0.0f, 0.0f };
    static Vec3  intersectionPosition = { 0.0f, 0.0f, 0.0f };
    static int   portalId;
    static float distance;

    IBase* const mesh = getPortalMesh(object, 0);
    if (!mesh)
        return 0;

    geom::IPortalCollider* const collider =
        static_cast<geom::IPortalCollider*>(mesh->queryInterface(geom::IID_IPortalCollider));
    Sector* const sector = static_cast<Sector*>(object->queryComponent(IID_ISector, IID_ISector));
    const Mat4& transformation = object->getAbsoluteTransformation();
    if (!collider->intersect(portalId, intersectionNormal, intersectionPosition, distance,
                             transformation, from, to, twoSided))
    {
        return 0;
    }

    PortalCollision* const collision = new PortalCollision;
    collision->portalId = portalId;
    collision->intersectionNormal = intersectionNormal;
    collision->intersectionPosition = intersectionPosition;
    collision->distance = distance;
    collision->portalObject = object;
    collision->sector = sector;
    collisions.push_back(collision);
    return collision;
}

// Moves every passing object's passing point from last frame's position to this frame's, finds the
// portals it crossed and reports each crossing, farthest first. An object in a sector first tests
// that sector's portals one-sided; only a crossing there leads on to the sweep for other sectors,
// which re-tests each two-sided portal just past every hit. deltaTime is unused.
void PortalManager::update(float deltaTime)
{
    (void)deltaTime;

    std::vector<IObject*> sectorObjects;
    PortalObjectsPred sectorPredicate;
    std::list<PortalCollision*> collisions;

    for (std::map<IObject*, PassingObjectData*>::iterator it = m_passingObjects.begin();
         it != m_passingObjects.end(); ++it)
    {
        IObject* const object = it->first;
        PassingObjectData* const data = it->second;
        if (!data)
            continue;

        // [PLATFORM] ELF reads the transformation before the coordinate (0x8326B08 / 0x8326B1E);
        // PE reads the coordinate first (0x6EC69E / 0x6EC6A8).
#if defined(_MSC_VER)
        const Vec3& coordinate = data->portalPassing->getPortalPassingCoordinate();
        const Mat4& transformation = object->getAbsoluteTransformation();
#else
        const Mat4& transformation = object->getAbsoluteTransformation();
        const Vec3& coordinate = data->portalPassing->getPortalPassingCoordinate();
#endif
        const Vec3 position = transformPoint(coordinate, transformation);

        Vec3 start = data->lastPosition;
        Vec3 direction = { position.x - start.x, position.y - start.y, position.z - start.z };
        normalise(direction);

        bool sweep = true;
        if (IObject* const sector = data->portalPassing->getSector())
        {
            PortalCollision* const exit =
                checkAddPortalCollision(collisions, sector, start, position, false);
            if (exit)
                start = stepPast(exit->intersectionPosition, direction);
            else
                sweep = false;
        }

        if (sweep)
        {
            // The sweep starts from the stored position, not from the step past an exit.
            const Vec3 delta = { position.x - start.x, position.y - start.y, position.z - start.z };
            sectorObjects.clear();
            objectManager->getObjectsFromGrid(data->lastPosition, delta, sectorObjects,
                                              &sectorPredicate, false, 0, false);
            for (std::vector<IObject*>::iterator candidate = sectorObjects.begin();
                 candidate != sectorObjects.end(); ++candidate)
            {
                PortalCollision* hit =
                    checkAddPortalCollision(collisions, *candidate, start, position, true);
                while (hit)
                {
                    const Vec3 retry = stepPast(hit->intersectionPosition, direction);
                    hit = checkAddPortalCollision(collisions, *candidate, retry, position, true);
                }
            }
        }

        // Moving against a portal's normal enters its sector; anything else leaves it. Each event
        // goes to the global, sector and object subscribers and is never freed, as shipped.
        collisions.sort(PortalCollisionPred());
        for (std::list<PortalCollision*>::iterator c = collisions.begin(); c != collisions.end(); ++c)
        {
            PortalCollision* const collision = *c;
            const Vec3& normal = collision->intersectionNormal;
            PortalEvent* event;
            if (direction.x * normal.x + direction.y * normal.y + direction.z * normal.z < 0.0f)
            {
                event = new PortalEvent(PORTAL_EVENT_ENTER_SECTOR, object, collision->portalObject,
                                        collision->portalId);
                collision->sector->addObject(object);
                data->portalPassing->setSector(collision->portalObject);
            }
            else
            {
                event = new PortalEvent(PORTAL_EVENT_LEAVE_SECTOR, object, collision->portalObject,
                                        collision->portalId);
                collision->sector->removeObject(object);
                data->portalPassing->setSector(0);
            }
            broadcastEvent(event);
            collision->sector->broadcastEvent(event);
            data->portalPassing->broadcastEvent(event);
            delete collision;
        }
        collisions.clear();

        data->lastPosition = position;
    }
}

} // namespace world
} // namespace hfe
} // namespace dice

// Portal interfaces have distinct virtual layouts: callbacks start with notification, while
// PortalPassing's destructor follows its three portal methods and Sector's follows IBase.
// Use each real interface declaration; a common guessed layout changes dispatch.
