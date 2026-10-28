// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining UrgencyMerger operations.

#include "UrgencyMerger.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// one instruction in the binary - the vptr store C++ emits on its own. The class owns nothing
// which is what makes it a pure interface rather than a partial implementation.
UrgencyGenerator::~UrgencyGenerator()
{
}

// Four stores after the vptr. `m_weights` is whatever the caller passed and BotManager passes
// null, so a merger from the bot pool has no weight array at all - whatever `getUrgency` does with
// that is the next thing to read, and it is not guessed at here.
UrgencyMerger::UrgencyMerger(int count, float* weights, UrgencyGenerator** generators)
    : m_count(count)
    , m_weights(weights)
    , m_generators(generators)
    , m_chosen(0)
{
}

UrgencyMerger::~UrgencyMerger()
{
    BF2_AI_FRONTIER("dice::hfe::ai::UrgencyMerger::~UrgencyMerger");
}

// [FRONTIER] Bot-specific urgency and modifier operations remain unimplemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace each stop when the corresponding merger behavior is recovered.

float UrgencyMerger::getUrgency(Bot* bot)
{
    return m_chosen ? m_chosen->getUrgency(bot) : 0.0f;
}

float UrgencyMerger::getActiveUrgency(Bot* bot)
{
    return m_chosen ? m_chosen->getActiveUrgency(bot) : 0.0f;
}

// The child with the highest positive urgency is chosen; the first one wins a tie.
float UrgencyMerger::updateActiveUrgency(Bot* bot)
{
    float best = 0.0f;
    m_chosen = 0;
    for (int i = 0; i < m_count; ++i)
    {
        const float urgency = m_generators[i] ? m_generators[i]->updateActiveUrgency(bot) : 0.0f;
        if (best < urgency)
        {
            best = urgency;
            m_chosen = m_generators[i]->getChosenGenerator();
        }
    }
    return best;
}

// Every child present must have changed target.
bool UrgencyMerger::hasChangedTarget(Bot* bot)
{
    bool changed = true;
    for (int i = 0; changed && i < m_count; ++i)
    {
        if (m_generators[i])
            changed = m_generators[i]->hasChangedTarget(bot);
    }
    return changed;
}

// Every child present must report a quotient change.
bool UrgencyMerger::quotientChanged(Bot* bot, float low, float high)
{
    bool changed = true;
    for (int i = 0; changed && i < m_count; ++i)
    {
        if (m_generators[i])
            changed = m_generators[i]->quotientChanged(bot, low, high);
    }
    return changed;
}

// Any child that changed its quotient or its target is enough - unlike BotBehaviour::hasChanged,
// which wants both.
bool UrgencyMerger::hasChanged(Bot* bot, float low, float high)
{
    bool changed = false;
    for (int i = 0; !changed && i < m_count; ++i)
    {
        if (m_generators[i])
            changed = m_generators[i]->quotientChanged(bot, low, high) ||
                      m_generators[i]->hasChangedTarget(bot);
    }
    return changed;
}

UrgencyGenerator* UrgencyMerger::getChosenGenerator()
{
    return m_chosen;
}

void UrgencyMerger::reset(Bot* bot)
{
    for (int i = 0; i < m_count; ++i)
    {
        if (m_generators[i])
            m_generators[i]->reset(bot);
    }
}

int UrgencyMerger::getSize()
{
    return m_count;
}

} // namespace ai
} // namespace hfe
} // namespace dice
