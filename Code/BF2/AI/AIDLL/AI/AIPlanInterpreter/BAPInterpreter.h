// Runs plan scripts through a table of entries, one per statement type and LOD level.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_BAPINTERPRETER_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_BAPINTERPRETER_H

#include "PlanInterpreter.h"
#include "BAPStatementType.h"
#include "../AIBot/BotMain.h"            // BotLodLevel

namespace dice {
namespace hfe {
namespace ai {

class BAPEntry;

class BAPInterpreter : public PlanInterpreter
{
public:
    // AISettings builds one per vehicle with BAPSTNumberOfTypes statement types.
    explicit BAPInterpreter(int numberOfTypes);
    virtual ~BAPInterpreter();

    virtual bool execute(BotScript* script, Bot* bot, bool& flagA, bool& flagB);

    // An entry fills its statement type's free slots from lod down to the lightest level,
    // stopping at the first slot already taken. False when its type is out of range or the
    // first slot is taken; a lod past the last level does nothing and still returns true.
    bool addEntry(BAPEntry* entry);                       // at BotLodFull
    bool addEntry(BotLodLevel lod, BAPEntry* entry);
    bool removeEntry(BotLodLevel lod, BAPEntry* entry);
    // Marks a type active without an entry, under the same rules as addEntry.
    bool activateEntry(BAPStatementType type, BotLodLevel lod);

private:
    enum { kLodLevels = 3 };

    /*  +4 */ int m_numberOfTypes;
    /*  +8 */ unsigned long long* m_activeTypes;   // one bit per statement type, per LOD level
    /* +12 */ BAPEntry*** m_entries;               // [type][lod]; the entries are not owned
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_BAPINTERPRETER_H
