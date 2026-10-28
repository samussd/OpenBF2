// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIStrategicObject operations.

#include "AIStrategicObject.h"

#include "../AIStrategic/SAI.h"          // EnemyStatistics - the four objects the constructor makes
#include "../Main/AIDllFrontier.h"
#include "../Main/AIMain.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes strategic-object state and its owned enemy-statistics records.

AIStrategicObject::AIStrategicObject(float radius, float f148)
    : m_position()      // the two-iteration zeroing loop at +132, in member-declaration position
{
    m_statistics[0] = new EnemyStatistics(6);
    m_statistics[1] = new EnemyStatistics(6);
    m_statistics[2] = new EnemyStatistics(2);
    m_statistics[3] = new EnemyStatistics(2);

    const std::set<int> empty;
    m_set20[0] = empty;
    m_set20[1] = empty;
    m_set44[0] = empty;
    m_set44[1] = empty;
    m_set68[0] = empty;
    m_set68[1] = empty;
    m_set92[0] = empty;
    m_set92[1] = empty;

    m_status[0] = StatusMap::Neutral;
    m_status[1] = StatusMap::Neutral;
    m_status[2] = StatusMap::Neutral;
    m_status[3] = StatusMap::Neutral;

    m_f148 = f148;

    m_size[0] = radius;
    m_size[1] = 0.0f;
    m_size[2] = 0.0f;

    m_f164[0] = radius;
    m_f164[1] = radius;
    m_f164[2] = radius;
    m_f164[3] = radius;

    for (int i = 0; i < 2; ++i)
    {
        m_needCurveInput[i] = 0.0f;
        m_friendlyRatio[i] = 0.0f;
        m_friendlyBattleStrength[i] = 0.0f;
        m_enemyBattleStrength[i] = 0.0f;
        m_unresolved212[i] = 0;
        m_resourceNeed[i] = 0;
        m_assignedSquadStrength[i] = 0;
    }

    m_attack[0] = true;
    m_attack[1] = true;

    // `m_index` (+240) is deliberately absent. The constructor does not touch it; see the header.
    m_allowedVehicleGroups = -1;
    m_expelledUnits = 0;
    m_category = -1;
}

// [FRONTIER] Destruction requires ordered statistics and container teardown.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when the destructor is recovered.
AIStrategicObject::~AIStrategicObject()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicObject::~AIStrategicObject");
}

// native set size, side is one-based.
// ELF getAssignedUnits(int) at returns the matching set; the two old STL layouts
// place their counts four bytes apart. SAI::updateStates tests the signed result against zero.
int AIStrategicObject::getNumberOfAssignedUnits(int side) const
{
    return static_cast<int>(m_set68[side - 1].size());
}

void AIStrategicObject::resetStatus()
{
    m_status[0] = m_status[2];
    m_status[1] = m_status[3];
}

// byte at this + 0xEB + side, no bounds guard.
bool AIStrategicObject::isAttack(int side) const
{
    return m_attack[side - 1];
}

// load this + 4*side. The first
// statistics pointer is at +4, so the side is one-based. The caller passes this
// borrowed address to EnemyStatistics::add(EnemyStatistics&); pointer versus
// reference return spelling is [INFERRED]. No null or side guard is present.
EnemyStatistics& AIStrategicObject::getBattleStatistics(int side) const
{
    return *m_statistics[side - 1];
}

// Returns the signed count of released resources, or zero when a commander refuses.
// Re-queries the SAI after the commander callback and reads the object index afterward.
// The downstream SAI release operation remains a named frontier.
int AIStrategicObject::releaseAssignedResources(int side)
{
    if (IAIMain::m_instance->getSAI(side)->isPlayerCommander())
        return 0;

    ISAI* const sai = IAIMain::m_instance->getSAI(side);
    return sai->releaseSquadsFromObject(m_index, side);
}

// 0x3522d0 and 0x3522f0 (Ghidra). Unchecked side.
std::set<int>& AIStrategicObject::getAvailableAttackers(int side)
{
    return m_set20[side - 1];
}

std::set<int>& AIStrategicObject::getAvailableDefenders(int side)
{
    return m_set44[side - 1];
}

// 0x351ff0 and 0x352040 (Ghidra).
bool AIStrategicObject::registerAvailableAttacker(int botId, int side)
{
    m_set20[side - 1].insert(botId);
    return true;
}

