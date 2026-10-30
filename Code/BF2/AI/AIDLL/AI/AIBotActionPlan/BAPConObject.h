// The base of the conditions on one object. [INFERRED] header: the class has no out-of-line
// function in the binary, only its RTTI (a BAPCondition) and its derived conditions.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECT_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECT_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConObject : public BAPCondition
{
public:
    explicit BAPConObject(unsigned int object) : m_object(object) {}

    /* +0x08 */ unsigned int m_object;      // an information handle
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECT_H
