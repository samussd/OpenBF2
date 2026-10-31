// Per-bot navigation state and owned paths. The bot owner is borrowed.

#ifndef AI_AIDLL_AI_AIBOT_BOTNAVIGATION_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_BOTNAVIGATION_H

#include <set>
#include <vector>

#include "../Main/AIPathfindingSearchTypes.h"   // QTPath, ClusterNodePath - both already written
#include "../../../../Math/Vec3.h"        // Vec2 and Vec3 both live here, and both are typedefs
                                          // to dice::hfe::Math::* - never forward-declare either

namespace dice {
namespace hfe {
namespace ai {

class BotMain;
class IAIPathfinding;
class Information;
class QuadTree;

class BotNavigation
{
public:
    explicit BotNavigation(BotMain* owner);
    ~BotNavigation();

    bool update(bool& a, bool* b);
    void updateOccationally();                  // dice's spelling - "Occationally"
    // One step along the path: true with the point to head for. waiting: nothing to head for yet.
    bool updatePath(Vec2& point, bool& arrived, bool& waiting, float distance);
    void updateNavPoints();
    void updateObstacles();
    void updateLastPoint(Information* info);
    void updateSpaceLeftAndRight();
    void updateClosestDistance();

    bool initPathfinding(const Vec3& to, float a, float b);
    bool initPathfinding(unsigned int quadTree, unsigned int polygon, const Vec3& to, float a, float b);
    bool doPolygonSearch(bool& a, bool* b);
    bool doClusterSearch(bool& a, bool* b);
    void calculatePoints();
    void optimizePoints();
    void testDirectPath();
    void retrace();

    bool stillOnPath();
    bool hasIllegalPath();
    // [INFERRED] accessor name for the +192 byte, which retail reads inline (BotManager::
    // actionComAgent). initPathfinding and updateOccationally set it; clearProgress
    // checkProgress, doPolygonSearch and setIsStuck clear it; updatePath tests it.
    bool isPathSearchPending() const { return m_flag192; }
    // [INFERRED] accessor names for the +193 and + (PE +0xC9 / +0x148), both read inline
    // by BotManager::actionPathfinding (, ).
    // +193: set to 1 only on initPathfinding's search-started path , cleared by the
    // constructor, initPathfinding's other exits and reset. +316: set by initPathfinding
    // cleared by update and reset.
    bool isSearchActive() const { return m_flag193; }
    // [INFERRED] accessor; BAPAMoveToFinding::update inlines the +44 loads.
    const Vec3& getSearchTarget() const { return m_searchTarget; }
    bool isSearchNew() const { return m_flag316; }
    bool isActive();
    // returns true, unconditionally, in both images.
    bool isRunning();
    // [INFERRED] accessor; BotMain::postPlanExecution inlines the loads.
    float getSideSpace(int side) const { return m_sideSpace[side]; }
    // False once the bot has got nowhere for too long.
    bool checkProgress();
    void clearProgress();
    bool checkArrivedAtPoint(float distance);
    void setIsStuck(bool stuck);

    bool pointWithinObstacleRadius(const Vec3& point);
    void getFirstPointBeyondDist(const Vec2& from, float distance, Vec2& out);
    bool getSecondPoint(Vec2& out);
    bool getLargestAngleDiff(float& out);
    // [INFERRED] accessor for the +188 distance (BAPAMoveToFinding::getTargetDistance reads it inline).
    float getClosestDistance() const { return m_sentinel188; }
    // Where a trace on a map starts: the bot's last valid place there.
    void getTraceData(int map, unsigned int& polygon, unsigned int& island, Vec3& position);
    // [INFERRED] accessor; AIPathfinding::trace inlines the load.
    int getVehicleNumber() const { return m_vehicleNumber; }

