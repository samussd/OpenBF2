// One behaviour a bot can choose: its per-bot urgency and the pattern that plans it.
// [FRONTIER] Plan generation remains a named stop.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BOTBEHAVIOUR_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BOTBEHAVIOUR_H

#include "UrgencyMerger.h"                   // UrgencyGenerator
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BotScript;
class Information;
class PlanGenerator;
class UrgeCurve;

class BotBehaviour : public UrgencyGenerator
{
public:
    // Allocates the per-bot urgency arrays. The urgencies themselves are left for the
    // subclass to set.
    BotBehaviour(float* weights, UrgeCurve* urgeCurve, int numberOfBots);
    virtual ~BotBehaviour();                                         // deletes the generator

    virtual float getUrgency(Bot* bot);                              // +0x08
    virtual float getActiveUrgency(Bot* bot);                        // +0x0C
    virtual void  updateActiveUrgency(Bot* bot);                     // +0x10
    virtual bool  hasChangedTarget(Bot* bot);                        // +0x14
    virtual bool  quotientChanged(Bot* bot, float low, float high);  // +0x18
    virtual bool  hasChanged(Bot* bot, float low, float high);       // +0x1C
    virtual UrgencyGenerator* getChosenGenerator();                  // +0x20
    virtual void  reset(Bot* bot);                                   // +0x24
    virtual float calculateUrgency(Bot* bot, bool flag, float value) = 0;   // +0x28
    virtual BotScript* generatePlan(Bot* bot);                       // +0x2C
    virtual bool  isAplicable(Information* information, Bot* bot);   // +0x30 (dice's spelling)

    PlanGenerator* getGenerator() const { return m_generator; }

protected:
    // [CHOSEN] helper: every behaviour constructor asks the AI settings for the bot count.
    static int getNumberOfBots();

    /* +0x04 */ unsigned char m_behaviourId;
    /* +0x05 */ unsigned char m_mode;                // the script's number after the pattern name
    // [INFERRED] per-bot target handles, -1 for none; the subclasses decide whether it exists.
    /* +0x08 */ int* m_targets;
    /* +0x0C */ PlanGenerator* m_generator;          // owned
    /* +0x10 */ float* m_urgency;                    // per bot
    /* +0x14 */ float* m_weights;                    // the script's weight set, not owned
    /* +0x18 */ bool* m_changedTarget;               // per bot
    /* +0x1C */ float* m_activeUrgency;              // per bot
    /* +0x20 */ UrgeCurve* m_urgeCurve;              // not owned
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BOTBEHAVIOUR_H
