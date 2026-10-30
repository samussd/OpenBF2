// Per-bot sight, hearing and communication state. Owns a view frustum and sensed-object records.

#ifndef AI_AIDLL_AI_AIBOT_BOTSENSES_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_BOTSENSES_H

#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "../../../../Math/Frustum.h"

namespace dice {
namespace hfe {
namespace ai {

class BotMain;
// The two things a bot's perception owns and deletes (SensedObject.h, FireObject.h). `reset`
// still stops where it would delete them: the Information release around each delete is not
// read yet. See the reset block in the .cpp.
class FireObject;
class IAIObject;
class Information;
class SensedObject;

class BotSenses
{
public:
    // (`operator new(0x94)` in SensingAgent's constructor).
    explicit BotSenses(BotMain* owner);
    ~BotSenses();
    void reset();                            // - (two arms frontier)
    // Sight, in phases over the ticks (see the .cpp).
    void see(const double& budget);
    // Hearing, the shots on record and sight, within the tick's budget.
    void updateSensing(const double& dt);
    // The object was seen, by the bot itself or by a bot riding with it.
    void event_seenObject(unsigned int handle, const Vec3& localPoint, unsigned int part,
                          Information* information, bool self, bool clear);
    // A shot the bot noticed, with its shooter.
    void event_projectile(FireObject* fire, unsigned int handle);
    // A sound from the object reached the bot (BotManager::actionHearing, through BotMain).
    void event_heardObject(unsigned int handle);

    // attested: an 11-byte body that returns `this + 112`.
    const std::map<unsigned int, SensedObject*>& getSensedObjects() const { return m_sensedObjects; }
    SensedObject* getSensedObjectFromHandle(unsigned int handle) const;
    // Whether a shot from the object is on record, with its time (-1000 when there is none).
    bool isAttacking(unsigned int handle, double& time) const;
    // [INFERRED] name; SensingAgent::evaluateFireObjects inlines the +84 load.
    const std::list<std::pair<FireObject*, unsigned int> >& getFireObjects() const { return m_list84; }

private:
    // How loud the object is to the bot, 0 when it is not heard.
    float hear(unsigned int handle, Information* information);
    // Goes over the heard objects, oldest first, for as long as the budget allows.
    void updateHearing(const double& budget);
    void updateSeeing(const double& budget);
    void updateFireObjects(const double& budget);
    bool isCloseAndInFront(Information* equipment, IAIObject* object);
    // [CHOSEN] helpers for blocks the binary inlines.
    void setThirdIgnored(std::vector<IAIObject*>& ignore, Information* target, int side);
    void writeThirdIgnored(std::vector<IAIObject*>& ignore, Information* target, int side);
    void tellRiders(Information* equipment, unsigned int handle, const Vec3& local, unsigned int part,
                    Information* target, bool clear);
    void tellOtherRiders(Information* equipment, SensedObject* so, unsigned int part, Information* target,
                         bool clear, const Vec3& eye, const Vec3& local, bool visible);
    void testPose(Information* equipment, SensedObject* so, const Vec3& eye, const Vec3& point,
                  const std::vector<IAIObject*>& ignore, bool lod, float radius, bool checkLying);

    typedef std::pair<double, SensedObject*>      TimedObject;   // a time and a thing
    typedef std::list<TimedObject>                TimedList;
    typedef std::pair<FireObject*, unsigned int>  FireEntry;
    typedef std::map<unsigned int, SensedObject*> SensedObjectMap;

    /*   0 */ BotMain* m_bot;                // the ctor argument, and the first store
    /*   4 */ Frustum* m_frustum;            // new(0x1E8) - . See the header block.
    // see: 1 when the equipment flies (0x400000), choosing the frustum's width.
    /*   8 */ int m_frustumGroup;
    /*  12 */ double m_gatherTime;             // = -1000000.0; see: when the candidates were gathered
    // the distance sort buffer - what `InfoDistPred` and `partial_sort` operate on. Held as a
    // member rather than a local, so the seeing path does not reallocate every tick.
    /*  20 */ std::vector<std::pair<float, unsigned int> > m_sortBuffer;
    /*  32 */ int m_frustumStep;                   // ctor: 0, reset: 0
    /*  36 */ int m_phase;                   // ctor: 0, reset: 0
    /*  40 */ int m_sortIndex;                   // ctor: 0, reset: 0
    // handle -> position in the list at +56. See the header block.
    /*  44 */ std::map<unsigned int, TimedList::iterator> m_seenNodes;
    /*  56 */ TimedList m_seen;
    // A cursor into the list above, seeded to end by the constructor's last statement.
    /*  60 */ TimedList::iterator m_seenCursor;
    // The objects the bot hears, and the same in the order updateHearing goes over them.
    /*  64 */ std::set<unsigned int> m_heardHandles;
    /*  76 */ TimedList m_heard;
    // A cursor into the list above - and unlike +60 the constructor leaves it default-constructed.
    // Only `reset` makes it valid. See the header block.
    /*  80 */ TimedList::iterator m_cursor80;
    /*  84 */ std::list<FireEntry> m_list84;
    // updateFireObjects: the shots' strengths, over all battle strengths and against the bot, and
    // that of the shooters still attacking it.
    /*  88 */ float m_firedStrength;         // ctor: 0, reset: 0
    /*  92 */ float m_firedStrengthOwn;      // ctor: 0, reset: 0
    /*  96 */ float m_attackStrength;
    /* 100 */ std::map<unsigned int, FireObject*> m_map100;
    /* 112 */ SensedObjectMap m_sensedObjects;
    // the only two containers `reset` does not empty. Element width 4 and trivially destructible
    // (the destructor deallocates without a per-element loop); the element type is [INFERRED].
    /* 124 */ std::vector<IAIObject*> m_candidates;   // see: the objects gathered in phase 0
    /* 136 */ std::vector<unsigned int> m_vector136;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTSENSES_H
