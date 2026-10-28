// [INFERRED] Source placement.

#include "Strategy.h"

#include "StrategyPrerequisite.h"
#include "../Main/StrategyManager.h"

namespace dice {
namespace hfe {
namespace ai {

// The modifier table is not passed through checkPointer.
Strategy::Strategy(const std::string& name)
    : m_treeModifiers(new TemperatureTreeModifiers())
    , m_aggression(0.5f)
    , m_abortLevel(0.0f)
    , m_prerequisiteWeight(1.0f)
    , m_timeLimit(1000000000.0f)
    , m_prerequisite(0)
    , m_abortCondition(0)
    , m_name(name)
{
    clearAllStrategicObjectsModifiers();
    m_specificStrategicObjectModifiers.resize(0, 0.0f);
}

Strategy::~Strategy()
{
    operator delete(m_treeModifiers);
}

TemperatureTreeModifiers* Strategy::getTemperatureTreeModifiers() const
{
    return m_treeModifiers;
}

void Strategy::setTemperatureTreeModifiers(TemperatureTreeModifiers* modifiers)
{
    m_treeModifiers = modifiers;
}

float Strategy::getAggression() const
{
    return m_aggression;
}

void Strategy::setAggression(float aggression)
{
    m_aggression = aggression;
}

unsigned int Strategy::getNumberOfAttacks() const
{
    return m_numberOfAttacks;
}

void Strategy::setNumberOfAttacks(unsigned int count)
{
    m_numberOfAttacks = count;
}

unsigned int Strategy::getNumberOfAttackConsiderations() const
{
    return m_numberOfAttackConsiderations;
}

void Strategy::setNumberOfAttackConsiderations(unsigned int count)
{
    m_numberOfAttackConsiderations = count;
}

unsigned int Strategy::getNumberOfDefences() const
{
    return m_numberOfDefences;
}

void Strategy::setNumberOfDefences(unsigned int count)
{
    m_numberOfDefences = count;
}

unsigned int Strategy::getNumberOfDefenceConsiderations() const
{
    return m_numberOfDefenceConsiderations;
}

void Strategy::setNumberOfDefenceConsiderations(unsigned int count)
{
    m_numberOfDefenceConsiderations = count;
}

void Strategy::setPrerequisite(StrategyPrerequisite* prerequisite)
{
    m_prerequisite = prerequisite;
}

void Strategy::setAbortCondition(StrategyPrerequisite* abortCondition)
{
    m_abortCondition = abortCondition;
}

float Strategy::calculatePrerequisite(float* friendly, float* enemy, Strategy* active, int side)
{
    StrategyManager::getInstance()->isDebugEnabled();
    if ((m_requiredPrecedingStrategies.empty() ||
         m_requiredPrecedingStrategies.find(active) != m_requiredPrecedingStrategies.end()) &&
        (m_prohibitedPrecedingStrategies.empty() ||
         m_prohibitedPrecedingStrategies.find(active) == m_prohibitedPrecedingStrategies.end()))
    {
        return m_prerequisite->evaluate(friendly, enemy, active == this, side) *
               m_prerequisiteWeight;
    }
    return 0.0f;
}

float Strategy::calculateAbortCondition(float* friendly, float* enemy, Strategy* active, int side)
{
    return m_abortLevel - m_abortCondition->evaluate(friendly, enemy, active == this, side);
}

void Strategy::setAbortLevel(float level)
{
    m_abortLevel = level;
}

float Strategy::getAbortLevel() const
{
    return m_abortLevel;
}

void Strategy::setPrerequisiteWeight(float weight)
{
    m_prerequisiteWeight = weight;
}

float Strategy::getPrerequisiteWeight() const
{
    return m_prerequisiteWeight;
}

void Strategy::setTreeModifier(TemperatureTreeNode node, float value)
{
    if (static_cast<unsigned int>(node) < 41)
        m_treeModifiers->m_node[node] = value;
}

float Strategy::getTreeModifier(TemperatureTreeNode node) const
{
    return m_treeModifiers->m_node[node];
}

// The all-types branch for type 6 is unreachable behind the range test, as shipped.
void Strategy::setTreeModifier(TemperatureTreeNode node, CombatStrengthType type, float value)
{
    if (static_cast<unsigned int>(node) < 41 && type >= 0 && type < 6)
    {
        if (type == 6)
        {
            for (int i = 0; i < 6; ++i)
                m_treeModifiers->m_combatStrength[node][i] = value;
        }
        else
        {
            m_treeModifiers->m_combatStrength[node][type] = value;
        }
    }
}

float Strategy::getTreeModifier(TemperatureTreeNode node, CombatStrengthType type) const
{
    return m_treeModifiers->m_combatStrength[node][type];
}

// The same body as the three-argument setTreeModifier.
void Strategy::setTreeModifierCombatStrength(TemperatureTreeNode node, CombatStrengthType type,
                                             float value)
{
    if (static_cast<unsigned int>(node) < 41 && type >= 0 && type < 6)
    {
        if (type == 6)
        {
            for (int i = 0; i < 6; ++i)
                m_treeModifiers->m_combatStrength[node][i] = value;
        }
        else
        {
            m_treeModifiers->m_combatStrength[node][type] = value;
        }
    }
}

float Strategy::getTreeModifierCombatStrength(TemperatureTreeNode node,
                                              CombatStrengthType type) const
{
    return m_treeModifiers->m_combatStrength[node][type];
}

bool Strategy::addRequiredPrecedingStrategy(Strategy* strategy)
{
    if (!strategy)
        return false;
    return m_requiredPrecedingStrategies.insert(strategy).second;
}

bool Strategy::addProhibitedPrecedingStrategy(Strategy* strategy)
{
    if (!strategy)
        return false;
    return m_prohibitedPrecedingStrategies.insert(strategy).second;
}

void Strategy::setStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type, float value)
{
    for (int status = 0; status < 3; ++status)
        m_strategicObjectsModifiers[status][type] = value;
}

float Strategy::getStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type) const
{
    float sum = 0.0f;
    for (int status = 0; status < 3; ++status)
        sum += m_strategicObjectsModifiers[status][type];
    return sum / 3.0f;
}

void Strategy::setStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type, float value,
                                           StatusMap::Status status)
{
    m_strategicObjectsModifiers[status][type] = value;
}

float Strategy::getStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type,
                                            StatusMap::Status status) const
{
    return m_strategicObjectsModifiers[status][type];
}

void Strategy::setSpecificStrategicObjectModifier(int index, float value)
{
    if (index < 0)
        return;
    if (index >= static_cast<int>(m_specificStrategicObjectModifiers.size()))
        m_specificStrategicObjectModifiers.resize(index + 1, 1.0f);
    m_specificStrategicObjectModifiers[index] = value;
}

float Strategy::getSpecificStrategicObjectModifier(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_specificStrategicObjectModifiers.size()))
        return m_specificStrategicObjectModifiers[index];
    return 1.0f;
}

const float* Strategy::getStrategicObjectsModifiers(StatusMap::Status status) const
{
    return m_strategicObjectsModifiers[status];
}

void Strategy::clearAllStrategicObjectsModifiers()
{
    for (int type = 0; type < 27; ++type)
        for (int status = 0; status < 3; ++status)
            m_strategicObjectsModifiers[status][type] = 1.0f;
}

const std::string& Strategy::getName() const
{
    return m_name;
}

} // namespace ai
} // namespace hfe
} // namespace dice
