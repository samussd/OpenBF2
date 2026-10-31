// Node pool for cluster-graph A* searches, including an intrusive free list for open/closed records.
// returnReset is intentionally empty.
// [FRONTIER] Remaining search operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the node-pool lifecycle and search operations.

#ifndef AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERNODEMANAGER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERNODEMANAGER_H

#include "../../Main/AIPathfindingSearchTypes.h"      // Hash, Vec3

#include <vector>

namespace dice {
namespace hfe {
namespace ai {

class ClusterGraph;
class QuadTree;
class ClusterNodeManager;

// A cluster on the search's frontier. getKey and the names below are what the shared search reads.
class ClusterNode
{
public:
    ClusterNode(unsigned short cluster, unsigned int depth);
    explicit ClusterNode(const ClusterNode& other);      // the place only: no links, no costs

    int getKey() const { return m_cluster; }
    ClusterNode* expandFirst(ClusterNodeManager* manager, int& status, unsigned short& edge);
    ClusterNode* expand(ClusterNodeManager* manager, int& status, unsigned short& edge);

    /* +0x00 */ ClusterNode* m_next;         // the free list and the hash chains
    /* +0x04 */ ClusterNode* m_previous;
    /* +0x08 */ ClusterNode* m_parent;
    /* +0x0C */ float m_g;
    /* +0x10 */ float m_f;
    /* +0x14 */ unsigned short m_cluster;
    /* +0x16 */ unsigned short m_portal;     // the portal it was entered by, 0xFFFF none
    /* +0x18 */ unsigned int m_depth;
};

class ClusterNodeManager
{
public:
    // The open/closed record of a cluster. Attested as a nested type by the mangled names of
    // `getNewOCData`, `recycleOCData` and `HashIterator<ClusterNodeManager::OCData, ...>`.
    class OCData
    {
    public:
        int getKey() const { return m_node->getKey(); }

        /*  +0 */ bool m_open;
        /*  +1 */ bool m_closed;
        /*  +4 */ ClusterNode* m_node;
        /*  +8 */ OCData* m_next;              // the free list and the hash chains
        /* +12 */ OCData* m_previous;
    };

    // the same prime the cluster search resource'S two hashes use. A literal in the binary.
    enum { kOCDataBuckets = 313 };

    ClusterNodeManager();
    ~ClusterNodeManager();       // , [FRONTIER]

    // Nothing to reset. Empty in the shipped binary.
    void returnReset() {}

    void setQuadTree(QuadTree* quadTree) { m_quadTree = quadTree; }
    void setClusterGraph(ClusterGraph* graph) { m_clusterGraph = graph; }
    void setGoal(ClusterNode* node);
    void setNodeOpen(ClusterNode* node);
    void setNodeClosed(ClusterNode* node);
    ClusterNode* getNewNode(unsigned short cluster, ClusterNode* parent, unsigned int depth);
    ClusterNode* getNewNode(const ClusterNode& other);
    void recycleNode(ClusterNode* node)
    {
        node->m_next = m_freeNodes;
        m_freeNodes = node;
    }
    // The next neighbour from portal `edge` on; status as in AStarResource::update.
    ClusterNode* getNeighbourOfNode(ClusterNode* node, unsigned short& edge, int& status);
    bool updateNode(ClusterNode* node, ClusterNode* existing, unsigned int maxDepth);
    void buildPath(ClusterNodePath* path, ClusterNode* node);
    void reset();
    OCData* getNewOCData();
    void recycleOCData(OCData* data);
    float calculateCost(const ClusterNode* node, unsigned short portal) const;
    float calculateHeuristicCost(float cost, unsigned short portal, const ClusterNode* node) const;
    const Vec3& getEntryPoint(unsigned short portal, const ClusterNode* node) const;
    // The cheapest portal out of a cluster to the neighbour of portal `edge`, and its match on the far
    // side; negative when the neighbour cannot be entered.
    float findBestPortalAndCost(unsigned short& best, unsigned short& portal, const ClusterNode* node,
                                unsigned short edge, unsigned short cluster) const;

    // layout, on the ELF, read store by store out of the constructor
    /*  +0 */ QuadTree* m_quadTree;          // = 0; `setQuadTree` writes it
    /*  +4 */ ClusterGraph* m_clusterGraph;  // = 0; `setClusterGraph` writes it
    /*  +8 */ Vec3 m_start;                  // = 0
    /* +20 */ Vec3 m_goalPosition;           // = 0
    /* +32 */ ClusterNode* m_goal;           // = 0
    /* +36 */ ClusterNode* m_freeNodes;      // = 0
    /* +40 */ int m_nodeCount;               // = 0; nodes made
    /* +44 */ OCData* m_freeOCData;          // = 0 the intrusive free list `recycleOCData` pushes to
    /* +48 */ int m_ocDataCount;             // = 0; records made
    /* +52 */ Hash<OCData, ClusterNodeManager, OCData>* m_ocDataHash;   // new Hash(313), then ->this
    /* +56 */ std::vector<ClusterNode*> m_pathNodes;   // buildPath's scratch
    /* +68 */ int m_row;                     // = 0; the portal row of the cost type
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERNODEMANAGER_H
