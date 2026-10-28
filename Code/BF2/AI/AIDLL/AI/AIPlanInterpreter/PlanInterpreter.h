// Executes a bot's plan script. BAPInterpreter is the only implementation.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_PLANINTERPRETER_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_PLANINTERPRETER_H

namespace dice {
namespace hfe {
namespace ai {

class Bot;
class BotScript;

class PlanInterpreter
{
public:
    virtual ~PlanInterpreter() {}
    // [INFERRED] meaning of the two flags; BAPInterpreter passes them to its entries.
    virtual bool execute(BotScript* script, Bot* bot, bool& flagA, bool& flagB) = 0;   // +0x08
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_PLANINTERPRETER_H
