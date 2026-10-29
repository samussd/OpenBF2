// Per-bot behavior urgency state: the behaviour the bot chose last and when.
// BotMain initializes m_behaviourTime after construction.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_BOTMEMORY_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_BOTMEMORY_H

namespace dice {
namespace hfe {
namespace ai {

class BotBehaviour;

class BotMemory
{
public:
    BotMemory();
    ~BotMemory();

    void reset();                   // [FRONTIER]
    // The factor on a behaviour's urgency at the game time: its urge curve over the time since
    // it was chosen when it is the current behaviour, else 1.
    float modifyBehaviourUrgency(BotBehaviour* behaviour, double time);

    // Retail x86 field offsets.
    // the id of the behaviour BotMain::decisionMaking chose last, -1 for none.
    /*  0 */ int    m_currentBehaviour; // = -1 and BotMain writes it again from outside
    /*  4 */ int    m_int4;         // = -1
    /*  8 */ float  m_float8;       // = 1.0f
    /* 12 */ int    m_int12;        // = 1
    // the game time of that choice. Initialized by BotMain after construction.
    /* 16 */ double m_behaviourTime;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTMEMORY_H
