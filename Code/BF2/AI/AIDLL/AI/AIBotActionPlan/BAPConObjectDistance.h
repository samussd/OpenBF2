// Compares the bot's distance to an object with a range from zero. It can use the object's
// estimated position, ignore height, or be true outside the range instead of inside it.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTDISTANCE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTDISTANCE_H

#include "BAPConObject.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConObjectDistance : public BAPConObject
{
public:
    BAPConObjectDistance(unsigned int object, float distance, bool estimated, bool flat, bool outside);
    virtual ~BAPConObjectDistance();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 18; }

private:
    /* +0x0C */ float m_minSqr;
    /* +0x10 */ float m_maxSqr;
    /* +0x14 */ bool m_estimated;
    /* +0x15 */ bool m_flat;
    /* +0x16 */ bool m_outside;
    /* +0x18 */ float m_lastSqr;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONOBJECTDISTANCE_H
