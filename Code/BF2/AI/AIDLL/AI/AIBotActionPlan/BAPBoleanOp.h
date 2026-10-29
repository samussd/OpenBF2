// Conditions built from other conditions (dice's spelling "Bolean"). The operation owns its operands
// and deletes them. BAPConNot negates its single operand.
// [INFERRED] BAPConNot shares this header.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPBOLEANOP_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPBOLEANOP_H

#include <list>

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPBoleanOp : public BAPCondition
{
public:
    virtual ~BAPBoleanOp();
    virtual void addCondition(BAPCondition* condition) = 0;   // +0x14

protected:
    /* +0x08 */ std::list<BAPCondition*> m_conditions;
};

class BAPConNot : public BAPBoleanOp
{
public:
    BAPConNot() {}
    explicit BAPConNot(BAPCondition* condition);
    virtual ~BAPConNot();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 14; }
    virtual void streamSelf(std::ostream& out) const;          // [FRONTIER]
    // Takes the first condition only; later ones are ignored and stay with the caller.
    virtual void addCondition(BAPCondition* condition);
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPBOLEANOP_H