    // A grenade the bot should know about: where, and the fuse time it had when it was set.
    // getGrenadeWarning gives the position and the time left of the 2.8 s since; true while some
    // is left.
    bool getGrenadeWarning(Vec3& position, double& timeLeft);
    void setGrenadeWarning(const Vec3& position, double time, bool flag);
    // [INFERRED] accessor; BBRandom::calculateUrgency inlines the load.
    bool getGrenadeFlag() const { return m_grenadeFlag; }
    // [INFERRED] accessor; BBAvoid::calculateUrgency inlines the load.
    const std::vector<unsigned int>& getObstacles() const { return m_obstacles; }

    void reset();                          // - (one arm frontier)
    void resetAll();

    // attested as a nested type by the container instantiations, which spell
    // `std::vector<dice::hfe::ai::BotNavigation::NavigationPoint>` in full. sizeof is 40, from
    // the constructor's divide; the members are [UNRESOLVED] because nothing recovered so far builds
    // one. Declared as a sized block rather than guessed at.
    struct NavigationPoint
    {
        // Initializes the index to -1 and clears the preceding 36 bytes.
        // [UNRESOLVED] The contents of that byte range remain unknown.
        NavigationPoint()
            : m_index36(-1)
        {
            m_point0.x = 0.0f;
            m_point0.y = 0.0f;
            m_point0.z = 0.0f;
            m_point12 = m_point0;
            m_position = m_point0;
        }

