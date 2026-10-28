#include "BBFireUnarmed.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBFireUnarmed::BBFireUnarmed(unsigned char behaviourId, unsigned char mode)
    : BotBehaviour(0, 0, getNumberOfBots())
{
    m_behaviourId = behaviourId;
    m_mode = mode;
    const int numberOfBots = getNumberOfBots();
    m_float24 = new float[numberOfBots];
    m_generator = 0;
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
        m_float24[i] = 0.0f;
        m_targets[i] = -1;
    }
}

BBFireUnarmed::~BBFireUnarmed()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFireUnarmed::~BBFireUnarmed");
}

float BBFireUnarmed::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFireUnarmed::calculateUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
