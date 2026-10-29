// Bot implementation: owns its behavior, movement, sensing and communication state.
// The manager schedules work through the Bot interface; unimplemented operations stop by name.

#include "BotMain.h"
#include "BotOrder.h"
#include "BotCallBack.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/AIMain.h"
#include "../AIStrategic/SAI.h"
#include "BotMemory.h"
#include "BotNavigation.h"
#include "BotSquad.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "SensingAgent.h"
#include "BotSenses.h"                   // getSensedObjects
#include "CommunicationAgent.h"
#include "BotSubObjects.h"
#include "../AIBotBehaviour/UrgencyMerger.h"
#include "../AIBotBehaviour/BotBehaviour.h"          // updateBot / addBehaviour
#include "../AIBotActionPlan/BotScript.h"             // planExecution: the plan's owner
#include "../AIPlanInterpreter/PlanInterpreter.h"
#include "../../../AIInterface/BFKnowledge/BFCollisionHandler.h"
#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../AIKnowledge/IPIUnit.h"                  // updateBot: the equipment type
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IBotManager.h"                     // updateBotVehicle
#include "WPEmpty.h"
#include "WPAltitudeMoveTo.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../AIUtils/checkPointer.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/Information.h"      // updatePlayerAction: the equipment's mobile plug-in
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIArmament.h"             // rearm
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../../../../Core/Debug.h"
#include <cfloat>       // DBL_MAX - the two 'never happened' timestamps

