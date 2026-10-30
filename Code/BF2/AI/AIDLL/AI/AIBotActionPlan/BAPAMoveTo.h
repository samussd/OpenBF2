// The base of the move actions: where to go, how fast, when it is finished, the pose to keep on the
// way (the soldier-pose component at +0x14), and the steering the moves share (getTargetDir). The
// work is the interpreter entries'; the statement alone does not run.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETO_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETO_H

#include "BAPAction.h"
#include "BAPAComponentSoldierPose.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;
class Bot;
class Information;

class BAPAMoveTo : public BAPAction, public BAPAComponentSoldierPose
{
public:
    explicit BAPAMoveTo(Bot* bot);
    virtual ~BAPAMoveTo();
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual bool getIsSprinting();                                           // +0x20
    virtual bool getIsMovingAway();                                          // +0x24
    virtual Vec2& getCurrentPoint();                                         // +0x28
    // One step of the move: the point to head for, and [UNRESOLVED] two flags. The base does nothing.
    virtual bool update(Vec2& point, bool& a, bool& b, float c);             // +0x2C
    virtual bool isFinished(Bot* bot);                                       // +0x30
    virtual bool getSecondPoint(Vec2& point);                                // +0x34
    virtual bool getLargestAngleDiff(float& angle);                          // +0x38
    virtual float getTargetDistance();                                       // +0x3C
    virtual unsigned int getObject();                                        // +0x40, -1: a position
    virtual float getSimpleTargetDistance(Bot* bot);                         // +0x44
    virtual float getSimpleTargetDistanceSqr(Bot* bot);                      // +0x48
    virtual bool usesPathfinding() = 0;                                      // +0x4C
    virtual bool withinDistance() = 0;                                       // +0x50
    virtual bool isStuck();                                                  // +0x54

    // Sprinting looks at a position, or with an object at that object (SensingAgent's move target).
    void setIsSprinting(Bot* bot, bool sprint, const Vec3& target, unsigned int object);
    Vec3& getCoordinate();
    float getMaxSpeed();
    float getMaxArriveSpeed();
    // The direction to steer this step, unit length. True: an obstacle on the way took the step over
    // (updateInfantryMoveToPosWithObstacles) or the bot is to keep its last direction.
    bool getTargetDir(Bot* bot, Vec2 target, Vec2& direction, Information* information, bool flag);

protected:
    /* +0x40 */ bool m_isSprinting;
    /* +0x44 */ Vec3 m_coordinate;
    /* +0x50 */ float m_maxSpeed;
    /* +0x54 */ float m_maxArriveSpeed;
    /* +0x58 */ BAPCondition* m_finishCondition;     // owned
    // The path finder found a way ([INFERRED]: the finding moves store initPathfinding's answer).
    // Not set by this constructor.
    /* +0x5C */ bool m_hasPath;
    /* +0x60 */ float m_float60;                     // [UNRESOLVED] = 0
    /* +0x64 */ float m_float64;                     // [UNRESOLVED] = 1
    /* +0x68 */ float m_float68;                     // [UNRESOLVED] = 0
    /* +0x6C */ Vec2 m_currentPoint;                 // = 0; the path's point to head for
    /* +0x74 */ float m_float74;                     // [UNRESOLVED] = 0
    /* +0x78 */ float m_float78;                     // [UNRESOLVED] = 0
    // The obstacle being avoided (an information handle, -1 none) and how. Written by
    // updateInfantryMoveToPosWithObstacles; [INFERRED] names.
    /* +0x80 */ unsigned int m_obstacle;
    /* +0x84 */ Vec2 m_avoidPoint;
    /* +0x8C */ float m_obstacleRadius;              // not set by the constructor
    /* +0x90 */ float m_passDistance;                // not set by the constructor
    /* +0x94 */ Vec2 m_obstaclePosition;             // where the obstacle was
    /* +0x9C */ bool m_obstacleFlag;                 // [UNRESOLVED] not set by the constructor
    // getTargetDir's cache: the time it holds to (-1000 at first; each call adds 0 s, so it holds
    // only within one game time), the direction and the answer.
    /* +0xA0 */ double m_nextDirTime;
    /* +0xA8 */ Vec2 m_lastDir;
    /* +0xB0 */ bool m_lastDirResult;                // not set by the constructor
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMOVETO_H
