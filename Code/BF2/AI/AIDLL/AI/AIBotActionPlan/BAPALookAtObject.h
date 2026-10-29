// Look at an object, by handle.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOKATOBJECT_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOKATOBJECT_H

#include "BAPALook.h"

namespace dice {
namespace hfe {
namespace ai {

// Retail spells the type BAPVariable<Vec3>; the template is not recovered and every recovered
// caller passes 0.
class BAPVariableVec3;

class BAPALookAtObject : public BAPALook
{
public:
    BAPALookAtObject(Bot* bot, unsigned int target, float f, BAPCondition* until,
                     unsigned long long controls, int planId, BAPVariableVec3* variable,
                     float f2);
    virtual ~BAPALookAtObject();

    virtual Vec3 getLookDirection(bool& valid);

    /* +0x30 */ Bot*             m_bot;
    /* +0x34 */ unsigned int     m_target;
    /* +0x38 */ BAPVariableVec3* m_variable;
    /* +0x3C */ float            m_float60;      // the constructor's last float
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOKATOBJECT_H