namespace dice {
namespace hfe {
namespace ai {

// The base destructor leaves member lifetime to the derived class.
Bot::~Bot()
{
}

// Returns base bot type 1; BotMain overrides this with type 0.
int Bot::getBotType() const
{
    return 1;
}

// Returns the context stored in Bot. BotMain inherits this accessor.
BAPContext* Bot::getScriptContext()
{
    return m_bapContext;
}

// Constructs the per-bot state and helper objects. The supplied player is borrowed.
BotMain::BotMain(IAIPlayer* player,
                 unsigned int id,
                 float* weights,
                 float f,
                 int i,
                 BotOrder* order,
                 BotMemory* memory,
                 PlanInterpreter* interpreter,
                 int n,
                 UrgencyGenerator* merger,
                 UrgencyGenerator** generators)
{
    m_debugger  = 0;
    m_lodLevel  = BotLodFull;           // = 0
    m_planInvalid = true;
    m_hasDefaultPlan = false;
    m_botId        = id;
    m_basicWeights = weights;
    m_float32      = f;
    m_obedience      = 1.0f;
    m_int40        = i;
    m_ptr52        = 0;
    m_spawnToStrategicAreaId        = -1;
    m_side        = n;
    m_vehicleMods  = 0;
    m_float48      = 0.01f;
    m_inBadVehicle = false;             // +89
    m_memory       = memory;
    m_vehicleChanged = false;           // +90
    m_inTransitionPhase = false;        // +91
    m_pad92        = 0;                 // +92
    m_deactivated  = false;             // +93
    m_activePlan = 0;
    m_planLock = false;                 // +120

    // List members construct themselves. [UNRESOLVED] Some element types remain unknown.

    m_currentEquipment            = -1;
    m_planInterpreter   = interpreter;
    m_urgencyMerger     = merger;
    m_urgencyGenerators = generators;
    m_ptr160 = 0;
    m_ptr164 = 0;
    m_ptr168 = 0;

    // sub-object 1 of 6
    m_fireCorrection = checkPointer(new FireCorrectionData());

    for (int k = 0; k < 5; ++k)
        (&m_changeTarget)[k] = static_cast<unsigned int>(-1);   // +204..+220
    m_mineTarget = -1;
    m_moveInclination = 0;
    m_fireInclination = 0;
    m_moveInclinationDelta = 1.0f;
    m_fireInclinationDelta = 1.0f;
    m_ptr232 = 0;
    m_ptr256 = 0;
    m_ptr260 = 0;
    m_firingTarget = -1;
    m_requestedSoldierPose = 1;
    m_poseTestTarget = -1;
    m_unresolvedCollision = -1;
    m_ptr252 = 0;

    // Use -DBL_MAX for uninitialized timestamps; magnitude sentinels use separate values.
    m_time276 = -DBL_MAX;
    m_poseChangeTime = -DBL_MAX;
    m_botStuck = false;
    m_poseChanging = false;
    for (int k = 0; k < 4; ++k)
        m_poseTestResults[k] = false;           // +300..+303
    m_airBorn = false;                  // +312
    for (int k = 0; k < 3; ++k)
        m_spawnCoord[k] = 0.0f;         // +316..+324
    m_deathCallBack = 0;                // +328
    m_spawnCallBack = 0;                // +332
    m_squadMember = 0;
    m_assignedAreas[0] = -1;
    m_assignedAreas[3] = -1;                   // +396
    m_assignedAreas[2] = -1;                   // +392
    m_ptr400 = 0;
    m_assignedAreas[1] = -1;                   // +388
    m_manDown = false;                  // +360
    m_isFiring = false;                 // +372
    m_fireAllowed = true;
    m_ptr404 = 0;
    m_ptr408 = 0;
    m_int412 = -1;
    m_stalledPlanTicks = 0;
    m_c4.position.x = 0.0f;              // +428..+436
    m_c4.position.y = 0.0f;
    m_c4.position.z = 0.0f;
    m_moveToStatus = 0;
    m_timeOfLastFailedStaticAttack = 0.0;
    m_timeOfLastFailedStaticSkyAttack = 0.0;
    m_avoidSide = 0;
    for (int k = 0; k < 3; ++k)
        (&m_wantsToSimulateArtillery)[k] = false;   // +500..+502
    m_avoidSideSwitched = false;        // +512
    m_collidedObject = -1;
    m_double516 = 0.0;
    for (int k = 0; k < 3; ++k)
        m_collidedObjectPosition[k] = 0;                // +528..+536

    // A different sentinel from the two above - -9999.0 is a magnitude, not a time.
    m_lastCheckMoveToFinishedTime = -9999.0;
    m_timeOfLastFailedSplashDamageAttack = -9999.0;
    m_slowDownToTarget    = -1;
    m_friendsInLineOfFireTime = 0.0;
    m_ignoreObstacles = false;          // +540
    m_isGoingForFlag = false;
    m_useStaticAttack = false;

    // copied component by component out of Vec3::getZero, which is what the binary does.
    const Vec3& zero = Vec3::getZero();
    m_takeOffPosition[0] = zero.x;
    m_takeOffPosition[1] = zero.y;
    m_takeOffPosition[2] = zero.z;

    m_grenadeThrownTime[0] = 0.0;
    m_grenadeThrownTime[1] = 0.0;
    m_grenadeThrownTime[2] = 0.0;

    // sub-objects 2 to 5
    m_navigation = checkPointer(new BotNavigation(this));
    m_squad      = checkPointer(new BotSquad(this));
    m_sensingAgent = checkPointer(new SensingAgent(this, mu8NBehaviours));

    // this byte is set in the middle of the allocations, between SensingAgent and
    // CommunicationAgent, and it is -1 rather than 0. Reproduced in position.
    m_gap64[0] = static_cast<char>(-1);  // +64
    m_communication = checkPointer(new CommunicationAgent(this));

    // Allocates matching urgency and behavior arrays, but clears only behavior pointers.
    // Urgency values remain uninitialized until written.
    m_behaviourUrgencies = checkPointer(new float[mu8NBehaviours]);
    m_behaviours = checkPointer(new BotBehaviour*[mu8NBehaviours]);
    for (unsigned k = 0; k < mu8NBehaviours; ++k)
        m_behaviours[k] = 0;

    // finishing someone else'S object - see the block above.
    m_memory->m_currentBehaviour = -1;
    m_memory->m_behaviourTime = 0.0;

    if (order)
        recieveOrder(order);            // dice's spelling

    m_waypoints = 0;
    m_ptr84  = 0;
    m_player = player;
    if (player)
        setPlayerInput();

    // zeroed A second time - see the block above.
    m_ptr160 = 0;
    m_ptr164 = 0;
    m_ptr168 = 0;

    // sub-object 6 of 6, and it is the smallest thing here at four bytes
    m_bapContext = checkPointer(new BAPContext());

    // 256 is BF2'S player cap - the same number BotManager's player-id table uses. Every bot can
    // sense every player, and the storage for that is committed at construction.
    m_sensedObjects.reserve(256);

    m_c4.placed = false;                 // the constructor's last field store
    reset();
}


// [CHOSEN, not recovered] Initial behavior count is zero until configured by the module.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.
unsigned char BotMain::mu8NBehaviours = 0;
// ELF .bss / PE .data . Set by prepareBehaviourPool, not per bot.
float* BotMain::maBasicWeights = 0;

// [FRONTIER] Remaining bot lifecycle operations stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.
BotMain::~BotMain()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::~BotMain");
}

// Releases each behavior's initialized state without deleting the behavior itself.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.
void BotMain::uninitAllBehaviours()
{
    for (int i = 0; i < mu8NBehaviours; ++i)
    {
        if (m_behaviours[i])
        {
            // Retail here is `m_behaviours[i]->something->uninit(this)` - see the block above.
            BF2_AI_FRONTIER("dice::hfe::ai::BotMain::uninitAllBehaviours (a behaviour is present)");
        }
    }
}

// Resets per-life bot state for reuse. Dependent operations may reach named frontier stops.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void BotMain::reset()
{
    m_double172 = 0.0;

    if (m_player)
    {
        // Retail calls IAIPlayer byte-slot +8 with no arguments. See the block above.
        BF2_AI_FRONTIER("dice::hfe::ai::BotMain::reset (IAIPlayer slot +8)");
    }

    if (m_activePlan)
    {
        // Retail destroys the plan through byte-slot +8. See the block above.
        BF2_AI_FRONTIER("dice::hfe::ai::BotMain::reset (active plan present)");
        m_activePlan = 0;
        m_planInvalid = true;
        m_sensingAgent->setOverride(false);
    }

    m_list128.clear();
    m_orderedBehaviour = -1;

    // the five. See the block above.
    uninitAllBehaviours();
    m_squad->reset();
    m_navigation->resetAll();
    m_sensingAgent->reset();
    m_communication->reset();

    m_stalledPlanTicks = 0;
    m_int200 = -1;
    m_poseTestTarget = -1;
    m_avoidSide = 0;
    m_moveToStatus = 0;
    m_collidedObject = -1;
    m_rearmTime = 0.0;
    m_unresolvedCollision = -1;
    m_grenadeThrownTime[0] = 0.0;
    m_grenadeThrownTime[1] = 0.0;
    m_grenadeThrownTime[2] = 0.0;
    m_timeOfLastFailedStaticAttack = 0.0;
    m_timeOfLastFailedStaticSkyAttack = 0.0;
    m_double516 = 0.0;
    m_wantsToSimulateArtillery = false;
    m_isSimulatingArtillery = false;
    m_poseForced = false;
    m_avoidSideSwitched = false;
    m_collidedObjectPosition[0] = 0;
    m_collidedObjectPosition[1] = 0;
    m_collidedObjectPosition[2] = 0;
    m_friendsInLineOfFireTime = 0.0;
    // three of the five, and not the first three: the change and revive targets survive a
    // reset while the three repair targets do not.
    m_wantedRepairTarget = static_cast<unsigned int>(-1);
    m_repairTarget       = static_cast<unsigned int>(-1);
    m_bridgeRepairTarget = static_cast<unsigned int>(-1);
    m_ignoreObstacles = false;
    m_slowDownToTarget = -1;
    m_lastCheckMoveToFinishedTime = -9999.0;
    m_isRepaired           = false;
    m_isEngineerTarget     = false;
    m_shouldWaitForRepairs = false;
    m_isGoingForFlag = false;
    m_useStaticAttack = false;
    // 9999.0, not the ctor's -DBL_MAX. See the block above.
    m_poseChangeTime = -9999.0;
    m_timeOfLastFailedSplashDamageAttack = -9999.0;
    m_takeOffPosition[0] = 0.0f;
    m_takeOffPosition[1] = 0.0f;
    m_takeOffPosition[2] = 0.0f;
}

// Both materialise a Boolean result. The pointer at +68 is the
// order's weights, not the BotOrder object. There is no copy, deletion or null-order guard.
bool BotMain::recieveOrder(BotOrder* order)
{
    if (!m_takingOrders)
        return false;
    m_currentOrders = order->m_weights;
    m_orderedBehaviour = static_cast<signed char>(order->m_behaviour);
    m_list128.clear();
    if (m_vehicleMods)
        calculateMods();
    return true;
}

// Binding a player immediately prepares its input; no null guard.
void BotMain::setPlayer(IAIPlayer* player)
{
    m_player = player;
    setPlayerInput();
}

// Clear only the 64 values; existing mapping/state bits survive.
// The engine's current getPlayerInput declaration is const void*, but both images write through
// the returned pointer. This cast preserves that existing ABI pending its wider type recovery.
void BotMain::setPlayerInput()
{
    m_playerInput = const_cast<io::PlayerInput*>(
        static_cast<const io::PlayerInput*>(m_player->getPlayerInput()));
    for (unsigned int i = 0; i != 64; ++i)
        m_playerInput->inputs[i] = 0.0f;
    m_playerInput->mappingFlags |= 1ULL;
    m_playerInput->setInputIsMapped(io::PIPitch, true);
    m_playerInput->setInputIsMapped(io::PIRoll, true);
    m_playerInput->setInputIsMapped(io::PIThrottle, true);
    m_playerInput->setInputIsMapped(io::PISprint, true);
    m_playerInput->setInputIsMapped(io::PIUse, true);
    m_playerInput->setInputIsMapped(io::PIMouseLook, true);
    m_playerInput->setInputIsMapped(io::PIMouseLookX, true);
    m_playerInput->setInputIsMapped(io::PIMouseLookY, true);
    m_playerInput->setInputIsMapped(io::PIFire, true);
    m_playerInput->setInputIsMapped(io::PIAltFire, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect1, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect2, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect3, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect4, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect5, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect6, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect7, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect8, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect9, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect1, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect2, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect3, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect4, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect5, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect6, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect7, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect8, true);
    m_playerInput->setInputIsMapped(io::PILie, true);
    m_playerInput->setInputIsMapped(io::PICrouch, true);
    m_playerInput->setInputIsMapped(io::PIUse, true); // repeated in both images
    m_playerInput->setInputIsMapped(io::PIReload, true);
    m_playerInput->setInputIsMapped(io::PIFlareFire, true);
    m_playerInput->setInputIsMapped(io::PIAction, true);
}

// ELF 0x18A2C0 / PE 0x1017EB20. [PLATFORM] Both products use the same factors, but the
// instructions multiply in different orders. Keep each order instead of reassociating it.
UrgencyGenerator** BotMain::getBehaviourUrgencyGenerators()
{
    return m_urgencyGenerators;
}

UrgencyGenerator* BotMain::getSelector()
{
    return m_urgencyMerger;
}

PlanInterpreter* BotMain::getPlanInterpreter()
{
    return m_planInterpreter;
}

void BotMain::setPlanInterpreter(PlanInterpreter* interpreter)
{
    m_planInterpreter = interpreter;
}

// Files a behaviour in slot index and, the first time its id is seen, under its mode. A null
// behaviour only clears the slot.
void BotMain::addBehaviour(BotBehaviour* behaviour, int index)
{
    if (behaviour)
    {
        if (m_behaviourIdsByMode.size() <= behaviour->getMode())
            m_behaviourIdsByMode.resize(behaviour->getMode() + 1);
        if (!m_behaviours[behaviour->getBehaviourId()])
            m_behaviourIdsByMode[behaviour->getMode()].push_back(behaviour->getBehaviourId());
    }
    m_behaviours[index] = behaviour;
    m_urgencyGenerators[index] = behaviour;
}
void BotMain::calculateMods()
{
    for (int i = 0; i < mu8NBehaviours; ++i)
    {
#if defined(_WIN32)
        if (m_takingOrders)
            m_behaviourUrgencies[i] = m_currentOrders[i] * m_vehicleMods[i] * maBasicWeights[i] * m_basicWeights[i];
        else
            m_behaviourUrgencies[i] = m_vehicleMods[i] * maBasicWeights[i] * m_basicWeights[i];
#else
        if (m_takingOrders)
            m_behaviourUrgencies[i] = m_currentOrders[i] * m_basicWeights[i] * maBasicWeights[i] * m_vehicleMods[i];
        else
            m_behaviourUrgencies[i] = maBasicWeights[i] * m_basicWeights[i] * m_vehicleMods[i];
#endif
    }
}

// 30/50/70 / /50/60/70: class statics, no this argument.
void BotMain::setNBehaviours(unsigned char n) { mu8NBehaviours = n; }
unsigned char BotMain::getNBehaviours() { return mu8NBehaviours; }
void BotMain::setBasicWeights(float* weights) { maBasicWeights = weights; }
float* BotMain::getBasicWeights() { return maBasicWeights; }

// per-bot accessors, unlike getBasicWeights above.
float* BotMain::getBasicBehaviourMods() { return m_basicWeights; }
float* BotMain::getCurrentBehaviourMods() { return m_behaviourUrgencies; }
void BotMain::setTakingOrders(bool taking) { m_takingOrders = taking; }

void BotMain::setSpawnCoord(const Vec3& coord)
{
    m_spawnCoord[0] = coord.x;
    m_spawnCoord[1] = coord.y;
    m_spawnCoord[2] = coord.z;
}

// Decision making and plan execution below remain frontiers; player binding, input preparation
// and initial order receipt above are recovered without claiming that the bot can execute them.


// The two plan-execution brackets. not vtable slots - `planExecution` at +0x010 is, but these
// two are ordinary members, so whatever calls them does so on a `BotMain*` and not through `Bot`.
// Before a step only the movement is released; the plan presses what it needs again.
void BotMain::prePlanExecution()
{
    resetAllControls(true, false, false);
}

// For a soldier, during the second after a friend stood in its line of fire: turn towards the
// side it is avoiding while that side has room, else switch to the other side when that one has
// room. Afterwards it avoids nothing. Retail does not guard the equipment.
void BotMain::postPlanExecution()
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(getCurrentEquipment());
    if (!(equipment->getObjectTypeFlags() & 0x200000u))
        return;
    const double sinceFriends =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime()
        - getFriendsInLineOfFireTime();
    if (!(1.0 > sinceFriends))
    {
        setAvoidSide(0);
        return;
    }
    if (m_avoidSide == 1)
    {
        if (getBotNavigation()->getSideSpace(0) > 0.5f)
            m_playerInput->setInput(io::PIYaw, -1.0f);
        else if (getBotNavigation()->getSideSpace(1) > 0.5f)
            setAvoidSide(2);
    }
    else
    {
        if (getBotNavigation()->getSideSpace(1) > 0.5f)
            m_playerInput->setInput(io::PIYaw, 1.0f);
        else if (getBotNavigation()->getSideSpace(0) > 0.5f)
            setAvoidSide(1);
    }
}

// The change target is chosen anew on every plan step.
void BotMain::resetChangeTarget()
{
    m_changeTarget = 0xFFFFFFFFu;
}

// Toggles the lie key: pressed when it is not held, released when it is. An unmapped key reads 0.
static void toggleLie(io::PlayerInput* input)
{
    const float held = (input->mappingFlags >> io::PILie) & 1 ? input->inputs[io::PILie] : 0.0f;
    input->setInput(io::PILie, held == 1.0f ? 0.0f : 1.0f);
}

// Presses the crouch and lie keys that take the soldier from its current pose to the requested
// one. Lower poses are only taken while the line-of-sight tests allow them, or when forced. A pose
// change in progress keeps the fire key released. A soldier with no pose is asked to stand.
// Retail returns early, skipping the fire release, for an unknown request while standing and after
// the unknown-pose error.
void BotMain::pollRequestedSoldierPose()
{
    const double time = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    Information* const equipment = informationManager->getInformation(m_currentEquipment);
    if (!equipment)
        return;
    bool detail;
    switch (equipment->getSoldierPose())
    {
    case SPStanding:
        switch (m_requestedSoldierPose)
        {
        case SPStanding:
            m_playerInput->setInput(io::PICrouch, 0.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
            m_poseChanging = false;
            break;
        case SPCrouching:
            if (!checkPoseOk(detail, SPCrouching) && !m_poseForced)
                break;
            m_playerInput->setInput(io::PICrouch, 1.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
            m_poseChanging = true;
            break;
        case SPLying:
            if (!checkPoseOk(detail, SPLying) && !m_poseForced)
                break;
            m_playerInput->setInput(io::PICrouch, 0.0f);
            toggleLie(m_playerInput);
            m_poseChangeTime = time;
            m_poseChanging = true;
            break;
        case SPSwimming:
            m_playerInput->setInput(io::PICrouch, 0.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
            m_poseChanging = false;
            break;
        default:
            return;
        }
        break;
    case SPCrouching:
        switch (m_requestedSoldierPose)
        {
        case SPStanding:
            m_playerInput->setInput(io::PILie, 0.0f);
            m_playerInput->setInput(io::PICrouch, 0.0f);
            clearTestResults();
            m_poseChanging = true;
            break;
        case SPCrouching:
            m_playerInput->setInput(io::PICrouch, 1.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
            m_poseChanging = false;
            break;
        case SPLying:
            if (!checkPoseOk(detail, SPLying) && !m_poseForced)
                break;
            m_playerInput->setInput(io::PICrouch, 0.0f);
            toggleLie(m_playerInput);
            m_poseChangeTime = time;
            m_poseChanging = true;
            break;
        case SPSwimming:
            m_playerInput->setInput(io::PICrouch, 0.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
            m_poseChanging = false;
            break;
        default:
            Debug(DebugTypeError, "AI/AIDLL/AI/AIBot/BotMain.cpp", 2723, "")
                << "Internal error, unknown pose.";
            return;
        }
        break;
    case SPLying:
        switch (m_requestedSoldierPose)
        {
        case SPStanding:
            m_playerInput->setInput(io::PICrouch, 0.0f);
            toggleLie(m_playerInput);
            clearTestResults();
            m_poseChangeTime = time;
            m_poseChanging = true;
            break;
        case SPCrouching:
            if (!checkPoseOk(detail, SPCrouching) && !m_poseForced)
                break;
            m_playerInput->setInput(io::PICrouch, 1.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
            m_poseChangeTime = time;
            m_poseChanging = true;
            break;
        case SPLying:
        case SPSwimming:
            m_playerInput->setInput(io::PICrouch, 0.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
            m_poseChanging = false;
            break;
        default:
            Debug(DebugTypeError, "AI/AIDLL/AI/AIBot/BotMain.cpp", 2760, "")
                << "Internal error, unknown pose.";
            return;
        }
        break;
    case SPSwimming:
        m_playerInput->setInput(io::PICrouch, 0.0f);
        m_playerInput->setInput(io::PILie, 0.0f);
        m_poseChanging = false;
        break;
    default:
        m_requestedSoldierPose = SPStanding;
        m_poseChanging = false;
        break;
    }
    m_poseForced = false;
    if (m_poseChanging && (equipment->getObjectTypeFlags() & 0x200000u))
        m_playerInput->setInput(io::PIFire, 0.0f);
}

// Whether the soldier may take the lower pose against its firing target. Any pose is fine with no
// target or a friendly one. Against an enemy only the tests made for that target count; detail
// receives the pose's second test flag.
bool BotMain::checkPoseOk(bool& detail, SoldierPose pose)
{
    // [PLATFORM] ELF captures the manager before the target callback.
    InformationManager* const manager = informationManager;
    Information* const target = manager->getInformation(getFiringTarget());
    if (!target || target->getSide() == getSide())
        return true;
    if (m_poseTestTarget != getFiringTarget())
        return false;
    if (pose == SPCrouching)
    {
        detail = m_poseTestResults[1];
        return m_poseTestResults[0];
    }
    detail = m_poseTestResults[3];
    return m_poseTestResults[2];
}

void BotMain::clearTestResults()
{
    m_poseTestTarget = 0xFFFFFFFFu;
    m_poseTestResults[0] = false;
    m_poseTestResults[1] = false;
    m_poseTestResults[2] = false;
    m_poseTestResults[3] = false;
}

// Bot interface implementations, in virtual declaration order.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.

// +0x008
void BotMain::setLodLevel(BotLodLevel level)
{
    m_lodLevel = level;
}

// +0x00C
BotLodLevel BotMain::getLodLevel() const
{
    return m_lodLevel;
}

// +0x010. One plan step. A projectile the bot fired that passed its point feeds the fire
// correction. Without a valid plan the bot asks its equipment's default behaviour (the man-down
// equipment's while down) for one. The plan and then the sensing agent's plan run. An invalid
// plan releases every control. Last come the weapon-select hold of a falling soldier and the
// pose input. Returns whether the plan is still valid.
bool BotMain::planExecution(const double&)
{
    bool ok = true;
    bool done = false;
    resetChangeTarget();
    IBFCollisionHandler* const handler = checkPointer(
        static_cast<IBFCollisionHandler*>(CommonSingletonServer::m_iBFCollisionHandler));
    if (handler->botProjectilePassedPoint(m_botId))
    {
        m_fireCorrection->setProjectile(handler->botProjectilePrePoint(m_botId),
                                        handler->botProjectilePostPoint(m_botId),
                                        handler->botProjectileTargetPoint(m_botId),
                                        handler->botProjectileMaxHeight(m_botId));
        handler->removeBotProjectile(m_botId);
    }

    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(getCurrentEquipment());
    if (m_planInvalid && m_activePlan)
    {
        delete m_activePlan;
        m_activePlan = 0;
        m_sensingAgent->setOverride(false);
    }
    if ((m_planInvalid || !m_activePlan) && equipment && equipment->getUnit())
    {
        IPIUnit* const unit = equipment->getUnit();
        const int type = unit->getEquipmentType();
        if (!m_manDown)
            unit->setTime();
        IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
        BotBehaviour* const behaviour = settings->getDefaultBehaviour(
            getManDown() ? settings->getManDownEquipmentType() : type);
        if (behaviour)
        {
            behaviour->getGenerator()->forceInitiated(this);
            m_activePlan = behaviour->generatePlan(this);
            if (m_activePlan)
            {
                m_activePlan->m_owner = this;
                m_hasDefaultPlan = true;
                m_planInvalid = false;
            }
        }
    }

    if (!m_planInvalid)
    {
        prePlanExecution();
        m_planInterpreter->execute(m_activePlan, this, ok, done);
        postPlanExecution();
        m_planInvalid = done || !ok;
        if (m_planInvalid)
        {
            setPlanLock(false);
            resetAllControls(true, true, true);
            m_sensingAgent->clearPlanTargetInformation();
        }
        m_sensingAgent->update();
        if (m_sensingAgent->hasPlan())
        {
            m_sensingAgent->disallowOverride(true);
            m_sensingAgent->executePlan();
            m_sensingAgent->disallowOverride(false);
        }
    }

    // Retail reads the current equipment again here without a null guard, then the mobile of the
    // equipment read above.
    Information* const current = informationManager->getInformation(m_currentEquipment);
    if (!(current->getObjectTypeFlags() & 0x200000u))
    {
        m_playerInput->setInput(io::PIWeaponSelect9, 0.0f);
    }
    else if (IPIMobile* const mobile = equipment->getMobile())
    {
        // A soldier falling at 1 m/s or more keeps a held weapon-select 9 down, past setInput.
        if (mobile->getSpeed().y <= -1.0f
            && ((m_playerInput->mappingFlags >> io::PIWeaponSelect9) & 1))
            m_playerInput->inputs[io::PIWeaponSelect9] = 1.0f;
    }
    pollRequestedSoldierPose();
    return !m_planInvalid;
}

// +0x014. The bot chooses what to do. A deactivated bot only drops its plan. The inclinations
// step first. A change of vehicle sets the bot up for its new equipment and tells the side's SAI;
// a vehicle the bot knows nothing about is an error ("Bot in bad vehicle!") and ends the call.
// Then the behaviours refresh their urgencies, the front of the highest priority list first, each
// moving out of its list as it is evaluated, until the slice of update time is spent. When the
// time runs out, the behaviours left in the lower lists (from list 1 down) refresh without the
// full evaluation and move up one list, so a behaviour that waits climbs. The urgency merger
// picks the behaviour, which writes the bot's plan; a new plan releases every control. The
// evaluated behaviours go back to the end of their own list. Returns the count of full
// evaluations, which BotManager::actionDecisionMaking banks in BotInfo +32 (see BotMain.h).
int BotMain::decisionMaking(const double& slice)
{
    std::list<int> evaluated;
    int count = 0;
    if (m_deactivated)
    {
        if (m_activePlan)
        {
            delete m_activePlan;
            m_activePlan = 0;
            m_planInvalid = true;
            m_sensingAgent->setOverride(false);
        }
        return 0;
    }

    const int previousBehaviour = m_memory->m_currentBehaviour;
    stepMoveInclination();
    stepFireInclination();
    if (!m_checkVehicle)
    {
        IAIObject* const vehicle = m_player->getCurrentVehicle();
        if (!vehicle)
        {
            m_currentEquipment = static_cast<unsigned int>(-1);
            m_checkVehicle = true;
        }
        else
        {
            m_checkVehicle = vehicle->getInformation(m_side) != m_currentEquipment;
        }
    }
    if (m_checkVehicle)
    {
        bool inVehicle;
        if (updateBotVehicle(&inVehicle))
        {
            if (m_activePlan)
            {
                delete m_activePlan;
                m_activePlan = 0;
                m_planInvalid = true;
                m_sensingAgent->setOverride(false);
            }
            if (!inVehicle || !informationManager->getInformation(m_currentEquipment))
            {
                Debug(DebugTypeError, "AI/AIDLL/AI/AIBot/BotMain.cpp", 749, "")
                    << "Bot in bad vehicle!";
                m_inBadVehicle = true;
                m_currentEquipment = static_cast<unsigned int>(-1);
                return 0;
            }
            updateBot();
            IAIMain::m_instance->getSAI(m_side)->setBotHasChangedEquipment(m_botId);
        }
        else if (m_inBadVehicle)
        {
            updateBot();
            m_navigation->reset();
            m_inBadVehicle = false;
            IAIMain::m_instance->getSAI(m_side)->setBotHasChangedEquipment(m_botId);
        }
        m_checkVehicle = false;
    }

    const IBFTimer* const timer = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double start = timer->getUpdateTime();
    if (m_vehicleChanged)
    {
        m_planInvalid = true;
        m_vehicleChanged = false;
    }
    if (m_planInvalid)
    {
        m_memory->m_currentBehaviour = -1;
        m_memory->m_behaviourTime = timer->getGameTime();
    }

    // The full pass: a behaviour's urgency, scaled while a plan runs by how long the current
    // behaviour has been chosen and by the current behaviour's weight for it.
    unsigned int mode = 0;
    bool timeOut = false;
    while (!timeOut && mode < m_behaviourIdsByMode.size())
    {
        std::list<unsigned short>& ids = m_behaviourIdsByMode[mode];
        if (ids.empty())
        {
            ++mode;
            continue;
        }
        const unsigned short id = ids.front();
        if (!m_behaviours[id])
        {
            ids.erase(ids.begin());     // a behaviour the equipment does not have
        }
        else
        {
            float urgency = m_behaviourUrgencies[id];
            if (!m_planInvalid && previousBehaviour != -1)
                urgency = urgency * (m_memory->modifyBehaviourUrgency(m_behaviours[id], timer->getGameTime())
                                     * m_behaviours[previousBehaviour]->getWeights()[id]);
            if (urgency > 0.0f)
            {
                ++count;
                m_behaviours[id]->calculateUrgency(this, true, urgency);
            }
            ids.erase(ids.begin());
            evaluated.push_back(id);
        }
        if (timer->getUpdateTime() >= start + slice)
            timeOut = true;
    }

    // Out of time: the lists below move up one, the highest list stays where it is.
    if (timeOut)
    {
        if (mode == 0)
            mode = 1;
        for (; mode < m_behaviourIdsByMode.size(); ++mode)
        {
            std::list<unsigned short>& ids = m_behaviourIdsByMode[mode];
            for (std::list<unsigned short>::iterator it = ids.begin(); it != ids.end();)
            {
                const unsigned short id = *it;
                if (!m_behaviours[id])
                {
                    it = ids.erase(it);
                    continue;
                }
                float urgency;
                if (!m_planInvalid && previousBehaviour != -1)
                    urgency = m_memory->modifyBehaviourUrgency(m_behaviours[id], timer->getGameTime())
                              * m_behaviourUrgencies[id]
                              * m_behaviours[previousBehaviour]->getWeights()[id];
                else
                    urgency = m_behaviourUrgencies[id];
                if (urgency > 0.0f)
                    m_behaviours[id]->calculateUrgency(this, false, urgency);
                ++it;
            }
            std::list<unsigned short>& higher = m_behaviourIdsByMode[mode - 1];
            higher.insert(higher.end(), ids.begin(), ids.end());
            ids.erase(ids.begin(), ids.end());
        }
    }

    if (m_planInvalid || m_urgencyMerger->hasChanged(this, 0.87f, 1.15f))
        m_urgencyMerger->updateActiveUrgency(this);
    // no null guard, as shipped
    BotBehaviour* const chosen = static_cast<BotBehaviour*>(m_urgencyMerger->getChosenGenerator());
    m_memory->m_currentBehaviour = chosen->getBehaviourId();
    m_memory->m_behaviourTime = timer->getGameTime();
    if (m_activePlan)
    {
        if (!m_planInvalid)
        {
            m_activePlan->m_owner = chosen;
        }
        else
        {
            delete m_activePlan;
            m_activePlan = 0;
            m_sensingAgent->setOverride(false);
        }
    }
    if (previousBehaviour != chosen->getBehaviourId())
        m_sensingAgent->clearPlanTargetInformation();

    BotScript* const plan = chosen->generatePlan(this);
    if (m_activePlan != plan)
    {
        if (m_activePlan)
        {
            m_activePlan->m_owner = this;
            delete m_activePlan;
            m_activePlan = 0;
            m_sensingAgent->setOverride(false);
        }
        resetAllControls(true, true, true);
    }
    m_activePlan = plan;
    if (!plan)
    {
        m_planInvalid = true;
    }
    else
    {
        m_planInvalid = false;
        m_hasDefaultPlan = false;
        plan->m_owner = this;
    }

    for (std::list<int>::iterator it = evaluated.begin(); it != evaluated.end(); ++it)
        m_behaviourIdsByMode[m_behaviours[*it]->getMode()].push_back(static_cast<unsigned short>(*it));
    return count;
}

// +0x018
void BotMain::cheatExecution()
{
    // five bytes - A bare `ret`. The hook exists and does nothing. Reproduced.
}

// +0x01C
void BotMain::see(const double& dt)
{
    m_sensingAgent->getBotSenses()->see(dt);
}

// +0x020
void BotMain::updateSensing(const double& dt)
{
    m_sensingAgent->getBotSenses()->updateSensing(dt);
}

// +0x024
bool BotMain::updatePathfinding(bool& a, bool* b)
{
    return m_navigation->update(a, b);
}

// +0x02C
// Re-reads the vehicle the player is in. A new one becomes the current equipment: the old
// equipment's strength drops by a quarter, the new one's grows by a third, and the bot's plan,
// navigation and controls start over. Staying in the same vehicle changes nothing.
// *out says whether the player is in anything at all. Returns false only when nothing changed.
bool BotMain::updateBotVehicle(bool* out)
{
    IAIObject* const vehicle = m_player->getCurrentVehicle();
    getPlan();                          // the result is discarded
    m_vehicleChanged = true;
    m_planInvalid = true;
    m_hasDefaultPlan = false;
    if (!vehicle)
    {
        m_navigation->reset();
        setCurrentEquipment(static_cast<unsigned int>(-1));
        if (out)
            *out = false;
        return true;
    }

    if (out)
        *out = true;
    const unsigned int sameHandle = vehicle->getInformation(m_side);
    if (sameHandle != static_cast<unsigned int>(-1) && getCurrentEquipment() == sameHandle && !m_manDown)
        return false;

    IBotManager::instance->updateBotVehicle(this, vehicle);
    m_airBorn = false;
    Information* const previous = informationManager->getInformation(getCurrentEquipment());
    m_lastChangeTime = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    const unsigned int handle = vehicle->getInformation(getSide());
    Information* const current = informationManager->getInformation(handle);
    if (previous)
        previous->setStrength(previous->getStrength() * 0.75f);
    if (!current)
    {
        setCurrentEquipment(static_cast<unsigned int>(-1));
        return true;
    }

    m_sensingAgent->deletePlan();
    current->setStrength(current->getStrength() * 1.3333334f);
    setCurrentEquipment(handle);
    setPlanLock(false);
    m_navigation->reset();
    m_takeOffPosition[0] = 0.0f;
    m_takeOffPosition[1] = 0.0f;
    m_takeOffPosition[2] = 0.0f;
    resetAllControls(true, true, true);
    return true;
}

// +0x030
// Sets the bot up for its current equipment: its plan interpreter, the behaviours that apply to
// the kit, the equipment's modifiers, a fresh memory and an initial route. A bot down on the
// ground uses the man-down equipment's behaviours. Without a unit nothing changes.
// [INFERRED] The ELF returns 1 after a full update and 0 without a unit; no caller reads it.
void BotMain::updateBot()
{
    Information* const equipment = informationManager->getInformation(getCurrentEquipment());
    if (!equipment || !equipment->getUnit())
        return;

    UrgencyGenerator** const generators = getBehaviourUrgencyGenerators();
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    const int type = m_manDown ? settings->getManDownEquipmentType()
                               : equipment->getUnit()->getEquipmentType();
    setPlanInterpreter(settings->getPlanInterpreter(type));
    const int count = settings->getNumberOfBehaviours();
    for (int i = 0; i < count; ++i)
    {
        BotBehaviour* behaviour = settings->getBehaviour(type, i);
        if (behaviour)
        {
            if (!behaviour->isAplicable(equipment, this))
                behaviour = 0;
            if (behaviour)
                behaviour->reset(this);
        }
        addBehaviour(behaviour, i);
        generators[i] = behaviour;
        if (behaviour && behaviour->getGenerator())
            behaviour->getGenerator()->unInit(this);
    }
    m_vehicleMods = settings->getVehicleMods(type);
    calculateMods();
    getSelector()->updateActiveUrgency(this);
    getMemory()->reset();

    // An aircraft starts climbing to 100 above where it is; anything else holds its position.
    if (!(equipment->getObjectTypeFlags() & 0x10))
    {
        Information* const current = informationManager->getInformation(m_currentEquipment);
        setWaypoints(checkPointer(new WPEmpty(current->getPosition())));
    }
    else
    {
        std::list<Vec3> points;
        const Vec3& position = equipment->getPosition();
        Vec3 point;
        point.x = position.x + 0.0f;
        point.y = position.y + 100.0f;
        point.z = position.z + 0.0f;
        points.push_back(point);
        setWaypoints(checkPointer(new WPAltitudeMoveTo(1.0f, 1.0f, 25.0f, 1, points)));
    }
}

// +0x034
BotSquad* BotMain::getBotSquad() const
{
    return m_squad;
}

// +0x038
float BotMain::getObedience()
{
    return m_obedience;
}

// +0x03C
float* BotMain::getCurrentOrders()
{
    return m_currentOrders;
}

// +0x040
WaypointList* BotMain::getWaypoints()
{
    return m_waypoints;
}

// +0x044
// The bot owns its list; the old one is deleted.
void BotMain::setWaypoints(WaypointList* w)
{
    delete m_waypoints;
    m_waypoints = w;
}

// +0x04C
void BotMain::recieveOrder(BotOrder* order, std::list<unsigned int>& targets)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::recieveOrder");
}

// +0x050
std::list<unsigned int>* BotMain::getOrderList(unsigned char behaviour)
{
    return (behaviour == m_orderedBehaviour) ? &m_list128 : &m_list132;
}

// +0x054
bool BotMain::hasPlan()
{
    return !m_planInvalid;
}

// +0x058
bool BotMain::hasDefaultPlan() const
{
    return m_hasDefaultPlan;
}

// +0x05C
void BotMain::invalidatePlan()
{
    m_planInvalid = true;
}

// +0x060
BotScript* BotMain::getPlan()
{
    return m_activePlan;
}

// +0x064
void BotMain::setPlan(BotScript* plan)
{
    m_activePlan = plan;
}

// +0x068
bool BotMain::getPlanLock() const
{
    return m_planLock;
}

// +0x06C
void BotMain::setPlanLock(bool b)
{
    m_planLock = b;
}

// +0x070
bool BotMain::isTakingOrders()
{
    return m_takingOrders;
}

// +0x074
bool BotMain::isOrderedBehaviour(unsigned char behaviour)
{
    return behaviour == m_orderedBehaviour;
}

// +0x078
bool BotMain::isOrderedObject(unsigned int handle, BotBehaviour* b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::isOrderedObject");
}

// +0x07C
void BotMain::setFiringTarget(unsigned int handle)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setFiringTarget");
}

// +0x080
unsigned int BotMain::getFiringTarget() const
{
    return m_firingTarget;
}

// +0x084
double BotMain::getFiringTargetTime() const
{
    return m_firingTargetTime;
}

// +0x088
SensingAgent* BotMain::getSensingAgent()
{
    return m_sensingAgent;
}

// +0x08C
void BotMain::insertBehaviourInformation(float f, const Vec3& v, unsigned char c)
{
    m_sensingAgent->insertBehaviourInformation(f, v, c);
}

// +0x090. The sensing agent's senses' records, keyed by information handle.
const std::map<unsigned int, SensedObject*>& BotMain::getSensedObjects() const
{
    return m_sensingAgent->getBotSenses()->getSensedObjects();
}

// +0x094
SensedObject* BotMain::getSensedObjectFromHandle(unsigned int handle) const
{
    return m_sensingAgent->getBotSenses()->getSensedObjectFromHandle(handle);
}

// +0x098
void BotMain::setUnresolvedCollisionObject(unsigned int handle)
{
    m_unresolvedCollision = handle;
}

// +0x09C
unsigned int BotMain::getUnresolvedCollisionHandle()
{
    return m_unresolvedCollision;
}

// +0x0A0
void* BotMain::getIncommingFire()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getIncommingFire");
}

// +0x0A4
float BotMain::getIncommingFireTotalStrength()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getIncommingFireTotalStrength");
}

// +0x0A8
float BotMain::getIncommingFireTotalStrengthAgainstSelf()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getIncommingFireTotalStrengthAgainstSelf");
}

// +0x0AC
float BotMain::getAttackerStrength() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getAttackerStrength");
}

// +0x0B0
CommunicationAgent* BotMain::getCommunicationAgent()
{
    return m_communication;
}

// forward to the bound player's getId, without a null guard.
int BotMain::getPlayerId() const
{
    return m_player->getId();
}

// +0x0B8
int BotMain::getInputId()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getInputId");
}

// +0x0BC
IAIPlayer* BotMain::getPlayer() const
{
    return m_player;
}

// The squared speed both images test, grouped (x*x + y*y) + z*z as the PE's x87 sequence is and
// summed in double, which is exactly the PE's 53-bit precision control (products of floats are
// exact at 53 bits). [PLATFORM] the ELF keeps the sum at 64-bit precision. A NaN sum is never
// "at most", which matches both images' unordered branch.
static bool speedSquaredAtMost(const Vec3& speed, float limit)
{
    const double x = speed.x, y = speed.y, z = speed.z;
    return (x * x + y * y) + z * z <= limit;
}

// Updates player actions, retaining stalled movement when a plan update was skipped.
void BotMain::updatePlayerAction(bool stalled)
{
    if (!stalled)
    {
        m_stalledPlanTicks = 0;
        return;
    }
    ++m_stalledPlanTicks;
    Information* const equipment = informationManager->getInformation(m_currentEquipment);
    if (m_stalledPlanTicks <= 15)
    {
        IPIMobile* const mobile = equipment->getMobile();
        if (mobile && !speedSquaredAtMost(mobile->getSpeed(), 0.01f))
        {
            m_playerInput->setInput(io::PILie, 0.0f);
            m_playerInput->setInput(io::PIFire, 0.0f);
            m_playerInput->setInput(io::PIAltFire, 0.0f);
            m_playerInput->setInput(io::PIReload, 0.0f);
            return;
        }
    }
    resetAllControls(true, true, true);
}

// read in outline only. Its effect depends on the
// equipment's flag word, Information::getFaceing (slot 16), the mobile speed and a function-static
// `minThrottle`. The first gate releases sprint, yaw, pitch and roll and sets the throttle against
// the speed along the facing (clamped by minThrottle) or to zero; the second releases both look
// axes; the third releases fire, alt-fire, the nine weapon and eight position selects, flares
// crouch and lie (conditionally), use and reload.
void BotMain::resetAllControls(bool movement, bool look, bool actions)
{
    static const float minThrottle = 0.4f;

    Information* const equipment = informationManager->getInformation(m_currentEquipment);
    // [UNRESOLVED] object-type flag 0x200000.
    const bool flag200000 = equipment && (equipment->getObjectTypeFlags() & 0x200000);

    if (movement)
    {
        m_playerInput->setInput(io::PISprint, 0.0f);
        m_playerInput->setInput(io::PIYaw, 0.0f);
        m_playerInput->setInput(io::PIPitch, 0.0f);
        m_playerInput->setInput(io::PIRoll, 0.0f);
        // The equipment is not null-checked here.
        if (!(equipment->getObjectTypeFlags() & 0x10))
        {
            float throttle = 0.0f;
            IPIMobile* const mobile = equipment->getMobile();
            if (mobile && !flag200000)
            {
                const Vec3 facing = equipment->getFaceing();
                const Vec3& speed = mobile->getSpeed();
                const double along = static_cast<double>(speed.x) * facing.x +
                                     static_cast<double>(speed.y) * facing.y +
                                     static_cast<double>(speed.z) * facing.z;
                double brake = along * 0.5f;
                if (brake < -minThrottle)
                    brake = -minThrottle;
                else if (brake > minThrottle)
                    brake = minThrottle;
                throttle = static_cast<float>(-brake);
            }
            m_playerInput->setInput(io::PIThrottle, throttle);
        }
    }

    if (look)
    {
        m_playerInput->setInput(io::PIMouseLookX, 0.0f);
        m_playerInput->setInput(io::PIMouseLookY, 0.0f);
    }

    if (actions)
    {
        m_playerInput->setInput(io::PIFire, 0.0f);
        m_playerInput->setInput(io::PIAltFire, 0.0f);
        for (int input = io::PIWeaponSelect1; input <= io::PIPositionSelect8; ++input)
            m_playerInput->setInput(static_cast<io::PlayerInputMap>(input), 0.0f);
        m_playerInput->setInput(io::PIFlareFire, 0.0f);
        if (isDead() || !flag200000)
        {
            m_playerInput->setInput(io::PICrouch, 0.0f);
            m_playerInput->setInput(io::PILie, 0.0f);
        }
        m_playerInput->setInput(io::PIUse, 0.0f);
        m_playerInput->setInput(io::PIReload, 0.0f);
    }
}

// deactivated bots are dead without consulting the player.
bool BotMain::isDead() const
{
    return m_deactivated || !m_player->getIsAlive();
}

// +0x0C8 (+0xC4). The equipment is updated before either
// the bot or its player; an absent/stale information handle skips only that equipment arm.
bool BotMain::setSide(int side)
{
    Information* const equipment = informationManager->getInformation(m_currentEquipment);
    if (equipment)
    {
        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        // IAIObject has no recovered typed interface yet. Retail calls its setSide at ELF
        // +0x128 / PE +0x124 through Information's object (+0x20 ELF / +0x28 PE).
        BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setSide (equipment IAIObject::setSide)");
    }
    m_side = side;
    m_player->setSide(side);
    return true;
}

// +0x0CC
void BotMain::setName(const std::string& name)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setName");
}

