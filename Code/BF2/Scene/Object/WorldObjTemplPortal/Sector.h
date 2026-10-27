// A sector: the volume a set of portals encloses. It tracks the objects inside it and the
// callbacks subscribed to its transitions. Directory attested; filename [INFERRED].

#ifndef SCENE_OBJECT_WORLDOBJTEMPLPORTAL_SECTOR_H     // [INFERRED] guard style
#define SCENE_OBJECT_WORLDOBJTEMPLPORTAL_SECTOR_H

#include <list>
#include <set>
#include <vector>

#include "PortalManager.h"

namespace dice {
namespace hfe {
namespace world {

// The component a sector object carries, under IID_ISector. The destructor follows IBase.
class ISector : public IBase
{
public:
    virtual ~ISector() {}                                                       // +0x10 / +0x14
    virtual void getObjects(std::list<IObject*>& out) const = 0;               // +0x18
    virtual void addObject(IObject* object) = 0;                                // +0x1C
    virtual void removeObject(IObject* object) = 0;                             // +0x20
};

class Sector : public ISector
{
public:
    Sector();                                   // one reference, one callback set per event type
    virtual ~Sector();                          // deletes the callback sets, not the callbacks

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // Appends the members in pointer order; the list is not cleared first.
    virtual void getObjects(std::list<IObject*>& out) const;
    virtual void addObject(IObject* object);
    virtual void removeObject(IObject* object);

    // The event type indexes the callback sets unchecked.
    void subscribeEvents(PortalEventType type, IPortalEventCallback* callback);
    void unSubscribeEvents(PortalEventType type, IPortalEventCallback* callback);
    void broadcastEvent(PortalEvent* event) const;

private:
    /* +0x04 */ int m_refCount;
    /* +0x08 */ std::set<IObject*> m_objects;
    /* +0x14 */ std::vector<std::set<IPortalEventCallback*>*> m_callbacks;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_WORLDOBJTEMPLPORTAL_SECTOR_H
