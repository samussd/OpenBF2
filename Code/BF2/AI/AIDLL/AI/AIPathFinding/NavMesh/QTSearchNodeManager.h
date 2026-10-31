// Node pool for navigation-mesh A* searches. Spare nodes form an intrusive free list.
// [UNRESOLVED] Meaning of the separate 1031-entry table.
// [FRONTIER] Remaining search operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the node-pool lifecycle and search operations.

#ifndef AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QTSEARCHNODEMANAGER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QTSEARCHNODEMANAGER_H

#include <set>
#include <vector>

#include "../../Main/IAIPathfinding.h"     // Vec3

namespace dice {
namespace hfe {
namespace ai {

class MaterialManager;
class QTPath;
class QuadTree;
class QTSearchNodeManager;

// A polygon on the search's frontier. getKey and the names below are what the shared search reads.
class QTSearchNode
{
public:
    QTSearchNode(unsigned int polygon, unsigned int depth);
    explicit QTSearchNode(const QTSearchNode& other);    // the place only: no links, no costs

    int getKey() const { return static_cast<int>(m_polygon); }
    QTSearchNode* expandFirst(QTSearchNodeManager* manager, int& status, unsigned short& edge);
    QTSearchNode* expand(QTSearchNodeManager* manager, int& status, unsigned short& edge);

    /* +0x00 */ QTSearchNode* m_next;        // the free list and the hash chains
    /* +0x04 */ QTSearchNode* m_previous;
    /* +0x08 */ QTSearchNode* m_parent;
    /* +0x0C */ QTSearchNode* m_entryParent; // the polygon the entry point was reached from
    /* +0x10 */ unsigned int m_polygon;
    /* +0x14 */ float m_g;
    /* +0x18 */ float m_f;
    /* +0x1C */ unsigned int m_depth;
    /* +0x20 */ Vec3 m_entry;                // the middle of the edge crossed into the polygon
};

class QTSearchNodeManager
{
public:
    // The eight-byte record of the table at +24. `operator new[]` is called without an array cookie
    // so the element is trivially destructible; the constructor zeroes both of its words. Name and
    // field names [INFERRED].
    // What the search knows of a polygon: a node seen there and whether it is open or closed.
    class NodeState
    {
    public:
        /*  +0 */ bool m_open;
        /*  +1 */ bool m_closed;
        /*  +4 */ QTSearchNode* m_node;
        /*  +8 */ NodeState* m_next;
    };

    class CacheEntry
    {
    public:
        CacheEntry()
            : m_next(0)
            , m_head(0)
        {
        }

        CacheEntry* m_next;
        NodeState* m_head;
    };

    // the same prime the search resource'S two hashes use. Written out as a literal in the binary.
    enum { kCacheEntries = 1031 };

    QTSearchNodeManager();
    explicit QTSearchNodeManager(QuadTree* quadTree);
    ~QTSearchNodeManager();                      // , [FRONTIER]

    // What a search left behind that the next one must not inherit.
    void returnReset();

    void setQuadTree(QuadTree* quadTree) { m_quadTree = quadTree; }
    void setGoal(QTSearchNode* node);
    void setNodeOpen(QTSearchNode* node);
    void setNodeClosed(QTSearchNode* node);
    QTSearchNode* getNewNode(unsigned int polygon, QTSearchNode* parent, unsigned int depth);
    QTSearchNode* getNewNode(const QTSearchNode& other);
    void recycleNode(QTSearchNode* node)
    {
        node->m_next = m_nodeCache;
        m_nodeCache = node;
    }
    // The neighbour across an edge at or after `edge`; status as in AStarResource::update.
    QTSearchNode* getNeighbourOfNode(QTSearchNode* node, unsigned short& edge, int& status);
    // Whether a cheaper way to `existing` runs through `node`, and takes it.
    bool updateNode(QTSearchNode* node, QTSearchNode* existing, unsigned int maxDepth);
    void buildPath(QTPath* path, QTSearchNode* node);
    void reset();
    float calculateCost(QTSearchNode* parent, QTSearchNode* node, const Vec3& entry) const;
    float calculateCostBetweenPolygons(unsigned int polygon, const Vec3& from, unsigned int other,
                                       const Vec3& to) const;
    float calculateHeuristicCost(float cost, const Vec3& position) const;
    void calculateEntryPoint(Vec3& entry, unsigned short edge, const QTSearchNode* node) const;

    // layout, on the ELF, read store by store out of the constructor
    /*  +0 */ QuadTree* m_quadTree;      // the one argument the second constructor takes
    /*  +4 */ QTSearchNode* m_goal;      // = 0
    /*  +8 */ QTSearchNode* m_nodeCache; // = 0 the intrusive free list `getSizeOfNodeCache` walks
    /* +12 */ int m_nodeCount;           // = 0; nodes made
    /* +16 */ NodeState* m_freeStates;   // = 0
    /* +20 */ CacheEntry* m_usedEntries; // = 0; the table entries in use
    /* +24 */ CacheEntry* m_table;       // new CacheEntry[kCacheEntries]
    /* +28 */ std::vector<QTSearchNode*> m_pathNodes;   // buildPath's scratch
    /* +40 */ const std::set<unsigned int>* m_additionalGoals;     // = 0; polygons that end the search too
    /* +44 */ const std::set<unsigned short>* m_allowedClusters;   // = 0; setAllowedClusters
    /* +48 */ bool m_usePathTopologySmoothing;                     // = false; setUsePathTopologySmoothing
    /* +52 */ MaterialManager* m_materials;                        // = 0; the vehicle's costs
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QTSEARCHNODEMANAGER_H
