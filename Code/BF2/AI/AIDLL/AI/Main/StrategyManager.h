// Owns the commander strategies, prerequisites and conditions the aiStrategy console builds, and
// the strategy forced on each side.

#ifndef AI_AIDLL_AI_MAIN_STRATEGYMANAGER_H
#define AI_AIDLL_AI_MAIN_STRATEGYMANAGER_H

#include <list>
#include <map>
#include <string>

#include "AIDllTypes.h"
#include "../AIStrategic/StatusMap.h"
#include "../AIStrategic/StrategyCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class SCArea;
class Strategy;
class StrategyPrerequisite;

class StrategyManager
{
public:
    // A strategy's weight for one named area, resolved to an object index by completeStrategies.
    class ObjectStrategyModifierTuple
    {
    public:
        std::string m_areaName;
        Strategy* m_strategy;
        float m_value;
    };

    StrategyManager();
    ~StrategyManager();                             // not virtual

    // Deletes every strategy, prerequisite and condition and clears the forced strategies and the
    // debug flag. The object modifiers and the area list are kept, as shipped. Returns true.
    virtual bool reset();                           // the only virtual, vtable slot 0

    static StrategyManager* getInstance();
    static void setInstance(StrategyManager *);

    bool getEnableDebug();
    void setEnableDebug(bool);
    bool isDebugEnabled();

    Strategy* getActiveStrategy();
    std::string getActiveStrategyName();            // returns by value
    StrategyPrerequisite* getActivePrerequisite();
    StrategyCondition* getActiveCondition();
    Strategy* getForcedStrategy(int);

    std::string getActivePrerequisiteName();
    // Empty when no condition is active.
    std::string getActiveConditionName();
    // Each lookup answers 0 for an unknown name.
    Strategy* getStrategy(std::string);
    StrategyCondition* getCondition(std::string);
    StrategyPrerequisite* getPrerequisite(std::string);
    // An unknown name clears the active entry; the strategy is then named "NULL" and the
    // prerequisite "". Each returns the new active entry.
    Strategy* setActiveStrategy(std::string);
    StrategyPrerequisite* setActivePrerequisite(std::string);
    StrategyCondition* setActiveCondition(std::string);
    // Needs an active strategy; without one it reports an error and does nothing.
    void setPrerequisite(std::string);
    // side is 1 or 2. An unknown name clears that side's forced strategy. Returns a message for
    // the console.
    std::string forceStrategy(std::string, int);

    // Each create makes its entry active. A taken name returns false: createStrategy then clears
    // the active strategy, the condition creators clear the active condition, and
    // createPrerequisite leaves the active prerequisite alone.
    bool createStrategy(std::string);
    bool createPrerequisite(std::string);
    bool createConstantCondition(std::string, StrategyCondition::CompareType,
                                 StrategyCondition::Compare, StrategyCondition::ConditionSide,
                                 StrategyCondition::Conditional, float constant);
    bool createHomogenousCondition(std::string, StrategyCondition::CompareType,
                                   StrategyCondition::Compare, StrategyCondition::Conditional);
    bool createHeterogeneousCondition(std::string, StrategyCondition::CompareType,
                                      StrategyCondition::Compare, StrategyCondition::ConditionSide,
                                      StrategyCondition::Conditional,
                                      StrategyCondition::Conditional);
    // The area is resolved by completeConditions.
    bool createAreaCondition(std::string, std::string areaName, StatusMap::Status);

    // Adds the named condition to the active prerequisite, which must exist. Weight 1 by default.
    void addCondition(std::string);
    void addCondition(std::string, float);
    // Queued for completeStrategies against the active strategy. Returns true.
    bool addSpecificObjectModifier(std::string areaName, float);
    // Need an active strategy; without one they report an error and return false.
    bool addRequiredPrecedingStrategy(std::string);
    bool addProhibitedPrecedingStrategy(std::string);
    // Resolve the queued area names once the strategic areas exist. The queues are not cleared.
    void completeStrategies();
    void completeConditions();
    // Every name followed by a newline.
    std::string listStrategies();
    std::string listPrerequisites();
    std::string listConditions();

    // the layout, from the 545-byte constructor cross-checked against the destructor
    // the three registries each come as A (name, active-pointer, map) triple, in the same order.
    /*  +4 */ std::string m_activeStrategyName;      // = "No Strategies"
    /*  +8 */ Strategy* m_activeStrategy;            // = 0
    /* +12 */ std::map<std::string, Strategy*> m_strategies;
    /* +24 */ std::list<ObjectStrategyModifierTuple> m_objectModifiers;
    /* +28 */ std::string m_activePrerequisiteName;  // = ""
    /* +32 */ StrategyPrerequisite* m_activePrerequisite;   // = 0
    /* +36 */ std::map<std::string, StrategyPrerequisite*> m_prerequisites;
    // Active condition iterator, initialized to end() and checked before dereference.
    /* +48 */ std::map<std::string, StrategyCondition*>::iterator m_activeCondition;
    /* +52 */ std::map<std::string, StrategyCondition*> m_conditions;
    /* +64 */ std::list<SCArea*> m_areas;            // not owned; the conditions map owns them
    /* +68 */ bool m_enableDebug;                    // = false
    // Forced strategy by team - 1. Callers must supply team 1 or 2; no bounds check is performed.
    /* +72 */ Strategy* m_forcedStrategy[2];         // = { 0, 0 }, indexed [team - 1]

    // first wins, and the destructor does test `== this`.
    static StrategyManager* m_instance;
};

// The aiStrategy console's receivers; 0 when there is no manager.
Strategy* getActiveStrategy();
StrategyCondition* getActiveCondition();

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_STRATEGYMANAGER_H
