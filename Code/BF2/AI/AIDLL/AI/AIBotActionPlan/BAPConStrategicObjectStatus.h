// Tests whether a strategic object has a status for a side, or with negate has none.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONSTRATEGICOBJECTSTATUS_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONSTRATEGICOBJECTSTATUS_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class AIStrategicObject;

class BAPConStrategicObjectStatus : public BAPCondition
{
public:
    BAPConStrategicObjectStatus(AIStrategicObject* object, int side, bool negate);
    virtual ~BAPConStrategicObjectStatus();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 16; }

private:
    /* +0x08 */ AIStrategicObject* m_object;
    /* +0x0C */ int m_side;
    /* +0x10 */ bool m_negate;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONSTRATEGICOBJECTSTATUS_H
