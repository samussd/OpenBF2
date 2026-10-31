#include "WPMoveTo.h"

#include "BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIStrategicEntity/AIStrategicArea.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"

#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

namespace {

float atLeast(float value, float least)
{
    return value <= least ? least : value;
}

} // namespace

// 0x1c92f0 (Ghidra). The last point gets the area, the others -1.
WPMoveTo::WPMoveTo(float radius, float passRadius, unsigned int count, const std::list<Vec3>& points, int area)
    : m_radius(radius)
    , m_passRadius(atLeast(passRadius, 2.0f))
    , m_count(count)
    , m_flag20(false)
{
    m_targetChanged = true;
    m_flag05 = false;
    m_wayPointError = false;
    m_float08 = -1.0f;
    m_int0C = 0;
    for (std::list<Vec3>::const_iterator it = points.begin(); it != points.end(); ++it)
        m_points.push_back(std::make_pair(*it, -1));
    if (!m_points.empty())
        m_points.back().second = area;
}

// 0x1b96c0 (ELF).
WPMoveTo::WPMoveTo(float radius, float passRadius, unsigned int count,
                   const std::list<std::pair<Vec3, int> >& points)
    : m_radius(radius)
    , m_passRadius(atLeast(passRadius, 2.0f))
    , m_count(count)
    , m_points(points)
    , m_flag20(false)
{
    m_targetChanged = true;
    m_flag05 = false;
    m_wayPointError = false;
    m_float08 = -1.0f;
    m_int0C = 0;
}

// 0x1c9a30 (Ghidra). The last point: not urgent inside its radius (at least the equipment's smallest
// radius); outside, the squared distance over 1.5 times that radius squared, from 0.1 to 1, twice
// over for an area its side holds; inside an area it does not hold the urgency ends, and its size is
// taken off the distance. A point before the last is dropped once passed (inside the pass radius,
// 10 m without an area, or in its area, or after an error) and the urgency stays 1. After an error a
// point in an area is drawn again. No null guard on the equipment, as shipped.
float WPMoveTo::getUrgency(const Vec3& position, Bot* bot)
{
    if (m_points.empty())
    {
        m_flag05 = true;
        return 0.0f;
    }
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    IPIPhysical* const physical = manager->getInformation(bot->getCurrentEquipment())->getPhysical();
    const Vec3& first = m_points.front().first;
    const float dx = first.x - position.x;
    const float dy = first.y - position.y;
    const float dz = first.z - position.z;
    float distanceSqr = dz * dz + dy * dy + dx * dx;
    if (m_points.size() == 1)
    {
        const float radius =
            atLeast(physical->getSmallestRadius(), static_cast<float>(std::ceil(m_radius)));
        const float radiusSqr = radius * radius;
        if (!(radiusSqr <= distanceSqr))
        {
            m_flag05 = true;
            m_wayPointError = false;
            return 0.0f;
        }
        AIStrategicArea* const area =
            AIStrategicObjectManager::getInstance()->getStrategicArea(m_points.front().second);
        float weight;
        if (!area)
            weight = m_wayPointError ? 0.0f : 1.0f;
        else
        {
            const int side = bot->getSide();
            weight = 2.0f;
            if (area->getStatus(side) == 0)
            {
                if (!area->isInside(position))
                {
                    distanceSqr = distanceSqr - area->m_size[side] * area->m_size[side];
                    weight = 1.0f;
                }
                else
                {
                    distanceSqr = 0.0f;
                    weight = 0.0f;
                }
            }
            if (m_wayPointError)
            {
                area->randomizePos(bot->getSide(), 0.9f, m_points.front().first);
                m_wayPointError = false;
            }
        }
        const float range = radiusSqr * 1.5f;
        m_flag05 = !(range <= distanceSqr);
        float urgency = distanceSqr / range;
        if (1.0f < urgency)
            urgency = 1.0f;
        if (urgency < 0.1f)
            urgency = 0.1f;
        return urgency * weight;
    }
    float radius = atLeast(physical->getSmallestRadius(), static_cast<float>(std::ceil(m_passRadius)));
    if (m_points.front().second == -1)
        radius = atLeast(physical->getSmallestRadius(), 10.0f);
    if (radius * radius <= distanceSqr)
    {
        AIStrategicArea* const area =
            AIStrategicObjectManager::getInstance()->getStrategicArea(m_points.front().second);
        if (!area)
        {
            if (!m_wayPointError)
                return 1.0f;
        }
        else if (!area->isInside(position))
        {
            if (!m_wayPointError)
                return 1.0f;
            area->randomizePos(bot->getSide(), 0.9f, m_points.front().first);
            m_wayPointError = false;
            return 1.0f;
        }
    }
    m_points.pop_front();
    m_targetChanged = true;
    m_wayPointError = false;
    return 1.0f;
}

bool WPMoveTo::getGoalPoint(Vec3& point)
{
    if (m_points.empty())
        return false;
    point = m_points.front().first;
    return true;
}

int WPMoveTo::getId()
{
    return 1;
}

float WPMoveTo::getWayPointRadius() const
{
    return m_radius;
}

void WPMoveTo::getCoords(std::list<Vec3>& out) const
{
    for (std::list<std::pair<Vec3, int> >::const_iterator it = m_points.begin(); it != m_points.end(); ++it)
        out.push_back(it->first);
}

} // namespace ai
} // namespace hfe
} // namespace dice
