#include "BBPAvoidCollision.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPAvoidCollision::BBPAvoidCollision()
    : BBPPattern()
{
    const int numberOfBots = getNumberOfBots();
    m_obstacle = new unsigned int[numberOfBots];
    m_relativeVelocity = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_relativeVelocity[i].x = 0.0f;
        m_relativeVelocity[i].y = 0.0f;
        m_relativeVelocity[i].z = 0.0f;
    }
    m_time = new float[numberOfBots];
}

BBPAvoidCollision::~BBPAvoidCollision()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision::~BBPAvoidCollision");
}

} // namespace ai
} // namespace hfe
} // namespace dice
