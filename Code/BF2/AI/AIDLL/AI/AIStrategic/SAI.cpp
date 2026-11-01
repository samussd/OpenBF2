// Strategic simulation and supporting statistics.
// [FRONTIER] Remaining strategic phases and object-query paths stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Implement and validate the remaining strategic operations.

#include "SAI.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "../Main/AIDllFrontier.h"
#include "../Main/AISettingsInterfaces.h"                  // IAISettings::getMaxNBots
#include "../Main/BotBehaviourManager.h"                   // getBehaviourId, by name
#include "../Main/CommonSingletonServer.h"
#include "../AIKnowledge/BFInformationEnvironment.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIKnowledge/UnitPredicate.h"
#include "../AIBot/BotCallBack.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotSquad.h"
#include "../AIBot/SquadCoordinator.h"
#include "../AIBot/SquadManager.h"
#include "../AIBot/SquadMember.h"                        // setBotHasChangedEquipment: setHavePath
#include "../AIStrategicEntity/AIStrategicObject.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../AIStrategicMap/BotSpawner.h"
#include "../AIUtils/checkPointer.h"
#include "../../../AIInterface/BFKnowledge/BFCommander.h"  // request-denial callbacks
#include "../../../AIInterface/BFKnowledge/BFGame.h"       // IAIGame counters
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"     // IAIPlayer::setSpawnGroup
#include "../../../AIInterface/BFKnowledge/BFTimer.h"      // IBFTimer::getGameTime
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"    // checkCommanderUseArtillery: impact height
#include "../AIStrategicEntity/AIStrategicArea.h"         // hostileTakeoveInProgress: the area centre
#include "../Main/AIRandom.h"
#include "../Main/StrategyManager.h"                       // chooseStrategy's forced strategy
#include "Strategy.h"
#include "../Main/IBotManager.h"                         // updateBotPositions' bot lookup
#include "../AIBot/WaypointList.h"
#include "../AIKnowledge/IPIMobile.h"                    // distributeResources' vehicle number
#include "../../../AIInterface/BFKnowledge/IAIObject.h"  // the soldier's information handle
#include "../../../../Core/Debug.h"                      // update's shipped invalid-state diagnostic

namespace dice {
namespace hfe {
namespace ai {

// The concrete callback names and signatures are ELF-attested. Their source-file placement here
// is [INFERRED]; only SAI.cpp is attested for this part of the strategic subsystem.
class SAIBotDCB : public IBotDeathCallBack
{
public:
    // The SAI pointer is non-owning, at +8 on both ports.
    SAIBotDCB(Bot* bot, ISAI* sai) : IBotDeathCallBack(bot), m_sai(sai) {}
    // PE deleting destructor : only base destruction.
    virtual ~SAIBotDCB() {}

    // Neither pointer is guarded.
    virtual void reportDeath()
    {
        m_sai->setBotIsDead(m_bot->getBotId());
    }

private:
    ISAI* m_sai;
};

class SAIBotSCB : public IBotSpawnCallBack
{
public:
    SAIBotSCB(Bot* bot, ISAI* sai) : IBotSpawnCallBack(bot), m_sai(sai) {}
    // PE deleting destructor : only base destruction.
    virtual ~SAIBotSCB() {}

    // Preserve the repeated virtual squad lookups and priority.
    // setBotIsAlive and the three recieve* callees retain their own named frontiers.
    virtual void reportSpawn()
    {
        m_sai->setBotIsAlive(m_bot->getBotId());
        if (m_bot->getBotSquad()->hasAttackOrder())
            m_bot->getBotSquad()->recieveAttackSquadOrder();
        else if (m_bot->getBotSquad()->hasDefendOrder())
            m_bot->getBotSquad()->recieveDefendSquadOrder();
        else if (m_bot->getBotSquad()->hasMoveOrderOrder())
            m_bot->getBotSquad()->recieveMoveSquadOrder();
    }

private:
    ISAI* m_sai;
};

static_assert(sizeof(void*) != 4 || sizeof(SAIBotDCB) == 12, "SAIBotDCB x86 layout");
static_assert(sizeof(void*) != 4 || sizeof(SAIBotSCB) == 12, "SAIBotSCB x86 layout");

// Running weighted statistics across size buckets. Its two array allocations are unchecked.
EnemyStatistics::EnemyStatistics(unsigned int size)
    : m_size(size)
    , m_weight(1.0f)
    , m_samples(0)
{
    m_strength = new float[size];
    m_count    = new float[m_size];

    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = 0.0f;
        m_count[i]    = 0.0f;
    }
}

EnemyStatistics::EnemyStatistics(unsigned int size, float weight)
    : m_size(size)
    , m_weight(weight)
    , m_samples(0)
{
    m_strength = new float[size];
    m_count    = new float[m_size];

    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = 0.0f;
        m_count[i]    = 0.0f;
    }
}

EnemyStatistics::~EnemyStatistics()
{
    if (m_strength)
        delete[] m_strength;

    if (m_count)
        delete[] m_count;
}

// Updates each bucket with (n * old + weight * observation) / (n + weight), using the
// entry sample count for every bucket. Integer counts are accumulated as floats.
void EnemyStatistics::sample(float* strength, int* count)
{
    const float samples = static_cast<float>(m_samples);
    const float total   = samples + m_weight;

    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = samples / total * m_strength[i];
        m_strength[i] = m_weight / total * strength[i] + m_strength[i];

        m_count[i] = static_cast<float>(m_samples) / total * m_count[i];
        m_count[i] = m_weight / total * static_cast<float>(count[i]) + m_count[i];
    }

    ++m_samples;
}

// both of these walk this object'S size, not the other's. Copying from a differently-sized
// statistics object reads past its arrays. Shipped; no check here either.
void EnemyStatistics::copy(EnemyStatistics& other)
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = other.m_strength[i];
        m_count[i]    = other.m_count[i];
    }

    m_samples = other.m_samples;
}

void EnemyStatistics::add(EnemyStatistics& other)
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] += other.m_strength[i];
        m_count[i]    += other.m_count[i];
    }

    m_samples += other.m_samples;
}

void EnemyStatistics::clear()
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = 0.0f;
        m_count[i]    = 0.0f;
    }

    m_samples = 0;
}

// CommanderInterface - one byte, no state, six orders. The constructor and destructor are five
// bytes each: a return.
CommanderInterface::CommanderInterface()
{
}

CommanderInterface::~CommanderInterface()
{
}


void CommanderInterface::giveAttackOrder(const Vec3& position, SquadCoordinator* squad)
{
    static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
        ->giveAttackOrder(position, squad->getSquadId() + 1, squad->getSide() + 1, 0);
}

void CommanderInterface::giveMoveOrder(const Vec3& position, SquadCoordinator* squad)
{
    static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
        ->giveMoveOrder(position, squad->getSquadId() + 1, squad->getSide() + 1, 0);
}

void CommanderInterface::giveDefendOrder(const Vec3& position, SquadCoordinator* squad)
{
    static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
        ->giveDefendOrder(position, squad->getSquadId() + 1, squad->getSide() + 1, 0);
}

void CommanderInterface::giveAttackOrder(const Vec3& position, Bot* bot)
{
    static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
        ->giveAttackOrder(position, bot->getBotSquad()->getSquadId(), bot->getSide(), 0);
}

void CommanderInterface::giveMoveOrder(const Vec3& position, Bot* bot)
{
    static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
        ->giveMoveOrder(position, bot->getBotSquad()->getSquadId(), bot->getSide(), 0);
}

void CommanderInterface::giveDefendOrder(const Vec3& position, Bot* bot)
{
    static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
        ->giveDefendOrder(position, bot->getBotSquad()->getSquadId(), bot->getSide(), 0);
}


// ISAI - the interface. Its destructor is : a vptr store and a return, like every other
// bare interface destructor in this subsystem. No registration and no roster - `ISAI` is not a
// singleton, it is a per-side object `AIMain` owns by pointer.
ISAI::~ISAI()
{
}

// the unwind path the
// constructor's possible throw needs is the other hundred bytes.
SAI* ISAI::create(int side, int gridWidth, int gridHeight, Vec2 min, Vec2 max)
{
    return new SAI(side, gridWidth, gridHeight, min, max);
}

// Starts the three integer states at -1 and the float accumulator at zero.
SAI::BotInfo::BotInfo(Bot* bot)
    : m_bot(bot)
    , m_assignedObject(-1)
    , m_lastOrderedSquare(-1)
    , m_occupiedObject(-1)
    , m_float10(0.0f)
    , m_lastOrderTime(0.0)
{
}

// Clears both 44-element state arrays, then sets each final normalization slot to one.
void SAI::clearStates(float* a, float* b)
{
    for (int i = 0; i <= 43; ++i)
    {
        a[i] = 0.0f;
        b[i] = 0.0f;
    }

    a[43] = 1.0f;
    b[43] = 1.0f;
}

