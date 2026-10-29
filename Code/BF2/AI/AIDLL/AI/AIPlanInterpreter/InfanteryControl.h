// Soldier control helpers shared by the infantry interpreter entries (dice's spelling "Infantery").
// [INFERRED] Source placement: retail names AI/AIDLL/AI/AIPlanInterpreter/InfanteryControl.cpp.
// Only infanteryResetControls is recovered.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_INFANTERYCONTROL_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_INFANTERYCONTROL_H

namespace dice {
namespace hfe {
namespace io {
class PlayerInput;
}
namespace ai {

class Bot;

// Releases the soldier's controls by group: movement (move, turn, sprint, action), look (centres
// the horizontal look, and sets the vertical one against the equipment's pitch), actions (fire,
// weapons, seats, use, reload, flares).
void infanteryResetControls(Bot* bot, io::PlayerInput* input, bool movement, bool look, bool actions);

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_INFANTERYCONTROL_H
