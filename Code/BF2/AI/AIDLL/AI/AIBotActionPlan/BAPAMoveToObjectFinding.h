// A move to an object along the path the bot's navigation finds. The path follows the object, and a
// bot closer than the move-away radius first walks clear of it.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOOBJECTFINDING_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOOBJECTFINDING_H

#include "BAPAMoveToObject.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAMoveToObjectFinding : public BAPAMoveToObject
{
public:
    BAPAMoveToObjectFinding(Bot* bot, unsigned int object, float radius, float maxSpeed, float maxArriveSpeed,
                            BAPCondition* finishCondition, unsigned long long controls, int planId,
                            BAPCondition* moveCondition, float moveAwayRadius, int intParameter);
    virtual ~BAPAMoveToObjectFinding();
    virtual void streamSelf(std::ostream& out) const;                        // [FRONTIER]
    virtual bool update(Vec2& point, bool& a, bool& b, float c);
    virtual bool isFinished(Bot* bot);
    virtual bool getIsMovingAway();
    virtual bool getLargestAngleDiff(float& angle);
    virtual float getTargetDistance();
    virtual bool usesPathfinding();
    virtual bool withinDistance();

private:
    // Starts the search: to the object, or from `distance` under the move-away radius to a point
    // walked clear of it.
    void initObjectFinding(float distance, const Vec3& direction, const Vec3& from, unsigned int quadTree,
                           unsigned int polygon, const Vec3& target, bool direct);

    /* +0xC0 */ bool m_searching;                    // initPathfinding's answer
    /* +0xC8 */ float m_floatC8[8];                  // [UNRESOLVED] +0xC8..+0xE4, = 0
    /* +0xE9 */ bool m_boolE9;                       // [UNRESOLVED] = false
    /* +0xEA */ bool m_boolEA;                       // [UNRESOLVED] not set by the constructor
    /* +0xEC */ int m_intEC;                         // [UNRESOLVED] = 1
    /* +0xF0 */ Bot* m_bot;
    /* +0xF4 */ BAPCondition* m_moveCondition;       // owned; held: the object has moved
    /* +0xF8 */ bool m_moveConditionHeld;            // = true
    /* +0xFC */ float m_moveAwayRadius;              // at most the radius
    /* +0x100 */ float m_moveAwayRadiusSqr;
    /* +0x104 */ bool m_arrived;                     // updatePath: at the end of the path
    /* +0x105 */ bool m_movingAway;
    /* +0x106 */ bool m_stepped;                     // the last update stepped along a path
    /* +0x107 */ bool m_followedSighting;            // the object was sensed and the path set for it
    /* +0x108 */ int m_intParameter;                 // [UNRESOLVED] the last constructor argument
    /* +0x10C */ double m_searchTime;                // = -1
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETOOBJECTFINDING_H
