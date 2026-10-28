// [FRONTIER] execute remains a named stop.

#include "BAPInterpreter.h"

#include "BAPEntry.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BAPInterpreter::BAPInterpreter(int numberOfTypes)
    : m_numberOfTypes(numberOfTypes)
{
    m_entries = new BAPEntry**[m_numberOfTypes];
    for (int type = 0; type < m_numberOfTypes; ++type)
    {
        m_entries[type] = new BAPEntry*[kLodLevels];
        for (int lod = 0; lod < kLodLevels; ++lod)
            m_entries[type][lod] = 0;
    }
    m_activeTypes = new unsigned long long[kLodLevels];
    for (int lod = 0; lod < kLodLevels; ++lod)
        m_activeTypes[lod] = 0;
}

BAPInterpreter::~BAPInterpreter()
{
    for (int type = 0; type < m_numberOfTypes; ++type)
    {
        for (int lod = 0; lod < kLodLevels; ++lod)
            m_entries[type][lod] = 0;
        delete[] m_entries[type];
        m_entries[type] = 0;
    }
    delete[] m_entries;
    m_entries = 0;
    delete[] m_activeTypes;
    m_activeTypes = 0;
}

bool BAPInterpreter::execute(BotScript*, Bot*, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPInterpreter::execute");
}

bool BAPInterpreter::addEntry(BAPEntry* entry)
{
    return addEntry(BotLodFull, entry);
}

bool BAPInterpreter::addEntry(BotLodLevel lod, BAPEntry* entry)
{
    const int type = entry->getTargetId();
    if (type >= m_numberOfTypes || m_entries[type][lod])
        return false;
    for (int level = lod; level < kLodLevels && !m_entries[type][level]; ++level)
    {
        m_entries[type][level] = entry;
        m_activeTypes[level] |= 1ULL << type;
    }
    return true;
}

// Unlike addEntry, every level from lod on is checked, not only the leading free ones.
bool BAPInterpreter::removeEntry(BotLodLevel lod, BAPEntry* entry)
{
    const int type = entry->getTargetId();
    if (type >= m_numberOfTypes)
        return false;
    for (int level = lod; level < kLodLevels; ++level)
    {
        if (m_entries[type][level] == entry)
        {
            m_entries[type][level] = 0;
            m_activeTypes[level] &= ~(1ULL << type);
        }
    }
    return true;
}

bool BAPInterpreter::activateEntry(BAPStatementType type, BotLodLevel lod)
{
    if (type >= m_numberOfTypes || m_entries[type][lod])
        return false;
    for (int level = lod; level < kLodLevels && !m_entries[type][level]; ++level)
        m_activeTypes[level] |= 1ULL << type;
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
