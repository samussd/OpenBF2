// Tracks, per bot, whether the pattern has been set up for that bot's current plan.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPPATTERN_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPPATTERN_H

#include "PlanGenerator.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPPattern : public PlanGenerator
{
public:
    BBPPattern();
    virtual ~BBPPattern();

    virtual bool isInitiated(Bot* bot);
    virtual void unInit(Bot* bot);
    virtual void forceInitiated(Bot* bot);

protected:
    // [CHOSEN] helper: every pattern constructor asks the AI settings for the bot count.
    static int getNumberOfBots();

    /* +0x04 */ bool* m_initiated;                  // per bot, false at construction
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPPATTERN_H
