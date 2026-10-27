// Portal-passing component state and object-scoped portal events. Filename [INFERRED].

#include "PortalPassing.h"

#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

PortalPassing::PortalPassing(const Vec3& portalPassingCoordinate, IObject* sector)
    : m_refCount(1)
    , m_portalPassingCoordinate(portalPassingCoordinate)
    , m_sector(sector)
{
    for (int type = 0; type < NUM_PORTAL_EVENT_TYPES; ++type)
        m_callbacks.push_back(new std::set<IPortalEventCallback*>());
}

PortalPassing::~PortalPassing()
{
    for (std::vector<std::set<IPortalEventCallback*>*>::iterator it = m_callbacks.begin();
         it != m_callbacks.end(); ++it)
    {
        delete *it;
    }
}

void PortalPassing::addRef()
{
    ++m_refCount;
}

int PortalPassing::getRef()
{
    return m_refCount;
}

int PortalPassing::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* PortalPassing::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<PortalPassing*>(this);
    if (iid == IID_IPortalPassing)
        return const_cast<PortalPassing*>(this);
    return 0;
}

IObject* PortalPassing::getSector() const
{
    return m_sector;
}

void PortalPassing::setSector(IObject* sector)
{
    m_sector = sector;
}

const Vec3& PortalPassing::getPortalPassingCoordinate() const
{
    return m_portalPassingCoordinate;
}

void PortalPassing::subscribeEvents(PortalEventType type, IPortalEventCallback* callback)
{
    m_callbacks[type]->insert(callback);
}

void PortalPassing::unSubscribeEvents(PortalEventType type, IPortalEventCallback* callback)
{
    m_callbacks[type]->erase(callback);
}

void PortalPassing::broadcastEvent(PortalEvent* event) const
{
    const std::set<IPortalEventCallback*>& callbacks = *m_callbacks[event->getType()];
    for (std::set<IPortalEventCallback*>::const_iterator it = callbacks.begin();
         it != callbacks.end(); ++it)
    {
        (*it)->handleEvent(event);
    }
}

} // namespace world
} // namespace hfe
} // namespace dice
