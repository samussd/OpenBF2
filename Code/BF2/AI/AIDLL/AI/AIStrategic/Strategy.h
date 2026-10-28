// A named commander strategy: attack and defence counts, per-object-type weights and the
// prerequisite that rates it against the strategic state.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGIC_STRATEGY_H
#define AI_AIDLL_AI_AISTRATEGIC_STRATEGY_H

#include <set>
#include <string>
#include <vector>

#include "StatusMap.h"
#include "TemperatureTree.h"
#include "../AIStrategicEntity/AIStrategicObject.h"

namespace dice {
namespace hfe {
namespace ai {

class StrategyPrerequisite;

// 412 bytes on the ELF. The object-type and node indices are unchecked unless noted.
class Strategy
{
public:
    explicit Strategy(const std::string& name);
    ~Strategy();

    TemperatureTreeModifiers* getTemperatureTreeModifiers() const;
    void setTemperatureTreeModifiers(TemperatureTreeModifiers* modifiers);   // not freed first

    float getAggression() const;
    void setAggression(float aggression);
    unsigned int getNumberOfAttacks() const;
    void setNumberOfAttacks(unsigned int count);
    unsigned int getNumberOfAttackConsiderations() const;
    void setNumberOfAttackConsiderations(unsigned int count);
    unsigned int getNumberOfDefences() const;
    void setNumberOfDefences(unsigned int count);
    unsigned int getNumberOfDefenceConsiderations() const;
    void setNumberOfDefenceConsiderations(unsigned int count);

    // Neither prerequisite is owned.
    void setPrerequisite(StrategyPrerequisite* prerequisite);
    void setAbortCondition(StrategyPrerequisite* abortCondition);
    // 0 unless active passes the required and prohibited preceding-strategy sets, else the
    // prerequisite's evaluation times the prerequisite weight. The prerequisite must be set.
    float calculatePrerequisite(float* friendly, float* enemy, Strategy* active, int side);
    // The abort level minus the abort condition's evaluation; the abort condition must be set.
    float calculateAbortCondition(float* friendly, float* enemy, Strategy* active, int side);
    void setAbortLevel(float level);
    float getAbortLevel() const;
    void setPrerequisiteWeight(float weight);
    float getPrerequisiteWeight() const;

    // The setters ignore a node of 41 or more and a combat-strength type outside 0..5.
    void setTreeModifier(TemperatureTreeNode node, float value);
    float getTreeModifier(TemperatureTreeNode node) const;
    void setTreeModifier(TemperatureTreeNode node, CombatStrengthType type, float value);
    float getTreeModifier(TemperatureTreeNode node, CombatStrengthType type) const;
    void setTreeModifierCombatStrength(TemperatureTreeNode node, CombatStrengthType type,
                                       float value);
    float getTreeModifierCombatStrength(TemperatureTreeNode node, CombatStrengthType type) const;

    // A null strategy is refused. Returns whether it was newly added.
    bool addRequiredPrecedingStrategy(Strategy* strategy);
    bool addProhibitedPrecedingStrategy(Strategy* strategy);

    // The two-argument forms cover all three statuses; the getter averages them.
    void setStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type, float value);
    float getStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type) const;
    void setStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type, float value,
                                     StatusMap::Status status);
    float getStrategicObjectsModifier(AIStrategicObject::StrategicObjectType type,
                                      StatusMap::Status status) const;
    // Per strategic-object index. The setter grows the table with 1.0; the getter answers 1.0
    // outside it. A negative index is ignored.
    void setSpecificStrategicObjectModifier(int index, float value);
    float getSpecificStrategicObjectModifier(int index) const;
    // The 27 object-type weights for one status.
    const float* getStrategicObjectsModifiers(StatusMap::Status status) const;
    // Every object-type weight back to 1.0.
    void clearAllStrategicObjectsModifiers();
    const std::string& getName() const;

    // Owned, but deleted with plain operator delete as shipped.
    /* +0x000 */ TemperatureTreeModifiers* m_treeModifiers;
    /* +0x004 */ float m_aggression;                   // = 0.5
    // The four counts are left unset by the constructor, as shipped.
    /* +0x008 */ unsigned int m_numberOfAttacks;
    /* +0x00C */ unsigned int m_numberOfDefences;
    /* +0x010 */ unsigned int m_numberOfAttackConsiderations;
    /* +0x014 */ unsigned int m_numberOfDefenceConsiderations;
    /* +0x018 */ float m_abortLevel;                   // = 0
    /* +0x01C */ float m_prerequisiteWeight;           // = 1
    // Seconds before an unchallenged strategy is re-rolled; only the console reaches it.
    /* +0x020 */ float m_timeLimit;                    // = 1e9
    /* +0x024 */ StrategyPrerequisite* m_prerequisite;  // = 0
    /* +0x028 */ StrategyPrerequisite* m_abortCondition; // = 0
    /* +0x02C */ unsigned char m_unresolved2C[4];      // never written
    /* +0x030 */ std::set<Strategy*> m_requiredPrecedingStrategies;
    /* +0x03C */ std::set<Strategy*> m_prohibitedPrecedingStrategies;
    /* +0x048 */ float m_strategicObjectsModifiers[3][27];   // [status][object type]
    /* +0x18C */ std::vector<float> m_specificStrategicObjectModifiers;
    /* +0x198 */ std::string m_name;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGIC_STRATEGY_H