bool AIStrategicObject::registerAvailableDefender(int botId, int side)
{
    m_set44[side - 1].insert(botId);
    return true;
}

// 0x352600..0x352780 (Ghidra).
bool AIStrategicObject::unregisterAvailableAttacker(int botId, int side)
{
    std::set<int>::iterator it = m_set20[side - 1].find(botId);
    return unregisterAvailableAttacker(it, side);
}

bool AIStrategicObject::unregisterAvailableAttacker(std::set<int>::iterator& it, int side)
{
    if (it == m_set20[side - 1].end())
        return false;
    it = m_set20[side - 1].erase(it);
    return true;
}

bool AIStrategicObject::unregisterAvailableDefender(int botId, int side)
{
    std::set<int>::iterator it = m_set44[side - 1].find(botId);
    return unregisterAvailableDefender(it, side);
}

bool AIStrategicObject::unregisterAvailableDefender(std::set<int>::iterator& it, int side)
{
    if (it == m_set44[side - 1].end())
        return false;
    it = m_set44[side - 1].erase(it);
    return true;
}

// 0x352310 and 0x3523e0 (Ghidra).
std::set<int>& AIStrategicObject::getAssignedUnits(int side)
{
    return m_set68[side - 1];
}

std::set<int>& AIStrategicObject::getPresentUnits(int side)
{
    return m_set92[side - 1];
}

bool AIStrategicObject::isPresentUnit(int botId, int side) const
{
    return m_set92[side - 1].find(botId) != m_set92[side - 1].end();
}

std::pair<std::set<int>::iterator, bool> AIStrategicObject::setAsPresentUnit(int botId, int side)
{
    return m_set92[side - 1].insert(botId);
}

// 0x352550 (Ghidra).
void AIStrategicObject::removeAsPresentUnit(std::set<int>::iterator& it, int side)
{
    it = m_set92[side - 1].erase(it);
}

// 0x352e80 and 0x352ef0 (Ghidra).
bool AIStrategicObject::isAllowedVehicleGroup(int group) const
{
    return ((m_allowedVehicleGroups >> group) & 1) != 0;
}

bool AIStrategicObject::isExpelledUnit(int equipmentType) const
{
    return ((m_expelledUnits >> equipmentType) & 1) != 0;
}

// 0x352ea0 (Ghidra).
bool AIStrategicObject::allowsSomeVehicleGroups(int groupMask) const
{
    return (m_allowedVehicleGroups & groupMask) != 0;
}

// 0x351fb0 (Ghidra).
void AIStrategicObject::setAttack(bool attack, int side)
{
    m_attack[side - 1] = attack;
}

// 0x352940 (Ghidra).
float AIStrategicObject::getAssignedPartOfWantedStrength(int side) const
{
    if (m_resourceNeed[side - 1] == 0)
        return 1.0f;
    return static_cast<float>(m_assignedSquadStrength[side - 1] / m_resourceNeed[side - 1]);
}

// The ten accessors. Every one of them is between seven and thirty-three bytes in the image.

// Returns type code 3; AIStrategicArea returns 0.
// [UNRESOLVED] These codes are not established as StrategicObjectType values.
int AIStrategicObject::getType() const
{
    return 3;
}

// - `return (char *)this + 8 * side + 124;`, i.e. the 1-based fold. See the header.
const Vec2& AIStrategicObject::getPosition(int side) const
{
    return m_position[side - 1];
}

// - two dword stores, a memberwise Vec2 copy.
void AIStrategicObject::setPosition(int side, const Vec2& position)
{
    m_position[side - 1] = position;
}

// Returns bit 27. [UNRESOLVED] No named StrategicObjectType enumerator covers that bit.
int AIStrategicObject::getObjectTypeFlags() const
{
    return 0x8000000;
}

bool AIStrategicObject::hasObjectTypeFlag(StrategicObjectType) const
{
    return false;
}

bool AIStrategicObject::hasObjectTypeFlags(int) const
{
    return false;
}

float AIStrategicObject::gravity(const Vec3&, int)
{
    return 0.0f;
}

float AIStrategicObject::gravity(const Vec2&, int)
{
    return 0.0f;
}

// Returns twice the side-specific size for both axes; base strategic objects are square.
float AIStrategicObject::getXExtent(int side) const
{
    return m_size[side] + m_size[side];
}

float AIStrategicObject::getZExtent(int side) const
{
    return m_size[side] + m_size[side];
}

} // namespace ai
} // namespace hfe
} // namespace dice
