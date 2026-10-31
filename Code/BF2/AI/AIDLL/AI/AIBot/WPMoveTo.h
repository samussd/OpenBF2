// A route of points, each with the strategic area it belongs to or -1.

#ifndef AI_AIDLL_AI_AIBOT_WPMOVETO_H
#define AI_AIDLL_AI_AIBOT_WPMOVETO_H

#include <list>
#include <utility>

#include "WaypointList.h"

namespace dice {
namespace hfe {
namespace ai {

class WPMoveTo : public WaypointList
{
public:
    WPMoveTo(float radius, float passRadius, unsigned int count, const std::list<Vec3>& points, int area);
    WPMoveTo(float radius, float passRadius, unsigned int count, const std::list<std::pair<Vec3, int> >& points);
    virtual ~WPMoveTo() {}

    // Passes each point on the way and is urgent until the last is reached.
    virtual float getUrgency(const Vec3& position, Bot* bot);
    virtual bool getGoalPoint(Vec3& point);
    virtual int getId();
    virtual float getWayPointRadius() const;

    void getCoords(std::list<Vec3>& out) const;

private:
    /* +0x10 */ float m_radius;                      // reaching the last point
    /* +0x14 */ float m_passRadius;                  // passing any other, at least 2
    /* +0x18 */ unsigned int m_count;                // [UNRESOLVED] 1 from the console, 2 from a squad
    /* +0x1C */ std::list<std::pair<Vec3, int> > m_points;
    /* +0x20 */ bool m_flag20;                       // [UNRESOLVED] = false
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_WPMOVETO_H
