// The pose part of a pose action: the pose to hold normally, under incoming fire and in danger,
// and when to switch.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACOMPONENTSOLDIERPOSE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACOMPONENTSOLDIERPOSE_H

#include "../../../AIInterface/BFKnowledge/SoldierPose.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class Bot;

class BAPAComponentSoldierPose
{
public:
    BAPAComponentSoldierPose(SoldierPose normalPose, SoldierPose incommingFirePose,
                             float fireObjectsThreshold, SoldierPose dangerPose,
                             float controlThreshold, unsigned int target, float timeDuration);
    virtual ~BAPAComponentSoldierPose();                                    // +0x00 / +0x04

    virtual void setNormalPose(SoldierPose pose);                           // +0x08
    virtual void setIncommingFirePose(SoldierPose pose);                    // +0x0C dice's spelling
    virtual void setFireObjectsThreshold(float threshold);                  // +0x10
    virtual void setDangerPose(SoldierPose pose);                           // +0x14
    virtual void setControlThreshold(float threshold);                      // +0x18
    virtual void setTimeDuration(float duration);                           // +0x1C
    virtual SoldierPose getNormalPose();                                    // +0x20
    virtual SoldierPose getIncommingFirePose() const;                       // +0x24
    virtual float getFireObjectsThreshold() const;                          // +0x28
    virtual SoldierPose getDangerPose() const;                              // +0x2C
    virtual float getControlThreshold() const;                              // +0x30
    virtual float getTimeDuration() const;                                  // +0x34
    virtual SoldierPose getCurrentPose() const;                             // +0x38
    virtual SoldierPose computeCurrentPose(float fire, float control, double time, Bot* bot,
                                           const Vec3& position);           // +0x3C

private:
    /* +0x04 */ SoldierPose m_normalPose;
    /* +0x08 */ SoldierPose m_incommingFirePose;
    // The constructor spreads it at random over [threshold, 6 x threshold).
    /* +0x0C */ float m_fireObjectsThreshold;
    /* +0x10 */ SoldierPose m_dangerPose;
    /* +0x14 */ SoldierPose m_currentPose;          // = the normal pose
    /* +0x18 */ float m_controlThreshold;           // clamped to [-1, 1]
    /* +0x1C */ float m_timeDuration;
    /* +0x20 */ double m_double20;                  // when the pose was last chosen; = -FLT_MAX
    // An information handle, or -1. While it names a soldier whose player has a soldier and that
    // soldier is not standing, the normal pose reads as crouching.
    /* +0x28 */ unsigned int m_target;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACOMPONENTSOLDIERPOSE_H
