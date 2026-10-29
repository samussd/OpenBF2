// The interface of a bot's plan debugger; BAPDebugger implements it. Only the slots are recovered.
// [UNRESOLVED] The enumerators of both enums. [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_BOTSCRIPTDEBUGGER_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_BOTSCRIPTDEBUGGER_H

namespace dice {
namespace hfe {
namespace ai {

class BotScript;

class BotScriptDebugger
{
public:
    enum DebugLevel {};
    enum DebugPlace {};

    virtual ~BotScriptDebugger() {}                                                      // +0x00 / +0x04
    virtual void debug(DebugLevel level, BotScript* script, DebugPlace place, int n) = 0; // +0x08
    virtual void clear() = 0;                                                            // +0x0C
    virtual void* getImplementation() { return 0; }                                     // +0x10
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_BOTSCRIPTDEBUGGER_H
