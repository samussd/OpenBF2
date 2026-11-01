// Holds its controls and finishes when the condition comes true.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAIDLE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAIDLE_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPAIdle : public BAPAction
{
public:
    BAPAIdle(BAPCondition* condition, unsigned long long controls, int planId);
    virtual ~BAPAIdle();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ BAPCondition* m_condition;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAIDLE_H
