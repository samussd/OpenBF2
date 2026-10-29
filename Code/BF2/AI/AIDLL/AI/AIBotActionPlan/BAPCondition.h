// A condition a plan statement tests each step, and the two constant conditions.
// [UNRESOLVED] The enum getConditionType returns; true is 40 and false 6.
// [INFERRED] BAPConTrue and BAPConFalse share this header.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONDITION_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONDITION_H

#include <iosfwd>

namespace dice {
namespace hfe {
namespace ai {

class Bot;

class BAPCondition
{
public:
    BAPCondition() : m_owner(0) {}
    virtual bool evaluate(Bot* bot) = 0;                 // +0x00
    virtual int getConditionType() const = 0;            // +0x04
    virtual void streamSelf(std::ostream& out) const;    // +0x08 [FRONTIER]
    virtual ~BAPCondition();                             // +0x0C / +0x10

    // [UNRESOLVED] The statement or condition holding this one; set by the holder, never read here.
    /* +0x04 */ void* m_owner;
};

class BAPConTrue : public BAPCondition
{
public:
    virtual bool evaluate(Bot*) { return true; }
    virtual int getConditionType() const { return 40; }
};

class BAPConFalse : public BAPCondition
{
public:
    virtual bool evaluate(Bot*) { return false; }
    virtual int getConditionType() const { return 6; }
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONDITION_H
