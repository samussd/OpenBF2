// Builds a bot's plan for one behaviour. Every BBP pattern is one.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_PLANGENERATOR_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_PLANGENERATOR_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class Bot;
class BotScript;

class PlanGenerator
{
public:
    virtual ~PlanGenerator() {}
    virtual BotScript* createPlan(Bot* bot) = 0;                     // +0x08
    virtual bool isInitiated(Bot* bot) = 0;                          // +0x0C
    virtual void unInit(Bot* bot) = 0;                               // +0x10
    virtual void forceInitiated(Bot* bot) = 0;                       // +0x14
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_PLANGENERATOR_H
