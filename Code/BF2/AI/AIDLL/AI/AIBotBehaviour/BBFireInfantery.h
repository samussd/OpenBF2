// Behaviour built by BBCreateFunctionFireInfantery. sizeof == 0x44.
// [FRONTIER] Generator set-up and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIREINFANTERY_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIREINFANTERY_H

#include "BBFire.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPFireInfantery;

class BBFireInfantery : public BBFire
{
public:
    BBFireInfantery(unsigned char behaviourId, unsigned char mode, BBPFireInfantery* generator,
                    UrgeCurve* urgeCurve, float* weights);
    virtual ~BBFireInfantery();

    virtual void initializeGenerator(BotMain* bot, int a, bool b, bool c);   // +0x34
    using BBFire::initializeGenerator;

protected:
    /* +0x40 */ BBPFireInfantery* m_fireInfantery;    // the same object as m_generator
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIREINFANTERY_H
