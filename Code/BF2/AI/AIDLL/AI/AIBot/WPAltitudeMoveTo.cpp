#include "WPAltitudeMoveTo.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

WPAltitudeMoveTo::WPAltitudeMoveTo(float a, float b, float c, unsigned int d,
                                   const std::list<Vec3>& points)
{
    m_flag04 = true;
    m_float08 = -1.0f;
    m_flag05 = false;
    m_wayPointError = false;
    m_float10 = a;
    m_float14 = b;
    m_float18 = 1e9f;
    m_float1C = c;
    m_uint20 = d;
    m_int0C = 4;
    m_points.assign(points.begin(), points.end());
}

WPAltitudeMoveTo::~WPAltitudeMoveTo()
{
    BF2_AI_FRONTIER("dice::hfe::ai::WPAltitudeMoveTo::~WPAltitudeMoveTo");
}

float WPAltitudeMoveTo::getUrgency(const Vec3&, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::WPAltitudeMoveTo::getUrgency");
}

bool WPAltitudeMoveTo::getGoalPoint(Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::WPAltitudeMoveTo::getGoalPoint");
}

int WPAltitudeMoveTo::getId()
{
    return 6;
}

float WPAltitudeMoveTo::getWayPointRadius() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::WPAltitudeMoveTo::getWayPointRadius");
}

} // namespace ai
} // namespace hfe
} // namespace dice
