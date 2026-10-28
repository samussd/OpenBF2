
#include "BotBehaviourManager.h"

#include "../AIBotBehaviour/UrgeCurve.h"     // UCConstant / UCLinear / UCXInverse - the three factories
#include "../AIUtils/checkPointer.h"         // inlined 26 times in the constructor
#include "AIDllFrontier.h"
#include "../AIBotBehaviour/BBAvoid.h"
#include "../AIBotBehaviour/BBBridgeRepair.h"
#include "../AIBotBehaviour/BBC4.h"
#include "../AIBotBehaviour/BBChange.h"
#include "../AIBotBehaviour/BBChangeLandingCraft.h"
#include "../AIBotBehaviour/BBChangeTeleport.h"
#include "../AIBotBehaviour/BBDefuseMines.h"
#include "../AIBotBehaviour/BBFire.h"
#include "../AIBotBehaviour/BBFire3d.h"
#include "../AIBotBehaviour/BBFireInfantery.h"
#include "../AIBotBehaviour/BBFireLargeBore.h"
#include "../AIBotBehaviour/BBFireUnarmed.h"
#include "../AIBotBehaviour/BBIdle.h"
#include "../AIBotBehaviour/BBMedicAssist.h"
#include "../AIBotBehaviour/BBMoveTo.h"
#include "../AIBotBehaviour/BBMoveToFixed.h"
#include "../AIBotBehaviour/BBRandom.h"
#include "../AIBotBehaviour/BBRevive.h"
#include "../AIBotBehaviour/BBRunOverSoldier.h"
#include "../AIBotBehaviour/BBTriggerable.h"
#include "../AIBotBehaviourPattern/BBPAvoidCollision2d.h"
#include "../AIBotBehaviourPattern/BBPAvoidCollision3d.h"
#include "../AIBotBehaviourPattern/BBPAvoidCollisionInfantery.h"
#include "../AIBotBehaviourPattern/BBPBridgeRepair.h"
#include "../AIBotBehaviourPattern/BBPC4.h"
#include "../AIBotBehaviourPattern/BBPChange.h"
#include "../AIBotBehaviourPattern/BBPChangeLandingCraft.h"
#include "../AIBotBehaviourPattern/BBPChangeTeleport.h"
#include "../AIBotBehaviourPattern/BBPDefuseMines.h"
#include "../AIBotBehaviourPattern/BBPFire2d.h"
#include "../AIBotBehaviourPattern/BBPFire3d.h"
#include "../AIBotBehaviourPattern/BBPFireInfantery.h"
#include "../AIBotBehaviourPattern/BBPGotoWaypoint2d.h"
#include "../AIBotBehaviourPattern/BBPGotoWaypoint3d.h"
#include "../AIBotBehaviourPattern/BBPGotoWaypointBoat.h"
#include "../AIBotBehaviourPattern/BBPGotoWaypointSoldier.h"
#include "../AIBotBehaviourPattern/BBPHeliFire3d.h"
#include "../AIBotBehaviourPattern/BBPIdle2d.h"
#include "../AIBotBehaviourPattern/BBPIdle3d.h"
#include "../AIBotBehaviourPattern/BBPIdleInfantery.h"
#include "../AIBotBehaviourPattern/BBPMedicAssist.h"
#include "../AIBotBehaviourPattern/BBPRandom.h"
#include "../AIBotBehaviourPattern/BBPRevive.h"
#include "../AIBotBehaviourPattern/BBPRunOverSoldier.h"
#include "../AIBotBehaviourPattern/BBPTriggerable.h"

