// The root of a bot's action plan: every plan statement is a BotScript. execute runs one step of
// the statement for a bot; ok reports whether the step could run and done whether the statement
// has finished. AIBotActionPlan is the retail folder of the statements (BAP = bot action plan).
// [INFERRED] Source placement; the destructor is emitted beside BAPStatement.cpp.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BOTSCRIPT_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BOTSCRIPT_H

namespace dice {
namespace hfe {
namespace ai {

class Bot;

class BotScript
{
public:
    BotScript() : m_owner(0) {}
    virtual bool execute(Bot* bot, bool& ok, bool& done) = 0;   // +0x00
    virtual ~BotScript();                                         // +0x04 / +0x08

    // [UNRESOLVED] Whoever holds the statement: the enclosing statement, or for a plan's root its
    // generator, behaviour and then bot, each stored untyped in turn. Nothing here reads it.
    /* +0x04 */ void* m_owner;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BOTSCRIPT_H
