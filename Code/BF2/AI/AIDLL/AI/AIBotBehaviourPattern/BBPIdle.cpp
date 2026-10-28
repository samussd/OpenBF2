#include "BBPIdle.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPIdle::BBPIdle(int behaviourId)
    : BBPPattern()
{
    m_behaviourId = behaviourId;
}

BBPIdle::~BBPIdle()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdle::~BBPIdle");
}

int BBPIdle::getType()
{
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
