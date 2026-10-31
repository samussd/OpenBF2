// Soldier control helpers shared by the infantry interpreter entries (dice's spelling "Infantery").
// [INFERRED] Source placement: retail names AI/AIDLL/AI/AIPlanInterpreter/InfanteryControl.cpp.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_INFANTERYCONTROL_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_INFANTERYCONTROL_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace io {
class PlayerInput;
}
namespace ai {

class Bot;
class Information;
class IPIControlInfo;

// Releases the soldier's controls by group: movement (move, turn, sprint, action), look (centres
// the horizontal look, and sets the vertical one against the equipment's pitch), actions (fire,
// weapons, seats, use, reload, flares).
// Always true.
bool infanteryResetControls(Bot* bot, io::PlayerInput* input, bool movement, bool look, bool actions);

// Takes the weapon out of its zoom.
void removeZoom(Bot* bot);

// Sets the throttle and sprint inputs for the speed the bot is to keep.
void infanteryControlThrottle(Bot* bot, io::PlayerInput* input, IPIControlInfo* controlInfo, float maxSpeed,
                              bool sprint);

// Whether the obstacle avoided around a point needs a new look: not when it is gone; otherwise when
// the point (XZ) is inside the radius or more than 1.5 beyond it.
bool needsUpdate(const Vec2& point, unsigned int obstacle, float radius);

// Picks the obstacle on the way to `ahead` and how to pass it. True: this step is lost.
bool updateInfantryMoveToPosWithObstacles(Bot* bot, const Vec2& ahead, const Vec3& coordinate,
                                          unsigned int object, Vec2& avoidPoint, unsigned int& obstacle,
                                          float& passDistance, Vec2& obstaclePosition,
                                          float& obstacleRadius, bool& obstacleFlag,
                                          Information* information, bool flag);

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_INFANTERYCONTROL_H
