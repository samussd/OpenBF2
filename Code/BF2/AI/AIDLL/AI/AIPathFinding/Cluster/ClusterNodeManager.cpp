// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining ClusterNodeManager operations.

#include "ClusterNodeManager.h"

#include "Cluster.h"
#include "ClusterGraph.h"
#include "../../../../../Core/Debug.h"

#include <cmath>

#include "../../AIUtils/checkPointer.h"
#include "../../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes empty node lists and a 313-bucket hash linked to this manager.

ClusterNodeManager::ClusterNodeManager()
    : m_quadTree(0)
    , m_clusterGraph(0)
    , m_goal(0)
    , m_freeNodes(0)
    , m_nodeCount(0)
    , m_freeOCData(0)
    , m_ocDataCount(0)
    , m_ocDataHash(checkPointer(new Hash<OCData, ClusterNodeManager, OCData>(kOCDataBuckets)))
    , m_row(0)
{
    m_start.x = 0.0f;
    m_start.y = 0.0f;
    m_start.z = 0.0f;
    m_goalPosition = m_start;
    m_ocDataHash->m_nodeManager = this;
}

// [FRONTIER] Cleanup of the hash, buckets, and OCData chain remains unimplemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when ordered node-manager teardown is recovered.
ClusterNodeManager::~ClusterNodeManager()
{
    BF2_AI_FRONTIER("dice::hfe::ai::ClusterNodeManager::~ClusterNodeManager");
}

// 0x32f9b0 (Ghidra).
ClusterNode::ClusterNode(unsigned short cluster, unsigned int depth)
    : m_next(0)
    , m_previous(0)
    , m_parent(0)
    , m_g(0.0f)
    , m_f(0.0f)
    , m_cluster(cluster)
    , m_portal(0xFFFF)
    , m_depth(depth)
{
}

// 0x32fa50 (Ghidra).
ClusterNode::ClusterNode(const ClusterNode& other)
    : m_next(0)
    , m_previous(0)
    , m_parent(0)
    , m_g(0.0f)
    , m_f(0.0f)
    , m_cluster(other.m_cluster)
    , m_portal(other.m_portal)
    , m_depth(0)
{
}

// 0x32fb10 (Ghidra).
ClusterNode* ClusterNode::expandFirst(ClusterNodeManager* manager, int& status, unsigned short& edge)
{
    edge = 0;
    ClusterNode* const neighbour = manager->getNeighbourOfNode(this, edge, status);
    ++edge;
    return neighbour;
}

// 0x32fb60 (Ghidra).
ClusterNode* ClusterNode::expand(ClusterNodeManager* manager, int& status, unsigned short& edge)
{
    ClusterNode* const neighbour = manager->getNeighbourOfNode(this, edge, status);
    ++edge;
    return neighbour;
}

namespace {

// 0x32fc60 (Ghidra). Logs a node that is its own ancestor within ten steps.
bool checkHeritage(const ClusterNode* node)
{
    int steps = 0;
    for (const ClusterNode* at = node->m_parent; at && steps < 10; at = at->m_parent, ++steps)
    {
        if (at == node)
        {
            Debug(DebugTypeError, "AI/AIDLL/AI/AIPathFinding/Cluster/ClusterNodeManager.cpp", 0x26, "")
                << "Circular heritage for " << static_cast<unsigned int>(at->m_cluster);
            return false;
        }
    }
    return true;
}

} // namespace

// 0x330520 (Ghidra). The goal is a copy; the node given goes back to the pool.
void ClusterNodeManager::setGoal(ClusterNode* node)
{
    if (!m_goal)
        m_goal = getNewNode(*node);
    else
    {
        m_goal->m_cluster = node->m_cluster;
        m_goal->m_portal = node->m_portal;
    }
    recycleNode(node);
}

// 0x330590 (Ghidra).
void ClusterNodeManager::setNodeOpen(ClusterNode* node)
{
    OCData* const data = getNewOCData();
    data->m_open = true;
    data->m_node = node;
    data->m_closed = false;
    OCData* const existing = m_ocDataHash->get(data);
    if (!existing)
        m_ocDataHash->insert(data);
    else
    {
        recycleOCData(data);
        existing->m_open = true;
    }
}

// 0x3306b0 (Ghidra). No null guard, as shipped: an open node has a record.
void ClusterNodeManager::setNodeClosed(ClusterNode* node)
{
    OCData* const data = getNewOCData();
    data->m_open = true;
    data->m_closed = false;
    data->m_node = node;
    OCData* const existing = m_ocDataHash->get(data);
    recycleOCData(data);
    existing->m_open = false;
    existing->m_closed = true;
}

