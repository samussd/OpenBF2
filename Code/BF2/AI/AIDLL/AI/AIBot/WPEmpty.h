// The list a bot holds while it has nowhere to go: one point, never reached.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_WPEMPTY_H
#define AI_AIDLL_AI_AIBOT_WPEMPTY_H

#include "WaypointList.h"

namespace dice {
namespace hfe {
namespace ai {

class WPEmpty : public WaypointList
{
public:
    explicit WPEmpty(const Vec3& position);
    virtual ~WPEmpty() {}

    // Moves the point to wherever it is asked about, and is never urgent.
    virtual float getUrgency(const Vec3& position, Bot* bot);
    virtual bool getGoalPoint(Vec3& point);
    virtual int getId();

private:
    /* +0x10 */ Vec3 m_position;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_WPEMPTY_H
