// True when the object has moved more than a distance since the last time the condition was
// true. The first test measures from the world origin.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTMOVED_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTMOVED_H

#include "BAPConObject.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConObjectMoved : public BAPConObject
{
public:
    BAPConObjectMoved(unsigned int object, float distance);
    virtual ~BAPConObjectMoved();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 28; }

private:
    /* +0x0C */ Vec3 m_lastPosition;
    /* +0x18 */ float m_distanceSqr;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTMOVED_H
