// Strategic area with explicit bounds or a control-point anchor.
// Control-point areas defer initialization until world objects exist.
// The base index is used to select the area's debug primitives.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICAREA_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICAREA_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "AIStrategicObject.h"

namespace dice {
namespace hfe {
namespace ai {

class AILandingZone;

class AIStrategicArea : public AIStrategicObject
{
public:
    // Per-map order-position offset and polygon state. Construction leaves the polygon field unset.
    class MapData
    {
    public:
        MapData()
        {
            // the island ID is left indeterminate on purpose. The zeroing loop steps four words
            // and writes three of them. Do not "fix" it.
            m_offset.x = 0.0f;
            m_offset.y = 0.0f;
            m_offset.z = 0.0f;
        }

        /*  +0 */ Vec3 m_offset;        // absolute until `initOrderPoses`, a delta afterwards
        /* +12 */ unsigned int m_island;
    };

    // One object at one distance, with a bit per vehicle group that reaches it there.
    struct AreaDistanceInfo { int m_groupMask; AIStrategicObject* m_object; float m_distance; };

    // [UNRESOLVED] List payloads whose original element types have not been identified.
    struct AreaWord   { int m_word; };
    struct AreaRecord { int m_words[6]; };

    AIStrategicArea(const std::string& name, unsigned int controlPointId, float radius);

    // [INFERRED] Initializes area state. Manager-created areas call this during initAreas;
    // control-point areas defer it until postLoadInit.
    void init();

    // Control-point areas defer shared construction until post-load position resolution.
    void internalConstructorCall(const Vec2& topLeft, const Vec2& centre);
    void postLoadInit();
    void initOrderPoses();

    // Writes the control-point position and radius; returns false for an unassigned id.
    bool getControlPointPosAndRadius(Vec3& position, float& radius);

    // Virtual declaration order: thirty-four ELF slots.
    virtual ~AIStrategicArea();                                    // slots 0/1,

    virtual int getType() const;                                   // slot 2,
    // slots 3 and 4 are AIStrategicObject::getPosition / ::setPosition - not overridden

    virtual Vec3 getOrderPos(int a, int b) const;                  // slot 5,
    // Weights both temperatures of the side by the strategy's modifiers for each type flag this
    // area has, for its status and front or safe position, and for this area. Returns the
    // temperature; the caller drops it. The status must be Owned, Hostile or Neutral.
    virtual float updateTemperature(const Strategy* strategy, int side);  // slot 6,
    virtual void update(int side, ISAI* sai, bool& changed);        // slot 7
    virtual int  getChangedAttackDefence(int side);                // slot 8,
    virtual float getControlPointTakeOverCounter();               // slot 9
    virtual bool isEnemyTakingOverBase(int side);                  // slot 10,
    // 0 unless owned; else the temperature plus the enemy strength, scaled down by the need
    // curve, and by 1.5 next to a hostile area or 1.25 next to a neutral one.
    virtual float calculateDefenseNeed(int side);                  // slot 11,
    virtual int  calculateAttackValue(int side);                   // slot 12,
    virtual int  calculateAttackCost(int side);                    // slot 13,
    virtual bool isNeighbour(AIStrategicObject* object) const;     // slot 14,
    virtual bool hasNeighbourWithStatus(StatusMap::Status status,
                                        int side) const;           // slot 15,
    virtual bool isNeighbouringObject(AIStrategicObject* object) const;   // slot 16,
    virtual bool hasNeighbouringObjectWithStatus(StatusMap::Status status,
                                                 int side) const;  // slot 17,
    virtual int  getObjectTypeFlags() const;                       // slot 18,
    // Tests a StrategicObjectType bit position, not a mask.
    virtual bool hasObjectTypeFlag(StrategicObjectType type) const;       // slot 19,
    virtual bool hasObjectTypeFlags(int flags) const;              // slot 20,
    virtual int  getTacticalStatus(int side) const;                // slot 21,
    virtual bool isAreaTakeable();                                 // slot 22,
    virtual int  collectSquadResources(int side, ISAI* sai);       // slot 23,
    // Orders up to count of the closest available squads here: to defend when the area is
    // owned, or neutral while the enemy is taking it, else to attack. Returns how many came.
    virtual int  collectAdditionalSquadResources(int count, int side, ISAI* sai);
                                                                   // slot 24,
    virtual int  releaseSurplus(int side);                         // slot 25,
    virtual float gravity(const Vec3& position, int side);         // slot 26,
    virtual float gravity(const Vec2& position, int side);         // slot 27,
    virtual bool retainBot(int side, int a, ISAI* sai);            // slot 28,
    virtual float getXExtent(int side) const;                      // slot 29,
    virtual float getZExtent(int side) const;                      // slot 30,
    virtual void setAsDirty(int side);                             // slot 31,
    virtual int  orderBot(int side, const std::list<AIStrategicObject*>& objects,
                          int a, ISAI* sai);                       // slot 32,

    // Additional virtual overload declared after the inherited slots.
    virtual float gravity(float distance, int side);               // slot 33,

