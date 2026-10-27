// Sector membership and sector-scoped portal events. Filename [INFERRED].

#include "Sector.h"

#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

Sector::Sector()
    : m_refCount(1)
{
    for (int type = 0; type < NUM_PORTAL_EVENT_TYPES; ++type)
        m_callbacks.push_back(new std::set<IPortalEventCallback*>());
}

Sector::~Sector()
{
    for (std::vector<std::set<IPortalEventCallback*>*>::iterator it = m_callbacks.begin();
         it != m_callbacks.end(); ++it)
    {
        delete *it;
    }
}

void Sector::addRef()
{
    ++m_refCount;
}

int Sector::getRef()
{
    return m_refCount;
}

int Sector::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* Sector::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<Sector*>(this);
    if (iid == IID_ISector)
        return const_cast<Sector*>(this);
    return 0;
}

void Sector::getObjects(std::list<IObject*>& out) const
{
    for (std::set<IObject*>::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
        out.push_back(*it);
}

void Sector::addObject(IObject* object)
{
    m_objects.insert(object);
}

void Sector::removeObject(IObject* object)
{
    m_objects.erase(object);
}

void Sector::subscribeEvents(PortalEventType type, IPortalEventCallback* callback)
{
    m_callbacks[type]->insert(callback);
}

void Sector::unSubscribeEvents(PortalEventType type, IPortalEventCallback* callback)
{
    m_callbacks[type]->erase(callback);
}

void Sector::broadcastEvent(PortalEvent* event) const
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
