// [INFERRED] Source placement.

#include "StrategyPrerequisite.h"

#include "StrategyCondition.h"
#include "../Main/StrategyManager.h"

namespace dice {
namespace hfe {
namespace ai {

// The constructor clears the list it has just built, as shipped.
StrategyPrerequisite::StrategyPrerequisite(const std::string& name)
    : m_name(name)
{
    m_conditions.clear();
}

StrategyPrerequisite::~StrategyPrerequisite()
{
}

void StrategyPrerequisite::addCondition(StrategyCondition* condition, float weight)
{
    if (condition)
        m_conditions.push_back(std::make_pair(condition, weight));
}

StrategyCondition* StrategyPrerequisite::popFirstCondition()
{
    if (m_conditions.empty())
        return 0;
    StrategyCondition* const first = m_conditions.front().first;
    m_conditions.pop_front();
    return first;
}

float StrategyPrerequisite::evaluate(float* friendly, float* enemy, bool isActiveStrategy, int side)
{
    float sum = 0.0f;
    for (std::list<std::pair<StrategyCondition*, float> >::iterator it = m_conditions.begin();
         it != m_conditions.end(); ++it)
    {
        if (!isActiveStrategy && it->first->isAbortCondition())
            continue;
        const float value = it->first->compare(friendly, enemy, side) * it->second;
        float term = value;
        switch (it->first->getConditionStrength())
        {
        case StrategyCondition::Required:
        case StrategyCondition::RequiredPositive:
            if (value < 0.0f)
                return 0.0f;
            break;
        case StrategyCondition::RequiredNegative:
            if (value > 0.0f)
                return 0.0f;
            break;
        case StrategyCondition::AdvisoryPositive:
            if (value < 0.0f)
                term = 0.0f;
            break;
        case StrategyCondition::AdvisoryNegative:
            if (value > 0.0f)
                term = 0.0f;
            break;
        default:
            break;
        }
        sum = term + sum;
    }
    StrategyManager::getInstance()->isDebugEnabled();
    return sum;
}

} // namespace ai
} // namespace hfe
} // namespace dice