// Creates strategic state for side. Grid dimensions and bounds are retained in the signature but unused.
SAI::SAI(int side, int, int, Vec2, Vec2)
    : m_flagAC(true)
    , m_behaviourIdle(0)
    , m_behaviourMoveTo(0)
    , m_behaviourAvoid(0)
    , m_behaviourFire(0)
    , m_intD8(0)
    , m_intDC(0)
    , m_intE0(0)
    // The strategy cursor starts at `m_strategies.end`, which on a freshly built empty vector is
    // the null `finish` pointer. The binary copies the vector's finish word into it.
    , m_activeStrategy(0)
    , m_enemyStats(0)
    , m_friendlyStats(0)
    , m_int18C(19)
    , m_float190(-992.0f)
    , m_side(side)
    , m_sideMask(1 << side)
    // the other side'S BIT, and the expression only has two answers - this is a two-sided game.
    , m_enemyMask((side == 1) ? 4 : 2)
    , m_flag1C0(false)
    , m_flag1C1(false)
    , m_flag1CC(false)
    , m_flag1CD(false)
    , m_flag1CE(true)
    , m_flag1CF(false)
    , m_flag1D0(true)
    , m_flag1D1(false)
    , m_strategicObjectIterator()
    , m_stateA(0)
    , m_stateB(0)
    , m_stateC(0)
    , m_stateD(0)
    , m_int1F4(0)
    , m_commander(0)
{
    m_commander = checkPointer(new CommanderInterface());

    m_behaviourIdle   = BotBehaviourManager::m_instance->getBehaviourId(std::string("Idle"));
    m_behaviourMoveTo = BotBehaviourManager::m_instance->getBehaviourId(std::string("MoveTo"));
    m_behaviourAvoid  = BotBehaviourManager::m_instance->getBehaviourId(std::string("Avoid"));
    m_behaviourFire   = BotBehaviourManager::m_instance->getBehaviourId(std::string("Fire"));

    // four deadlines off one clock read. Three at a minute out and one at ten seconds, with the
    // fifth left at zero - so three slow reviews and one fast one, all stamped at construction.
    const double now =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    m_time80 = now + 10.0;
    m_time68 = now + 60.0;
    m_time70 = now + 60.0;
    m_time78 = now + 60.0;
    m_time88 = 0.0;

    m_stateA = checkPointer(new float[44]);
    m_stateB = checkPointer(new float[44]);
    m_stateC = checkPointer(new float[44]);
    m_stateD = checkPointer(new float[44]);

    // the pairing is (A, C) and (B, D), not (A) and (C, D).
    clearStates(m_stateA, m_stateC);
    clearStates(m_stateB, m_stateD);

    // Not checkPointer'ed, unlike everything above.
    m_enemyStats    = new EnemyStatistics(6);
    m_friendlyStats = new EnemyStatistics(6);

    for (int i = 0; i <= 5; ++i)
    {
        m_enemyStrengths[i] = 0.0f;
        m_enemyCounts[i] = 0.0f;
        m_friendlyStrengths[i] = 0.0f;
        m_friendlyCounts[i] = 0.0f;
    }

    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

    m_botInfo.resize(settings->getMaxNBots());

    for (int i = 0; i < settings->getMaxNBots(); ++i)
        m_botInfo[i] = 0;
}

// Clears the strategies' heat and prerequisite, the strength snapshots, the update cursor and the
// deny requests, restarts the four review deadlines as the constructor does, and resets every
// squad, both sides', through the shared squad manager.
bool SAI::reset()
{
    for (std::vector<StrategyContainer>::iterator it = m_strategies.begin();
         it != m_strategies.end(); ++it)
    {
        it->m_prerequisite = 0.0f;
        it->m_heat = 0.0f;
    }
    for (int i = 0; i < 6; ++i)
    {
        m_enemyStrengths[i] = 0.0f;
        m_enemyCounts[i] = 0.0f;
        m_friendlyStrengths[i] = 0.0f;
        m_friendlyCounts[i] = 0.0f;
    }
    m_flag1C0 = false;
    m_flag1C1 = false;
    m_flag1CE = true;
    m_float190 = -1000.0f;
    m_int18C = 19;
    m_intD8 = 0;
    m_intDC = 0;
    for (std::list<DenyRequest*>::iterator it = m_denyRequests.begin();
         it != m_denyRequests.end();)
    {
        delete *it;
        it = m_denyRequests.erase(it);
    }

    const double now =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    m_time80 = now + 10.0;
    m_time68 = now + 60.0;
    m_time70 = now + 60.0;
    m_time78 = now + 60.0;
    m_time88 = 0.0;
    SquadManager::m_instance->reset();
    m_flagAC = true;
    return true;
}

// [FRONTIER] Strategy and distribution operations remain incomplete.
// [FRONTIER] Destruction stops until ordered teardown is implemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace each stop when its corresponding behavior is recovered.
SAI::~SAI()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::~SAI");
}


// Map-dimension setters intentionally ignore their arguments, matching the shipped bodies.
void SAI::setMapXDimension(int)
{
}

void SAI::setMapYDimension(int)
{
}


// Each addition points the active strategy back at the first one.
void SAI::addStrategy(Strategy* strategy)
{
    StrategyContainer container;
    container.m_heat = 0.0f;
    container.m_prerequisite = 0.0f;
    container.m_startTime = 0.0f;
    container.m_activations = 1;
    container.m_strategy = strategy;
    m_strategies.push_back(container);
    m_activeStrategy = &m_strategies[0];
}

// One resumable pass, with a budget check after each
// advancing stage. Equality and NaN pass the budget check; state 4's frequency gate instead
// requires an ordered strict greater-than. The prefix always runs, even for exhausted budgets.
// State 5 and 16..18 are holes. The scheduled helpers retain their own named frontiers.
void SAI::update(double budget)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double start = timer->getUpdateTime();
    updateRequestDenyTimers();

    for (std::vector<BotInfo*>::iterator it = m_botInfo.begin(); it != m_botInfo.end(); ++it)
    {
        if (*it)
        {
#if defined(_MSC_VER)
            // [PLATFORM] PE reloads the entry after the timer callback and does not round
            // the interval to float before adding it to the stored float.
            const double increase = timer->getTimeIncrease();
            BotInfo* const info = *it;
            info->m_float10 = static_cast<float>(info->m_float10 + increase);
#else
            BotInfo* const info = *it;
            const float increase = static_cast<float>(timer->getTimeIncrease());
            info->m_float10 += increase;
#endif
        }
    }

    switch (m_int18C)
    {
    case 19:
        m_int18C = 0;
        // fall through
    case 0:
        m_int18C = 1;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 1:
        updateStrengths();
        m_int18C = 2;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 2:
    {
        const bool next = !m_int1F4;
        clearStates(next ? m_stateB : m_stateA, next ? m_stateD : m_stateC);
        m_int18C = 3;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    }
    case 3:
    {
        const bool next = !m_int1F4;
        updateStates(next ? m_stateB : m_stateA, next ? m_stateD : m_stateC);
        m_int18C = 4;
        m_int1F4 = next;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    }
    case 4:
    {
#if defined(_MSC_VER)
        // [PLATFORM] PE retains settings across getGameTime; ELF reloads it afterward.
        const IAISettings* const settings =
            static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
        const double gameTime = timer->getGameTime();
        const double elapsed = gameTime - m_float190;
#else
        const double gameTime = timer->getGameTime();
        const double elapsed = gameTime - m_float190;
        const IAISettings* const settings =
            static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
#endif
        if (!(elapsed > settings->getSAIUpdateFrequency()))
            return;
        prepareUpdate();
        m_int18C = 6;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    }
    case 6:
        if (!m_flag1D1)
        {
            if (!updateStrategicObjects())
                return;
            checkCommanderUseArtillery();
        }
        m_int18C = 7;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 7:
        if (!m_flag1D1)
            chooseStrategy();
        m_int18C = 8;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 8:
        modifyStrategicObjectsTemperatures();
        m_int18C = 9;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 9:
        updateBotPositions();
        m_int18C = 10;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 10:
        if (!m_flag1D1)
            updateDistribution();
        m_int18C = 11;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 11:
        if (!m_flag1D1)
            distributeResources();
        if (!haveAnyOwnedCps(m_side))
        {
            m_int18C = 15;
            return;
        }
        m_int18C = 12;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 12:
        if (!m_flag1D1)
            distributeDefence();
        m_int18C = 13;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 13:
        if (!m_flag1D1)
            distributeAttack();
        m_int18C = 14;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 14:
        // The ELF calls this empty direct helper; PE eliminates both the test and call.
        if (!m_flag1D1)
            distributeArtillery();
        m_int18C = 19;
        timer->getUpdateTime(); // Shipped final sample, discarded without another budget check.
        return;
    case 15:
        if (!m_flag1D1)
            distributeWithNoAreas();
        m_int18C = 19;
        timer->getUpdateTime();
        return;
    default:
        Debug(DebugTypeError, "AI/AIDLL/AI/AIStrategic/SAI.cpp", 1044, "")
            << "Unhadled case in SAI::Update()."; // Preserve the shipped spelling and source line.
        return;
    }
}

// Null bots do nothing; dead bots first receive (-1,-1,13).
// Squad registration is now a typed call; its narrower relocation/formation frontiers remain
// visible in the owning bodies. Only a completed registration reaches the publication tail.
void SAI::addBot(Bot* bot)
{
    if (!bot)
        return;

    if (bot->isDead())
        bot->setSpawnTarget(-1, -1, 13);

    SquadManager::m_instance->addSquadMember(bot->getPlayerId(), m_side);

    // Recovered tail, reached only after squad registration returns.
    // The vector is ELF +0x94 / PE +0xAC; port-specific STL layouts must not be copied as offsets.
    m_vector94.push_back(bot);
    bot->setDeathCallBack(checkPointer(new SAIBotDCB(bot, this)));
    bot->setSpawnCallBack(checkPointer(new SAIBotSCB(bot, this)));
}

void SAI::setBotIsDead(int botId)
{
    handleDeadBot(botId);
}

// 0x338db0 (Ghidra). The bot's object and kit wait for its respawn; no spawn group is chosen.
void SAI::handleDeadBot(int botId)
{
    BotInfo* const info = m_botInfo[botId];
    if (!info)
        return;
    const int kit = info->m_bot->getSpawnKit();
    const int object = info->m_assignedObject;
    info->remove(this);
    m_map5C.insert(std::make_pair(botId, std::make_pair(object, kit)));
    info->m_bot->getPlayer()->setSpawnGroup(0);
    info->setAsDead();
}

