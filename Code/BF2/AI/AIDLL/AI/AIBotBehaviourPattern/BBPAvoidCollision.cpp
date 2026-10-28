#include "BBPAvoidCollision.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPAvoidCollision::BBPAvoidCollision()
    : BBPPattern()
{
    const int numberOfBots = getNumberOfBots();
    m_int0C = new int[numberOfBots];
    m_vec10 = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec10[i].x = 0.0f;
        m_vec10[i].y = 0.0f;
        m_vec10[i].z = 0.0f;
    }
    m_int14 = new int[numberOfBots];
}

BBPAvoidCollision::~BBPAvoidCollision()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision::~BBPAvoidCollision");
}

} // namespace ai
} // namespace hfe
} // namespace dice