// +0x0D0
bool BotMain::isDeactivated() const
{
    return m_deactivated;
}

// +0x0D4
void BotMain::setAsDeactivated(bool b)
{
    m_deactivated = b;
}

// +0x0D8
unsigned int BotMain::getCurrentEquipment() const
{
    return m_currentEquipment;
}

// +0x0DC
void BotMain::setCurrentEquipment(unsigned int handle)
{
    m_currentEquipment = handle;
}

// +0x0E0
int BotMain::getSide() const
{
    return m_side;
}

// +0x0E4
int BotMain::getBotType() const
{
    return 0;
}

// +0x0E8
unsigned int BotMain::getBotId() const
{
    return m_botId;
}

// +0x0EC
BotMemory* BotMain::getMemory()
{
    return m_memory;
}

// +0x0F0
const std::string& BotMain::getName() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getName");
}

// +0x0F4
void BotMain::setTimeOfLastGrenadeThrown(double t)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setTimeOfLastGrenadeThrown");
}

// +0x0F8
double BotMain::getTimeOfGrenadeThrown(int i) const
{
    return m_grenadeThrownTime[i];
}

// +0x0FC
BotNavigation* BotMain::getBotNavigation()
{
    return m_navigation;
}

// no player null guard.
int BotMain::getSpawnPoint() const
{
    return m_player->getSpawnGroup();
}