// 0x338ae0 (Ghidra). A live bot on a unit joins the closest strategic object. The first spawn of the
// round resets the bridges.
void SAI::setBotIsAlive(int botId)
{
    BotInfo* const info = m_botInfo[botId];
    if (!info)
        return;
    if (m_flagAC)
    {
        AIStrategicObjectManager::getInstance()->resetBridges();
        m_flagAC = false;
    }
    Bot* const bot = info->m_bot;
    if (bot->isDead())
        return;
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    if (!equipment || !equipment->getUnit())
        return;
    int map = -1;
    if (equipment->getMobile())
        map = equipment->getMobile()->getVehicleNumber();
    const int type = equipment->getUnit()->getEquipmentType();
    AIStrategicObject* const closest =
        AIStrategicObjectManager::getInstance()->getClosestObject(equipment->getPosition(), type, map, m_side);
    info->respawn(closest, this);
    m_map5C.erase(botId);
}

// 0x339300 (Ghidra). A dead bot only leaves the waiting list.
void SAI::removeBot(Bot* bot)
{
    const int botId = bot->getBotId();
    if (!m_botInfo[botId])
        return;
    const std::map<int, std::pair<int, int> >::iterator waiting = m_map5C.find(botId);
    if (waiting == m_map5C.end())
        m_botInfo[botId]->remove(this);
    else
        m_map5C.erase(waiting);
    delete m_botInfo[botId];
    m_botInfo[botId] = 0;
}

// Both test the requested bit; x86 masks shift count.
bool SAI::isEnemy(int side) const
{
    return (static_cast<unsigned int>(m_enemyMask) & (1u << (side & 31))) != 0;
}

// the same bit test on the friendly mask.
bool SAI::isFriend(int side) const
{
    return (static_cast<unsigned int>(m_sideMask) & (1u << (side & 31))) != 0;
}

float* SAI::getEnemyStrengths(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getEnemyStrengths");
}

int* SAI::getEnemyTypes(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getEnemyTypes");
}

float* SAI::getFriendlyStrengths(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getFriendlyStrengths");
}

int* SAI::getFriendlyTypes(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getFriendlyTypes");
}

// 0x33d5f0 (Ghidra). The bot's squad member has no path any more.
void SAI::setBotHasChangedEquipment(int botId)
{
    const int playerId = IBotManager::instance->getBot(botId)->getPlayerId();
    SquadMember* const member = SquadManager::m_instance->getSquadMember(playerId, m_side);
    if (member)
        member->setHavePath(false);
    handleChangingBot(botId);
}

// find, update the second member of the existing pair, or do nothing.
// Missing bot IDs are not inserted. The first member of the pair is left untouched.
void SAI::setNewBotKit(int botId, int kit)
{
    std::map<int, std::pair<int, int> >::iterator it = m_map5C.find(botId);
    if (it != m_map5C.end())
        it->second.second = kit;
}

// return the member map's address
// at +0x5C / +0x68. The caller copies the map; this getter neither copies nor mutates it.
// [INFERRED] const-reference spelling, as documented on the shared declaration.
const std::map<int, std::pair<int, int> >& SAI::getDeadBots() const
{
    return m_map5C;
}

void SAI::setPlayerIsCommander(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::setPlayerIsCommander");
}

// byte getter at +0x1D1 / +0x221.
bool SAI::isPlayerCommander()
{
    return m_flag1D1;
}

bool SAI::playerRequestsArtillery(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsArtillery");
}

bool SAI::playerRequestsUav(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsUav");
}

bool SAI::playerRequestsSupply(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsSupply");
}

bool SAI::playerRequestsVehicleDrop(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsVehicleDrop");
}

void SAI::updateRequestDenyTimers()
{
    // Cancellation deliberately retains the shipped
    // invalid-iterator defect: erases, then reads the erased
    // node; frees it, then reads [edi]. A nonempty queue
    // therefore makes the following ++it unsafe. Do not turn this into erase(it++).
    if (m_flag1D1)
    {
        for (auto it = m_denyRequests.begin(); it != m_denyRequests.end(); ++it)
        {
            ::operator delete(*it);
            m_denyRequests.erase(it);
        }
        return;
    }

    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    for (auto it = m_denyRequests.begin(); it != m_denyRequests.end(); )
    {
        // The comparison is ordered and strict: equality and NaN retain the request.
        if (now > (*it)->m_deadline)
        {
            switch ((*it)->m_type)
            {
            case 0:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denySupplyRequest(m_side, (*it)->m_playerId);
                break;
            case 1:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denyArtilleryRequest(m_side, (*it)->m_playerId);
                break;
            case 2:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    ArtilleryUnavailable(m_side, (*it)->m_playerId);
                break;
            case 3:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denyUavRequest(m_side, (*it)->m_playerId);
                break;
            case 4:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denyVehicleDropRequest(m_side, (*it)->m_playerId);
                break;
            }
            // Both ports reload the payload after the callback, then advance before erase.
            ::operator delete(*it);
            m_denyRequests.erase(it++);
        }
        else
            ++it;
    }
}

// The AI commander fires artillery at most once a minute, checking every ten seconds once the
// first minute has passed, and only with at least two bots allowed, no human commander and a
// standing piece. It shells a control point the enemy is taking (nine times in ten, unless
// friendlies are within the safe distance), otherwise an enemy camping out of position.
void SAI::checkCommanderUseArtillery()
{
    bool tooFewBots = false;
    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);
    if (settings)
        tooFewBots = settings->getMaxNBots() <= 1;
    if (m_flag1D1 || tooFewBots)
        return;

    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    IBFCommander* const commander = static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander);
    if (!commander->gameHasArtillery(m_side) || !commander->anyArtilleryPiecesLeft(m_side))
        return;
    if (now <= m_time68 || now <= m_time88)
        return;
    m_time88 = now + 10.0;

    SquadManager* const squads = SquadManager::m_instance;
    const IAITerrain* const terrain =
        static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
    AIStrategicObject* object;
    Vec2 target = { 0.0f, 0.0f };
    if (hostileTakeoveInProgress(object, target) && AIRandom::m_instance->nextFloat() > 0.1f)
    {
        const Vec3 impact = { target.x, terrain->getTerrainAltitude(target.x, target.y), target.y };
        if (!squads->friendliesCloseTo(impact, squads->getArtillerySafeDistance(), m_side))
        {
            m_time68 = now + 60.0;
            commander->fireArtillery(m_side, -1, target);
            squads->incomingArtillery(impact, m_side);
            return;
        }
    }

    if (!squads->haveCampingHumans(m_side, target))
        return;
    const Vec3 impact = { target.x, terrain->getTerrainAltitude(target.x, target.y), target.y };
    m_time68 = now + 60.0;
    commander->fireArtillery(m_side, -1, target);
    squads->incomingArtillery(impact, m_side);
}

// The first control point whose capture by the enemy is under way and below three quarters; the
// position is drawn uniformly from the box centred on the point's side position that reaches
// the area's centre.
bool SAI::hostileTakeoveInProgress(AIStrategicObject*& object, Vec2& position)
{
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    AIStrategicObject* candidate = 0;
    bool found = false;
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin();
         it != objects.end(); ++it)
    {
        candidate = *it;
        if (!candidate || !candidate->hasObjectTypeFlag(AIStrategicObject::ControlPoint)
            || !candidate->isEnemyTakingOverBase(m_side))
        {
            continue;
        }
        if (candidate->getControlPointTakeOverCounter() < 0.75f)
        {
            object = candidate;
            found = true;
            break;
        }
    }

    if (candidate && found)
    {
        const Vec2& centre = static_cast<AIStrategicArea*>(candidate)->m_centre;
        const Vec2& at = candidate->getPosition(m_side);
        position.x = AIRandom::m_instance->nextFloat() * ((at.x - centre.x) * 2.0f) + centre.x;
        position.y = AIRandom::m_instance->nextFloat() * ((at.y - centre.y) * 2.0f) + centre.y;
    }
    return found;
}

// Whether the position lies in the control point of the closest object, if the side owns it. A
// positive radius can only widen the point's own.
bool SAI::isPosInsideOwnedControlPoint(AIStrategicObject*& object, Vec3 position, float radius)
{
    AIStrategicObject* const closest =
        AIStrategicObjectManager::getInstance()->getClosestObject(position, m_side);
    if (!closest->hasObjectTypeFlag(AIStrategicObject::ControlPoint)
        || closest->getStatus(m_side) != StatusMap::Owned)
        return false;

    Vec3 centre = {};
    float pointRadius = 0.0f;
    bool active;
    static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getControlPointInfo(
        static_cast<AIStrategicArea*>(closest)->m_controlPointId, centre, pointRadius, active);
    const float reach = radius > 0.0f && radius > pointRadius ? radius : pointRadius;
    const float dx = position.x - centre.x;
    const float dy = position.y - centre.y;
    const float dz = position.z - centre.z;
    if (!active || !(dx * dx + dy * dy + dz * dz < reach * reach))
        return false;
    object = closest;
    return true;
}

// 0x338590 (Ghidra). Any strategic object the side owns, whatever its type.
bool SAI::haveAnyOwnedCps(int side)
{
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end();
         ++it)
    {
        if (*it && (*it)->getStatus(side) == StatusMap::Owned)
            return true;
    }
    return false;
}

bool SAI::haveAnyOwnedAndTakeableCps(int side)
{
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end();
         ++it)
    {
        if (*it && (*it)->getStatus(side) == StatusMap::Owned
            && static_cast<AIStrategicArea*>(*it)->isAreaTakeable())
            return true;
    }
    return false;
}

bool SAI::ownAllTakeableCps(int side)
{
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end();
         ++it)
    {
        if (*it && (*it)->getStatus(side) != StatusMap::Owned
            && static_cast<AIStrategicArea*>(*it)->isAreaTakeable())
            return false;
    }
    return true;
}

// Returns the squad manager's count of freed squads. The object is looked up by index only to
// read its index back.
int SAI::releaseSquadsFromObject(int objectIndex, int side)
{
    AIStrategicObject* const object =
        AIStrategicObjectManager::getInstance()->getStrategicObject(objectIndex);
    return SquadManager::m_instance->unAssignSquads(object->getIndex(), side);
}

