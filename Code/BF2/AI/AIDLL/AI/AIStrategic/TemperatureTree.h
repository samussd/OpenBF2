// Per-strategy weights for the strategic temperature tree.
// [INFERRED] Source placement; TemperatureTree itself is not recovered here.

#ifndef AI_AIDLL_AI_AISTRATEGIC_TEMPERATURETREE_H
#define AI_AIDLL_AI_AISTRATEGIC_TEMPERATURETREE_H

namespace dice {
namespace hfe {
namespace ai {

// Values beyond the 41 node weights exist only in the combat-strength table. [UNRESOLVED] names.
enum TemperatureTreeNode : int;
enum CombatStrengthType : int;

// 1508 bytes on both ports: one weight per node, then six combat-strength weights for each of
// 56 rows. Strategy only addresses the first 41 rows.
struct TemperatureTreeModifiers
{
    TemperatureTreeModifiers();

    // Every weight back to 1.0.
    void reset();

    /* +0x000 */ float m_node[41];
    /* +0x0A4 */ float m_combatStrength[56][6];
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGIC_TEMPERATURETREE_H
