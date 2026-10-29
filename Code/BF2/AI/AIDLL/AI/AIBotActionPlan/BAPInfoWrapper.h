// A statement that runs another and tags it with information about what the plan is doing.
// getInfoType names the information; the base answers 9.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPINFOWRAPPER_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPINFOWRAPPER_H

#include "BAPStatement.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPInfoWrapper : public BAPStatement
{
public:
    BAPInfoWrapper(BAPStatement* statement, int planId);
    virtual ~BAPInfoWrapper();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual void streamSelf(std::ostream& out) const;     // [FRONTIER]
    // [UNRESOLVED] The enum of the information types; idle is 4.
    virtual int getInfoType() const;                      // +0x20

    // The wrapped statement. [UNRESOLVED] Retail's base never deletes it; IWIdle does.
    /* +0x14 */ BAPStatement* m_statement;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPINFOWRAPPER_H
