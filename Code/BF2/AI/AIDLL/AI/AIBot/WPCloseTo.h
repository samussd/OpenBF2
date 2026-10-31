// Staying close to an object: its bounding sphere's centre, followed as it moves.

#ifndef AI_AIDLL_AI_AIBOT_WPCLOSETO_H
#define AI_AIDLL_AI_AIBOT_WPCLOSETO_H

#include "WaypointList.h"

namespace dice {
namespace hfe {
namespace ai {

class WPCloseTo : public WaypointList
{
public:
    WPCloseTo(unsigned int object, float radius);
    virtual ~WPCloseTo() {}

    virtual float getUrgency(const Vec3& position, Bot* bot);
    virtual bool getGoalPoint(Vec3& point);
    virtual int getId();
    virtual float getWayPointRadius() const;

    // [INFERRED] accessor; BBPGotoWaypointSoldier::createPlan reads the field directly.
    unsigned int getObject() const { return m_object; }
    float modifyMaxSpeed(float speed);

private:
    /* +0x10 */ unsigned int m_object;
    /* +0x14 */ float m_radius;                      // at least 5, and twice the object's smallest radius
    /* +0x18 */ float m_distanceSqr;                 // getUrgency's last; = 25
    /* +0x1C */ Vec3 m_position;                     // where the plan was made for
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_WPCLOSETO_H
