#include "BBMoveToFixed.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBMoveToFixed::BBMoveToFixed(unsigned char behaviourId, unsigned char mode)
    : BotBehaviour(0, 0, getNumberOfBots())
{
    m_behaviourId = behaviourId;
    m_targets = 0;
    m_generator = 0;
    m_mode = mode;
    const int numberOfBots = getNumberOfBots();
    m_float24 = new float[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
        m_float24[i] = 0.0f;
    }
}

BBMoveToFixed::~BBMoveToFixed()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMoveToFixed::~BBMoveToFixed");
}

float BBMoveToFixed::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMoveToFixed::calculateUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
