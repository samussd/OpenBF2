#include "BBPDefuseMines.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPDefuseMines::BBPDefuseMines(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
    m_int0C = new int[getNumberOfBots()];
    m_int10 = new int[getNumberOfBots()];
    const int numberOfBots = getNumberOfBots();
    m_vec14 = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec14[i].x = 0.0f;
        m_vec14[i].y = 0.0f;
        m_vec14[i].z = 0.0f;
    }
    m_vec18 = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec18[i].x = 0.0f;
        m_vec18[i].y = 0.0f;
        m_vec18[i].z = 0.0f;
    }
    for (int i = 0; i < getNumberOfBots(); ++i)
    {
        m_int0C[i] = -1;
        m_int10[i] = -1;
    }
}

BBPDefuseMines::~BBPDefuseMines()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPDefuseMines::~BBPDefuseMines");
}

BotScript* BBPDefuseMines::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPDefuseMines::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