// 0x33d6d0 (Ghidra). The cursor names the bot. Always 1.
int SAI::releaseUnitFromObject(int object, std::set<int>::iterator& it)
{
    AIStrategicObject* const strategic = AIStrategicObjectManager::getInstance()->getStrategicObject(object);
    BotInfo* const info = m_botInfo[*it];
    info->removeAssigned(strategic, it, true, this);
    info->free(false, this);
    return 1;
}

// 0x339250 (Ghidra).
bool SAI::foundDeadBot(int botId)
{
    return m_setF0.erase(botId) != 0;
}

// 0x33d770 (Ghidra).
bool SAI::hasChangedEquipment(int botId)
{
    return m_setB0.find(botId) != m_setB0.end();
}

// 0x33d7d0 (Ghidra).
void SAI::equipmentChangedFinished(int botId)
{
    m_setB0.erase(botId);
}

// 0x33d870 (Ghidra). Answers the object the bot occupied.
int SAI::assignBot(std::set<int>::iterator& it, int object, int botId, bool attacker)
{
    const int occupied = m_botInfo[botId]->m_occupiedObject;
    m_botInfo[botId]->assignToObject(it, AIStrategicObjectManager::getInstance()->getStrategicObject(object), attacker,
                                     this);
    return occupied;
}

bool SAI::strategicObjectsHasChanged()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::strategicObjectsHasChanged");
}

void SAI::registerTeleporter(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::registerTeleporter");
}

float SAI::getStrategyHeat(unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getStrategyHeat");
}

void SAI::listStrategies()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::listStrategies");
}

int SAI::getActiveStrategy()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getActiveStrategy");
}

// insert (point,1), increment only on duplicate.
// The native map lives at retail ELF +0x1D8 / PE +0x228; no key validation.
void SAI::addSpawnToPoint(int point)
{
    const std::pair<std::map<int, int>::iterator, bool> inserted =
        m_map1D8.insert(std::make_pair(point, 1));
    if (!inserted.second)
        ++inserted.first->second;
}

int SAI::getSpawnsToPoint(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getSpawnsToPoint");
}

// 0x33e240 (Ghidra). No bound check on the id.
int SAI::getBotCurrentPos(int botId) const
{
    const BotInfo* const info = m_botInfo[botId];
    return info ? info->m_occupiedObject : -22;
}

// 0x33e270 (Ghidra).
int SAI::getBotOrderedPos(int botId) const
{
    const BotInfo* const info = m_botInfo[botId];
    return info ? info->m_assignedObject : -44;
}

// 0x33e2a0 (Ghidra).
int SAI::getBotLastOrderedPos(int botId) const
{
    const BotInfo* const info = m_botInfo[botId];
    return info ? info->m_lastOrderedSquare : -1;
}

// Accumulate into the previous totals, then decay; no initial clear.
// Nonempty information can still reach the named Information/IPIUnit forwarding frontiers.
void SAI::updateStrengths()
{
    std::vector<unsigned int> handles;
    UnitPredicate* predicate = checkPointer(new UnitPredicate);
    static_cast<IAIInformationEnvironment*>(AISingletonServer::m_iaiInformationEnvironment)->
        getAllInformation(handles, m_side, true, predicate, nullptr);

#if defined(_MSC_VER)
    // [PLATFORM] PE retains the last secondary lookup's manager for the next root lookup.
    // ELF reloads the global at the start of each root, and before each secondary callback.
    InformationManager* manager = handles.empty() ? nullptr : informationManager;
#endif
    for (auto it = handles.begin(); it != handles.end(); ++it)
    {
#if !defined(_MSC_VER)
        InformationManager* manager = informationManager;
#endif
        Information* root = manager->getInformation(*it);
        int secondary = root->getFirstSecondaryObjectIterator();
        const int rootSide = root->getSide();
        const bool friendly = rootSide == m_side;
        float* strengths = friendly ? m_friendlyStrengths : m_enemyStrengths;
        float* counts = friendly ? m_friendlyCounts : m_enemyCounts;
        Information* current = root;
        do
        {
            // Keep the selected unit across isOccupied/getSecurity, even if a callback replaces it.
            IPIUnit* unit = current->getUnit();
            if (unit->isOccupied())
            {
                const float security = current->getSecurity();
                for (int category = 0; category != 6; ++category)
                {
                    // Both shipped x87 paths retain the product through the add before storing
                    // float. Widen explicitly so modern SSE does not round the product early.
                    strengths[category] = static_cast<float>(
                        static_cast<double>(security) * unit->getBattleStrengths()[category]
                        + strengths[category]);
                }
                const int category = current->getPhysical()->getStrengthType();
                counts[category] += security;
            }

#if !defined(_MSC_VER)
            manager = informationManager;
#endif
            // Iteration always belongs to the original record, with the current SAI side.
            const unsigned int handle = root->getSecondaryObject(secondary, m_side);
#if defined(_MSC_VER)
            manager = informationManager;
#endif
            current = manager->getInformation(handle);
        } while (current);
    }

    for (int category = 0; category != 6; ++category)
    {
        if (m_friendlyStrengths[category] > 0.001f)
            m_friendlyStrengths[category] *= 0.5f;
        if (m_friendlyCounts[category] > 0.001f)
            m_friendlyCounts[category] *= 0.5f;
        if (m_enemyStrengths[category] > 0.001f)
            m_enemyStrengths[category] *= 0.5f;
        if (m_enemyCounts[category] > 0.001f)
            m_enemyCounts[category] *= 0.5f;
    }
    delete predicate;
}

// Updates strategic state in callback order with live vector iteration and repeated
// Route/Bridge tests. Non-area objects reuse the preceding area's output and border flag.
void SAI::updateStates(float* a, float* b)
{
    float* selected = 0;
    bool neighbour = false;
    const int enemySide = m_side == 1 ? 2 : 1;
    IAIGame* const game = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame);
    a[13] = static_cast<float>(game->getTickets(m_side));
    b[13] = static_cast<float>(game->getTickets(enemySide));
    a[14] = static_cast<float>(game->getNumberOfControlPoints(m_side));
    b[14] = static_cast<float>(game->getNumberOfControlPoints(enemySide));

    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    a[15] = static_cast<float>(now);
    b[15] = static_cast<float>(now);
    StrategyContainer* const end = m_strategies.empty()
        ? m_strategies.data() : m_strategies.data() + m_strategies.size();
    a[16] = m_activeStrategy == end ? 0.0f : static_cast<float>(now - m_activeStrategy->m_startTime);
    b[16] = 0.0f;
    a[17] = static_cast<float>(m_list1BC.size());
    a[18] = static_cast<float>(m_list1B8.size());
    a[19] = 0.0f; b[19] = 0.0f;
    a[20] = 0.0f; b[20] = 0.0f;
    a[21] = 0.0f; b[21] = 0.0f;
    a[39] = 0.0f; b[39] = 0.0f;
    a[38] = 0.0f; b[38] = 0.0f;
    b[41] = 0.0f; a[41] = 0.0f;
    a[40] = 0.0f; b[40] = 0.0f;
    a[42] = 0.0f; b[42] = 0.0f;

    std::set<int> attack;
    std::set<int> defence;
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if ((*it)->getType() == 0)
        {
            switch ((*it)->getStatus(m_side))
            {
            case StatusMap::Owned:
                a[19] += 1.0f;
                b[21] += 1.0f;
                selected = a;
                neighbour = (*it)->hasNeighbourWithStatus(StatusMap::Hostile, m_side);
                if (neighbour) { a[39] += 1.0f; b[41] += 1.0f; }
                else           { a[40] += 1.0f; b[42] += 1.0f; }
                break;
            case StatusMap::Hostile:
                a[21] += 1.0f;
                b[19] += 1.0f;
                selected = b;
                neighbour = (*it)->hasNeighbourWithStatus(StatusMap::Owned, m_side);
                if (neighbour) { b[39] += 1.0f; a[41] += 1.0f; }
                else           { b[40] += 1.0f; a[42] += 1.0f; }
                break;
            case StatusMap::Neutral:
                a[20] += 1.0f;
                b[20] += 1.0f;
                if ((*it)->hasNeighbouringObjectWithStatus(StatusMap::Owned, m_side))
                    a[38] += 1.0f;
                if ((*it)->hasNeighbouringObjectWithStatus(StatusMap::Hostile, m_side))
                    b[38] += 1.0f;
                selected = 0;
                break;
            default:
                selected = 0;
                neighbour = false;
                break;
            }
        }

        if ((*it)->getNumberOfAssignedUnits(m_side) > 0)
        {
            if ((*it)->isAttack(m_side))
                attack.insert((*it)->getIndex());
            else
                defence.insert((*it)->getIndex());
        }

        if (selected)
        {
            // Each test reloads *it, including after a preceding virtual callback replaced it.
            static const AIStrategicObject::StrategicObjectType areaFlags[] = {
                AIStrategicObject::Flank, AIStrategicObject::Base, AIStrategicObject::Close,
                AIStrategicObject::Centre, AIStrategicObject::Remote
            };
            for (int i = 0; i != 5; ++i)
            {
                if ((*it)->hasObjectTypeFlag(areaFlags[i]))
                {
                    selected[22 + i] += 1.0f;
                    if (neighbour)
                        selected[33 + i] += 1.0f;
                }
            }
            static const AIStrategicObject::StrategicObjectType otherFlags[] = {
                AIStrategicObject::Route, AIStrategicObject::Bridge,
                AIStrategicObject::Route, AIStrategicObject::Bridge,
                AIStrategicObject::North, AIStrategicObject::West,
                AIStrategicObject::South, AIStrategicObject::East
            };
            static const int indices[] = { 27, 28, 27, 28, 29, 30, 31, 32 };
            for (int i = 0; i != 8; ++i)
                if ((*it)->hasObjectTypeFlag(otherFlags[i]))
                    selected[indices[i]] += 1.0f;
        }
    }
    b[17] = static_cast<float>(attack.size());
    b[18] = static_cast<float>(defence.size());
}

