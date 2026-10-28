// Script spellings of the strategy enums. Every reader takes one token and compares it
// case-sensitively; an unknown word stores one past the last named value. Every writer prints
// nothing for a value it has no name for, except Status, which prints "Unknown".
// The file name is attested by the ELF's static initializer; the folder is [INFERRED].
// The shipped file also holds operators for other types that are not recovered here.

#ifndef AI_AIDLL_AI_AIUTILS_AISTREAMOPERATORS_H
#define AI_AIDLL_AI_AIUTILS_AISTREAMOPERATORS_H

#include <istream>
#include <ostream>

#include "../AIStrategic/StatusMap.h"
#include "../AIStrategic/StrategyCondition.h"
#include "../AIStrategicEntity/AIStrategicObject.h"

namespace dice {
namespace hfe {
namespace ai {

enum BotLodLevel : int;
enum BAPStatementType : int;

std::istream& operator>>(std::istream& stream, StrategyCondition::ConditionStrength& value);
std::ostream& operator<<(std::ostream& stream, StrategyCondition::ConditionStrength value);
std::istream& operator>>(std::istream& stream, StrategyCondition::CompareType& value);
std::ostream& operator<<(std::ostream& stream, StrategyCondition::CompareType value);
std::istream& operator>>(std::istream& stream, StrategyCondition::ConditionSide& value);
std::ostream& operator<<(std::ostream& stream, StrategyCondition::ConditionSide value);
// Also reads the symbols "=", ">=", "<=", "/", ">/", "</", "-", ">-" and "<-".
std::istream& operator>>(std::istream& stream, StrategyCondition::Compare& value);
std::ostream& operator<<(std::ostream& stream, StrategyCondition::Compare value);
// Reads "HeavyNaval" and both "Infantry" and "Infantery"; writes "BattleShip" and "Infantery".
std::istream& operator>>(std::istream& stream, StrategyCondition::Conditional& value);
std::ostream& operator<<(std::ostream& stream, StrategyCondition::Conditional value);
// NeutralType is spelled "Neutral".
std::istream& operator>>(std::istream& stream, AIStrategicObject::StrategicObjectType& value);
std::ostream& operator<<(std::ostream& stream, AIStrategicObject::StrategicObjectType value);
std::istream& operator>>(std::istream& stream, StatusMap::Status& value);
std::ostream& operator<<(std::ostream& stream, StatusMap::Status value);
// Also reads "0", "1" and "2". Any other word, "None" included, reads as BLLNone.
std::istream& operator>>(std::istream& stream, BotLodLevel& value);
// Reads each type with or without its BAPST prefix, and AimAt also as "AImAt".
std::istream& operator>>(std::istream& stream, BAPStatementType& value);

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIUTILS_AISTREAMOPERATORS_H
