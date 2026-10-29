#include "BAPALookAtObject.h"

#include "BAPCondition.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BAPALookAtObject::BAPALookAtObject(Bot* bot, unsigned int target, float f, BAPCondition* until,
                                   unsigned long long controls, int planId,
                                   BAPVariableVec3* variable, float f2)
    : BAPALook(f, until, controls, planId)
    , m_bot(bot)
    , m_target(target)
    , m_variable(variable)
    , m_float60(f2)
{
    m_until->m_owner = this;
}

BAPALookAtObject::~BAPALookAtObject()
{
    if (m_until)
        delete m_until;
}

// [FRONTIER] [TEMP] [RETIRE-WHEN: aidll-02-skeleton] 608 bytes, read with the look entry.
Vec3 BAPALookAtObject::getLookDirection(bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPALookAtObject::getLookDirection");
}

} // namespace ai
} // namespace hfe
} // namespace dice
