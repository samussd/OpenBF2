// Base interface and shared state for strategic objects. Preserve virtual declaration order.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECT_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECT_H

#include <list>
#include <set>

// Vec2 and Vec3 cannot be forward-declared in this tree: both are typedefs to
// `dice::hfe::Math::Vec2/Vec3`, so a local `struct Vec3;` is a redefinition with a different basic
// type (C2371). The header has to come in.
#include "../../../../Math/Vec3.h"
#include "../AIStrategic/StatusMap.h"

namespace dice {
namespace hfe {
namespace ai {

class EnemyStatistics;
class ISAI;
class Strategy;

class AIStrategicObject
{
public:
    // Strategic object classifications used by console scripts. Unknown values have no printed name.
    enum StrategicObjectType
    {
        Flank          = 1,
        Centre         = 2,
        Base           = 3,
        Close          = 4,
        Remote         = 5,
        AirSpawner     = 6,
        LandSpawner    = 7,
        NavalSpawner   = 8,
        SoldierSpawner = 9,
        StrongPoint    = 10,
        ChokePoint     = 11,
        Bridge         = 12,
        AirField       = 13,
        SupplyPoint    = 14,
        Route          = 15,
        ControlPoint   = 16,
        North          = 17,
        West           = 18,
        South          = 19,
        East           = 20,
        // 21 has no name in the writer
        Front          = 22,
        Safe           = 23,
        Enemy          = 24,
        // 25 has no name in the writer
        NeutralType    = 26     // spelled "Neutral" by the writer; renamed here only because
                                // `StatusMap::Status::Neutral` would collide at this scope
    };

    AIStrategicObject(float radius, float f148);

    // the vtable, , thirty-three slots
    virtual ~AIStrategicObject();                                      // slots 0/1,

    virtual int getType() const;                                       // slot 2,
    virtual const Vec2& getPosition(int side) const;                   // slot 3,
    virtual void setPosition(int side, const Vec2& position);          // slot 4,

    // Slots 5..17, 21..25, 28, 31 and 32 are `__cxa_pure_virtual` in this class's table. Every name
    // and parameter list below is taken from the matching slot of `AIStrategicArea`'s vtable; the
    // return types are [INFERRED] from whether the override leaves its result in eax or in st0.
    virtual Vec3 getOrderPos(int a, int b) const = 0;                  // slot 5
    virtual float updateTemperature(const Strategy* strategy, int side) = 0;              // slot 6
    virtual void update(int side, ISAI* sai, bool& changed) = 0;                          // slot 7; paired Area body has no return
    virtual int  getChangedAttackDefence(int side) = 0;                                   // slot 8
    virtual float getControlPointTakeOverCounter() = 0;                                  // slot 9; ST0 float chain
    virtual bool isEnemyTakingOverBase(int side) = 0;                                     // slot 10
    virtual float calculateDefenseNeed(int side) = 0;                                     // slot 11
    virtual int  calculateAttackValue(int side) = 0;                                      // slot 12
    virtual int  calculateAttackCost(int side) = 0;                                       // slot 13
    virtual bool isNeighbour(AIStrategicObject* object) const = 0;                        // slot 14
    virtual bool hasNeighbourWithStatus(StatusMap::Status status, int side) const = 0;    // slot 15
    virtual bool isNeighbouringObject(AIStrategicObject* object) const = 0;               // slot 16
    virtual bool hasNeighbouringObjectWithStatus(StatusMap::Status status,
                                                 int side) const = 0;                     // slot 17

    virtual int  getObjectTypeFlags() const;                           // slot 18,
    virtual bool hasObjectTypeFlag(StrategicObjectType type) const;    // slot 19,
    virtual bool hasObjectTypeFlags(int flags) const;                  // slot 20,

    virtual int  getTacticalStatus(int side) const = 0;                                   // slot 21
    virtual bool isAreaTakeable() = 0;                                                    // slot 22
    virtual int  collectSquadResources(int side, ISAI* sai) = 0;                          // slot 23
    virtual int  collectAdditionalSquadResources(int count, int side, ISAI* sai) = 0;     // slot 24
    virtual int  releaseSurplus(int side) = 0;                                            // slot 25

    virtual float gravity(const Vec3& position, int side);             // slot 26,
    virtual float gravity(const Vec2& position, int side);             // slot 27,

    virtual bool retainBot(int side, int a, ISAI* sai) = 0;                               // slot 28

    virtual float getXExtent(int side) const;                          // slot 29,
    virtual float getZExtent(int side) const;                          // slot 30,

    virtual void setAsDirty(int side) = 0;                                                // slot 31
    virtual int  orderBot(int side, const std::list<AIStrategicObject*>& objects,
                          int a, ISAI* sai) = 0;                                          // slot 32

