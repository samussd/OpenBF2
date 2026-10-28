// A weighted list of conditions whose sum rates how well a strategy suits the strategic state.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGIC_STRATEGYPREREQUISITE_H
#define AI_AIDLL_AI_AISTRATEGIC_STRATEGYPREREQUISITE_H

#include <list>
#include <string>
#include <utility>

namespace dice {
namespace hfe {
namespace ai {

class StrategyCondition;

class StrategyPrerequisite
{
public:
    explicit StrategyPrerequisite(const std::string& name);
    ~StrategyPrerequisite();

    // Appends; a null condition is ignored. The conditions are not owned.
    void addCondition(StrategyCondition* condition, float weight);
    // Removes and returns the first condition, null when there is none.
    StrategyCondition* popFirstCondition();
    // Sums each condition's weighted compare after its strength filter. A Required or
    // RequiredPositive term below zero, or a RequiredNegative term above zero, makes the whole
    // result 0. Abort conditions count only for the active strategy.
    float evaluate(float* friendly, float* enemy, bool isActiveStrategy, int side);

    /* +0x0 */ std::list<std::pair<StrategyCondition*, float> > m_conditions;
    /* +0x4 */ std::string m_name;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGIC_STRATEGYPREREQUISITE_H
