#include "BAPInterpreter.h"

#include "BAPEntry.h"
#include "../AIBotActionPlan/BAPStatement.h"
#include "../../../../Core/Debug.h"
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

// A dead or just respawned bot skips its plan: ok and not done. A statement type the bot's LOD
// does not interpret is an error that fails the step and finishes it. A type with an entry runs
// through the entry, any other through the statement's own execute.
// [CHOSEN] The type and LOD are written as numbers; the enum writers are not recovered.
bool BAPInterpreter::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    if (bot->isJustSpawned())
    {
        done = false;
        ok = true;
        return true;
    }
    const BAPStatementType type = static_cast<BAPStatement*>(script)->getId();
    const BotLodLevel lod = bot->getLodLevel();
    if (!((m_activeTypes[lod] >> type) & 1))
    {
        Debug(DebugTypeError, "AI/AIDLL/AI/AIPlanInterpreter/BAPInterpreter.cpp", 154, "")
            << bot->getName() << " " << bot->getBotId() << " lod level " << static_cast<int>(lod)
            << " cannot interpret \n" << " (" << static_cast<int>(type) << ") "
            << static_cast<int>(type) << ".";
        done = true;
        ok = false;
        return false;
    }
    BAPEntry* const entry = m_entries[type][lod];
    if (!entry)
        return script->execute(bot, ok, done);
    return entry->execute(script, bot, ok, done);
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