// 0x330730 (Ghidra). Leaves edge on the portal it answered for. A neighbour's best portal is taken
// once, at its lowest portal number.
ClusterNode* ClusterNodeManager::getNeighbourOfNode(ClusterNode* node, unsigned short& edge, int& status)
{
    unsigned short best = 0;
    unsigned short portal = 0xFFFF;
    const Cluster& cluster = m_clusterGraph->getCluster(node->m_cluster);
    if (edge < cluster.m_numberOfPortals)
    {
        float cost = findBestPortalAndCost(best, portal, node, edge, node->m_cluster);
        while (edge < cluster.m_numberOfPortals)
        {
            if (0.0f <= cost && portal != 0xFFFF && node->m_portal != edge && edge <= best)
            {
                const ClusterPortal* const exit =
                    m_clusterGraph->getClusterPortal(node->m_cluster, static_cast<unsigned short>(m_row), best);
                ClusterNode* const child = getNewNode(exit->m_neighbour, node, node->m_depth + 1);
                OCData* const probe = getNewOCData();
                probe->m_open = true;
                probe->m_closed = false;
                probe->m_node = child;
                const OCData* const state = m_ocDataHash->get(probe);
                if (!state)
                {
                    checkHeritage(child);
                    child->m_portal = portal;
                    if (child->m_cluster == m_goal->m_cluster)
                        status = 3;
                    else
                    {
                        status = 0;
                        child->m_g = cost;
                        child->m_f = calculateHeuristicCost(cost, portal, child);
                    }
                }
                else if (state->m_open)
                    status = 1;
                else if (state->m_closed)
                    status = 2;
                else
                {
                    Debug(DebugTypeError, "AI/AIDLL/AI/AIPathFinding/Cluster/ClusterNodeManager.cpp", 0xe9, "")
                        << "This case should not exists!";
                    status = 4;
                }
                recycleOCData(probe);
                return child;
            }
            ++edge;
            cost = findBestPortalAndCost(best, portal, node, edge, node->m_cluster);
        }
    }
    status = 4;
    return 0;
}

// 0x330b10 (Ghidra). A recycled node keeps its costs and its portal, as shipped.
ClusterNode* ClusterNodeManager::getNewNode(unsigned short cluster, ClusterNode* parent, unsigned int depth)
{
    ClusterNode* node = m_freeNodes;
    if (!node)
    {
        node = new ClusterNode(cluster, depth);
        ++m_nodeCount;
    }
    else
    {
        m_freeNodes = node->m_next;
        node->m_cluster = cluster;
        node->m_depth = depth;
    }
    node->m_parent = parent;
    return node;
}

// 0x330b90 (Ghidra). A new node copies the place only (its constructor), a recycled one all.
ClusterNode* ClusterNodeManager::getNewNode(const ClusterNode& other)
{
    ClusterNode* node = m_freeNodes;
    if (!node)
    {
        node = new ClusterNode(other);
        ++m_nodeCount;
    }
    else
    {
        m_freeNodes = node->m_next;
        *node = other;
    }
    return node;
}

// 0x330c30 (Ghidra). Every node a record holds goes back to the pool, then the records and the goal.
void ClusterNodeManager::reset()
{
    while (m_ocDataHash->m_used)
    {
        OCData* const data = m_ocDataHash->m_used->m_head;
        recycleNode(data->m_node);
        m_ocDataHash->remove(data);
        recycleOCData(data);
    }
    if (m_goal)
    {
        recycleNode(m_goal);
        m_goal = 0;
    }
}

// 0x330df0 (Ghidra). Not past the depth limit and not to the start.
bool ClusterNodeManager::updateNode(ClusterNode* node, ClusterNode* existing, unsigned int maxDepth)
{
    const unsigned int depth = node->m_depth + 1;
    if (maxDepth == depth || !existing->m_parent)
        return false;
    const unsigned short row = static_cast<unsigned short>(m_row);
    const unsigned short edge = m_clusterGraph->findClusterPortalWithNeighbour(node->m_cluster, row, existing->m_cluster);
    unsigned short best;
    unsigned short portal;
    const float g = findBestPortalAndCost(best, portal, node, edge, node->m_cluster);
    if (g < 0.0f)
        return false;
    const float f = calculateHeuristicCost(g, portal, existing);
    if (!(f < existing->m_f))
        return false;
    existing->m_portal = m_clusterGraph->getPortalForNeighbour(existing->m_cluster, row, node->m_cluster);
    existing->m_parent = node;
    checkHeritage(existing);
    existing->m_g = g;
    existing->m_f = f;
    existing->m_depth = depth;
    existing->m_portal = portal;
    return true;
}

