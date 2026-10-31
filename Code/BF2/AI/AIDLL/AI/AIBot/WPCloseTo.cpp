#include "WPCloseTo.h"

#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../../../../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

// The object's position, or the world centre of its sphere.
Vec3 centreOf(Information* object)
{
    IPIPhysical* const physical = object->getPhysical();
    Vec3 centre;
    if (!physical)
    {
        centre = object->getPosition();
        return centre;
    }
    const Mat4& matrix = object->getTransformation();
    const Vec3 offset = physical->getSphereLocalOffset();
    centre.x = matrix.m[8] * offset.z + matrix.m[0] * offset.x + matrix.m[4] * offset.y;
    centre.y = matrix.m[1] * offset.x + matrix.m[5] * offset.y + matrix.m[9] * offset.z;
    centre.z = offset.x * matrix.m[2] + offset.y * matrix.m[6] + offset.z * matrix.m[10];
    const Vec3& position = object->getPosition();
    centre.x = position.x + centre.x;
    centre.y = position.y + centre.y;
    centre.z = position.z + centre.z;
    return centre;
}

} // namespace

// 0x1c8780 (Ghidra).
WPCloseTo::WPCloseTo(unsigned int object, float radius)
    : m_object(object)
    , m_radius(radius <= 5.0f ? 5.0f : radius)
    , m_distanceSqr(25.0f)
{
    m_targetChanged = true;
    m_flag05 = false;
    m_wayPointError = false;
    m_float08 = -1.0f;
    m_int0C = 2;
    m_position.x = 0.0f;
    m_position.y = 0.0f;
    m_position.z = 0.0f;
    Information* const information = informationManager->getInformation(object);
    if (!information)
        return;
    IPIPhysical* const physical = information->getPhysical();
    if (physical)
    {
        const float wide = physical->getSmallestRadius() + physical->getSmallestRadius();
        if (m_radius <= wide)
            m_radius = wide;
    }
    m_position = centreOf(information);
}

// 0x1c8ba0 (Ghidra). Squared distance over 4 radii squared, at most 1; inside the radius none. The
// target has changed once the object moved a radius from where the plan went.
float WPCloseTo::getUrgency(const Vec3& position, Bot*)
{
    if (m_wayPointError)
        return 0.0f;
    Information* const object = informationManager->getInformation(m_object);
    if (!object)
    {
        m_flag05 = true;
        return 0.0f;
    }
    const Vec3 centre = centreOf(object);
    const float x = centre.x - position.x;
    const float y = centre.y - position.y;
    const float z = centre.z - position.z;
    const float distanceSqr = x * x + y * y + z * z;
    m_distanceSqr = distanceSqr;
    const float radiusSqr = m_radius * m_radius;
    float urgency = distanceSqr / (radiusSqr * 4.0f);
    if (1.0f < urgency)
        urgency = 1.0f;
    if (!(radiusSqr <= distanceSqr))
    {
        m_targetChanged = true;
        return 0.0f;
    }
    const float mx = m_position.x - centre.x;
    const float my = m_position.y - centre.y;
    const float mz = m_position.z - centre.z;
    if (radiusSqr < mx * mx + my * my + mz * mz)
    {
        m_position = centre;
        m_targetChanged = true;
    }
    return urgency;
}

bool WPCloseTo::getGoalPoint(Vec3& point)
{
    if (!informationManager->getInformation(m_object))
        return false;
    point = m_position;
    return true;
}

int WPCloseTo::getId()
{
    return 0;
}

float WPCloseTo::getWayPointRadius() const
{
    return m_radius;
}

// 0x1c8e30 (Ghidra). Keeps the pace near the object: its speed inside the radius, up to 1.33 times
// it at twice the radius. A still object means stopping inside the radius.
float WPCloseTo::modifyMaxSpeed(float speed)
{
    Information* const object = informationManager->getInformation(m_object);
    if (!object)
        return speed;
    IPIMobile* const mobile = object->getMobile();
    if (!mobile)
        return m_distanceSqr <= m_radius ? 0.0f : speed;
    float limit;
    if (m_distanceSqr <= m_radius)
        limit = mobile->getSpeedMagnitude();
    else
    {
        if (m_radius < m_distanceSqr + m_distanceSqr)
            return speed;
        limit = mobile->getSpeedMagnitude() * 1.33f * m_distanceSqr / m_radius;
    }
    return speed < limit ? speed : limit;
}

} // namespace ai
} // namespace hfe
} // namespace dice