        // The edge crossed at this point, its two ends; the point itself lies on it.
        Vec3 m_point0;
        Vec3 m_point12;
        Vec3 m_position;        // +24; getFirstPointBeyondDist reads its x and z
        int  m_index36;         // = -1; the polygon (hasIllegalPath, stillOnPath)
    };

private:
    /*   0 */ BotMain* m_bot;              // the ctor argument, and the first store
    // initPathfinding's a and b, [UNRESOLVED] roles (a move passes 0 and its radius).
    /*   4 */ float m_float4;              // = 2.0f
    /*   8 */ float m_float8;              // = 2.0f
    /*  12 */ float m_float8Sqr;           // initPathfinding; the constructor leaves it
    // Where the search starts: the bot's last valid place on the mesh.
    /*  16 */ int   m_int16;               // = -1; its quadtree (island)
    /*  20 */ int   m_int20;               // = -1; its polygon
    /*  24 */ Vec3  m_startPosition;       // = 0
    /*  36 */ int   m_int36;               // = -1; the target's polygon
    /*  40 */ unsigned char m_targetMaterial;  // = 0; the material of that polygon
    /*  41 */ char  m_pad41[3];
    // [INFERRED] the position the search goes toward (BAPAMoveToFinding::update compares its
    // target with it). = 0.
    /*  44 */ Vec3  m_searchTarget;
    /*  56 */ int   m_int56;               // not a gap - `reset` zeroes it
    // Owns polygon and cluster path reservations, released through virtual slots +156 and
    // +164 during reset. [UNRESOLVED] The interface type is not yet known.
    /*  60 */ IAIPathfinding* m_pathOwner;  // = 0
    /*  64 */ QuadTree* m_quadTree;         // = 0; hasIllegalPath's polygons
    // update: run the polygon search (set when the points are dropped), or the cluster one
    /*  68 */ bool  m_flag68;
    /*  69 */ char  m_pad69[3];
    /*  72 */ QTPath* m_polygonPath;       // new(0x18) - the fine path
    // the pathfinding resource held for the polygon search, -1 for none (released in reset)
    /*  76 */ unsigned int m_polygonResource;   // = -1
    /*  80 */ int   m_vehicleNumber;       // reset: the equipment's IPIMobile::getVehicleNumber
    /*  84 */ int   m_equipmentType;       // initPathfinding; the constructor leaves it
    // objects within 50 m to steer around (updateObstacles)
    /*  88 */ std::vector<unsigned int> m_obstacles;
    // the walkable result - see the header note. The constructor's dead resize targets this.
    /* 100 */ std::vector<NavigationPoint> m_points;
    /* 112 */ unsigned int m_startQuadTree;  // initPathfinding, as m_int16; the constructor leaves it
    /* 116 */ int   m_int116;              // = 0
    /* 120 */ bool  m_flag120;
    /* 121 */ char  m_pad121[3];
    /* 124 */ ClusterNodePath* m_clusterPath;   // new(0x1C) - the coarse path
    /* 128 */ unsigned int m_clusterResource;   // = -1, as m_polygonResource
    // `allocate(20)` with 0 / 0 / self / self is an `_Rb_tree` header at 16 + sizeof(value_type)
    // so the value type is four bytes - a set of pointers or handles. Which is [UNRESOLVED];
    // `void*` holds the width and claims nothing.
    /* 132 */ std::set<unsigned short> m_allowedClusters;   // doPolygonSearch: where the stage may go
    /* 144 */ double m_progressTime;       // = 0.0; when the bot last got anywhere
    /* 152 */ double m_lastInputTime;      // = 0.0; when it last pressed to move
    /* 160 */ double m_double160;          // = 0.0
    /* 168 */ double m_jumpTime;           // = 0.0; -666: jump on the next step (updatePath)
    /* 176 */ Vec3  m_stillPosition;       // where checkProgress last looked for a jump
    // is 9,999,999.0f - a "no distance yet" sentinel, and a third kind after the
    // DBL_MAX timestamps and -9999.0 magnitudes BotMain uses. Do not normalise them together.
    /* 188 */ float m_sentinel188;         // = 9999999.0f
    /* 192 */ bool  m_flag192;             // = false
    /* 193 */ bool  m_flag193;             // = false
    /* 194 */ bool  m_flag194;             // not padding - `reset` clears it
    /* 195 */ bool  m_flag195;             // = false
    /* 196 */ char  m_gap196[4];           // [UNRESOLVED] - never written
    /* 200 */ int   m_int200;              // = -1
    /* 204 */ int   m_int204;              // = -1
    /* 208 */ int   m_int208;              // = -1
    /* 212 */ int   m_int212;              // = -1
    // getTraceData's cache: the position and map the place in m_int200.. was read for.
    /* 216 */ Vec3  m_tracePosition;       // = 0
    /* 228 */ int   m_traceMap;            // the constructor leaves it
    /* 232 */ float m_smallestRadius;      // initPathfinding: the equipment's physical
    /* 236 */ bool  m_isSoldier;           // initPathfinding: the equipment is a soldier
    /* 237 */ char  m_pad237[3];
    /* 240 */ int   m_int240;              // = 0
    // [INFERRED] name: isActive holds for three seconds after it.
    /* 244 */ double m_lastActiveTime;     // not a gap - `reset` zeroes it
    // resetAll clears this field.
    /* 252 */ int   m_obstacleCount;       // resetAll: 0; updateObstacles
    /* 256 */ Vec3  m_grenadePosition;     // = 0
    /* 268 */ double m_grenadeTime;        // = 0.0; resetAll: -9999.0
    /* 276 */ bool  m_grenadeFlag;         // = false [UNRESOLVED] meaning; BBRandom reacts only to it
    /* 277 */ char  m_pad277[3];
    /* 280 */ Vec3  m_progressPosition;    // where the bot last got to
    // Room beside the bot, up to 2 m: to its left and its right (updateSpaceLeftAndRight).
    /* 292 */ float m_sideSpace[2];        // reset: 0
    /* 300 */ double m_largestAngleTime;   // Initially -1.0; independent of the distance sentinel.
    /* 308 */ float m_largestAngle;        // getLargestAngleDiff's last answer
    /* 312 */ float m_pathLength;          // updateClosestDistance: along the navigation points
    // `reset`'s first statement clears the byte here.
    /* 316 */ bool  m_flag316;             // reset: false
    /* 317 */ char  m_pad317[3];
    // The stage of the polygon search: from where the points end to a portal two nodes on, or
    // to the target. The constructor leaves the polygons and the flag.
    /* 320 */ Vec3  m_stageFrom;           // = 0
    /* 332 */ Vec3  m_stageTo;             // = 0
    /* 344 */ unsigned int m_stageFromPolygon;
    /* 348 */ unsigned int m_stageToPolygon;
    /* 352 */ bool  m_stageReachesTarget;
    /* 353 */ char  m_pad353[3];
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTNAVIGATION_H