// +0x104
int BotMain::getSpawnKit() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getSpawnKit");
}

// +0x108
void BotMain::getSpawnTarget(int& a, int& b) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getSpawnTarget");
}

// Keep fresh player/side/id reads
// after each callback, and clear the strategic area only after the SAI notification.
void BotMain::setSpawnTarget(int spawnGroup, int kit)
{
    m_player->setSpawnGroup(spawnGroup);
    m_player->setSpawnKit(kit);
    ISAI* const sai = IAIMain::m_instance->getSAI(m_side);
    sai->setNewBotKit(m_botId, kit);
    m_spawnToStrategicAreaId = -1;
}

// Set the engine spawn metadata, notify the side's SAI
// then store the strategic area. No player/SAI null guard or value validation is present.
void BotMain::setSpawnTarget(int strategicArea, int spawnGroup, int kit)
{
    m_player->setSpawnGroup(spawnGroup);
    m_player->setSpawnKit(kit);
    ISAI* const sai = IAIMain::m_instance->getSAI(m_side);
    sai->setNewBotKit(m_botId, kit);
    m_spawnToStrategicAreaId = strategicArea;
}

// +0x114
int BotMain::getSpawnToStrategicAreaId()
{
    return m_spawnToStrategicAreaId;
}

// +0x118. True while the player is dead and on the first call after it lives again.
bool BotMain::isJustSpawned()
{
    if (!m_player->getIsAlive())
    {
        m_wasDead = true;
        return true;
    }
    if (!m_wasDead)
        return false;
    m_wasDead = false;
    return true;
}