// Prepares admitted bots and resource strengths. Admission retains its Bot pointer
// across callbacks; strength/removal traversals reload the element. Preserve all four
// id queries during removal.
void SAI::prepareUpdate()
{
    for (std::vector<Bot*>::iterator it = m_vector94.begin(); it != m_vector94.end(); ++it)
    {
        Bot* const bot = *it;
        if (bot)
        {
            BotInfo* const info = checkPointer(new BotInfo(bot));
            m_botInfo[bot->getBotId()] = info;
            if (bot->isDead())
            {
                m_map5C.insert(std::make_pair(static_cast<int>(bot->getBotId()), std::make_pair(-1, 13)));
                bot->getPlayer()->setSpawnGroup(0);
                bot->setIsInTransitionPhase(true);
            }
            else
                setBotIsAlive(bot->getBotId());
            (void)m_setFC.find(bot->getBotId()); // shipped discarded lookup
        }
    }

    // [PLATFORM] PE frees/nulls all three vector pointers; ELF erase retains capacity.
#if defined(_MSC_VER)
    std::vector<Bot*>().swap(m_vector94);
#else
    m_vector94.erase(m_vector94.begin(), m_vector94.end());
#endif
    clearSpawnsToPoints();
    for (std::list<int>::iterator it = m_list110.begin(); it != m_list110.end(); ++it)
        handleTeleportingBot(*it);
    m_list110.clear();

    for (int i = 0; i != 6; ++i)
    {
        m_friendlyStrengths[i] = 0.0f;
        m_friendlyCounts[i] = 0.0f;
    }
    for (std::vector<BotInfo*>::iterator it = m_botInfo.begin(); it != m_botInfo.end(); ++it)
    {
        if (*it && !(*it)->m_bot->isDead())
        {
            // [PLATFORM] ELF captures the manager before the equipment callback. PE does so
            // only afterward, and does not read it at all when the handle's low half is zero.
#if !defined(_MSC_VER)
            InformationManager* const manager = informationManager;
#endif
            const unsigned int handle = (*it)->m_bot->getCurrentEquipment();
#if defined(_MSC_VER)
            if ((handle & 0xffffu) == 0)
                continue;
            InformationManager* const manager = informationManager;
#endif
            Information* const information = manager->getInformation(handle);
            if (information && information->getUnit())
            {
                IPIUnit* const unit = information->getUnit();
                // Assignment, not summation: the last live unit supplies all six strengths.
                for (int i = 0; i != 6; ++i)
                    m_friendlyStrengths[i] = unit->getBattleStrengths()[i];
                const int category = information->getPhysical()->getStrengthType();
                m_friendlyCounts[category] += 1.0f;
            }
        }
    }

    for (std::map<int, std::pair<int, int> >::iterator it = m_map5C.begin(); it != m_map5C.end();)
    {
        IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
        BotInfo* const info = m_botInfo[it->first];
        if (!info)
        {
            std::map<int, std::pair<int, int> >::iterator removed = it++;
            m_map5C.erase(removed);
        }
        else
        {
            Bot* const bot = info->m_bot;
            if (settings->isRespawnAllowed())
                BotSpawner::findSuitableSpawnGroup(m_enemyStrengths, m_enemyCounts,
                    m_friendlyStrengths, m_friendlyCounts, it->second.first, bot);
            ++it;
        }
    }

    for (std::vector<Bot*>::iterator it = m_vectorA0.begin(); it != m_vectorA0.end(); ++it)
    {
        if (m_botInfo[(*it)->getBotId()])
        {
            m_botInfo[(*it)->getBotId()]->remove(this);
            delete m_botInfo[(*it)->getBotId()];
            m_botInfo[(*it)->getBotId()] = 0;
        }
    }
#if defined(_MSC_VER)
    std::vector<Bot*>().swap(m_vectorA0);
#else
    m_vectorA0.erase(m_vectorA0.begin(), m_vectorA0.end());
#endif
}

// Reset mapped values
// retaining keys and nodes. The live native end is checked after each iterator advance.
void SAI::clearSpawnsToPoints()
{
    for (std::map<int, int>::iterator it = m_map1D8.begin(); it != m_map1D8.end(); ++it)
        it->second = 0;
}

// Remaining direct SAI::update dependencies. Paired callers and helper return paths establish
// these signatures; the strategic work behind them remains unrecovered.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// BotInfo::remove: / , paired callers discard
// the result and neither body materializes a return. Assigned/free/present-unit removal is absent.
// 0x33f3d0 (Ghidra). Out of the object it was sent to or the one it occupies, and of the free bots.
void SAI::BotInfo::remove(SAI* sai)
{
    const int botId = m_bot->getBotId();
    AIStrategicObjectManager* const objects = AIStrategicObjectManager::getInstance();
    AIStrategicObject* const occupied = objects->getStrategicObject(m_occupiedObject);
    AIStrategicObject* const assigned = objects->getStrategicObject(m_assignedObject);
    if (assigned)
        removeAssigned(assigned, true, sai);
    if (assigned || occupied)
    {
        if (!assigned && sai->m_setFC.find(botId) != sai->m_setFC.end())
            free(true, sai);
        occupied->removeAsPresentUnit(botId, sai->m_side);
    }
    sai->m_set44.erase(botId);
}

// 0x33ff30 (Ghidra).
void SAI::BotInfo::setAsDead()
{
    m_assignedObject = -1;
    m_lastOrderedSquare = -1;
    m_occupiedObject = -1;
    m_float10 = 0.0f;
}

// 0x33f950 (Ghidra).
void SAI::BotInfo::assignToObject(std::set<int>::iterator& it, AIStrategicObject* object, bool attacker, SAI* sai)
{
    m_assignedObject = object->getIndex();
    m_lastOrderedSquare = object->getIndex();
    m_lastOrderTime = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    AIStrategicObject* const occupied = AIStrategicObjectManager::getInstance()->getStrategicObject(m_occupiedObject);
    if (attacker)
        occupied->unregisterAvailableAttacker(it, sai->m_side);
    else
        occupied->unregisterAvailableDefender(it, sai->m_side);
    object->getAssignedUnits(sai->m_side).insert(m_bot->getBotId());
}

// 0x33f290 (Ghidra). The shipped body also looks the bot up in m_setFC and drops the result.
void SAI::BotInfo::occupy(std::set<int>::iterator& it, AIStrategicObject* from,
                          AIStrategicObject* to, SAI* sai)
{
    const int botId = m_bot->getBotId();
    to->setAsPresentUnit(botId, sai->m_side);
    if (from->unregisterAvailableAttacker(botId, sai->m_side))
        to->registerAvailableAttacker(botId, sai->m_side);
    else if (from->unregisterAvailableDefender(botId, sai->m_side))
        to->registerAvailableDefender(botId, sai->m_side);
    from->removeAsPresentUnit(it, sai->m_side);
    m_occupiedObject = to->getIndex();
}

// 0x33f1f0 (Ghidra).
void SAI::BotInfo::occupy(AIStrategicObject* from, AIStrategicObject* to, SAI* sai)
{
    std::set<int>& present = from->getPresentUnits(sai->m_side);
    std::set<int>::iterator it = present.find(m_bot->getBotId());
    occupy(it, from, to, sai);
}

// 0x33f660 (Ghidra). The same erase choice as free below: the shipped code erases m_setFC at
// the lookup's result.
void SAI::BotInfo::removeAssigned(AIStrategicObject* object, std::set<int>::iterator& it,
                                  bool release, SAI* sai)
{
    std::set<int>& assigned = object->getAssignedUnits(sai->m_side);
    assigned.erase(it++);
    if (release)
    {
        sai->m_setFC.erase(m_bot->getBotId());
    }
    else
    {
        AIStrategicObject* const occupied =
            AIStrategicObjectManager::getInstance()->getStrategicObject(m_occupiedObject);
        if (object->isAttack(sai->m_side))
            occupied->registerAvailableAttacker(m_bot->getBotId(), sai->m_side);
        else
            occupied->registerAvailableDefender(m_bot->getBotId(), sai->m_side);
    }
    m_assignedObject = -1;
    m_lastOrderTime = 0.0;
}

// 0x33f5b0 (Ghidra).
void SAI::BotInfo::removeAssigned(AIStrategicObject* object, bool release, SAI* sai)
{
    std::set<int>& assigned = object->getAssignedUnits(sai->m_side);
    std::set<int>::iterator it = assigned.find(m_bot->getBotId());
    removeAssigned(object, it, release, sai);
}

// 0x33f0e0 (Ghidra). The shipped body also looks the bot up in m_setFC and drops the result.
void SAI::BotInfo::respawn(AIStrategicObject* object, SAI* sai)
{
    m_occupiedObject = object->getIndex();
    object->setAsPresentUnit(m_bot->getBotId(), sai->m_side);
    sai->m_set44.insert(m_bot->getBotId());
}

// 0x33fb10 (Ghidra).
void SAI::BotInfo::assign(std::set<int>::iterator& it, bool attacker, SAI* sai)
{
    AIStrategicObject* const object =
        AIStrategicObjectManager::getInstance()->getStrategicObject(m_occupiedObject);
    if (attacker)
        object->registerAvailableAttacker(m_bot->getBotId(), sai->m_side);
    else
        object->registerAvailableDefender(m_bot->getBotId(), sai->m_side);
    it = sai->m_set44.erase(it);
    sai->m_setFC.insert(m_bot->getBotId());
}

// 0x33ffd0 (Ghidra).
int SAI::BotInfo::setLastOrderedSquare(int square)
{
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    m_lastOrderedSquare = square;
    m_lastOrderTime = now;
    return square;
}

