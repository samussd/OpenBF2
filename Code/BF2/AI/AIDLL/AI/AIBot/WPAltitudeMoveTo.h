// An aircraft's route: points to fly through at a given altitude.
// [INFERRED] Source placement and the roles of the constructor's numbers.
// [FRONTIER] Following the route remains a named stop.

#ifndef AI_AIDLL_AI_AIBOT_WPALTITUDEMOVETO_H
#define AI_AIDLL_AI_AIBOT_WPALTITUDEMOVETO_H

#include <list>

#include "WaypointList.h"

namespace dice {
namespace hfe {
namespace ai {

class WPAltitudeMoveTo : public WaypointList
{
public:
    WPAltitudeMoveTo(float a, float b, float c, unsigned int d, const std::list<Vec3>& points);
    virtual ~WPAltitudeMoveTo();

    virtual float getUrgency(const Vec3& position, Bot* bot);
    virtual bool getGoalPoint(Vec3& point);
    virtual int getId();
    virtual float getWayPointRadius() const;

private:
    /* +0x10 */ float m_float10;            // [UNRESOLVED] the constructor's first number
    /* +0x14 */ float m_float14;            // [UNRESOLVED] the second
    /* +0x18 */ float m_float18;            // [UNRESOLVED] 1e9 at construction
    /* +0x1C */ float m_float1C;            // [UNRESOLVED] the third
    /* +0x20 */ unsigned int m_uint20;      // [UNRESOLVED] the fourth
    /* +0x24 */ std::list<Vec3> m_points;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_WPALTITUDEMOVETO_H
