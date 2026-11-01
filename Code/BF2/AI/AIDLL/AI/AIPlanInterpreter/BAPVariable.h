// A value a plan shares between its statements. It lives in the bot's script context, which deletes
// it when the plan goes. [INFERRED] The constructor: retail inlines it in every plan generator.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_BAPVARIABLE_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_BAPVARIABLE_H

#include "BAPContext.h"

namespace dice {
namespace hfe {
namespace ai {

template <typename T>
class BAPVariable : public BotScriptVariable
{
public:
    BAPVariable(BAPContext* context, const T& value)
        : m_context(context)
        , m_value(value)
        , m_owned(0)
    {
        context->addVariable(this);
    }

    virtual ~BAPVariable()
    {
        if (m_owned)
            ::operator delete(m_owned);
    }

    /* +0x04 */ BAPContext* m_context;
    /* +0x08 */ T m_value;
    // [UNRESOLVED] Null in every recovered constructor, and the destructor frees it if set.
    void* m_owned;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_BAPVARIABLE_H