// 0x33fdf0 (Ghidra). [CHOSEN] The shipped code erases m_setFC at the lookup's result, which is
// undefined when the bot is missing; erasing by key gives the same result when it is present.
void SAI::BotInfo::free(AIStrategicObject* object, bool attacker, std::set<int>::iterator& it,
                        SAI* sai)
{
    const int botId = m_bot->getBotId();
    if (attacker)
        object->unregisterAvailableAttacker(it, sai->m_side);
    else
        object->unregisterAvailableDefender(it, sai->m_side);
    sai->m_setFC.erase(botId);
    sai->m_set44.insert(m_bot->getBotId());
}

// 0x33fc80 (Ghidra). The same erase choice as above.
void SAI::BotInfo::free(bool unregister, SAI* sai)
{
    const int botId = m_bot->getBotId();
    AIStrategicObject* const object =
        AIStrategicObjectManager::getInstance()->getStrategicObject(m_occupiedObject);
    if (unregister)
    {
        if (!object->unregisterAvailableAttacker(botId, sai->m_side))
            object->unregisterAvailableDefender(botId, sai->m_side);
        sai->m_setFC.erase(botId);
    }
    sai->m_set44.insert(m_bot->getBotId());
}

// 0x338fe0 (Ghidra). A bot that is not free gives up its assigned object (and its busy place),
// or else the object it occupies; either way its last order is forgotten. Then it occupies the
// closest strategic object for its equipment, unless that is where it is.
void SAI::handleChangingBot(int botId)
{
    BotInfo* const info = m_botInfo[botId];
    if (!info)
        return;
    if (m_set44.find(botId) == m_set44.end())
    {
        if (info->m_assignedObject != -1)
        {
            AIStrategicObject* const assigned =
                AIStrategicObjectManager::getInstance()->getStrategicObject(info->m_assignedObject);
            info->removeAssigned(assigned, true, this);
            info->free(false, this);
        }
        else if (info->m_occupiedObject != -1)
        {
            info->free(true, this);
        }
    }
    info->setLastOrderedSquare(-1);

    Information* const equipment = informationManager->getInformation(info->m_bot->getCurrentEquipment());
    if (!equipment || !equipment->getUnit())
        return;
    IPIMobile* const mobile = equipment->getMobile();
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    const int vehicleNumber = mobile ? mobile->getVehicleNumber() : -1;
    const int type = equipment->getUnit()->getEquipmentType();
    // no null guard on the closest object, as shipped
    AIStrategicObject* const closest =
        manager->getClosestObject(equipment->getPosition(), type, vehicleNumber, m_side);
    if (closest->getIndex() == info->m_occupiedObject)
        return;
    info->occupy(manager->getStrategicObject(info->m_occupiedObject), closest, this);
}

// Void is inferred from complete
// bodies and their callers. The spawn-group selector and dead-bot/respawn chain remain absent.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void SAI::handleTeleportingBot(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::handleTeleportingBot");
}

bool SAI::updateStrategicObjects()
{
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    bool changed = false;
    if (m_flag1D0)
    {
        m_flag1CF = false;
        m_flag1D0 = false;
        m_strategicObjectIterator = objects.begin();
        m_enemyStats->clear();
        m_friendlyStats->clear();
    }

    // Both ports dereference unconditionally and process exactly one object. The concrete
    // update returns void. There is no empty-vector guard or inner budget check.
    (*m_strategicObjectIterator)->update(m_side, this, changed);
    m_flag1CF = m_flag1CF || changed;

    // The callback may change the cursor/entry/side. Capture status before the getter
    // then load the selected destination; the shipped numeric split is zero/nonzero.
    AIStrategicObject* const object = *m_strategicObjectIterator;
    const int side = m_side;
    const StatusMap::Status status = object->getStatus(side);
    EnemyStatistics& statistics = object->getBattleStatistics(side);
    if (status == 0)
        m_enemyStats->add(statistics);
    else
        m_friendlyStats->add(statistics);

    ++m_strategicObjectIterator;
    if (m_strategicObjectIterator != objects.end())
        return false;

    if (m_flag1CF)
    {
        // Retain this invocation's vector, but reload its live end, entry and side.
        for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin();
             it != objects.end(); ++it)
        {
            (*it)->setAsDirty(m_side);
            (*it)->releaseAssignedResources(m_side);
        }
    }
    m_flag1D0 = true;
    return true;
}

// 0x339a10 (Ghidra). The band is (0.83, 1.2), from doubles narrowed to float.
void SAI::chooseStrategy()
{
    float* const friendly = m_int1F4 ? m_stateB : m_stateA;
    float* const enemy = m_int1F4 ? m_stateD : m_stateC;
    StrategyManager::getInstance()->isDebugEnabled();

    Strategy* const forced = StrategyManager::getInstance()->getForcedStrategy(m_side);
    if (forced)
    {
        // The forced strategy's prerequisite is computed and dropped.
        forced->calculatePrerequisite(friendly, enemy, m_activeStrategy->m_strategy, m_side);
        for (std::vector<StrategyContainer>::iterator it = m_strategies.begin();
             it != m_strategies.end(); ++it)
        {
            if (it->m_strategy == forced)
                changeStrategy(it);
        }
        return;
    }

    bool outOfBand = false;
    for (std::vector<StrategyContainer>::iterator it = m_strategies.begin();
         it != m_strategies.end(); ++it)
    {
        it->m_prerequisite = it->m_strategy->calculatePrerequisite(
            friendly, enemy, m_activeStrategy->m_strategy, m_side);
        if (it->m_prerequisite < 0.0f)
            it->m_prerequisite = 0.0f;
        float ratio;
        if (it->m_prerequisite != 0.0f)
            ratio = it->m_heat / it->m_prerequisite;
        else
            ratio = it->m_heat == 0.0f ? 1.0f : 0.0f;
        if (ratio <= 0.83f || 1.2f <= ratio)
            outOfBand = true;
    }

    if (!outOfBand)
    {
        // The limit grows from 1x towards 2x of the strategy's time limit with each kept turn.
        const float half = std::pow(0.5f, static_cast<float>(m_activeStrategy->m_activations));
        const float scale = (0.5f - half) + (0.5f - half) + 1.0f;
        const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
        if (now - m_activeStrategy->m_startTime < scale * m_activeStrategy->m_strategy->m_timeLimit)
            return;
        ++m_activeStrategy->m_activations;
    }

    float total = 0.0f;
    for (std::vector<StrategyContainer>::iterator it = m_strategies.begin();
         it != m_strategies.end(); ++it)
    {
        it->m_heat = it->m_prerequisite;
        total += it->m_prerequisite <= 0.0f ? 0.0f : it->m_prerequisite;
    }

    float draw = static_cast<float>(static_cast<double>(AIRandom::m_instance->next()) *
                                    2.3283064e-10f * total);
    std::vector<StrategyContainer>::iterator it = m_strategies.begin();
    for (; it != m_strategies.end(); ++it)
    {
        draw -= it->m_heat <= 0.0f ? 0.0f : it->m_heat;
        if (draw <= 0.0f)
            break;
    }
    if (it == m_strategies.end())
        --it;
    changeStrategy(it);
}

// 0x339610 (Ghidra).
void SAI::changeStrategy(std::vector<StrategyContainer>::iterator& it)
{
    StrategyContainer* const container = &*it;
    if (m_activeStrategy == container)
        return;
    m_activeStrategy = container;

    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator object = objects.begin();
         object != objects.end(); ++object)
    {
        (*object)->releaseAssignedResources(m_side);
        // free erases the bot from the set and advances the cursor.
        std::set<int>& attackers = (*object)->getAvailableAttackers(m_side);
        for (std::set<int>::iterator bot = attackers.begin(); bot != attackers.end();)
            m_botInfo[*bot]->free(*object, true, bot, this);
        std::set<int>& defenders = (*object)->getAvailableDefenders(m_side);
        for (std::set<int>::iterator bot = defenders.begin(); bot != defenders.end();)
            m_botInfo[*bot]->free(*object, false, bot, this);
    }

    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    m_activeStrategy->m_activations = 1;
    m_activeStrategy->m_startTime = static_cast<float>(now);
}

// Weights every strategic object's temperature by the active strategy.
void SAI::modifyStrategicObjectsTemperatures()
{
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end();
         ++it)
        (*it)->updateTemperature(m_activeStrategy->m_strategy, m_side);
}