    int getNumberOfAssignedUnits(int side) const;
    bool isAttack(int side) const;
    void setAttack(bool attack, int side);
    // Assigned squad strength over the resource need, divided as integers; 1 without a need.
    float getAssignedPartOfWantedStrength(int side) const;
    // Whether any vehicle group in the mask is allowed here.
    bool allowsSomeVehicleGroups(int groupMask) const;
    // Borrowed mutable statistics at m_statistics[side - 1]; reference spelling is [INFERRED].
    EnemyStatistics& getBattleStatistics(int side) const;
    int releaseAssignedResources(int side);
    // Bot ids free to attack or to defend this object, by side (1 or 2). Unchecked side.
    std::set<int>& getAvailableAttackers(int side);
    std::set<int>& getAvailableDefenders(int side);
    // Each returns true.
    bool registerAvailableAttacker(int botId, int side);
    bool registerAvailableDefender(int botId, int side);
    // False when the bot was not listed. The iterator forms erase it and advance the cursor.
    bool unregisterAvailableAttacker(int botId, int side);
    bool unregisterAvailableAttacker(std::set<int>::iterator& it, int side);
    bool unregisterAvailableDefender(int botId, int side);
    bool unregisterAvailableDefender(std::set<int>::iterator& it, int side);
    // Bot ids assigned to this object, and bot ids standing in it, by side.
    std::set<int>& getAssignedUnits(int side);
    std::set<int>& getPresentUnits(int side);
    bool isPresentUnit(int botId, int side) const;
    std::pair<std::set<int>::iterator, bool> setAsPresentUnit(int botId, int side);
    // Erases the bot and advances the cursor.
    void removeAsPresentUnit(std::set<int>::iterator& it, int side);
    // Bit tests on the vehicle groups allowed here and the equipment types sent away.
    bool isAllowedVehicleGroup(int group) const;
    bool isExpelledUnit(int equipmentType) const;

    // [INFERRED] accessor names/forms. SAI::updateStates and the area's neighbour scans inline
    // these loads: status is at +0x70 + 4*side and index at +0xF0 in both shipped x86 ports.
    StatusMap::Status getStatus(int side) const { return m_status[side - 1]; }
    void resetStatus();      // both sides back to the statuses in m_status[2] and [3]
    int getIndex() const { return m_index; }

    // [CHOSEN, not recovered] Public fields allow manager access; the original may have
    // used friendship instead. Offsets document ELF layout and do not constrain modern STL sizes.
    /*   +4 */ EnemyStatistics* m_statistics[4];   // new EnemyStatistics(6), (6), (2), (2) in order
    // Per-side sets of bot ids, which also index SAI's BotInfo table.
    /*  +20 */ std::set<int> m_set20[2]; // getAvailableAttackers
    /*  +44 */ std::set<int> m_set44[2]; // getAvailableDefenders
    /*  +68 */ std::set<int> m_set68[2]; // getAssignedUnits
    /*  +92 */ std::set<int> m_set92[2]; // getPresentUnits

    // All four set to 2. `StatusMap::Status(2)` is `Neutral`, which is the only reason this member
    // carries that name - [INFERRED], on the strength of one coincidence.
    /* +116 */ StatusMap::Status m_status[4];

    // one-based: `getPosition(side)` is `m_position[side - 1]`. See the header note.
    /* +132 */ Vec2 m_position[2];

    /* +148 */ float m_f148;        // the constructor's second argument; -666.0f from every caller
    /* +152 */ float m_size[3];     // [0] = the radius argument, [1] and [2] zeroed; read by side
    // The side's temperature at [side - 1] and the same without its units at [side + 1]. All four
    // start at the radius argument; Area::update rebuilds them and updateTemperature weights them.
    /* +164 */ float m_f164[4];
    // Paired Area::update reads/writes these as floats, not the formerly guessed ints.
    // [INFERRED] names; one-based side pairs, all zeroed by the base constructor.
    /* +180 */ float m_needCurveInput[2];
    /* +188 */ float m_friendlyRatio[2];
    /* +196 */ float m_friendlyBattleStrength[2];
    /* +204 */ float m_enemyBattleStrength[2];
    /* +212 */ int m_unresolved212[2]; // zeroed; these two preceding counts remain unidentified
    /* +220 */ int m_resourceNeed[2];
    /* +228 */ int m_assignedSquadStrength[2];
    /* +236 */ bool  m_attack[2];   // both true; isAttack(side) reads [side - 1]
    /* +240 */ int   m_index;       // not initialised here - addStrategicObject assigns it
    /* +244 */ int   m_allowedVehicleGroups;   // bit per vehicle group; -1, every group
    /* +248 */ int   m_expelledUnits;          // bit per equipment type; 0, none
    /* +252 */ int   m_category;    // 1 until createFromControlPoint or create sets it
    /* +256 */ std::set<int> m_set256;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECT_H
