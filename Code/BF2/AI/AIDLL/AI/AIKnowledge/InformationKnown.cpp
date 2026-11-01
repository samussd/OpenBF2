
#include "InformationKnown.h"

#include <cmath>

#include "BFInformationEnvironment.h"
#include "IPIMobile.h"
#include "IPIPhysical.h"
#include "IPIUnit.h"
#include "InformationManager.h"
#include "../AIBotBehaviour/CustomCurve.h"                          // SCurve::calculate
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"

namespace dice {
namespace hfe {
namespace ai {

InformationKnown::InformationKnown(IAIObject* object, unsigned int types, double time,
                                   float degeneration, float basicTemp, float allowedTimeDiff,
                                   int side)
    : Information(object, -1.0)
    , m_timeStamp(time)
    , m_degeneration(degeneration)
    , m_ownerSide(side)
    , m_contacts(0)
{
    m_allowedTimeDiff = allowedTimeDiff;
    m_strength = basicTemp;
    m_basicTemp = basicTemp;
    m_objectTypeFlags = types;
    m_side = m_object->getSide();
    rememberObject();
}

InformationKnown::~InformationKnown()
{
}

// A known information is sort 1 (see Information.h).
int InformationKnown::getInformationSort()
{
    return 1;
}

void InformationKnown::rememberObject()
{
    m_lastPosition = m_object->getPosition();
    m_lastTransformation = m_object->getTransformation();
}

void InformationKnown::forEachSecondary(void (Information::*action)())
{
    if (!isPrimary())
        return;
    int iterator = -1;
    const int side = getSide();
    Information* secondary =
        informationManager->getInformation(getFirstSecondaryObject(iterator, side));
    while (secondary)
    {
        (secondary->*action)();
        secondary = informationManager->getInformation(getSecondaryObject(iterator, side));
    }
}

// Within the allowed time difference only the time stamp moves. Otherwise everything is read
// again, and the primary object tells the side's environment where it now stands. Answers
// whether it read the object.
bool InformationKnown::setTime(double time)
{
    if (time <= m_time + static_cast<double>(m_allowedTimeDiff))
    {
        m_timeStamp = time;
        return false;
    }

    if (m_contacts == 0)
    {
        rememberObject();
        if (m_mobile)
            m_mobile->setLostContact();
    }
    if (m_mobile)
        m_mobile->setTime(time);

    if (isPrimary())
    {
        int iterator = -1;
        const int side = getSide();
        Information* secondary =
            informationManager->getInformation(getFirstSecondaryObject(iterator, side));
        while (secondary)
        {
            secondary->setTime(time);
            secondary = informationManager->getInformation(getSecondaryObject(iterator, side));
        }
    }

    m_side = m_object->getSide();
    if (m_unit)
    {
        m_physical->setTime(time);
        m_unit->calculateStrMod(m_physical->getHealth());
    }
    setTimeOnPlugIns(time);
    m_time = time;
    if (isPrimary())
    {
        static_cast<IAIInformationEnvironmentModifiers*>(
            CommonSingletonServer::m_iaiInformationEnvironmentModifiers)
            ->addAndUpdatePosition(m_ownerSide, m_unit != 0, m_object->getInformation(m_ownerSide));
    }
    m_timeStamp = time;
    return true;
}

// Raises the certainty to at least the given amount, by backdating the stamp as far as the
// degeneration curve needs. Zero wipes the stamp.
bool InformationKnown::setTime(double time, float security)
{
    float wanted = 0.0f;
    if (security >= 0.0f)
        wanted = security > 1.0f ? 1.0f : security;
    if (!(getSecurity() < wanted))
        return false;

    setTime(time);
    if (wanted == 0.0f)
    {
        m_timeStamp = 0.0;
        return true;
    }
    const float back = 1.0f / (wanted * m_degeneration);
    m_time = -1.0;
    return setTime(static_cast<double>(static_cast<float>(time) - back));
}

double InformationKnown::getTimeStamp() const
{
    return m_timeStamp;
}

void InformationKnown::addContact()
{
    ++m_contacts;
    if (m_mobile)
        m_mobile->setHasContact();
    forEachSecondary(&Information::addContact);
}

void InformationKnown::removeContact()
{
    if (m_contacts > 0 && --m_contacts == 0)
    {
        rememberObject();
        if (m_mobile)
            m_mobile->setLostContact();
    }
    forEachSecondary(&Information::removeContact);
}

void InformationKnown::removeAllContacts()
{
    if (m_contacts > 0)
    {
        m_contacts = 0;
        rememberObject();
        if (m_mobile)
            m_mobile->setLostContact();
    }
    forEachSecondary(&Information::removeAllContacts);
}

bool InformationKnown::hasContact() const
{
    return m_contacts > 0;
}

float InformationKnown::getSecurity(double time)
{
    return 1.0f - SCurve::calculate(static_cast<float>(time - m_timeStamp) / m_degeneration);
}

float InformationKnown::getSecurity()
{
    if (m_contacts >= 1)
        return 1.0f;
    return getSecurity(static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime());
}

Vec3& InformationKnown::getPosition()
{
    return m_contacts ? const_cast<Vec3&>(m_object->getPosition()) : m_lastPosition;
}

float InformationKnown::getAltitude()
{
    return static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain)->getAltitude(getPosition());
}

// The lowest height above the ground in ten steps along the direction.
float InformationKnown::getAltitude(const Vec3& direction)
{
    const IAITerrain* const terrain = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y
                                   + direction.z * direction.z);
    Vec3 probe = getPosition();
    float lowest = terrain->getAltitude(probe);

    const float scale = 1.0f / 10.0f;
    const float step = length / 10.0f;
    probe.x = direction.x * scale + probe.x;
    probe.y = direction.y * scale + probe.y;
    probe.z = direction.z * scale + probe.z;
    for (float travelled = step; travelled < length; travelled += step)
    {
        probe.x = direction.x * scale + probe.x;
        probe.y = direction.y * scale + probe.y;
        probe.z = direction.z * scale + probe.z;
        const float altitude = terrain->getAltitude(probe);
        if (altitude < lowest)
            lowest = altitude;
    }
    return lowest;
}

Mat4& InformationKnown::getTransformation()
{
    return m_contacts ? const_cast<Mat4&>(m_object->getTransformation()) : m_lastTransformation;
}

// The remembered third row, even in contact.
Vec3 InformationKnown::getFaceing()
{
    Vec3 facing;
    facing.x = m_lastTransformation.m[8];
    facing.y = m_lastTransformation.m[9];
    facing.z = m_lastTransformation.m[10];
    return facing;
}

int InformationKnown::getSide() const
{
    return m_side;
}

} // namespace ai
} // namespace hfe
} // namespace dice
