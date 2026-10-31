// A move to an object. Finished when the finish condition holds or the bot is within the radius.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOOBJECT_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOOBJECT_H

#include "BAPAMoveTo.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAMoveToObject : public BAPAMoveTo
{
public:
    BAPAMoveToObject(Bot* bot, unsigned int object, float radius, float maxSpeed, float maxArriveSpeed,
                     BAPCondition* finishCondition, unsigned long long controls, int planId);
    virtual ~BAPAMoveToObject();
    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual void streamSelf(std::ostream& out) const;                        // [FRONTIER]
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual bool isFinished(Bot* bot);
    virtual unsigned int getObject();
    virtual float getSimpleTargetDistance(Bot* bot);
    virtual float getSimpleTargetDistanceSqr(Bot* bot);

protected:
    /* +0xB4 */ unsigned int m_object;
    /* +0xB8 */ float m_radius;
    /* +0xBC */ float m_radiusSqr;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOOBJECT_H
