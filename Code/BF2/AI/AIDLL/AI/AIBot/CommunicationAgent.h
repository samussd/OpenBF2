// Per-bot radio message queue and timing state.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_COMMUNICATIONAGENT_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_COMMUNICATIONAGENT_H

#include <list>
#include <map>
#include <queue>
#include <vector>

namespace dice {
namespace hfe {
namespace ai {

class BotMain;
class CommunicationHub;
class Information;
struct AIRadioMessageSettings;

// Both opaque, and declared this way elsewhere in the tree too - see BFConsole.h for why no
// enumerator name is recoverable from either binary.
enum AIRadioChannelType : int;
enum AIRadioMessageType : int;

// Radio-message record. [UNRESOLVED] Some fields are not yet identified.
struct AIRadioMessageInfo
{
    // 0x1d3260 (Ghidra). The copy constructor (0x1d32e0) copies field by field.
    AIRadioMessageInfo()
        : m_float0(1.0f), m_senderPlayerId(-1), m_double8(0.0), m_bool16(false), m_bool17(false),
          m_type(18)
    {
    }

    /*  0 */ float  m_float0;              // [UNRESOLVED] = 1.0f
    /*  4 */ int    m_senderPlayerId;      // = -1
    /*  8 */ double m_double8;             // [UNRESOLVED] = 0.0
    /* 16 */ bool   m_bool16;              // [UNRESOLVED] = false
    /* 17 */ bool   m_bool17;              // [UNRESOLVED] = false
    /* 20 */ int    m_type;                // [INFERRED] = 18, one past the last message type
};

class CommunicationAgent
{
public:
    // Queued priority message. The integer at +8 determines ordering.
    struct CAPrioMessage
    {
        // The whole of the ordering. A `std::priority_queue` with `std::less` is a MAX-heap, so
        // the largest value here is what the bot says next.
        bool operator<(const CAPrioMessage& rhs) const { return m_priority < rhs.m_priority; }

        /*  0 */ AIRadioMessageType m_type;
        /*  4 */ AIRadioChannelType m_channel;
        /*  8 */ unsigned int m_priority;
        /* 12 */ int m_int12;                   // [UNRESOLVED] both known callers pass -1
        /* 16 */ double m_time;                 // when it may be said: enqueue time + delay
        /* 24 */ float m_delay;
    };

    explicit CommunicationAgent(BotMain* owner);
    // The implicit destructor releases member containers; no custom cleanup is required.

    void reset();
    void update();
    void enqueueRadioMessage(AIRadioMessageType type, AIRadioChannelType channel,
                             unsigned int priority, float delay, int value);

    // attested by a 7-byte body: `return m_receivedMessages.data + type;`
    const std::list<AIRadioMessageInfo>& getReceivedMessages(AIRadioMessageType type) const
    { return m_receivedMessages[type]; }
    // [INFERRED] accessor; BBAvoid::calculateUrgency inlines the load.
    double getHonkedAtTime() const { return m_honkedAtTime; }
    // The first received message of the type from the player, copied out; false for none.
    bool getReceivedMessage(AIRadioMessageInfo& out, AIRadioMessageType type, int playerId) const;
    // Whether a message of the type was received from the player (from anyone for -1).
    bool receivedMessageExists(AIRadioMessageType type, int playerId);
    // Marks the sender's unanswered messages of the type as answered; true if there were any.
    bool confirmReceivedMessage(AIRadioMessageType type, int playerId);
    // The bot saw an enemy it was not in contact with (BotSenses::event_seenObject).
    void setSeenNewEnemy(Information* information);
    // Files a message from a player. Types 0 and 1 cancel each other's earlier messages.
    void receiveRadioMessage(const AIRadioMessageSettings& settings, int senderId, bool self);

private:
    void clearOutOldMessages(double time);
    void pollForRadioMessage(double time);
    void sendCommunicationAgentMessages(double time);
    void sendCallForMedic(Information* information);
    void shutOffHorn(Information* information);
    bool peekRadioMessage(CAPrioMessage& message);
    bool dequeueRadioMessage(CAPrioMessage& message);
    bool waitWithSendingMessage(const CAPrioMessage& message, double time);
    void postEvent(const CAPrioMessage& message);

    typedef std::priority_queue<CAPrioMessage, std::vector<CAPrioMessage>,
                                std::less<CAPrioMessage> > MessageQueue;

    /*   0 */ BotMain* m_bot;                  // the ctor argument, and the first store
    // the constructor never sets this - `reset` does, and the constructor's last statement is a
    // call to `reset`. Same shape as `CommunicationHub`'s own two uninitialised timestamps.
    /*   4 */ CommunicationHub* m_hub;         // = CommunicationHub::m_instance
    // one inbox per message type - eighteen lists, indexed by the enum. See the header block.
    /*   8 */ std::vector<std::list<AIRadioMessageInfo> > m_receivedMessages;
    // The types already queued, with the time each may be said.
    /*  20 */ std::map<AIRadioMessageType, double> m_queuedTypes;
    // what the bot says next. See the header block.
    /*  32 */ MessageQueue m_queue;            // +32..+47: a 12-byte vector and an empty comparator
    // Eighteen doubles, one per type, zeroed by `reset` and paired with the max-send-period cache
    // below. [INFERRED] that these are the per-type last-sent timestamps.
    /*  48 */ std::vector<double> m_lastSentTime;
    /*  60 */ double m_double60;               // reset: 0.0; the last poll
    // the two cached columns of `AIRadioMessages.ai`. Filled once, in the constructor.
    /*  68 */ std::vector<float> m_receiveTimeToLive;
    /*  80 */ std::vector<float> m_maxSendPeriod;
    /*  92 */ int m_int92;                     // reset: -1, this subsystem's null-index convention
    // honkHorn writes both: the time the bot's own soldier was honked at, and the time it last
    // honked. [INFERRED] names.
    /*  96 */ double m_honkedAtTime;           // reset: 0.0
    /* 104 */ double m_lastHonkTime;           // reset: 0.0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_COMMUNICATIONAGENT_H
