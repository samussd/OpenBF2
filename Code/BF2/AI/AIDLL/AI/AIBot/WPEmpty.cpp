#include "WPEmpty.h"

namespace dice {
namespace hfe {
namespace ai {

WPEmpty::WPEmpty(const Vec3& position)
{
    m_float08 = -1.0f;
    m_flag04 = true;
    m_flag05 = false;
    m_wayPointError = false;
    m_position = position;
    m_int0C = 1;
}

// The flag write at +4 is the shipped one.
float WPEmpty::getUrgency(const Vec3& position, Bot*)
{
    m_flag04 = true;
    m_position = position;
    return 0.0f;
}

bool WPEmpty::getGoalPoint(Vec3& point)
{
    point = m_position;
    return true;
}

int WPEmpty::getId()
{
    return 8;
}

} // namespace ai
} // namespace hfe
} // namespace dice