    std::vector<AILandingZone*>& getLandingZones();
    const std::vector<AILandingZone*>& getLandingZones() const;
    // Rebuild the side's distances to every reachable object when they are stale: one search
    // per vehicle group, then the groups merged into m_distances.
    void validateDistances(int side);
    void validateDistances(int side, int group);
    // Uniform-cost search over the neighbour graph from this area, through objects that allow a
    // group in the mask. Neutral steps cost 1.1 times their length and hostile ones 5 times.
    // Fills each object's best distance and route (the object first, this area last), and the
    // (distance, object) pairs sorted.
    void getDistances(int side, std::list<std::pair<float, AIStrategicObject*> >& sorted,
                      std::map<AIStrategicObject*, std::list<AIStrategicObject*> >& routes,
                      std::map<AIStrategicObject*, float>& distances, int groupMask);
    // Merges the group's sorted distances into m_distances; an equal distance only adds the
    // group's bit to the entry already there, whatever its object.
    void insertUnitDistances(int side, int group);
    // Bit test on m_landingZoneUsers. Callers pass an equipment type or a vehicle number.
    // Not virtual.
    bool isLandingZoneUser(int index);

// [FRONTIER] Additional area operations remain undeclared until their signatures are established.

    // layout, ELF offsets, read store by store out of the constructor
    // The base occupies 0..267. MSVC's own offsets will differ - a libstdc++ `std::list` is four
    // bytes and a `std::string` is four. These are documentation, not a layout claim for this build.
    /* +268 */ std::list<AreaWord> m_list268;      // [UNRESOLVED] - landing zones? node 12 B
    /* +272 */ Vec2 m_centre;                      // zeroed; getXExtent/getZExtent subtract it
    /* +280 */ MapData* m_mapData;                 // new MapData[IAIPathfinding::getNumberOfMaps]
    // Separate vectors for neighboring areas and other neighboring strategic objects.
    /* +284 */ std::vector<AIStrategicObject*> m_neighbours;
    /* +296 */ std::vector<AIStrategicObject*> m_neighbouringObjects;
    /* +308 */ std::string m_name;                 // the area name the script gave
    /* +312 */ int m_objectTypeFlags;              // 0; `getObjectTypeFlags` returns exactly this
    /* +316 */ unsigned int m_controlPointId;      // the second script argument, e.g. 502
    // [UNRESOLVED] Both fields are initialized to 25; their meanings are unknown.
    /* +320 */ int m_i320;                         // = 25
    /* +324 */ int m_i324;                         // = 25
    // Landing zones associated with this area.
    /* +328 */ std::vector<AILandingZone*> m_landingZones;
    /* +340 */ int m_landingZoneUsers;             // 0, none
// Per-vehicle-group arrays; each allocation has at least one element.
// [INFERRED] Container element types remain partly uncertain. Side-indexed flags use side - 1.
    /* +344 */ bool m_distancesDirty[2];           // both = true [INFERRED] name
    /* +346 */ char m_pad346[2];
    /* +348 */ bool* m_groupsDirty[2];             // per side, new bool[groups], all true [INFERRED] name
    /* +356 */ bool m_sideEnabled[2];             // both true; [INFERRED] name, update side gates
    /* +358 */ bool m_b358;                        // false [UNRESOLVED] (the constructor's)
    /* +359 */ char m_pad359[1];
    // getDistances' outputs, [side - 1][group].
    /* +360 */ std::list<std::pair<float, AIStrategicObject*> >* m_sortedDistances[2];
    /* +368 */ std::map<AIStrategicObject*, std::list<AIStrategicObject*> >* m_routes[2];
    /* +376 */ std::map<AIStrategicObject*, float>* m_groupDistances[2];
    // Per-side distance records, indexed by side - 1 like m_distancesDirty.
    /* +384 */ std::list<AreaDistanceInfo> m_distances[2];
    /* +392 */ std::list<AreaRecord> m_list392;    // node
    // Stored doubled extent, initialized to twice m_size[1].
    /* +396 */ float m_f396;                       // = 2 * m_size[1] [UNRESOLVED] meaning
    /* +400 */ int m_i400;                         // 1 [UNRESOLVED]
    /* +404 */ bool m_b404;                        // true
    /* +405 */ bool m_statusChanged;               // false; update stores its changed output here
    // Additional state initialized by internalConstructorCall.
    /* +406 */ bool m_enemyTakeover[2];            // both false; [INFERRED] name, update's side flags
    /* +408 */ float m_f408;                       // = 1.0f [UNRESOLVED]
    /* +412 */ float m_f412;                       // = 1.0f [UNRESOLVED]
    /* +416 */ int m_i416;                         // 0 [UNRESOLVED]
    /* +420 */ int m_i420;                         // 0 [UNRESOLVED]
    /* +424 */ int m_i424;                         // 0 [UNRESOLVED]
    // Selects control-point initialization; explicit-position construction sets it false
    // and uses control point id -1.
    /* +428 */ bool m_fromControlPoint;            // true
    // Raised by `initOrderPoses` and by nothing else; both constructors clear it. [INFERRED]
    /* +429 */ bool m_orderPosesInitialised;       // false
    /* +430 */ char m_pad430[2];                   // tail padding to the observed 432
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICAREA_H
