// [INFERRED] Source placement.

#include "TemperatureTree.h"

namespace dice {
namespace hfe {
namespace ai {

TemperatureTreeModifiers::TemperatureTreeModifiers()
{
    reset();
}

void TemperatureTreeModifiers::reset()
{
    for (unsigned int i = 0; i < 41; ++i)
        m_node[i] = 1.0f;
    for (unsigned int row = 0; row < 56; ++row)
        for (unsigned int type = 0; type < 6; ++type)
            m_combatStrength[row][type] = 1.0f;
}

} // namespace ai
} // namespace hfe
} // namespace dice
