#include "BBPAvoidCollision3d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPAvoidCollision3d::BBPAvoidCollision3d(int behaviourId)
    : BBPAvoidCollision()
{
    m_behaviourId = behaviourId;
    const int numberOfBots = getNumberOfBots();
    m_vec1C = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec1C[i].x = 0.0f;
        m_vec1C[i].y = 0.0f;
        m_vec1C[i].z = 0.0f;
    }
}

BBPAvoidCollision3d::~BBPAvoidCollision3d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision3d::~BBPAvoidCollision3d");
}

BotScript* BBPAvoidCollision3d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision3d::createPlan");
}

void BBPAvoidCollision3d::init(Bot*, Vec3&, unsigned int, Vec3&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision3d::init");
}

} // namespace ai
} // namespace hfe
} // namespace dice