// 0x33d910 (Ghidra). Only strategic areas take part. A bot standing in an area moves to the
// neighbouring object that pulls hardest on its position, if that object accepts its vehicle group
// and equipment. A bot assigned to or available at an area it stands in is retained there once it
// follows a move-to route with at most one sensed object and has waited 20 seconds on foot or 35
// otherwise since its last order.
void SAI::updateBotPositions()
{
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    IBotManager* const bots = IBotManager::instance;
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator object = objects.begin();
         object != objects.end(); ++object)
    {
        if ((*object)->getType() != 0)
            continue;
        AIStrategicArea* const area = static_cast<AIStrategicArea*>(*object);

        std::set<int>& present = area->getPresentUnits(m_side);
        for (std::set<int>::iterator it = present.begin(); it != present.end();)
        {
            Bot* const bot = bots->getBotFromId(*it);
            Information* const information =
                informationManager->getInformation(bot->getCurrentEquipment());
            if (!information || !information->getUnit())
            {
                ++it;
                continue;
            }
            const int equipmentType = information->getUnit()->getEquipmentType();
            const int group = settings->getVehicleGroup(equipmentType);
            AIStrategicObject* best = area;
            float bestGravity = area->gravity(information->getPosition(), m_side);
            for (std::vector<AIStrategicObject*>::const_iterator neighbour =
                     area->m_neighbouringObjects.begin();
                 neighbour != area->m_neighbouringObjects.end(); ++neighbour)
            {
                if ((*neighbour)->isAllowedVehicleGroup(group) &&
                    !(*neighbour)->isExpelledUnit(equipmentType) && (*neighbour)->getType() == 0 &&
                    !static_cast<AIStrategicArea*>(*neighbour)->isLandingZoneUser(equipmentType))
                {
                    const float gravity = (*neighbour)->gravity(information->getPosition(), m_side);
                    if (bestGravity < gravity)
                    {
                        bestGravity = gravity;
                        best = *neighbour;
                    }
                }
            }
            if (best == area)
                ++it;
            else
                m_botInfo[*it]->occupy(it, area, best, this);
        }

        const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
        // The shipped code repeats this loop for each of the three sets.
        const auto retainSettled = [&](std::set<int>& units)
        {
            for (std::set<int>::iterator it = units.begin(); it != units.end(); ++it)
            {
                Bot* const bot = bots->getBotFromId(*it);
                const float wait = bot->getCurrentEquipment() ==
                                           bot->getPlayer()->getSoldier()->getInformation(m_side)
                                       ? 20.0f
                                       : 35.0f;
                if (bot->getWaypoints()->m_flag05 && bot->getWaypoints()->getId() == 1 &&
                    static_cast<double>(wait) < now - m_botInfo[*it]->m_lastOrderTime &&
                    bot->getSensedObjects().size() < 2 && area->isPresentUnit(*it, m_side))
                {
                    area->retainBot(m_side, *it, this);
                    m_botInfo[*it]->m_lastOrderTime = now;
                }
            }
        };
        retainSettled(area->getAssignedUnits(m_side));
        retainSettled(area->getAvailableAttackers(m_side));
        retainSettled(area->getAvailableDefenders(m_side));
    }
}

// 0x339e60 (Ghidra). Totals the side's strategic strength: m_intD8 sums the first strength of every
// available attacker and of every assigned bot at an object it attacks, m_intDC the second of the
// defenders likewise, and m_intE0 the averaged strength of the bots standing in objects less the
// averaged strength of those counted. While m_flag1CD or m_flag1CC is set, the available attackers
// or defenders are freed instead, and m_flag1CE records that one was. Bots whose equipment has no
// unit information are skipped.
void SAI::updateDistribution()
{
    IBotManager* const bots = IBotManager::instance;
    const auto unitOf = [&](int botId) -> IPIUnit*
    {
        Information* const information =
            informationManager->getInformation(bots->getBotFromId(botId)->getCurrentEquipment());
        return information ? information->getUnit() : 0;
    };
    const auto averageStrength = [](IPIUnit* unit) -> float
    {
        float sum = 0.0f;
        for (int i = 0; i < unit->getNStrategicStrengths(); ++i)
            sum += unit->getStrategicStrengths()[i];
        return sum / static_cast<float>(unit->getNStrategicStrengths());
    };

    float attack = 0.0f;
    float defence = 0.0f;
    m_flag1CE = false;
    m_intD8 = 0;
    m_intDC = 0;
    m_intE0 = 0;
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator object = objects.begin();
         object != objects.end(); ++object)
    {
        std::set<int>& attackers = (*object)->getAvailableAttackers(m_side);
        if (!m_flag1CD)
        {
            for (std::set<int>::iterator it = attackers.begin(); it != attackers.end(); ++it)
            {
                IPIUnit* const unit = unitOf(*it);
                if (unit)
                {
                    m_intD8 += static_cast<int>(unit->getStrategicStrengths()[0]);
                    attack += averageStrength(unit);
                }
            }
        }
        else
        {
            while (!attackers.empty())
            {
                m_botInfo[*attackers.begin()]->free(true, this);
                m_flag1CE = true;
            }
        }

        std::set<int>& defenders = (*object)->getAvailableDefenders(m_side);
        if (!m_flag1CC)
        {
            for (std::set<int>::iterator it = defenders.begin(); it != defenders.end(); ++it)
            {
                IPIUnit* const unit = unitOf(*it);
                if (unit)
                {
                    m_intDC += static_cast<int>(unit->getStrategicStrengths()[1]);
                    defence += averageStrength(unit);
                }
            }
        }
        else
        {
            while (!defenders.empty())
            {
                m_botInfo[*defenders.begin()]->free(true, this);
                m_flag1CE = true;
            }
        }

        std::set<int>& present = (*object)->getPresentUnits(m_side);
        for (std::set<int>::iterator it = present.begin(); it != present.end(); ++it)
        {
            IPIUnit* const unit = unitOf(*it);
            if (unit)
                m_intE0 += static_cast<int>(averageStrength(unit));
        }

        std::set<int>& assigned = (*object)->getAssignedUnits(m_side);
        for (std::set<int>::iterator it = assigned.begin(); it != assigned.end(); ++it)
        {
            IPIUnit* const unit = unitOf(*it);
            if (!unit)
                continue;
            if ((*object)->isAttack(m_side))
            {
                m_intD8 += static_cast<int>(unit->getStrategicStrengths()[0]);
                attack += averageStrength(unit);
            }
            else
            {
                m_intDC += static_cast<int>(unit->getStrategicStrengths()[1]);
                defence += averageStrength(unit);
            }
        }
    }
    m_intE0 -= static_cast<int>(defence + attack);
}

// 0x33a690 (Ghidra). Splits the side between attack and defence: everything attacks while it owns
// no area, everything defends when nothing is left to take, and otherwise the active strategy's
// aggression is the attack share. Each free bot is placed in the object it occupies, or the
// closest one that takes it, retained there when that is a new order, and made an attacker
// while the attack total stays within the attack share, else a defender. Then the side is told
// to withdraw attackers or defenders when their share of all strength passes 1.2 times theirs,
// depending on what the last bot became. The shipped body also allocates a cleared flag per
// bot and two lists, and never reads them.
void SAI::distributeResources()
{
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    const float* const friendly = m_int1F4 ? m_stateB : m_stateA;
    float attackShare;
    float defenceShare = 0.0f;
    if (friendly[StrategyCondition::NumberOfFriendlyAreas] == 0.0f)
    {
        attackShare = 1.0f;
    }
    else if (friendly[StrategyCondition::NumberOfNeutralAreas] != 0.0f ||
             friendly[StrategyCondition::NumberOfHostileAreas] != 0.0f)
    {
        attackShare = m_activeStrategy->m_strategy->getAggression();
        defenceShare = 1.0f - m_activeStrategy->m_strategy->getAggression();
    }
    else
    {
        attackShare = 0.0f;
        defenceShare = 1.0f;
    }

    bool lastWasDefender = true;
    for (std::set<int>::iterator it = m_set44.begin(); it != m_set44.end();)
    {
        BotInfo* const info = m_botInfo[*it];
        Information* const information =
            info ? informationManager->getInformation(info->m_bot->getCurrentEquipment()) : 0;
        if (!information)
        {
            ++it;
            continue;
        }
        AIStrategicObject* object = manager->getStrategicObject(info->m_occupiedObject);
        if (object)
        {
            object->setAsPresentUnit(*it, m_side);
        }
        else
        {
            IPIUnit* const unit = information->getUnit();
            if (unit)
            {
                const int vehicleNumber =
                    information->getMobile() ? information->getMobile()->getVehicleNumber() : -1;
                object = manager->getClosestObject(information->getPosition(),
                                                   unit->getEquipmentType(), vehicleNumber, m_side);
            }
            if (!object)
            {
                ++it;
                continue;
            }
            info->respawn(object, this);
        }
        if (object->getIndex() != info->m_lastOrderedSquare)
        {
            object->retainBot(m_side, *it, this);
            info->setLastOrderedSquare(object->getIndex());
        }

        // Not checked when the bot already occupied an object, as shipped.
        IPIUnit* const unit = information->getUnit();
        const int total = m_intD8 + m_intDC;
        bool attacker;
        if (total == 0)
            attacker = 0.5f <= attackShare;
        else if (attackShare <= 0.0f)
            attacker = false;
        else if (defenceShare <= 0.0f)
            attacker = true;
        else
            attacker = static_cast<double>(m_intD8) / static_cast<double>(total) <= attackShare;
        if (attacker)
            m_intD8 += static_cast<int>(unit->getStrategicStrengths()[0]);
        else
            m_intDC += static_cast<int>(unit->getStrategicStrengths()[1]);
        float sum = 0.0f;
        for (int i = 0; i < unit->getNStrategicStrengths(); ++i)
            sum += unit->getStrategicStrengths()[i];
        m_intE0 -= static_cast<int>(sum / static_cast<float>(unit->getNStrategicStrengths()));
        lastWasDefender = !attacker;
        info->assign(it, attacker, this);
    }

    const float total = static_cast<float>(m_intDC + m_intD8 + m_intE0);
    if (total == 0.0f)
    {
        m_flag1CD = false;
        m_flag1CC = false;
        return;
    }
    if (static_cast<double>(m_intD8) / total > 1.2f * static_cast<double>(attackShare) &&
        !lastWasDefender)
    {
        m_flag1CD = true;
        m_flag1CE = true;
    }
    else
    {
        m_flag1CD = false;
    }
    if (static_cast<double>(m_intDC) / total > 1.2f * static_cast<double>(defenceShare) &&
        lastWasDefender)
    {
        m_flag1CC = true;
        m_flag1CE = true;
    }
    else
    {
        m_flag1CC = false;
    }
}

// 0x33ae70 (Ghidra). Category by category when the map has categories.
void SAI::distributeDefence()
{
    if (m_activeStrategy->m_strategy->getNumberOfDefences() == 0)
        return;
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    if (manager->getNumberOfCategories() == 0)
    {
        categoryDistributeDefence(manager->getStrategicObjects());
        return;
    }
    for (int category = 0; category < manager->getNumberOfCategories(); ++category)
        categoryDistributeDefence(manager->getStrategicObjectsOfCategory(category));
}

