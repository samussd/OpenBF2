// Strategy conditions: one measured quantity of the strategic state compared with a target.
// [INFERRED] Source placement; the SC* subclasses share this file.

#ifndef AI_AIDLL_AI_AISTRATEGIC_STRATEGYCONDITION_H
#define AI_AIDLL_AI_AISTRATEGIC_STRATEGYCONDITION_H

#include <string>

namespace dice {
namespace hfe {
namespace ai {

// The script parsers store one past the last name for an unknown word, so every enum below also
// holds that value.
class StrategyCondition
{
public:
    enum Compare
    {
        Equal, EqualGreater, EqualSmaller,
        Quotient, QuotientGreater, QuotientSmaller,
        Difference, DifferenceGreater, DifferenceSmaller
    };

    enum CompareType { Crisp, Fuzzy, FuzzySqr };

    enum ConditionSide { Friendly, Enemy, Both };

    enum ConditionStrength
    {
        Required, RequiredPositive, RequiredNegative,
        Advisory, AdvisoryPositive, AdvisoryNegative
    };

    // Index into the 44-float state arrays SAI::updateStates fills.
    enum Conditional
    {
        Carrier, HeavyNaval, LCVP, Naval, Bomber, Fighter, AntiAir, Air, Infantry, Artillery,
        AntiTank, HeavyAttack, FastAttack, Ticket, ControlPoint, Time, StartTime, Attacks,
        Defences, NumberOfFriendlyAreas, NumberOfNeutralAreas, NumberOfHostileAreas, Flank, Base,
        Close, Centre, Remote, Route, Bridge, North, West, South, East, FrontFlank, FrontBase,
        FrontClose, FrontCentre, FrontRemote, FrontNeutral, Front, Safe, EnemyObject, UnReachable,
        UnitConstant
    };

    StrategyCondition(const std::string& name, float targetValue, Compare compare,
                      ConditionSide side, CompareType compareType, Conditional conditional);
    virtual ~StrategyCondition();                                       // slots 0/1

    // SCConstant 0, SCHomogeneous 1, SCHeterogeneous 2, SCArea 3, this base 4.
    virtual int getType() const;                                        // slot 2
    // Weighted by the prerequisite; positive favours the strategy. friendly and enemy are the
    // 44-float state arrays.
    virtual float compare(float* friendly, float* enemy, int side) const = 0;   // slot 3

    void setTargetValue(float value);
    float getTargetValue() const;
    void setConditionStrength(ConditionStrength strength);
    ConditionStrength getConditionStrength() const;
    bool setIsAbortCondition(bool on);
    bool getIsAbortCondition() const;
    bool isAbortCondition() const;

protected:
    float internalCompare(float value, float reference) const;
    // 1.0 when the comparison holds, else -1.0; 0.0 for an unknown Compare.
    float internalCompareCrisp(float value, float reference) const;
    // A signed distance from the target, -1000/1000 when a quotient divides by zero.
    float internalCompareFuzzy(float value, float reference) const;

public:
    /* +0x04 */ bool m_isAbortCondition;           // = false
    /* +0x08 */ float m_targetValue;
    /* +0x0C */ Compare m_compare;
    /* +0x10 */ CompareType m_compareType;
    /* +0x14 */ ConditionSide m_side;
    /* +0x18 */ Conditional m_conditional;
    /* +0x1C */ ConditionStrength m_strength;      // = Required
    /* +0x20 */ std::string m_name;
};

// One side's quantity against a script constant.
class SCConstant : public StrategyCondition
{
public:
    SCConstant(const std::string& name, CompareType compareType, Compare compare,
               ConditionSide side, Conditional conditional, float constant, float targetValue);

    virtual int getType() const;
    // Friendly reads the friendly array; Enemy and Both read the enemy array.
    virtual float compare(float* friendly, float* enemy, int side) const;

    /* +0x24 */ float m_constant;
};

// The same quantity on two arrays: friendly against enemy for Both, else one side against itself.
class SCHomogeneous : public StrategyCondition
{
public:
    // The side is always Both.
    SCHomogeneous(const std::string& name, CompareType compareType, Compare compare,
                  Conditional conditional, float targetValue);

    virtual int getType() const;
    // 0.0 when the side is not Friendly, Enemy or Both.
    virtual float compare(float* friendly, float* enemy, int side) const;
};

// Two quantities, read from the arrays the side selects as in SCHomogeneous.
class SCHeterogeneous : public StrategyCondition
{
public:
    SCHeterogeneous(const std::string& name, CompareType compareType, Compare compare,
                    ConditionSide side, Conditional conditional, Conditional other,
                    float targetValue);

    virtual int getType() const;
    virtual float compare(float* friendly, float* enemy, int side) const;

    /* +0x24 */ Conditional m_other;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGIC_STRATEGYCONDITION_H
