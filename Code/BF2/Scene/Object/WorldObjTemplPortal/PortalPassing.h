// The portal-passing component: which sector its object is in, its passing point, and the
// callbacks subscribed to its transitions. Directory attested; filename [INFERRED].

#ifndef SCENE_OBJECT_WORLDOBJTEMPLPORTAL_PORTALPASSING_H     // [INFERRED] guard style
#define SCENE_OBJECT_WORLDOBJTEMPLPORTAL_PORTALPASSING_H

#include <set>
#include <vector>

#include "PortalManager.h"

namespace dice {
namespace hfe {
namespace world {

class PortalPassing : public IPortalPassing
{
public:
    // One reference, one callback set per event type.
    PortalPassing(const Vec3& portalPassingCoordinate, IObject* sector);
    virtual ~PortalPassing();                   // deletes the callback sets, not the callbacks

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual IObject* getSector() const;
    virtual void setSector(IObject* sector);
    virtual const Vec3& getPortalPassingCoordinate() const;

    // The event type indexes the callback sets unchecked.
    void subscribeEvents(PortalEventType type, IPortalEventCallback* callback);
    void unSubscribeEvents(PortalEventType type, IPortalEventCallback* callback);
    void broadcastEvent(PortalEvent* event) const;

private:
    /* +0x04 */ int      m_refCount;
    /* +0x08 */ Vec3     m_portalPassingCoordinate;
    /* +0x14 */ IObject* m_sector;
    /* +0x18 */ std::vector<std::set<IPortalEventCallback*>*> m_callbacks;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_WORLDOBJTEMPLPORTAL_PORTALPASSING_H
