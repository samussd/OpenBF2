// Look along a fixed world direction.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOKINDIR_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOKINDIR_H

#include "BAPALook.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPALookInDir : public BAPALook
{
public:
    BAPALookInDir(const Vec3& direction, float f, BAPCondition* until,
                  unsigned long long controls, int planId);
    virtual ~BAPALookInDir();

    // Always valid.
    virtual Vec3 getLookDirection(bool& valid);

    /* +0x30 */ Vec3 m_direction;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPALOOKINDIR_H
