#include "BBPBridgeRepair.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPBridgeRepair::BBPBridgeRepair(int behaviourId)
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
    m_bool18 = new bool[numberOfBots];
    m_vec1C = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec1C[i].x = 0.0f;
        m_vec1C[i].y = 0.0f;
        m_vec1C[i].z = 0.0f;
    }
    m_vec20 = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec20[i].x = 0.0f;
        m_vec20[i].y = 0.0f;
        m_vec20[i].z = 0.0f;
    }
    for (int i = 0; i < getNumberOfBots(); ++i)
    {
        m_int0C[i] = -1;
        m_int10[i] = -1;
        m_bool18[i] = false;
    }
}

BBPBridgeRepair::~BBPBridgeRepair()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPBridgeRepair::~BBPBridgeRepair");
}

BotScript* BBPBridgeRepair::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPBridgeRepair::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
