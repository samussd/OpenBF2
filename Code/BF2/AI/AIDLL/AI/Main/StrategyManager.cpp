// ELF 0x3f7a60..0x3fad10 (Ghidra), and the aiStrategy console classes 361-408 of AIModule.cpp's
// static initializer.

#include "StrategyManager.h"

#include "../AIStrategic/SCArea.h"
#include "../AIStrategic/Strategy.h"
#include "../AIStrategic/StrategyPrerequisite.h"
#include "../AIStrategicEntity/AIStrategicArea.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../AIUtils/AIStreamOperators.h"                // the enum arguments and getConditionStrength
#include "../AIUtils/checkPointer.h"
#include "../../../../Core/ConsoleObjectMacro.h"

namespace dice {
namespace hfe {
namespace ai {

StrategyManager* StrategyManager::m_instance = 0;

// Initializes strategy collections and installs the singleton when none exists.
StrategyManager::StrategyManager()
    : m_activeStrategy(0)
    , m_activePrerequisite(0)
    , m_enableDebug(false)
{
    m_forcedStrategy[0] = 0;
    m_forcedStrategy[1] = 0;

    m_activeCondition = m_conditions.end();
    m_activeStrategyName.assign("No Strategies");

    if (!m_instance)
        m_instance = this;
}

// Resets strategy state and releases the owned collections.
StrategyManager::~StrategyManager()
{
    reset();

    if (m_instance == this)
        m_instance = 0;
}

// Static access to the concrete strategy manager, whose destructor is nonvirtual.
StrategyManager* StrategyManager::getInstance()
{
    return m_instance;
}

void StrategyManager::setInstance(StrategyManager* instance)
{
    m_instance = instance;
}

bool StrategyManager::getEnableDebug()
{
    return m_enableDebug;
}

void StrategyManager::setEnableDebug(bool on)
{
    m_enableDebug = on;
}

bool StrategyManager::isDebugEnabled()
{
    return m_enableDebug;
}

Strategy* StrategyManager::getActiveStrategy()
{
    return m_activeStrategy;
}

// by value - the shipped function is `__stdcall` with a hidden return slot, i.e. it copies the
// member rather than handing back a reference to it.
std::string StrategyManager::getActiveStrategyName()
{
    return m_activeStrategyName;
}

StrategyPrerequisite* StrategyManager::getActivePrerequisite()
{
    return m_activePrerequisite;
}

// the cursor is checked before it is dereferenced, and a miss returns null rather than a sentinel.
StrategyCondition* StrategyManager::getActiveCondition()
{
    if (m_activeCondition == m_conditions.end())
        return 0;
    return m_activeCondition->second;
}

// 1-based, and unclamped - see the member note in the header. `team == 0` reads the dword that
// holds `m_enableDebug`.
Strategy* StrategyManager::getForcedStrategy(int team)
{
    return m_forcedStrategy[team - 1];
}

// The strategy placeholder is misspelled "No Strategiws" here, unlike the constructor's.
bool StrategyManager::reset()
{
    m_forcedStrategy[0] = 0;
    m_forcedStrategy[1] = 0;
    m_enableDebug = false;

    for (std::map<std::string, Strategy*>::iterator it = m_strategies.begin();
         it != m_strategies.end();)
    {
        delete it->second;
        m_strategies.erase(it++);
    }
    m_activeStrategyName = "No Strategiws";
    m_activeStrategy = 0;

    for (std::map<std::string, StrategyPrerequisite*>::iterator it = m_prerequisites.begin();
         it != m_prerequisites.end();)
    {
        delete it->second;
        m_prerequisites.erase(it++);
    }
    m_activePrerequisiteName = "No Prerequisites";
    m_activePrerequisite = 0;

    for (std::map<std::string, StrategyCondition*>::iterator it = m_conditions.begin();
         it != m_conditions.end();)
    {
        delete it->second;
        m_conditions.erase(it++);
    }
    m_activeCondition = m_conditions.end();
    return true;
}

std::string StrategyManager::getActivePrerequisiteName()
{
    return m_activePrerequisiteName;
}

std::string StrategyManager::getActiveConditionName()
{
    if (m_activeCondition == m_conditions.end())
        return "";
    return m_activeCondition->first;
}

Strategy* StrategyManager::getStrategy(std::string name)
{
    const std::map<std::string, Strategy*>::iterator it = m_strategies.find(name);
    return it != m_strategies.end() ? it->second : 0;
}

StrategyCondition* StrategyManager::getCondition(std::string name)
{
    const std::map<std::string, StrategyCondition*>::iterator it = m_conditions.find(name);
    return it != m_conditions.end() ? it->second : 0;
}

StrategyPrerequisite* StrategyManager::getPrerequisite(std::string name)
{
    const std::map<std::string, StrategyPrerequisite*>::iterator it = m_prerequisites.find(name);
    return it != m_prerequisites.end() ? it->second : 0;
}

Strategy* StrategyManager::setActiveStrategy(std::string name)
{
    const std::map<std::string, Strategy*>::iterator it = m_strategies.find(name);
    if (it == m_strategies.end())
    {
        m_activeStrategyName = "NULL";
        m_activeStrategy = 0;
    }
    else
    {
        m_activeStrategyName = it->first;
        m_activeStrategy = it->second;
    }
    return m_activeStrategy;
}

StrategyPrerequisite* StrategyManager::setActivePrerequisite(std::string name)
{
    const std::map<std::string, StrategyPrerequisite*>::iterator it = m_prerequisites.find(name);
    if (it == m_prerequisites.end())
    {
        m_activePrerequisiteName = "";
        m_activePrerequisite = 0;
    }
    else
    {
        m_activePrerequisiteName = it->first;
        m_activePrerequisite = it->second;
    }
    return m_activePrerequisite;
}

// [PLATFORM] On a miss the shipped code returns the value field of end(); null is returned here.
StrategyCondition* StrategyManager::setActiveCondition(std::string name)
{
    m_activeCondition = m_conditions.find(name);
    return getActiveCondition();
}

void StrategyManager::setPrerequisite(std::string name)
{
    if (!m_activeStrategy)
    {
        Debug(DebugTypeError, "AI/AIDLL/AI/Main/StrategyManager.cpp", 182, "")
            << "Cannot set prerequisite " << name << " - there is no active strategy.";
        return;
    }
    m_activeStrategy->setPrerequisite(getPrerequisite(name));
}

// An out-of-range side is appended as a raw character, as shipped.
std::string StrategyManager::forceStrategy(std::string name, int side)
{
    std::string reply;
    if (static_cast<unsigned int>(side - 1) < 2)
    {
        const std::map<std::string, Strategy*>::iterator it = m_strategies.find(name);
        if (it == m_strategies.end())
        {
            m_forcedStrategy[side - 1] = 0;
            reply += "No active strategy chosen : ";
        }
        else
        {
            m_forcedStrategy[side - 1] = it->second;
            reply += "Strategy is activated : ";
        }
        reply += name;
    }
    else
    {
        reply += "Side must be 1 or 2, current side: ";
        reply += static_cast<char>(side);
    }
    return reply;
}

bool StrategyManager::createStrategy(std::string name)
{
    if (m_strategies.find(name) != m_strategies.end())
    {
        m_activeStrategyName = "NULL";
        m_activeStrategy = 0;
        return false;
    }
    Strategy* const strategy = checkPointer(new Strategy(name));
    m_strategies[name] = strategy;
    m_activeStrategyName = name;
    m_activeStrategy = strategy;
    return true;
}

bool StrategyManager::createPrerequisite(std::string name)
{
    if (m_prerequisites.find(name) != m_prerequisites.end())
        return false;
    StrategyPrerequisite* const prerequisite = checkPointer(new StrategyPrerequisite(name));
    m_prerequisites[name] = prerequisite;
    m_activePrerequisiteName = name;
    m_activePrerequisite = prerequisite;
    return true;
}

bool StrategyManager::createConstantCondition(std::string name,
                                              StrategyCondition::CompareType compareType,
                                              StrategyCondition::Compare compare,
                                              StrategyCondition::ConditionSide side,
                                              StrategyCondition::Conditional conditional,
                                              float constant)
{
    if (m_conditions.find(name) != m_conditions.end())
    {
        m_activeCondition = m_conditions.end();
        return false;
    }
    StrategyCondition* const condition = checkPointer(
        new SCConstant(name, compareType, compare, side, conditional, constant, 0.0f));
    m_activeCondition = m_conditions.insert(std::make_pair(name, condition)).first;
    return true;
}

bool StrategyManager::createHomogenousCondition(std::string name,
                                                StrategyCondition::CompareType compareType,
                                                StrategyCondition::Compare compare,
                                                StrategyCondition::Conditional conditional)
{
    if (m_conditions.find(name) != m_conditions.end())
    {
        m_activeCondition = m_conditions.end();
        return false;
    }
    StrategyCondition* const condition = checkPointer(
        new SCHomogeneous(name, compareType, compare, conditional, 0.0f));
    m_activeCondition = m_conditions.insert(std::make_pair(name, condition)).first;
    return true;
}

bool StrategyManager::createHeterogeneousCondition(std::string name,
                                                   StrategyCondition::CompareType compareType,
                                                   StrategyCondition::Compare compare,
                                                   StrategyCondition::ConditionSide side,
                                                   StrategyCondition::Conditional conditional,
                                                   StrategyCondition::Conditional other)
{
    if (m_conditions.find(name) != m_conditions.end())
    {
        m_activeCondition = m_conditions.end();
        return false;
    }
    StrategyCondition* const condition = checkPointer(
        new SCHeterogeneous(name, compareType, compare, side, conditional, other, 0.0f));
    m_activeCondition = m_conditions.insert(std::make_pair(name, condition)).first;
    return true;
}

bool StrategyManager::createAreaCondition(std::string name, std::string areaName,
                                          StatusMap::Status status)
{
    if (m_conditions.find(name) != m_conditions.end())
    {
        m_activeCondition = m_conditions.end();
        return false;
    }
    SCArea* const area = checkPointer(new SCArea(name, areaName, -1, status));
    m_activeCondition =
        m_conditions.insert(std::make_pair(name, static_cast<StrategyCondition*>(area))).first;
    m_areas.push_back(area);
    return true;
}

void StrategyManager::addCondition(std::string name)
{
    addCondition(name, 1.0f);
}

void StrategyManager::addCondition(std::string name, float weight)
{
    m_activePrerequisite->addCondition(getCondition(name), weight);
}

// The active strategy is not checked; completeStrategies dereferences it.
bool StrategyManager::addSpecificObjectModifier(std::string areaName, float value)
{
    ObjectStrategyModifierTuple tuple;
    tuple.m_areaName = areaName;
    tuple.m_strategy = m_activeStrategy;
    tuple.m_value = value;
    m_objectModifiers.push_back(tuple);
    return true;
}

bool StrategyManager::addRequiredPrecedingStrategy(std::string name)
{
    if (!m_activeStrategy)
    {
        Debug(DebugTypeError, "AI/AIDLL/AI/Main/StrategyManager.cpp", 193, "")
            << "Cannot add required preceding strategy " << name
            << " - there is no active strategy.";
        return false;
    }
    return m_activeStrategy->addRequiredPrecedingStrategy(getStrategy(name));
}

bool StrategyManager::addProhibitedPrecedingStrategy(std::string name)
{
    if (!m_activeStrategy)
    {
        Debug(DebugTypeError, "AI/AIDLL/AI/Main/StrategyManager.cpp", 204, "")
            << "Cannot add prohibited preceding strategy " << name
            << " - there is no active strategy.";
        return false;
    }
    return m_activeStrategy->addProhibitedPrecedingStrategy(getStrategy(name));
}

// Moves the strategic-area manager's active area as a side effect.
void StrategyManager::completeStrategies()
{
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    for (std::list<ObjectStrategyModifierTuple>::iterator it = m_objectModifiers.begin();
         it != m_objectModifiers.end(); ++it)
    {
        manager->setActive(it->m_areaName);
        const AIStrategicArea* const area = manager->getActiveArea();
        if (area)
            it->m_strategy->setSpecificStrategicObjectModifier(area->getIndex(), it->m_value);
    }
}

void StrategyManager::completeConditions()
{
    for (std::list<SCArea*>::iterator it = m_areas.begin(); it != m_areas.end(); ++it)
        (*it)->updateArea();
}

std::string StrategyManager::listStrategies()
{
    std::string list;
    for (std::map<std::string, Strategy*>::iterator it = m_strategies.begin();
         it != m_strategies.end(); ++it)
    {
        list += it->first;
        list += "\n";
    }
    return list;
}

std::string StrategyManager::listPrerequisites()
{
    std::string list;
    for (std::map<std::string, StrategyPrerequisite*>::iterator it = m_prerequisites.begin();
         it != m_prerequisites.end(); ++it)
    {
        list += it->first;
        list += "\n";
    }
    return list;
}

std::string StrategyManager::listConditions()
{
    std::string list;
    for (std::map<std::string, StrategyCondition*>::iterator it = m_conditions.begin();
         it != m_conditions.end(); ++it)
    {
        list += it->first;
        list += "\n";
    }
    return list;
}

// 0x3fea00.
Strategy* getActiveStrategy()
{
    return StrategyManager::getInstance() ? StrategyManager::getInstance()->getActiveStrategy() : 0;
}

// 0x3fea80.
StrategyCondition* getActiveCondition()
{
    return StrategyManager::getInstance() ? StrategyManager::getInstance()->getActiveCondition() : 0;
}

// Console bindings, in the shipped class order. The tree-modifier classes 369-372 are not bound:
// their TemperatureTreeNode reader is not recovered, and the shipped scripts do not use them.

namespace {

const char* const kAiStrategyConsoleTag = "aiStrategy";

} // anonymous namespace

BF2_CONSOLE_OBJECT("aiStrategy", 2, 0)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(createStrategy, StrategyManager::getInstance()->createStrategy,
                                   StrategyManager::getInstance() != 0,
                                   std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setActiveStrategy,
                                   StrategyManager::getInstance()->setActiveStrategy,
                                   StrategyManager::getInstance() != 0,
                                   std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(getActiveStrategyName,
                                StrategyManager::getInstance()->getActiveStrategyName,
                                std::string, StrategyManager::getInstance() != 0,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_PROPERTY_FN(aggression, float, getActiveStrategy() != 0,
                        getActiveStrategy()->getAggression(),
                        getActiveStrategy()->setAggression, kAiStrategyConsoleTag)
BF2_CONSOLE_FIELD(timeLimit, float, getActiveStrategy(), m_timeLimit, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(getPrerequisiteWeight, getActiveStrategy()->getPrerequisiteWeight,
                                float, getActiveStrategy() != 0, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setPrerequisiteWeight,
                                   getActiveStrategy()->setPrerequisiteWeight,
                                   getActiveStrategy() != 0, float, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(listStrategies, StrategyManager::getInstance()->listStrategies,
                                std::string, StrategyManager::getInstance() != 0,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_PROPERTY_FN(numberOfAttacks, int, getActiveStrategy() != 0,
                        getActiveStrategy()->getNumberOfAttacks(),
                        getActiveStrategy()->setNumberOfAttacks, kAiStrategyConsoleTag)
BF2_CONSOLE_PROPERTY_FN(numberOfDefences, int, getActiveStrategy() != 0,
                        getActiveStrategy()->getNumberOfDefences(),
                        getActiveStrategy()->setNumberOfDefences, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setPrerequisite, StrategyManager::getInstance()->setPrerequisite,
                                   StrategyManager::getInstance() != 0,
                                   std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_RET_ACTIVE(addRequiredPrecedingStrategy,
                                StrategyManager::getInstance()->addRequiredPrecedingStrategy,
                                bool, StrategyManager::getInstance() != 0,
                                std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_RET_ACTIVE(addProhibitedPrecedingStrategy,
                                StrategyManager::getInstance()->addProhibitedPrecedingStrategy,
                                bool, StrategyManager::getInstance() != 0,
                                std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_2_3_STATIC_ACTIVE(setStrategicObjectsModifier,
                                     getActiveStrategy()->setStrategicObjectsModifier,
                                     getActiveStrategy() != 0,
                                     AIStrategicObject::StrategicObjectType, float,
                                     StatusMap::Status, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_2_RET_ACTIVE(getStrategicObjectsModifier,
                                  getActiveStrategy()->getStrategicObjectsModifier,
                                  float, getActiveStrategy() != 0,
                                  AIStrategicObject::StrategicObjectType, StatusMap::Status,
                                  kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_STATIC_ACTIVE(clearAllStrategicObjectsModifiers,
                                   getActiveStrategy()->clearAllStrategicObjectsModifiers,
                                   getActiveStrategy() != 0, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_2_RET_ACTIVE(addSpecificObjectModifier,
                                StrategyManager::getInstance()->addSpecificObjectModifier,
                                bool, StrategyManager::getInstance() != 0,
                                std::string, float, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_STATIC_ACTIVE(completeStrategies,
                                   StrategyManager::getInstance()->completeStrategies,
                                   StrategyManager::getInstance() != 0, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(createPrerequisite,
                                   StrategyManager::getInstance()->createPrerequisite,
                                   StrategyManager::getInstance() != 0,
                                   std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setActivePrerequisite,
                                   StrategyManager::getInstance()->setActivePrerequisite,
                                   StrategyManager::getInstance() != 0,
                                   std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(getActivePrerequisiteName,
                                StrategyManager::getInstance()->getActivePrerequisiteName,
                                std::string, StrategyManager::getInstance() != 0,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(listPrerequisites,
                                StrategyManager::getInstance()->listPrerequisites,
                                std::string, StrategyManager::getInstance() != 0,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_2_STATIC_ACTIVE(addCondition, StrategyManager::getInstance()->addCondition,
                                     StrategyManager::getInstance() != 0,
                                     std::string, float, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setActiveCondition,
                                   StrategyManager::getInstance()->setActiveCondition,
                                   StrategyManager::getInstance() != 0,
                                   std::string, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(getActiveConditionName,
                                StrategyManager::getInstance()->getActiveConditionName,
                                std::string, StrategyManager::getInstance() != 0,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(listConditions, StrategyManager::getInstance()->listConditions,
                                std::string, StrategyManager::getInstance() != 0,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_6_RET_ACTIVE(createConstantCondition,
                                StrategyManager::getInstance()->createConstantCondition,
                                bool, StrategyManager::getInstance() != 0,
                                std::string, StrategyCondition::CompareType,
                                StrategyCondition::Compare, StrategyCondition::ConditionSide,
                                StrategyCondition::Conditional, float, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_4_RET_ACTIVE(createHomogenousCondition,
                                StrategyManager::getInstance()->createHomogenousCondition,
                                bool, StrategyManager::getInstance() != 0,
                                std::string, StrategyCondition::CompareType,
                                StrategyCondition::Compare, StrategyCondition::Conditional,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_6_RET_ACTIVE(createHeterogeneousCondition,
                                StrategyManager::getInstance()->createHeterogeneousCondition,
                                bool, StrategyManager::getInstance() != 0,
                                std::string, StrategyCondition::CompareType,
                                StrategyCondition::Compare, StrategyCondition::ConditionSide,
                                StrategyCondition::Conditional, StrategyCondition::Conditional,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_3_RET_ACTIVE(createAreaCondition,
                                StrategyManager::getInstance()->createAreaCondition,
                                bool, StrategyManager::getInstance() != 0,
                                std::string, std::string, StatusMap::Status, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setConditionStrength, getActiveCondition()->setConditionStrength,
                                   getActiveCondition() != 0,
                                   StrategyCondition::ConditionStrength, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(getConditionStrength, getActiveCondition()->getConditionStrength,
                                StrategyCondition::ConditionStrength, getActiveCondition() != 0,
                                kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setIsAbortCondition, getActiveCondition()->setIsAbortCondition,
                                   getActiveCondition() != 0, bool, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(isAbortCondition, getActiveCondition()->isAbortCondition,
                                bool, getActiveCondition() != 0, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_0_STATIC_ACTIVE(completeConditions,
                                   StrategyManager::getInstance()->completeConditions,
                                   StrategyManager::getInstance() != 0, kAiStrategyConsoleTag)
BF2_CONSOLE_PROPERTY_FN(targetValue, float, getActiveCondition() != 0,
                        getActiveCondition()->getTargetValue(),
                        getActiveCondition()->setTargetValue, kAiStrategyConsoleTag)
BF2_CONSOLE_METHOD_2_RET_ACTIVE(forceStrategy, StrategyManager::getInstance()->forceStrategy,
                                std::string, StrategyManager::getInstance() != 0,
                                std::string, int, kAiStrategyConsoleTag)
BF2_CONSOLE_PROPERTY_FN(enableDebug, bool, StrategyManager::getInstance() != 0,
                        StrategyManager::getInstance()->getEnableDebug(),
                        StrategyManager::getInstance()->setEnableDebug, kAiStrategyConsoleTag)

// Handed to the console registry by AIModule::init.
ConsoleObjectList& aiStrategyConsoleObjects()
{
    return consoleBindingList(kAiStrategyConsoleTag);
}

} // namespace ai
} // namespace hfe
} // namespace dice