// +0x11C
IBotDeathCallBack* BotMain::getDeathCallBack() const
{
    return m_deathCallBack;
}

// destroy the previous callback before storing its replacement.
// No same-pointer guard: passing the current callback leaves the shipped dangling pointer.
void BotMain::setDeathCallBack(IBotDeathCallBack* cb)
{
    if (m_deathCallBack)
        delete m_deathCallBack;
    m_deathCallBack = cb;
}

// +0x124
IBotSpawnCallBack* BotMain::getSpawnCallBack() const
{
    return m_spawnCallBack;
}

// same ownership ordering as the death callback.
void BotMain::setSpawnCallBack(IBotSpawnCallBack* cb)
{
    if (m_spawnCallBack)
        delete m_spawnCallBack;
    m_spawnCallBack = cb;
}

// +0x12C
void BotMain::setIsFiring(bool b)
{
    m_isFiring = b;
}

// +0x130
bool BotMain::isFiring()
{
    return m_isFiring;
}

// +0x134
void BotMain::setAllowFire(bool b)
{
    m_fireAllowed = b;
}

// +0x138
bool BotMain::isFireAllowed()
{
    return m_fireAllowed;
}

// +0x13C
void BotMain::setSquadMember(SquadMember* m)
{
    m_squadMember = m;
}

