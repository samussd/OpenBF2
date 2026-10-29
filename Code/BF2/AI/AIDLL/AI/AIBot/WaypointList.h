// The route a bot is following. getId names the concrete list: 0 WPCloseTo, 1 WPMoveTo,
// 2 WPPatrol, 3 WPBeachLanding, 4 WPMoveToBeachLanding, 5 WPAltitudeCloseTo, 6 WPAltitudeMoveTo,
// 7 WPAltitudePatrol, 8 WPEmpty. Only this base is recovered.
// [INFERRED] Source placement. The ELF emits the bodies below as inline copies, so they stay here.

#ifndef AI_AIDLL_AI_AIBOT_WAYPOINTLIST_H
#define AI_AIDLL_AI_AIBOT_WAYPOINTLIST_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class Bot;

class WaypointList
{
public:
    virtual ~WaypointList() {}                                      // +0 / +1
    virtual float getUrgency(const Vec3& position, Bot* bot) = 0;   // +2
    virtual bool getGoalPoint(Vec3& point) = 0;                     // +3
    virtual int getId() = 0;                                        // +4
    virtual void setWayPointError(bool error) { m_wayPointError = error; }   // +5
    virtual bool getWayPointError() const { return m_wayPointError; }        // +6
    virtual float getWayPointRadius() const { return 0.0f; }                 // +7

    // The subclass constructors write these; WPMoveTo stores true, false, false.
    // [INFERRED] name: BBMoveTo copies it into its changed-target flag.
    /* +0x04 */ bool m_targetChanged;
    /* +0x05 */ bool m_flag05;         // [UNRESOLVED]; SAI::updateBotPositions requires it set
    /* +0x06 */ bool m_wayPointError;
    /* +0x08 */ float m_float08;       // [UNRESOLVED] -1 in WPEmpty and WPAltitudeMoveTo
    /* +0x0C */ int m_int0C;           // [UNRESOLVED] WPEmpty 1, WPAltitudeMoveTo 4
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_WAYPOINTLIST_H
