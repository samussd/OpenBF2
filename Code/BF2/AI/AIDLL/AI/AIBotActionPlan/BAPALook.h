// Where a bot should look. The statement does nothing by itself: the interpreter entry
// (EntryMouseTurretLookAt, statement type 23) asks it for a direction each tick and turns the
// view towards it until the condition holds.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOK_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOK_H

#include "BAPAction.h"

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPALook : public BAPAction
{
public:
    // [INFERRED] the float's meaning; every recovered caller passes 0.
    BAPALook(float f, BAPCondition* until, unsigned long long controls, int planId);
    // Leaves the condition alone: each subclass deletes it.
    virtual ~BAPALook();

    // Refuses to run without an entry: ok = false, done = true.
    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    // The direction to look in, world space; valid is false when there is none.
    virtual Vec3 getLookDirection(bool& valid) = 0;                // +0x20

    /* +0x14 */ BAPCondition* m_until;
    /* +0x18 */ float m_float24;          // the constructor's float
    /* +0x1C */ float m_float28;          // ctor: 0
    /* +0x20 */ float m_float32;          // ctor: 0
    /* +0x24 */ Vec3  m_vec36;            // ctor: 0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOK_H