// +0x140
SquadMember* BotMain::getSquadMember()
{
    return m_squadMember;
}

// +0x144
bool BotMain::isBotStuck()
{
    return m_botStuck;
}

// +0x148
void BotMain::setBotStuck(bool b)
{
    m_botStuck = b;
}

// +0x14C. A placed charge counts for 240 s.
void BotMain::placeC4(int placementType, Vec3 v, AIStrategicObject* o)
{
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    m_c4.placed = true;
    m_c4.placementType = placementType;
    m_c4.target = o;
    m_c4.position = v;
    m_c4.placeTime = now;
    m_c4.lifetime = 240.0f;
}

// +0x150
bool BotMain::isC4Placed()
{
    return m_c4.placed;
}

// +0x154
// The placement type and the position stay.
void BotMain::removeC4()
{
    m_c4.placed = false;
    m_c4.target = 0;
    m_c4.placeTime = 0.0f;
    m_c4.lifetime = 0.0f;
}

// +0x158
Bot::C4Info BotMain::getC4()
{
    return m_c4;
}

// +0x15C. A charge past its lifetime is forgotten.
void BotMain::updateC4()
{
    if (!m_c4.placed)
        return;
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (now > m_c4.placeTime + m_c4.lifetime)
        removeC4();
}

// +0x160
void BotMain::enableBotScriptDebugger(bool /*enable*/)
{
    // defect, reproduced: the argument is discarded and the debugger is always cleared.
    m_debugger = 0;
}

