// Armament a side remembers. updateWeapons only marks the list stale; the next reader rebuilds it.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENTKNOWN_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENTKNOWN_H

#include "IPIArmament.h"

namespace dice {
namespace hfe {
namespace ai {

class IPIArmamentKnown : public IPIArmament
{
public:
    explicit IPIArmamentKnown(Information* information);
    virtual ~IPIArmamentKnown();

    virtual void setTime(double time);                                  // +4
    virtual int  getActiveWeapon() const;                               // +5
    virtual unsigned int getWeapon(int index);                          // +7
    virtual std::vector<unsigned int>& getWeapons();                    // +8
    virtual void updateWeapons();                                       // +12
    virtual void updateWeaponsInternal();                               // +16

private:
    /* +0x18 */ bool m_stale;
    // [CHOSEN, not recovered] Retail reads this but no body writes it; -1 is "no weapon".
    /* +0x1C */ int  m_activeWeapon;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENTKNOWN_H
