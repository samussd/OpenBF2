#include "BAPAResetControls.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BAPAResetControls::BAPAResetControls(bool movement, bool look, bool actions, int planId)
    : BAPAction(planId)
    , m_movement(movement)
    , m_look(look)
    , m_actions(actions)
    , m_input(0)
{
}

BAPAResetControls::~BAPAResetControls()
{
}

bool BAPAResetControls::execute(Bot*, bool& ok, bool& done)
{
    ok = true;
    done = true;
    return true;
}

unsigned long long BAPAResetControls::getControls()
{
    return m_controls;
}

BAPStatementType BAPAResetControls::getId() const
{
    return BAPSTResetControls;
}

bool BAPAResetControls::hasDefault()
{
    return false;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPAResetControls::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAResetControls::streamSelf");
}

bool BAPAResetControls::hasSingleInput()
{
    return m_controls != 0;
}

int BAPAResetControls::getInput()
{
    return m_input;
}

} // namespace ai
} // namespace hfe
} // namespace dice
