// True while the object's health is at or below a value. A gone object is not.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTHEALTH_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTHEALTH_H

#include "BAPConObject.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConObjectHealth : public BAPConObject
{
public:
    BAPConObjectHealth(unsigned int object, float health);
    virtual ~BAPConObjectHealth();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 22; }

private:
    /* +0x0C */ float m_health;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTHEALTH_H
