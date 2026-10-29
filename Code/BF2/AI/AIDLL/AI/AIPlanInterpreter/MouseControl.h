// Mouse-look control: turns a bot's view towards a direction by writing the look inputs.
// [INFERRED] Source placement: retail names the translation unit MouseControl.cpp; it sits with
// the other control helpers (TankControl, CarControl, PlaneControl).

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_MOUSECONTROL_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_MOUSECONTROL_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {

struct Mat4;

namespace io {
class PlayerInput;
}

namespace ai {

class Bot;
class IPIControlInfo;

// A PD controller on the two angles between the camera and the direction. pitchError and
// yawError receive the new angles (radians); the previous ones come back in on the next call.
// Within allowedDeviation the input is zeroed (only when the controller's static switch is on).
// [INFERRED] the last three: a bias and the yaw and pitch angular speeds; every recovered caller
// passes 0.
void mouseControlLookAtDirection(Bot* bot, const Vec3& direction, const Mat4& camera,
                                 float& pitchError, float previousPitchError,
                                 float& yawError, float previousYawError,
                                 float allowedDeviation, IPIControlInfo* control,
                                 io::PlayerInput* input, float bias, float yawSpeed,
                                 float pitchSpeed);

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_MOUSECONTROL_H
