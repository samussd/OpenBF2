// Readers 0x3d5f00..0x3d8cc0 (Ghidra), writers beside each. The tables keep the shipped comparison
// order; no token appears twice, so the order does not change a result.

#include "AIStreamOperators.h"

#include <string>

namespace dice {
namespace hfe {
namespace ai {

namespace {

struct NamedValue
{
    const char* name;
    int value;
};

const NamedValue kConditionStrengthTokens[] = {
    { "Required", 0 }, { "RequiredPositive", 1 }, { "RequiredNegative", 2 },
    { "Advisory", 3 }, { "AdvisoryPositive", 4 }, { "AdvisoryNegative", 5 },
};
const char* const kConditionStrengthNames[] = {
    "Required", "RequiredPositive", "RequiredNegative",
    "Advisory", "AdvisoryPositive", "AdvisoryNegative",
};

const NamedValue kCompareTypeTokens[] = { { "Crisp", 0 }, { "Fuzzy", 1 }, { "FuzzySqr", 2 } };
const char* const kCompareTypeNames[] = { "Crisp", "Fuzzy", "FuzzySqr" };

const NamedValue kConditionSideTokens[] = { { "Friendly", 0 }, { "Enemy", 1 }, { "Both", 2 } };
const char* const kConditionSideNames[] = { "Friendly", "Enemy", "Both" };

const NamedValue kCompareTokens[] = {
    { "Equal", 0 }, { "=", 0 }, { "EqualGreater", 1 }, { ">=", 1 }, { "EqualSmaller", 2 }, { "<=", 2 },
    { "Quotient", 3 }, { "/", 3 }, { "QuotientGreater", 4 }, { ">/", 4 },
    { "QuotientSmaller", 5 }, { "</", 5 }, { "Difference", 6 }, { "-", 6 },
    { "DifferenceGreater", 7 }, { ">-", 7 }, { "DifferenceSmaller", 8 }, { "<-", 8 },
};
const char* const kCompareNames[] = {
    "Equal", "EqualGreater", "EqualSmaller", "Quotient", "QuotientGreater", "QuotientSmaller",
    "Difference", "DifferenceGreater", "DifferenceSmaller",
};

const NamedValue kConditionalTokens[] = {
    { "Ticket", 13 }, { "ControlPoint", 14 }, { "Time", 15 }, { "StartTime", 16 },
    { "Attacks", 17 }, { "Defences", 18 }, { "Carrier", 0 }, { "HeavyNaval", 1 }, { "Naval", 3 },
    { "Bomber", 4 }, { "Fighter", 5 }, { "AntiAir", 6 }, { "Air", 7 }, { "LCVP", 2 },
    { "Infantry", 8 }, { "Infantery", 8 }, { "Artillery", 9 }, { "AntiTank", 10 },
    { "HeavyAttack", 11 }, { "FastAttack", 12 }, { "NumberOfFriendlyAreas", 19 },
    { "NumberOfNeutralAreas", 20 }, { "NumberOfHostileAreas", 21 }, { "Flank", 22 },
    { "Base", 23 }, { "Close", 24 }, { "Centre", 25 }, { "Remote", 26 }, { "North", 29 },
    { "East", 32 }, { "South", 31 }, { "West", 30 }, { "Route", 27 }, { "Bridge", 28 },
    { "FrontFlank", 33 }, { "FrontBase", 34 }, { "FrontClose", 35 }, { "FrontCentre", 36 },
    { "FrontRemote", 37 }, { "FrontNeutral", 38 }, { "Front", 39 }, { "Safe", 40 },
    { "EnemyObject", 41 }, { "UnReachable", 42 }, { "UnitConstant", 43 },
};
const char* const kConditionalNames[] = {
    "Carrier", "BattleShip", "LCVP", "Naval", "Bomber", "Fighter", "AntiAir", "Air", "Infantery",
    "Artillery", "AntiTank", "HeavyAttack", "FastAttack", "Ticket", "ControlPoint", "Time",
    "StartTime", "Attacks", "Defences", "NumberOfFriendlyAreas", "NumberOfNeutralAreas",
    "NumberOfHostileAreas", "Flank", "Base", "Close", "Centre", "Remote", "Route", "Bridge",
    "North", "West", "South", "East", "FrontFlank", "FrontBase", "FrontClose", "FrontCentre",
    "FrontRemote", "FrontNeutral", "Front", "Safe", "EnemyObject", "UnReachable", "UnitConstant",
};

const NamedValue kObjectTypeTokens[] = {
    { "Flank", 1 }, { "Centre", 2 }, { "Base", 3 }, { "Close", 4 }, { "Remote", 5 },
    { "AirSpawner", 6 }, { "LandSpawner", 7 }, { "NavalSpawner", 8 }, { "SoldierSpawner", 9 },
    { "StrongPoint", 10 }, { "ChokePoint", 11 }, { "Bridge", 12 }, { "AirField", 13 },
    { "SupplyPoint", 14 }, { "Route", 15 }, { "ControlPoint", 16 }, { "Front", 22 },
    { "Safe", 23 }, { "Enemy", 24 }, { "Neutral", 26 }, { "North", 17 }, { "West", 18 },
    { "South", 19 }, { "East", 20 },
};
const char* const kObjectTypeNames[] = {
    0, "Flank", "Centre", "Base", "Close", "Remote", "AirSpawner", "LandSpawner", "NavalSpawner",
    "SoldierSpawner", "StrongPoint", "ChokePoint", "Bridge", "AirField", "SupplyPoint", "Route",
    "ControlPoint", "North", "West", "South", "East", 0, "Front", "Safe", "Enemy", 0, "Neutral",
};

// Stores unknown for an unmatched token, including when the read fails.
template <typename T, int N>
std::istream& readNamed(std::istream& stream, T& value, const NamedValue (&tokens)[N], int unknown)
{
    std::string token;
    stream >> token;
    value = static_cast<T>(unknown);
    for (int i = 0; i < N; ++i)
    {
        if (token == tokens[i].name)
        {
            value = static_cast<T>(tokens[i].value);
            break;
        }
    }
    return stream;
}

template <int N>
std::ostream& writeNamed(std::ostream& stream, int value, const char* const (&names)[N])
{
    if (value >= 0 && value < N && names[value])
        stream << names[value];
    return stream;
}

} // namespace

std::istream& operator>>(std::istream& stream, StrategyCondition::ConditionStrength& value)
{
    return readNamed(stream, value, kConditionStrengthTokens, 6);
}

std::ostream& operator<<(std::ostream& stream, StrategyCondition::ConditionStrength value)
{
    return writeNamed(stream, value, kConditionStrengthNames);
}

std::istream& operator>>(std::istream& stream, StrategyCondition::CompareType& value)
{
    return readNamed(stream, value, kCompareTypeTokens, 3);
}

std::ostream& operator<<(std::ostream& stream, StrategyCondition::CompareType value)
{
    return writeNamed(stream, value, kCompareTypeNames);
}

std::istream& operator>>(std::istream& stream, StrategyCondition::ConditionSide& value)
{
    return readNamed(stream, value, kConditionSideTokens, 3);
}

std::ostream& operator<<(std::ostream& stream, StrategyCondition::ConditionSide value)
{
    return writeNamed(stream, value, kConditionSideNames);
}

std::istream& operator>>(std::istream& stream, StrategyCondition::Compare& value)
{
    return readNamed(stream, value, kCompareTokens, 9);
}

std::ostream& operator<<(std::ostream& stream, StrategyCondition::Compare value)
{
    return writeNamed(stream, value, kCompareNames);
}

std::istream& operator>>(std::istream& stream, StrategyCondition::Conditional& value)
{
    return readNamed(stream, value, kConditionalTokens, 44);
}

std::ostream& operator<<(std::ostream& stream, StrategyCondition::Conditional value)
{
    return writeNamed(stream, value, kConditionalNames);
}

std::istream& operator>>(std::istream& stream, AIStrategicObject::StrategicObjectType& value)
{
    return readNamed(stream, value, kObjectTypeTokens, 27);
}

std::ostream& operator<<(std::ostream& stream, AIStrategicObject::StrategicObjectType value)
{
    return writeNamed(stream, value, kObjectTypeNames);
}

// The shipped reader makes one more comparison after "Neutral" and ignores its result.
std::istream& operator>>(std::istream& stream, StatusMap::Status& value)
{
    static const NamedValue tokens[] = { { "Owned", 0 }, { "Hostile", 1 }, { "Neutral", 2 } };
    return readNamed(stream, value, tokens, 3);
}

std::ostream& operator<<(std::ostream& stream, StatusMap::Status value)
{
    switch (value)
    {
    case StatusMap::Owned:   return stream << "Owned";
    case StatusMap::Hostile: return stream << "Hostile";
    case StatusMap::Neutral: return stream << "Neutral";
    default:                 return stream << "Unknown";
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