namespace {

// An area and its defence need or attack value; sorted by it.
struct SquareInfo
{
    int m_index;
    float m_need;
    bool operator<(const SquareInfo& other) const { return m_need < other.m_need; }
};

// Heap order that brings the area with the least assigned part of its wanted strength first.
struct sobOrdering
{
    int m_side;
    bool operator()(AIStrategicObject* a, AIStrategicObject* b) const
    {
        return a->getAssignedPartOfWantedStrength(m_side) > b->getAssignedPartOfWantedStrength(m_side);
    }
};

} // anonymous namespace

// 0x33af10 (Ghidra). Areas needing more than 0.1 are ranked by need; past twice the strategy's
// defences, those below 0.9 of the need at that rank are released with the rest. While the side
// is withdrawing defenders and nothing was freed, the least needy area is released instead of
// filling the others. [PLATFORM] Equal heap keys may pop in another order than libstdc++'s.
void SAI::categoryDistributeDefence(const std::vector<AIStrategicObject*>& objects)
{
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    const unsigned int defences = m_activeStrategy->m_strategy->getNumberOfDefences();
    std::list<SquareInfo> squares;
    int released = 0;
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end();
         ++it)
    {
        if (!*it)
            continue;
        const float need = (*it)->calculateDefenseNeed(m_side);
        if (0.1f < need)
        {
            const SquareInfo square = { (*it)->getIndex(), need };
            squares.push_back(square);
        }
        else if (need <= 0.0f && !(*it)->isAttack(m_side))
        {
            released += (*it)->releaseAssignedResources(m_side);
        }
    }
    squares.sort();
    squares.reverse();
    if (squares.empty())
        return;

    float cut = 0.0f;
    unsigned int rank = 0;
    std::list<SquareInfo>::iterator square = squares.begin();
    for (; square != squares.end(); ++square)
    {
        ++rank;
        if (rank == defences * 2)
            cut = square->m_need;
        else if (defences * 2 < rank &&
                 static_cast<double>(square->m_need) < static_cast<double>(cut) * 0.9f)
            break;
    }
    while (square != squares.end())
    {
        AIStrategicObject* const object = manager->getStrategicObject(square->m_index);
        if (object && !object->isAttack(m_side))
            released += object->releaseAssignedResources(m_side);
        square = squares.erase(square);
    }
    if (squares.empty())
        return;

    if (m_flag1CC && !m_flag1CE && released == 0)
    {
        squares.reverse();
        for (square = squares.begin(); square != squares.end(); ++square)
        {
            AIStrategicObject* const object = manager->getStrategicObject(square->m_index);
            if (object && !object->isAttack(m_side))
            {
                released += object->releaseAssignedResources(m_side);
                if (0 < released)
                    break;
            }
        }
        return;
    }

    const sobOrdering ordering = { m_side };
    std::vector<AIStrategicObject*> heap;
    unsigned int count = 0;
    for (square = squares.begin(); square != squares.end(); ++square)
    {
        AIStrategicObject* const object = manager->getStrategicObject(square->m_index);
        if (!object)
            continue;
        object->setAttack(false, m_side);
        heap.push_back(object);
        std::push_heap(heap.begin(), heap.end(), ordering);
        ++count;
        if (defences <= count)
            break;
    }
    while (!heap.empty())
    {
        std::pop_heap(heap.begin(), heap.end(), ordering);
        AIStrategicObject* const object = heap.back();
        heap.pop_back();
        if (object->collectAdditionalSquadResources(1, m_side, this) != 0)
        {
            heap.push_back(object);
            std::push_heap(heap.begin(), heap.end(), ordering);
        }
    }
}

// 0x33b9b0 (Ghidra). Category by category when the map has categories.
void SAI::distributeAttack()
{
    if (m_activeStrategy->m_strategy->getNumberOfAttacks() == 0)
        return;
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    if (manager->getNumberOfCategories() == 0)
    {
        categoryDistributeAttack(manager->getStrategicObjects());
        return;
    }
    for (int category = 0; category < manager->getNumberOfCategories(); ++category)
        categoryDistributeAttack(manager->getStrategicObjectsOfCategory(category));
}

// 0x33ba50 (Ghidra). Areas worth more than 0.001 are ranked by value; past the strategy's
// attacks, those below 0.8 of the last value within them, or past twice the attacks, are released
// with the rest. While the side is withdrawing attackers and nothing was freed, the least valuable
// is dropped too and the most valuable attacked one is released instead of filling. Otherwise the
// areas already attacked come first, then the others in value order, while at most the strategy's
// attacks are chosen before each one, so one more can join. [PLATFORM] Heap ties as in defence.
void SAI::categoryDistributeAttack(const std::vector<AIStrategicObject*>& objects)
{
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    const unsigned int attacks = m_activeStrategy->m_strategy->getNumberOfAttacks();
    std::list<SquareInfo> squares;
    int released = 0;
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end();
         ++it)
    {
        if (!*it)
            continue;
        const float value = (*it)->calculateAttackValue(m_side);
        if (0.001f < value)
        {
            const SquareInfo square = { (*it)->getIndex(), value };
            squares.push_back(square);
        }
    }
    squares.sort();
    squares.reverse();
    if (squares.empty())
        return;

    float cut = 0.0f;
    unsigned int rank = 0;
    std::list<SquareInfo>::iterator square = squares.begin();
    for (; square != squares.end(); ++square)
    {
        ++rank;
        if (rank <= attacks)
            cut = square->m_need;
        else if (attacks * 2 < rank ||
                 static_cast<double>(square->m_need) < static_cast<double>(cut) * 0.8f)
            break;
    }
    while (square != squares.end())
    {
        AIStrategicObject* const object = manager->getStrategicObject(square->m_index);
        if (object && object->isAttack(m_side))
            released += object->releaseAssignedResources(m_side);
        square = squares.erase(square);
    }
    const bool withdrawing = m_flag1CD && !m_flag1CE && released == 0;
    if (withdrawing && !squares.empty())
        squares.pop_back();
    if (squares.empty())
        return;

    if (withdrawing)
    {
        for (square = squares.begin(); square != squares.end(); ++square)
        {
            AIStrategicObject* const object = manager->getStrategicObject(square->m_index);
            if (object && object->isAttack(m_side))
            {
                released += object->releaseAssignedResources(m_side);
                if (0 < released)
                    return;
            }
        }
        return;
    }

    std::list<SquareInfo> chosen;
    for (square = squares.begin(); square != squares.end();)
    {
        if (attacks < chosen.size())
            break;
        AIStrategicObject* const object = manager->getStrategicObject(square->m_index);
        if (object && object->isAttack(m_side))
        {
            chosen.push_back(*square);
            square = squares.erase(square);
        }
        else
        {
            ++square;
        }
    }
    for (square = squares.begin(); square != squares.end() && chosen.size() <= attacks; ++square)
        chosen.push_back(*square);

    for (std::list<SquareInfo>::iterator it = chosen.begin(); it != chosen.end(); ++it)
    {
        AIStrategicObject* const object = manager->getStrategicObject(it->m_index);
        if (object)
        {
            object->setAttack(true, m_side);
            object->collectSquadResources(m_side, this);
        }
    }

    const sobOrdering ordering = { m_side };
    std::vector<AIStrategicObject*> heap;
    for (std::list<SquareInfo>::iterator it = chosen.begin(); it != chosen.end(); ++it)
    {
        AIStrategicObject* const object = manager->getStrategicObject(it->m_index);
        if (!object)
            continue;
        heap.push_back(object);
        std::push_heap(heap.begin(), heap.end(), ordering);
    }
    while (!heap.empty())
    {
        std::pop_heap(heap.begin(), heap.end(), ordering);
        AIStrategicObject* const object = heap.back();
        heap.pop_back();
        if (object->collectAdditionalSquadResources(1, m_side, this) != 0)
        {
            heap.push_back(object);
            std::push_heap(heap.begin(), heap.end(), ordering);
        }
    }
}

// 0x33b6e0 (Ghidra).
void SAI::distributeWithNoAreas()
{
    AIStrategicObjectManager* const objects = AIStrategicObjectManager::getInstance();
    if (objects->getNumberOfCategories() == 0)
    {
        categoryDistributeWithNoAreas(objects->getStrategicObjects());
        return;
    }
    for (int i = 0; i < objects->getNumberOfCategories(); ++i)
        categoryDistributeWithNoAreas(objects->getStrategicObjectsOfCategory(i));
}

// 0x33b770 (Ghidra). Every object is attacked or defended as the side holds it, with two more squads.
void SAI::categoryDistributeWithNoAreas(const std::vector<AIStrategicObject*>& objects)
{
    std::list<int> indices;
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it)
        if (*it)
            indices.push_back((*it)->getIndex());
    for (std::list<int>::const_iterator it = indices.begin(); it != indices.end(); ++it)
    {
        AIStrategicObject* const object = AIStrategicObjectManager::getInstance()->getStrategicObject(*it);
        if (object)
        {
            object->setAttack(object->getStatus(m_side) != 0, m_side);
            object->collectAdditionalSquadResources(2, m_side, this);
        }
    }
}

// Recovered empty helper: , (frame prologue/epilogue and return only).
// PE SAI::update omits the empty call at state 14.
void SAI::distributeArtillery()
{
}

// SampleHelper: complete paired bodies, unsigned live bound and no index/size guards.
SampleHelper::SampleHelper(unsigned int size) : m_size(size)
{
}

void SampleHelper::addSample(float* input, int index, float* strength, int* count)
{
    for (unsigned int i = 0; i < m_size; ++i)
        strength[i] = input[i] + strength[i];
    ++count[index];
}

void SampleHelper::clear(float* strength, int* count)
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        strength[i] = 0.0f;
        count[i] = 0;
    }
}

// 0x341b80 (Ghidra).
CommanderInterface* SAI::getCommanderInterface() const
{
    return m_commander;
}

} // namespace ai
} // namespace hfe
} // namespace dice