// +0x164
BotScriptDebugger* BotMain::getDebugger()
{
    return m_debugger;
}

// +0x168 is `Bot::getScriptContext` and is defined above, beside `Bot::getBotType`. BotMain does not
// override it - see the note in BotMain.h.

// +0x16C
void BotMain::event_projectile(FireObject* f, unsigned int h)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_projectile");
}

// +0x170
void BotMain::event_shotHit(int i, world::IObject* o, const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_shotHit");
}

// +0x174
void BotMain::event_shotMissed(int i, const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_shotMissed");
}

// +0x178
void BotMain::event_heardObject(unsigned int h)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_heardObject");
}

// +0x17C
bool BotMain::isInTransitionPhase() const
{
    return m_inTransitionPhase;
}

// +0x180
void BotMain::setIsInTransitionPhase(bool b)
{
    m_inTransitionPhase = b;
}

// +0x184
void BotMain::addToMoveInclination(float f)
{
    m_moveInclinationDelta += f;
}

// +0x188
void BotMain::addToFireInclination(float f)
{
    m_fireInclinationDelta += f;
}

// Once per decision: the inclination fades by a fifth, to zero once at or below 0.01, and takes a
// fifth of what was added since.
void BotMain::stepMoveInclination()
{
    if (m_moveInclination > 0.01f)
        m_moveInclination *= 0.8f;
    else
        m_moveInclination = 0.0f;
    m_moveInclination = 0.2f * m_moveInclinationDelta + m_moveInclination;
    m_moveInclinationDelta = 0.0f;
}

// The same for the fire inclination.
void BotMain::stepFireInclination()
{
    if (m_fireInclination > 0.01f)
        m_fireInclination *= 0.8f;
    else
        m_fireInclination = 0.0f;
    m_fireInclination = 0.2f * m_fireInclinationDelta + m_fireInclination;
    m_fireInclinationDelta = 0.0f;
}

// +0x18C
float BotMain::getMoveInclination()
{
    return m_moveInclination;
}

// +0x190
float BotMain::getFireInclination()
{
    return m_fireInclination;
}

// +0x194. A new pose is taken when forced, or when no key press is under way and the last lie
// toggle is over a second old. A forced request marks itself for the next poll.
void BotMain::requestSoldierPose(int pose, bool force)
{
    const double time = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (pose != m_requestedSoldierPose && ((!m_poseChanging && time > 1.0 + m_poseChangeTime) || force))
        m_requestedSoldierPose = pose;
    if (force)
        m_poseForced = true;
}

// +0x198
int BotMain::getRequestedSolderPose()
{
    return m_requestedSoldierPose;
}

// +0x19C
void BotMain::setPoseStatus(int pose, unsigned int h, bool a, bool b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setPoseStatus");
}

// +0x1A0
void BotMain::event_airborn()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_airborn");
}

// +0x1A4
bool BotMain::isAirBorn() const
{
    return m_airBorn;
}

// +0x1A8
void BotMain::setLastChangeTime(double t)
{
    m_lastChangeTime = t;
}

// +0x1AC
void BotMain::setManDown(bool b)
{
    m_manDown = b;
}

// +0x1B0
bool BotMain::getManDown() const
{
    return m_manDown;
}

// +0x1B4. A standing bot tops every known weapon of its equipment up by an eighth once 6 s
// of game time have built up, then starts counting again. Retail walks a copy of the weapon list.
void BotMain::rearm(Information* i)
{
    static const double REARM_PERIOD = 6.0;
    static const float REARM_PERCENT = 0.125f;
    if (m_manDown)
        return;
    m_rearmTime += static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getTimeIncrease();
    if (!(REARM_PERIOD < m_rearmTime))
        return;
    IPIArmament* const armament = i->getArmament();
    const std::vector<unsigned int> weapons = armament->getWeapons();
    for (std::vector<unsigned int>::const_iterator it = weapons.begin(); it != weapons.end(); ++it)
    {
        if (*it == 0xFFFFFFFFu)
            continue;
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(*it);
        if (weapon)
            armament->rearm(weapon, REARM_PERCENT, getSide());
    }
    m_rearmTime = 0.0;
}

