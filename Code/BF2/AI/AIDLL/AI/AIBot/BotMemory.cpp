// [INFERRED] Source placement.

#include "BotMemory.h"

#include "../AIBotBehaviour/BotBehaviour.h"
#include "../AIBotBehaviour/UrgeCurve.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes behavior-memory sentinels. [UNRESOLVED] The meanings of +4, +8 and +12 remain unknown.
BotMemory::BotMemory()
    : m_currentBehaviour(-1)
    , m_int4(-1)
    , m_float8(1.0f)
    , m_int12(1)
{
    // m_behaviourTime is deliberately not initialised here. `BotMain`'s constructor writes it from
    // outside, immediately after allocating this object. Adding it would be an invented guard.
}

// Intentionally empty destructor.
BotMemory::~BotMemory()
{
}

void BotMemory::reset()
{
    m_int4 = -1;
    m_int12 = 1;
    m_float8 = 1.0f;
    m_currentBehaviour = -1;
}

// 0x19f2d0 (Ghidra).
float BotMemory::modifyBehaviourUrgency(BotBehaviour* behaviour, double time)
{
    if (behaviour->getBehaviourId() != m_currentBehaviour)
        return 1.0f;
    return behaviour->getUrgeCurve()->calculate(static_cast<float>(time - m_behaviourTime));
}

} // namespace ai
} // namespace hfe
} // namespace dice