namespace dice {
namespace hfe {
namespace ai {

BotBehaviourManager* BotBehaviourManager::m_instance = 0;

// Behavior factories registered by name.

namespace {

// - builds one `BBAvoid`.
class BBCreateFunctionAvoid : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBMoveTo`.
class BBCreateFunctionMoveTo : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBMoveToFixed`.
class BBCreateFunctionMoveToFixed : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBMedicAssist`.
class BBCreateFunctionMedicAssist : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBIdle`.
class BBCreateFunctionIdle : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFire`.
class BBCreateFunctionFire : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFire3d`.
class BBCreateFunctionFire3d : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireInfantery`.
class BBCreateFunctionFireInfantery : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireUnarmed`.
class BBCreateFunctionFireUnarmed : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireLargeBore`.
class BBCreateFunctionFireLargeBore : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTakeCover`.
class BBCreateFunctionTakeCover : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTakeCoverInfantry`.
class BBCreateFunctionTakeCoverInfantry : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireDriver`.
class BBCreateFunctionFireDriver : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBChange`.
class BBCreateFunctionChange : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBChangeLandingCraft`.
class BBCreateFunctionChangeLandingCraft : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBChangeTeleport`.
class BBCreateFunctionChangeTeleport : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBSuicide`.
class BBCreateFunctionSuicide : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBCallForMedic`.
class BBCreateFunctionCallForMedic : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBRevive`.
class BBCreateFunctionRevive : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTakeCover3d`.
class BBCreateFunctionTakeCover3d : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBC4`.
class BBCreateFunctionC4 : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBRunOverSoldier`.
class BBCreateFunctionRunOverSoldier : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBBridgeRepair`.
class BBCreateFunctionBridgeRepair : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBDefuseMines`.
class BBCreateFunctionDefuseMines : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBRandom`.
class BBCreateFunctionRandom : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTriggerable`.
class BBCreateFunctionTriggerable : public BBCreateFunction
{
public:
    virtual BotBehaviour* operator()(int behaviourId, std::string& name, int mode,
                                     UrgeCurve* urgeCurve, float* weights);
};

} // anonymous namespace

// Initializes the behavior registry and its owned collections.

BotBehaviourManager::BotBehaviourManager()
{
    m_instance = this;

    checkPointer(m_createFunctions["BBAvoid"]              = new BBCreateFunctionAvoid());
    checkPointer(m_createFunctions["BBMoveTo"]             = new BBCreateFunctionMoveTo());
    checkPointer(m_createFunctions["BBMoveToFixed"]        = new BBCreateFunctionMoveToFixed());
    checkPointer(m_createFunctions["BBMedicAssist"]        = new BBCreateFunctionMedicAssist());
    checkPointer(m_createFunctions["BBIdle"]               = new BBCreateFunctionIdle());
    checkPointer(m_createFunctions["BBFire"]               = new BBCreateFunctionFire());
    checkPointer(m_createFunctions["BBFire3d"]             = new BBCreateFunctionFire3d());
    checkPointer(m_createFunctions["BBFireInfantery"]      = new BBCreateFunctionFireInfantery());
    checkPointer(m_createFunctions["BBFireUnarmed"]        = new BBCreateFunctionFireUnarmed());
    checkPointer(m_createFunctions["BBFireLargeBore"]      = new BBCreateFunctionFireLargeBore());
    checkPointer(m_createFunctions["BBTakeCover"]          = new BBCreateFunctionTakeCover());
    checkPointer(m_createFunctions["BBTakeCoverInfantry"]  = new BBCreateFunctionTakeCoverInfantry());
    checkPointer(m_createFunctions["BBFireDriver"]         = new BBCreateFunctionFireDriver());
    checkPointer(m_createFunctions["BBChange"]             = new BBCreateFunctionChange());
    checkPointer(m_createFunctions["BBChangeLandingCraft"] = new BBCreateFunctionChangeLandingCraft());
    checkPointer(m_createFunctions["BBChangeTeleport"]     = new BBCreateFunctionChangeTeleport());
    checkPointer(m_createFunctions["BBSuicide"]            = new BBCreateFunctionSuicide());
    checkPointer(m_createFunctions["BBCallForMedic"]       = new BBCreateFunctionCallForMedic());
    checkPointer(m_createFunctions["BBRevive"]             = new BBCreateFunctionRevive());
    checkPointer(m_createFunctions["BBTakeCover3d"]        = new BBCreateFunctionTakeCover3d());
    checkPointer(m_createFunctions["BBC4"]                 = new BBCreateFunctionC4());
    checkPointer(m_createFunctions["BBRunOverSoldier"]     = new BBCreateFunctionRunOverSoldier());
    checkPointer(m_createFunctions["BBBridgeRepair"]       = new BBCreateFunctionBridgeRepair());
    checkPointer(m_createFunctions["BBDefuseMines"]        = new BBCreateFunctionDefuseMines());
    checkPointer(m_createFunctions["BBRandom"]             = new BBCreateFunctionRandom());
    checkPointer(m_createFunctions["BBTriggerable"]        = new BBCreateFunctionTriggerable());
}

// Clears behavior state and registry containers. Factory objects are not deleted here.

BotBehaviourManager::~BotBehaviourManager()
{
    m_instance = 0;
    reset();
}

// The only virtual, slot 0.
// It drops everything the `.ai` scripts defined - the behaviour names, their ids and the urge
// curves - and keeps the twenty-six create functions, which are compiled in and outlive any level.
// The curve loop is read literally: for each node, load the mapped pointer at `+20`, and if it is
// non-null call `(*(*ptr + 4))(ptr)` - Itanium vtable slot 1, the deleting destructor, i.e. source
// `delete`. Then advance, rebalance-for-erase, release the key string and free the node.

bool BotBehaviourManager::reset()
{
    m_behaviourIds.clear();

    std::map<std::string, UrgeCurve*, NoCaseStringCompare>::iterator it = m_urgeCurves.begin();
    while (it != m_urgeCurves.end())
    {
        if (it->second)
            delete it->second;
        m_urgeCurves.erase(it++);
    }

    m_behaviourNames.clear();
    return true;
}

// The behaviour name/id table.

// Associates a behavior ID with its script-visible name.
void BotBehaviourManager::setBehaviourName(int id, std::string name)
{
    m_behaviourNames.resize(id + 1);
    m_behaviourNames[id] = name;
    m_behaviourIds[name] = id;
}

// . -1 for "no such behaviour"; `create` below tests for exactly that.
int BotBehaviourManager::getBehaviourId(std::string name)
{
    std::map<std::string, int, NoCaseStringCompare>::iterator it = m_behaviourIds.find(name);
    if (it != m_behaviourIds.end())
        return it->second;

    return -1;
}

// Requires a nonnegative id. The signed upper-bound comparison intentionally permits
// negative values, which would index before the vector.
std::string BotBehaviourManager::getBehaviourName(int id)
{
    if (id < (int)m_behaviourNames.size())
        return m_behaviourNames[id];

    return "";
}

// Creates a named behavior through a registered factory.

BotBehaviour* BotBehaviourManager::create(std::string behaviourName, std::string createFunctionName,
                                std::string name, int mode, std::string urgeCurveName,
                                float* weights, std::string& message)
{
    const int behaviourId = getBehaviourId(behaviourName);
    if (behaviourId == -1)
    {
        message = "'" + behaviourName + "' doesn't exist";
        return 0;
    }

    UrgeCurve* urgeCurve = 0;
    std::map<std::string, UrgeCurve*, NoCaseStringCompare>::iterator curve =
        m_urgeCurves.find(urgeCurveName);
    if (curve != m_urgeCurves.end())
        urgeCurve = curve->second;

    BBCreateFunction* const createFunction = m_createFunctions[createFunctionName];
    if (createFunction)
        return (*createFunction)(behaviourId, name, mode, urgeCurve, weights);

    message = "AIConsole::constructBehaviour, internal error";
    return 0;
}

// Creates and registers an urgency curve under the supplied name.

std::string BotBehaviourManager::createUCConstant(std::string& name, float value)
{
    m_urgeCurves[name] = new UCConstant(value);
    return "UCConstant created";
}

std::string BotBehaviourManager::createUCLinear(std::string& name, float slope, float offset)
{
    m_urgeCurves[name] = new UCLinear(slope, offset);
    return "UCLinear created";
}

std::string BotBehaviourManager::createUCXInverse(std::string& name, float numerator, float offset,
                                                  float slope, float bias)
{
    m_urgeCurves[name] = new UCXInverse(numerator, offset, slope, bias);
    return "UCXInverse created";
}

// Each factory picks its pattern by script name; an unknown name builds the behaviour without one.
// The six that no shipped script uses remain stops.

namespace {

BotBehaviour* BBCreateFunctionAvoid::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPAvoidCollision* generator = 0;
    if (name == "BBPAvoidCollision2d")
        generator = checkPointer(new BBPAvoidCollision2d(behaviourId));
    else if (name == "BBPAvoidCollision3d")
        generator = checkPointer(new BBPAvoidCollision3d(behaviourId));
    else if (name == "BBPAvoidCollisionInfantery")
        generator = checkPointer(new BBPAvoidCollisionInfantery(behaviourId));
    return checkPointer(new BBAvoid(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionMoveTo::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPGotoWaypoint* generator = 0;
    if (name == "BBPGotoWaypoint2d")
        generator = checkPointer(new BBPGotoWaypoint2d(behaviourId));
    else if (name == "BBPGotoWaypointBoat")
        generator = checkPointer(new BBPGotoWaypointBoat(behaviourId));
    else if (name == "BBPGotoWaypoint3d")
        generator = checkPointer(new BBPGotoWaypoint3d(behaviourId));
    else if (name == "BBPGotoWaypointInfantery")
        generator = checkPointer(new BBPGotoWaypointSoldier(behaviourId));
    return checkPointer(new BBMoveTo(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

// Builds the behaviour with no pattern; the name, the urge curve and the weights are ignored.
BotBehaviour* BBCreateFunctionMoveToFixed::operator()(int behaviourId, std::string&, int mode,
                                    UrgeCurve*, float*)
{
    return checkPointer(new BBMoveToFixed(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode)));
}

BotBehaviour* BBCreateFunctionMedicAssist::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPMedicAssist")
        generator = checkPointer(new BBPMedicAssist(behaviourId));
    return checkPointer(new BBMedicAssist(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionIdle::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPIdle* generator = 0;
    if (name == "BBPIdle2d")
        generator = checkPointer(new BBPIdle2d(behaviourId));
    else if (name == "BBPIdle3d")
        generator = checkPointer(new BBPIdle3d(behaviourId));
    else if (name == "BBPIdleInfantery")
        generator = checkPointer(new BBPIdleInfantery(behaviourId));
    return checkPointer(new BBIdle(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionFire::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPFire* generator = 0;
    if (name == "BBPFire2d")
        generator = checkPointer(new BBPFire2d(behaviourId));
    else if (name == "BBPFireInfantery")
        generator = checkPointer(new BBPFireInfantery(behaviourId));
    return checkPointer(new BBFire(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionFire3d::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPFire* generator = 0;
    if (name == "BBPFire3d")
        generator = checkPointer(new BBPFire3d(behaviourId));
    else if (name == "BBPHeliFire3d")
        generator = checkPointer(new BBPHeliFire3d(behaviourId));
    return checkPointer(new BBFire3d(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionFireInfantery::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPFireInfantery* generator = 0;
    if (name == "BBPFireInfantery")
        generator = checkPointer(new BBPFireInfantery(behaviourId));
    return checkPointer(new BBFireInfantery(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

// Builds the behaviour with no pattern; the name, the urge curve and the weights are ignored.
BotBehaviour* BBCreateFunctionFireUnarmed::operator()(int behaviourId, std::string&, int mode,
                                    UrgeCurve*, float*)
{
    return checkPointer(new BBFireUnarmed(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode)));
}

BotBehaviour* BBCreateFunctionFireLargeBore::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPFire* generator = 0;
    if (name == "BBPFire2d")
        generator = checkPointer(new BBPFire2d(behaviourId));
    else if (name == "BBPFireInfantery")
        generator = checkPointer(new BBPFireInfantery(behaviourId));
    return checkPointer(new BBFireLargeBore(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights, false));
}

// Creation of BBTakeCover and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
BotBehaviour* BBCreateFunctionTakeCover::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionTakeCover::operator()");
}

// Creation of BBTakeCoverInfantry and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
BotBehaviour* BBCreateFunctionTakeCoverInfantry::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionTakeCoverInfantry::operator()");
}

// Creation of BBFireDriver and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
BotBehaviour* BBCreateFunctionFireDriver::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionFireDriver::operator()");
}

BotBehaviour* BBCreateFunctionChange::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPChange")
        generator = checkPointer(new BBPChange(behaviourId));
    return checkPointer(new BBChange(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionChangeLandingCraft::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPChangeLandingCraft")
        generator = checkPointer(new BBPChangeLandingCraft(behaviourId));
    else if (name == "BBPChange")
        generator = checkPointer(new BBPChange(behaviourId));
    return checkPointer(new BBChangeLandingCraft(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionChangeTeleport::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPChangeTeleport")
        generator = checkPointer(new BBPChangeTeleport(behaviourId));
    return checkPointer(new BBChangeTeleport(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

// Creation of BBSuicide and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
BotBehaviour* BBCreateFunctionSuicide::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionSuicide::operator()");
}

// Creation of BBCallForMedic and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
BotBehaviour* BBCreateFunctionCallForMedic::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionCallForMedic::operator()");
}

BotBehaviour* BBCreateFunctionRevive::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    BBPRevive* generator = 0;
    if (name == "BBPRevive")
        generator = checkPointer(new BBPRevive(behaviourId));
    return checkPointer(new BBRevive(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

// Creation of BBTakeCover3d and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
BotBehaviour* BBCreateFunctionTakeCover3d::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionTakeCover3d::operator()");
}

BotBehaviour* BBCreateFunctionC4::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPC4")
        generator = checkPointer(new BBPC4(behaviourId));
    return checkPointer(new BBC4(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionRunOverSoldier::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPRunOverSoldier")
        generator = checkPointer(new BBPRunOverSoldier(behaviourId));
    return checkPointer(new BBRunOverSoldier(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionBridgeRepair::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPBridgeRepair")
        generator = checkPointer(new BBPBridgeRepair(behaviourId));
    return checkPointer(new BBBridgeRepair(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionDefuseMines::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPDefuseMines")
        generator = checkPointer(new BBPDefuseMines(behaviourId));
    return checkPointer(new BBDefuseMines(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionRandom::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPRandom")
        generator = checkPointer(new BBPRandom(behaviourId));
    return checkPointer(new BBRandom(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

BotBehaviour* BBCreateFunctionTriggerable::operator()(int behaviourId, std::string& name, int mode,
                                    UrgeCurve* urgeCurve, float* weights)
{
    PlanGenerator* generator = 0;
    if (name == "BBPTriggerable")
        generator = checkPointer(new BBPTriggerable(behaviourId));
    return checkPointer(new BBTriggerable(static_cast<unsigned char>(behaviourId),
                               static_cast<unsigned char>(mode), generator, urgeCurve, weights));
}

} // anonymous namespace

} // namespace ai
} // namespace hfe
} // namespace dice