// +0x1B8
void BotMain::setChangeTarget(unsigned int h)
{
    m_changeTarget = h;
}

// +0x1BC
unsigned int BotMain::getChangeTarget() const
{
    return m_changeTarget;
}

// +0x1C0
void BotMain::setRepairTarget(unsigned int h)
{
    m_repairTarget = h;
}

// +0x1C4
unsigned int BotMain::getRepairTarget() const
{
    return m_repairTarget;
}

// +0x1C8
void BotMain::setWantedRepairTarget(unsigned int h)
{
    m_wantedRepairTarget = h;
}

// +0x1CC
unsigned int BotMain::getWantedRepairTarget() const
{
    return m_wantedRepairTarget;
}

// +0x1D0
void BotMain::setBridgeRepairTarget(unsigned int h)
{
    m_bridgeRepairTarget = h;
}

// +0x1D4
unsigned int BotMain::getBridgeRepairTarget() const
{
    return m_bridgeRepairTarget;
}

// +0x1D8
void BotMain::setReviveTarget(unsigned int h)
{
    m_reviveTarget = h;
}

// +0x1DC
unsigned int BotMain::getReviveTarget() const
{
    return m_reviveTarget;
}

// +0x1E0
void BotMain::setIsRepaired(bool b)
{
    m_isRepaired = b;
}

// +0x1E4
bool BotMain::getIsRepaired() const
{
    return m_isRepaired;
}

// +0x1E8
void BotMain::setIsEngineerTarget(bool b)
{
    m_isEngineerTarget = b;
}

// +0x1EC
bool BotMain::getIsEngineerTarget() const
{
    return m_isEngineerTarget;
}

// +0x1F0
void BotMain::setShouldWaitForRepairs(bool b)
{
    m_shouldWaitForRepairs = b;
}

// +0x1F4
bool BotMain::getShouldWaitForRepairs() const
{
    return m_shouldWaitForRepairs;
}

// +0x1F8
void BotMain::setMineTarget(unsigned int h)
{
    m_mineTarget = h;
}

// +0x1FC
unsigned int BotMain::getMineTarget() const
{
    return m_mineTarget;
}

// +0x200
void BotMain::setNewAssignedAreas(int a, int b)
{
    m_assignedAreas[0] = a;
    m_assignedAreas[1] = b;
}

// +0x204
bool BotMain::hasSameAssignedAreas(int a)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::hasSameAssignedAreas");
}

// +0x208
void BotMain::copyAssignedAreas()
{
    m_assignedAreas[2] = m_assignedAreas[0];
    m_assignedAreas[3] = m_assignedAreas[1];
}

// +0x20C
void BotMain::setCurrentWaypoint(Vec3 v, int i)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setCurrentWaypoint");
}

// +0x210
const Vec3& BotMain::getCurrentWaypoint()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getCurrentWaypoint");
}

// +0x214
void BotMain::resetAssignedAreas()
{
    // only two of the four. The copies at [1] and [3] are left alone; reproduced.
    m_assignedAreas[0] = -1;
    m_assignedAreas[2] = -1;
}

// +0x218
void BotMain::setTimeOfLastFailedStaticSkyAttack(double t)
{
    m_timeOfLastFailedStaticSkyAttack = t;
}

// +0x21C
double BotMain::getTimeOfLastFailedStaticSkyAttack() const
{
    return m_timeOfLastFailedStaticSkyAttack;
}

// +0x220
void BotMain::setTimeOfLastFailedStaticAttack(double t)
{
    m_timeOfLastFailedStaticAttack = t;
}

// +0x224
double BotMain::getTimeOfLastFailedStaticAttack() const
{
    return m_timeOfLastFailedStaticAttack;
}

// +0x228
void BotMain::setBotWantsToSimulateArtillery(bool b)
{
    m_wantsToSimulateArtillery = b;
}

// +0x22C
bool BotMain::getBotWantsToSimulateArtillery() const
{
    return m_wantsToSimulateArtillery;
}

// +0x230
void BotMain::setBotIsSimulatingArtillery(bool b)
{
    m_isSimulatingArtillery = b;
}

// +0x234
bool BotMain::getBotIsSimulatingArtillery() const
{
    return m_isSimulatingArtillery;
}

// +0x238. Side 0 avoids nothing. Moving from one side to the other marks the switch, and a
// further switch to another side reports true. [UNRESOLVED] Retail takes an AvoidSide enum.
bool BotMain::setAvoidSide(int side)
{
    const bool switchedAgain = m_avoidSideSwitched && side != m_avoidSide && side != 0;
    if (side == 0)
        m_avoidSideSwitched = false;
    else if (side != m_avoidSide && m_avoidSide != 0)
        m_avoidSideSwitched = true;
    m_avoidSide = side;
    return switchedAgain;
}

// +0x23C
int BotMain::getAvoidSide() const
{
    return m_avoidSide;
}

// +0x240
int BotMain::getMoveToStatus() const
{
    return m_moveToStatus;
}

// +0x244
void BotMain::setMoveToStatus(int status)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setMoveToStatus");
}

// +0x248
void BotMain::setCollidedObject(unsigned int h)
{
    m_collidedObject = h;
}

// +0x24C
unsigned int BotMain::getCollidedObject() const
{
    return m_collidedObject;
}

// +0x250
void BotMain::setCollidedObjectPosition(const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setCollidedObjectPosition");
}

// +0x254
const Vec3& BotMain::getCollidedObjectPosition() const
{
    return *reinterpret_cast<const Vec3*>(m_collidedObjectPosition);
}

// +0x258
void BotMain::setIgnoreObstacles(bool b)
{
    // it also clears the avoid side - one setter, two fields. Reproduced.
    m_ignoreObstacles = b;
    m_avoidSide = 0;
}

// +0x25C
bool BotMain::getIgnoreObstacles() const
{
    return m_ignoreObstacles;
}

// +0x260
void BotMain::setSlowDownToTarget(unsigned int h)
{
    m_slowDownToTarget = h;
}

// +0x264
unsigned int BotMain::getSlowDownToTarget() const
{
    return m_slowDownToTarget;
}

// +0x268
double BotMain::getFriendsInLineOfFireTime() const
{
    return m_friendsInLineOfFireTime;
}

// +0x26C
void BotMain::setFriendsInLineOfFireTime(double t)
{
    m_friendsInLineOfFireTime = t;
}

// +0x270
void BotMain::setLastCheckMoveToFinishedTime(double t)
{
    m_lastCheckMoveToFinishedTime = t;
}

// +0x274
double BotMain::getLastCheckMoveToFinishedTime() const
{
    return m_lastCheckMoveToFinishedTime;
}

// +0x278
void BotMain::setIsGoingForFlag(bool b)
{
    m_isGoingForFlag = b;
}

// +0x27C
bool BotMain::getIsGoingForFlag() const
{
    return m_isGoingForFlag;
}

// +0x280
bool BotMain::botInPoseTransition() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::botInPoseTransition");
}

// +0x284
void BotMain::setUseStaticAttack(bool b)
{
    m_useStaticAttack = b;
}

// +0x288
bool BotMain::getUseStaticAttack() const
{
    return m_useStaticAttack;
}

// +0x28C
void BotMain::setTimeOfLastFailedSplashDamageAttack(double t)
{
    m_timeOfLastFailedSplashDamageAttack = t;
}

// +0x290
double BotMain::getTimeOfLastFailedSplashDamageAttack() const
{
    return m_timeOfLastFailedSplashDamageAttack;
}

// +0x294
void BotMain::setTakeOffPosition(const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setTakeOffPosition");
}

// +0x298
const Vec3& BotMain::getTakeOffPosition() const
{
    return *reinterpret_cast<const Vec3*>(m_takeOffPosition);
}

// +0x29C
void BotMain::setQuadSeen()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setQuadSeen");
}

// +0x2A0
bool BotMain::isAttacking(unsigned int handle, double& out) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::isAttacking");
}

// +0x2A4
void BotMain::event_shotTimeOut(int i)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_shotTimeOut");
}

// +0x2A8
void BotMain::event_firing(bool& b, int i, const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_firing");
}

} // namespace ai
} // namespace hfe
} // namespace dice