// 0x330f30 (Ghidra). From the start position out of the first cluster, from the stored portal
// costs after that; a blocked pair gives -1.
float ClusterNodeManager::calculateCost(const ClusterNode* node, unsigned short portal) const
{
    if (!node)
        return 0.0f;
    float cost;
    if (node->m_portal == 0xFFFF)
    {
        const Vec3& at = m_clusterGraph->getClusterPortal(node->m_cluster, static_cast<unsigned short>(m_row), portal)->m_position;
        const float dx = at.x - m_start.x;
        const float dy = at.y - m_start.y;
        const float dz = at.z - m_start.z;
        cost = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    else
    {
        cost = m_clusterGraph->getCostBetweenClustersPortals(node->m_cluster, node->m_portal, portal,
                                                             static_cast<unsigned short>(m_row));
        if (cost < 0.0f)
            return -1.0f;
    }
    return node->m_g + cost;
}

// 0x331060 (Ghidra). The cost so far plus the straight distance to the goal.
float ClusterNodeManager::calculateHeuristicCost(float cost, unsigned short portal, const ClusterNode* node) const
{
    const Vec3& at = getEntryPoint(portal, node);
    const float dx = m_goalPosition.x - at.x;
    const float dy = m_goalPosition.y - at.y;
    const float dz = m_goalPosition.z - at.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz) + cost;
}

// 0x331110 (Ghidra). From the start on: each cluster and the portal it was entered by.
void ClusterNodeManager::buildPath(ClusterNodePath* path, ClusterNode* node)
{
    m_pathNodes.clear();
    for (ClusterNode* at = node; at; at = at->m_parent)
        m_pathNodes.push_back(at);
    for (int i = static_cast<int>(m_pathNodes.size()) - 1; i >= 0; --i)
        path->m_nodes.push_back(std::pair<unsigned short, unsigned short>(m_pathNodes[i]->m_cluster,
                                                                          m_pathNodes[i]->m_portal));
}

// 0x331280 (Ghidra).
ClusterNodeManager::OCData* ClusterNodeManager::getNewOCData()
{
    OCData* data = m_freeOCData;
    if (!data)
    {
        ++m_ocDataCount;
        data = checkPointer(new OCData);
        data->m_open = true;
        data->m_closed = false;
        data->m_node = 0;
        data->m_next = 0;
    }
    else
        m_freeOCData = data->m_next;
    return data;
}

// 0x3313f0 (Ghidra).
void ClusterNodeManager::recycleOCData(OCData* data)
{
    data->m_next = m_freeOCData;
    m_freeOCData = data;
}

// 0x331410 (Ghidra).
const Vec3& ClusterNodeManager::getEntryPoint(unsigned short portal, const ClusterNode* node) const
{
    if (!node->m_parent)
        return m_start;
    return m_clusterGraph->getClusterPortal(node->m_cluster, static_cast<unsigned short>(m_row), portal)->m_position;
}

// 0x331470 (Ghidra). The binary also takes the node's portal, unused.
float ClusterNodeManager::findBestPortalAndCost(unsigned short& best, unsigned short& portal, const ClusterNode* node,
                                                unsigned short edge, unsigned short cluster) const
{
    const unsigned short row = static_cast<unsigned short>(m_row);
    const unsigned short neighbour = m_clusterGraph->getClusterPortal(cluster, row, edge)->m_neighbour;
    if (neighbour >= m_clusterGraph->getNumberOfClusters() || m_clusterGraph->getCluster(neighbour).m_state == 0)
        return -1.0f;
    float bestCost = 1000000000.0f;
    portal = 0xFFFF;
    const unsigned short portals = m_clusterGraph->getCluster(cluster).m_numberOfPortals;
    for (unsigned short i = 0; i < portals; ++i)
    {
        const ClusterPortal* const candidate = m_clusterGraph->getClusterPortal(cluster, row, i);
        if (candidate->m_neighbour != neighbour)
            continue;
        const float cost = calculateCost(node, i);
        if (cost < bestCost && 0.0f <= cost)
        {
            best = i;
            portal = m_clusterGraph->getCorrespongingPortalInNeighbour(candidate, row);
            bestCost = cost;
        }
    }
    if (portal == 0xFFFF)
        return -1.0f;
    return bestCost;
}

} // namespace ai
} // namespace hfe
} // namespace dice
