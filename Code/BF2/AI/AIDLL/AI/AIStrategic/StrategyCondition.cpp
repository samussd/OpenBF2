// [INFERRED] Source placement.
// Each compare queries isDebugEnabled and discards it, as shipped; the debug output is gone.

#include "StrategyCondition.h"

#include <cmath>

#include "../Main/StrategyManager.h"

namespace dice {
namespace hfe {
namespace ai {

StrategyCondition::StrategyCondition(const std::string& name, float targetValue, Compare compare,
                                     ConditionSide side, CompareType compareType,
                                     Conditional conditional)
    : m_isAbortCondition(false)
    , m_targetValue(targetValue)
    , m_compare(compare)
    , m_compareType(compareType)
    , m_side(side)
    , m_conditional(conditional)
    , m_strength(Required)
    , m_name(name)
{
}

StrategyCondition::~StrategyCondition()
{
}

int StrategyCondition::getType() const
{
    return 4;
}

void StrategyCondition::setTargetValue(float value)
{
    m_targetValue = value;
}

float StrategyCondition::getTargetValue() const
{
    return m_targetValue;
}

void StrategyCondition::setConditionStrength(ConditionStrength strength)
{
    m_strength = strength;
}

StrategyCondition::ConditionStrength StrategyCondition::getConditionStrength() const
{
    return m_strength;
}

bool StrategyCondition::setIsAbortCondition(bool on)
{
    m_isAbortCondition = on;
    return on;
}

bool StrategyCondition::getIsAbortCondition() const
{
    return m_isAbortCondition;
}

bool StrategyCondition::isAbortCondition() const
{
    return m_isAbortCondition;
}

float StrategyCondition::internalCompare(float value, float reference) const
{
    switch (m_compareType)
    {
    case Crisp:
        return internalCompareCrisp(value, reference);
    case Fuzzy:
        return internalCompareFuzzy(value, reference);
    case FuzzySqr:
    {
        const float fuzzy = internalCompareFuzzy(value, reference);
        float sign = 0.0f;
        if (fuzzy > 0.0f)
            sign = 1.0f;
        else if (fuzzy < 0.0f)
            sign = -1.0f;
        return fuzzy * fuzzy * sign;
    }
    default:
        return 0.0f;
    }
}

// The Difference forms compute exactly what the Equal forms do. A NaN fails every comparison.
float StrategyCondition::internalCompareCrisp(float value, float reference) const
{
    switch (m_compare)
    {
    case Equal:
    case Difference:
        return m_targetValue == value - reference ? 1.0f : -1.0f;
    case EqualGreater:
    case DifferenceGreater:
        return value - reference >= m_targetValue ? 1.0f : -1.0f;
    case EqualSmaller:
    case DifferenceSmaller:
        return value - reference <= m_targetValue ? 1.0f : -1.0f;
    case Quotient:
        if (reference == 0.0f)
            return -1.0f;
        return m_targetValue == value / reference ? 1.0f : -1.0f;
    case QuotientGreater:
        if (reference == 0.0f)
            return 1.0f;
        return value / reference >= m_targetValue ? 1.0f : -1.0f;
    case QuotientSmaller:
        if (reference == 0.0f)
            return -1.0f;
        return value / reference <= m_targetValue ? 1.0f : -1.0f;
    default:
        return 0.0f;
    }
}

float StrategyCondition::internalCompareFuzzy(float value, float reference) const
{
    switch (m_compare)
    {
    case Equal:
    case Difference:
        return -std::fabs(value - reference - m_targetValue);
    case EqualGreater:
    case DifferenceGreater:
        return value - reference - m_targetValue;
    case EqualSmaller:
    case DifferenceSmaller:
        return -(value - reference - m_targetValue);
    case Quotient:
        if (reference == 0.0f)
            return -1000.0f;
        return -std::fabs(value / reference - m_targetValue);
    case QuotientGreater:
        if (reference == 0.0f)
            return 1000.0f;
        return value / reference - m_targetValue;
    case QuotientSmaller:
        if (reference == 0.0f)
            return -1000.0f;
        return -(value / reference - m_targetValue);
    default:
        return 0.0f;
    }
}

SCConstant::SCConstant(const std::string& name, CompareType compareType, Compare compare,
                       ConditionSide side, Conditional conditional, float constant,
                       float targetValue)
    : StrategyCondition(name, targetValue, compare, side, compareType, conditional)
    , m_constant(constant)
{
}

int SCConstant::getType() const
{
    return 0;
}

float SCConstant::compare(float* friendly, float* enemy, int) const
{
    const float* const values = m_side == Friendly ? friendly : enemy;
    const float result = internalCompare(values[m_conditional], m_constant);
    StrategyManager::getInstance()->isDebugEnabled();
    return result;
}

SCHomogeneous::SCHomogeneous(const std::string& name, CompareType compareType, Compare compare,
                             Conditional conditional, float targetValue)
    : StrategyCondition(name, targetValue, compare, Both, compareType, conditional)
{
}

int SCHomogeneous::getType() const
{
    return 1;
}

float SCHomogeneous::compare(float* friendly, float* enemy, int) const
{
    const float* first;
    const float* second;
    if (m_side == Friendly)
    {
        first = friendly;
        second = friendly;
    }
    else if (m_side == Enemy)
    {
        first = enemy;
        second = enemy;
    }
    else if (m_side == Both)
    {
        first = friendly;
        second = enemy;
    }
    else
    {
        return 0.0f;
    }
    const float result = internalCompare(first[m_conditional], second[m_conditional]);
    StrategyManager::getInstance()->isDebugEnabled();
    return result;
}

SCHeterogeneous::SCHeterogeneous(const std::string& name, CompareType compareType,
                                 Compare compare, ConditionSide side, Conditional conditional,
                                 Conditional other, float targetValue)
    : StrategyCondition(name, targetValue, compare, side, compareType, conditional)
    , m_other(other)
{
}

int SCHeterogeneous::getType() const
{
    return 2;
}

float SCHeterogeneous::compare(float* friendly, float* enemy, int) const
{
    const float* first;
    const float* second;
    if (m_side == Friendly)
    {
        first = friendly;
        second = friendly;
    }
    else if (m_side == Enemy)
    {
        first = enemy;
        second = enemy;
    }
    else if (m_side == Both)
    {
        first = friendly;
        second = enemy;
    }
    else
    {
        return 0.0f;
    }
    const float result = internalCompare(first[m_conditional], second[m_other]);
    StrategyManager::getInstance()->isDebugEnabled();
    return result;
}

} // namespace ai
} // namespace hfe
} // namespace dice
