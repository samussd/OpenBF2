// Aim-correction state initialization.

#include "BotSubObjects.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes aim-correction state while leaving byte ranges +0..+3 and +68..+83
// untouched. [UNRESOLVED] The contents of those ranges remain unknown.
FireCorrectionData::FireCorrectionData()
    : m_flag4(true)
    , m_flag5(true)
    , m_flag84(false)
{
    // +0..+3 and +68..+83 are deliberately untouched - see above.
    for (int i = 0; i < 15; ++i)
        m_zero8[i] = 0;
    m_projectilePre = Vec3::getZero();
    m_projectilePost = Vec3::getZero();
    m_projectileTarget = Vec3::getZero();
}

void FireCorrectionData::setProjectile(const Vec3& pre, const Vec3& post, const Vec3& target,
                                       float maxHeight)
{
    m_projectilePre = pre;
    m_projectilePost = post;
    m_projectileTarget = target;
    m_projectileMaxHeight = maxHeight;
    m_flag5 = true;
    m_flag84 = true;
    m_flag85 = true;
}


} // namespace ai
} // namespace hfe
} // namespace dice
