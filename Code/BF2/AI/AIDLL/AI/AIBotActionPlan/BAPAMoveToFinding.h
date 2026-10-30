// A move to a position along the path the bot's navigation finds. Finished when the finish
// condition holds or the bot is between the two radii of the position.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOFINDING_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOFINDING_H

#include "BAPAMoveTo.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAMoveToFinding : public BAPAMoveTo
{
public:
    BAPAMoveToFinding(Bot* bot, const Vec3& position, float maxSpeed, float maxArriveSpeed,
                      BAPCondition* finishCondition, unsigned long long controls, int planId,
                      float radius, float minRadius);
    virtual ~BAPAMoveToFinding();
    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual void streamSelf(std::ostream& out) const;                        // [FRONTIER]
    virtual bool update(Vec2& point, bool& a, bool& b, float c);             // [FRONTIER]
    virtual bool isFinished(Bot* bot);
    virtual bool getSecondPoint(Vec2& point);
    virtual bool getLargestAngleDiff(float& angle);
    virtual float getTargetDistance();
    virtual bool usesPathfinding();
    virtual bool withinDistance();
    virtual bool isStuck();

private:
    /* +0xB4 */ float m_floatB4[8];                  // [UNRESOLVED] +0xB4..+0xD0, = 0
    /* +0xD4 */ bool m_boolD4;                       // [UNRESOLVED] = false
    /* +0xD5 */ bool m_boolD5;                       // [UNRESOLVED] = false
    /* +0xD8 */ int m_intD8;                         // [UNRESOLVED] = 1
    /* +0xDC */ Bot* m_bot;
    /* +0xE0 */ float m_radius;
    /* +0xE4 */ float m_minRadius;
    /* +0xE8 */ float m_radiusSqr;
    /* +0xEC */ float m_minRadiusSqr;
    /* +0xF0 */ Vec2 m_validPoint;                   // [INFERRED] update: a valid position near the target
    /* +0xF8 */ bool m_boolF8;                       // [UNRESOLVED] updatePath's second flag
    /* +0xF9 */ bool m_pathStarted;                  // [INFERRED] update: initPathfinding was asked
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOFINDING_H
