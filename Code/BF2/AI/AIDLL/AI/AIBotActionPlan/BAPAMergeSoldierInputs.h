// Merges what the bot's plans asked of the soldier's view and movement into one set of player
// inputs. The last statement of every sensing plan the soldier pattern builds.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMERGESOLDIERINPUTS_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMERGESOLDIERINPUTS_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAMergeSoldierInputs : public BAPAction
{
public:
    explicit BAPAMergeSoldierInputs(int planId);
    virtual ~BAPAMergeSoldierInputs();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

    // the mouse controller's yaw and pitch errors from the last tick (radians)
    /* +0x14 */ float m_float20;          // ctor: 0
    /* +0x18 */ float m_float24;          // ctor: 0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAMERGESOLDIERINPUTS_H
