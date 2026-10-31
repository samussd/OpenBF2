// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining QTSearchNodeManager operations.

#include "QTSearchNodeManager.h"

#include "QuadTree.h"
#include "../Material/MaterialManager.h"
#include "../../Main/AIPathfindingSearchTypes.h"
#include "../../../../../Core/Debug.h"

#include <cmath>

#include "../../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Constructors share initialization except for the optional quad-tree pointer.
// Keep both overloads; observed stores do not establish member declaration order.

QTSearchNodeManager::QTSearchNodeManager()
    : m_quadTree(0)
    , m_goal(0)
    , m_nodeCache(0)
    , m_nodeCount(0)
    , m_freeStates(0)
    , m_usedEntries(0)
    , m_table(new CacheEntry[kCacheEntries])
    , m_additionalGoals(0)
    , m_allowedClusters(0)
    , m_usePathTopologySmoothing(false)
    , m_materials(0)
{
}

QTSearchNodeManager::QTSearchNodeManager(QuadTree* quadTree)
    : m_quadTree(quadTree)
    , m_goal(0)
    , m_nodeCache(0)
    , m_nodeCount(0)
    , m_freeStates(0)
    , m_usedEntries(0)
    , m_table(new CacheEntry[kCacheEntries])
    , m_additionalGoals(0)
    , m_allowedClusters(0)
    , m_usePathTopologySmoothing(false)
    , m_materials(0)
{
}

// [FRONTIER] Node-manager table and cache-chain teardown remains unimplemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when all owned-resource cleanup is recovered.
QTSearchNodeManager::~QTSearchNodeManager()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QTSearchNodeManager::~QTSearchNodeManager");
}

// 0x30fc20 (Ghidra).
void QTSearchNodeManager::returnReset()
{
    m_usePathTopologySmoothing = false;
    m_additionalGoals = 0;
}

// 0x30eaa0 (Ghidra).
QTSearchNode::QTSearchNode(unsigned int polygon, unsigned int depth)
    : m_next(0)
    , m_previous(0)
    , m_parent(0)
    , m_entryParent(0)
    , m_polygon(polygon)
    , m_g(0.0f)
    , m_f(0.0f)
    , m_depth(depth)
{
    m_entry.x = 0.0f;
    m_entry.y = 0.0f;
    m_entry.z = 0.0f;
}

// 0x30eb60 (Ghidra).
QTSearchNode::QTSearchNode(const QTSearchNode& other)
    : m_next(0)
    , m_previous(0)
    , m_parent(0)
    , m_entryParent(0)
    , m_polygon(other.m_polygon)
    , m_g(0.0f)
    , m_f(0.0f)
    , m_depth(0)
    , m_entry(other.m_entry)
{
}

// 0x30ec80 (Ghidra).
QTSearchNode* QTSearchNode::expandFirst(QTSearchNodeManager* manager, int& status, unsigned short& edge)
{
    edge = 0;
    QTSearchNode* const neighbour = manager->getNeighbourOfNode(this, edge, status);
    ++edge;
    return neighbour;
}

// 0x30ecd0 (Ghidra).
QTSearchNode* QTSearchNode::expand(QTSearchNodeManager* manager, int& status, unsigned short& edge)
{
    QTSearchNode* const neighbour = manager->getNeighbourOfNode(this, edge, status);
    ++edge;
    return neighbour;
}

namespace {

int entryOf(int polygon)
{
    const int index = polygon % QTSearchNodeManager::kCacheEntries;
    return index < 0 ? index + QTSearchNodeManager::kCacheEntries : index;
}

} // namespace

// 0x30f430 (Ghidra). The goal is a copy; the node given goes back to the pool.
void QTSearchNodeManager::setGoal(QTSearchNode* node)
{
    if (!m_goal)
        m_goal = getNewNode(*node);
    else
        m_goal->m_polygon = node->m_polygon;
    m_goal->m_entry = node->m_entry;
    recycleNode(node);
}

// 0x30f4c0 (Ghidra). A recycled state keeps its closed flag, as shipped; open is tested first.
void QTSearchNodeManager::setNodeOpen(QTSearchNode* node)
{
    const int key = node->getKey();
    CacheEntry& entry = m_table[entryOf(key)];
    for (NodeState* state = entry.m_head; state; state = state->m_next)
    {
        if (state->m_node->getKey() == key)
        {
            state->m_open = true;
            return;
        }
    }
    NodeState* state = m_freeStates;
    if (!state)
    {
        state = new NodeState;
        state->m_closed = false;
    }
    else
        m_freeStates = state->m_next;
    state->m_open = true;
    state->m_node = node;
    state->m_next = entry.m_head;
    if (!entry.m_head)
    {
        entry.m_next = m_usedEntries;
        m_usedEntries = &entry;
    }
    entry.m_head = state;
}

// 0x30f5a0 (Ghidra). No null guard, as shipped: an open node has a state.
void QTSearchNodeManager::setNodeClosed(QTSearchNode* node)
{
    const int key = node->getKey();
    NodeState* state = m_table[entryOf(key)].m_head;
    while (state && state->m_node->getKey() != key)
        state = state->m_next;
    state->m_open = false;
    state->m_closed = true;
}

// 0x30f610 (Ghidra). The returned node is the caller's to recycle for every status but 0 and 3.
// A material of negative cost, or a polygon out of the allowed clusters, gives status 4.
QTSearchNode* QTSearchNodeManager::getNeighbourOfNode(QTSearchNode* node, unsigned short& edge, int& status)
{
    unsigned int polygon = m_quadTree->getNeighbourOfPolygon(node->m_polygon, static_cast<unsigned char>(edge));
    while (polygon == 0xFFFFFFFFu)
    {
        ++edge;
        if (m_quadTree->getPolygon(node->m_polygon)->m_numberOfVertices <= edge)
        {
            status = 4;
            return 0;
        }
        polygon = m_quadTree->getNeighbourOfPolygon(node->m_polygon, static_cast<unsigned char>(edge));
    }
    QTSearchNode* const child = getNewNode(polygon, node, node->m_depth + 1);
    const int key = child->getKey();
    const NodeState* state = m_table[entryOf(key)].m_head;
    while (state && state->m_node->getKey() != key)
        state = state->m_next;
    const Polygon* const record = m_quadTree->getPolygon(polygon);
    if (0.0f <= m_materials->getMaterialCost(record->m_material)
        && (!m_allowedClusters || m_allowedClusters->find(record->m_cluster) != m_allowedClusters->end()))
    {
        if (!state)
        {
            Vec3 entry;
            entry.x = 0.0f;
            entry.y = 0.0f;
            entry.z = 0.0f;
            calculateEntryPoint(entry, edge, node);
            child->m_entry = entry;
            child->m_g = calculateCost(node, child, child->m_entry);
            if (child->m_polygon != m_goal->m_polygon
                && (!m_additionalGoals || m_additionalGoals->find(polygon) == m_additionalGoals->end()))
            {
                child->m_f = calculateHeuristicCost(child->m_g, entry);
                status = 0;
                return child;
            }
            child->m_f = calculateCostBetweenPolygons(node->m_polygon, entry, polygon, m_goal->m_entry)
                + child->m_g;
            status = 3;
            return child;
        }
        if (state->m_open)
        {
            status = 1;
            return child;
        }
        if (state->m_closed)
        {
            status = 2;
            return child;
        }
        Debug(DebugTypeError, "AI/AIDLL/AI/AIPathFinding/NavMesh/QTSearchNodeManager.cpp", 0x121, "")
            << "This case should not exists!";
    }
    status = 4;
    return child;
}

// 0x30fa70 (Ghidra). A recycled node keeps its costs, entry and links but for the parents.
QTSearchNode* QTSearchNodeManager::getNewNode(unsigned int polygon, QTSearchNode* parent, unsigned int depth)
{
    QTSearchNode* node = m_nodeCache;
    if (!node)
    {
        node = new QTSearchNode(polygon, depth);
        ++m_nodeCount;
    }
    else
    {
        m_nodeCache = node->m_next;
        node->m_polygon = polygon;
        node->m_depth = depth;
    }
    node->m_parent = parent;
    node->m_entryParent = parent;
    return node;
}

// 0x30faf0 (Ghidra). A new node copies the place only (its constructor), a recycled one all.
QTSearchNode* QTSearchNodeManager::getNewNode(const QTSearchNode& other)
{
    QTSearchNode* node = m_nodeCache;
    if (!node)
    {
        node = new QTSearchNode(other);
        ++m_nodeCount;
    }
    else
    {
        m_nodeCache = node->m_next;
        *node = other;
    }
    return node;
}

// 0x30fba0 (Ghidra). Every node a state holds goes back to the pool, then the states and the goal.
// As shipped this also forgets the allowed clusters, and each search calls it after they are set.
void QTSearchNodeManager::reset()
{
    while (m_usedEntries)
    {
        NodeState* const head = m_usedEntries->m_head;
        NodeState* state = head;
        recycleNode(state->m_node);
        while (state->m_next)
        {
            state = state->m_next;
            recycleNode(state->m_node);
        }
        state->m_next = m_freeStates;
        m_freeStates = head;
        m_usedEntries->m_head = 0;
        m_usedEntries = m_usedEntries->m_next;
    }
    if (m_goal)
    {
        recycleNode(m_goal);
        m_goal = 0;
    }
    m_allowedClusters = 0;
}

// 0x30fc40 (Ghidra). Not past the depth limit, not back to where the node came from and not to
// the start. Takes it when under the known cost by more than 0.5.
bool QTSearchNodeManager::updateNode(QTSearchNode* node, QTSearchNode* existing, unsigned int maxDepth)
{
    const unsigned int depth = node->m_depth + 1;
    if (maxDepth == depth || node->m_entryParent == existing || !existing->m_parent)
        return false;
    Vec3 entry;
    entry.x = 0.0f;
    entry.y = 0.0f;
    entry.z = 0.0f;
    unsigned char edge;
    m_quadTree->getNeighbourIndexForPolygon(edge, node->m_polygon, existing->m_polygon);
    // checkIfNodeHasAdequateSize (0x310580) always answers true.
    calculateEntryPoint(entry, edge, node);
    const float g = calculateCost(node, existing, entry);
    const float f = calculateHeuristicCost(g, entry);
    if (!(f + 0.5f < existing->m_f))
        return false;
    existing->m_entryParent = node;
    existing->m_entry = entry;
    existing->m_f = f;
    existing->m_g = g;
    existing->m_parent = node;
    existing->m_depth = depth;
    return true;
}

// 0x30fda0 (Ghidra). A blocked step (negative cost) is not added to.
float QTSearchNodeManager::calculateCost(QTSearchNode* parent, QTSearchNode* node, const Vec3& entry) const
{
    if (!parent)
        return 0.0f;
    float cost = calculateCostBetweenPolygons(parent->m_polygon, parent->m_entry, node->m_polygon, entry);
    if (0.0f < cost)
        cost = cost + parent->m_g;
    return cost;
}

// 0x30fe20 (Ghidra). The distance at the material's cost. Smoothing also weighs steep polygons and
// sharp turns between polygons, for steps of 0.25 m or more.
float QTSearchNodeManager::calculateCostBetweenPolygons(unsigned int polygon, const Vec3& from, unsigned int other,
                                                        const Vec3& to) const
{
    float cost = m_materials->getMaterialCost(m_quadTree->getMaterialOfPolygon(polygon));
    if (cost < 0.0f)
        return -1.0f;
    const float dx = from.x - to.x;
    const float dy = from.y - to.y;
    const float dz = from.z - to.z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!m_usePathTopologySmoothing)
        return distance * cost;
    float turn = 0.0f;
    if (0.25f <= distance)
    {
        const Vec3& normal = m_quadTree->getNormalOfPolygon(polygon);
        const Vec3& otherNormal = m_quadTree->getNormalOfPolygon(other);
        float flat = normal.y * 5.0f - 4.0f;
        if (flat <= 0.0f)
            flat = 0.0f;
        float straight = (otherNormal.z * normal.z + otherNormal.x * normal.x + otherNormal.y * normal.y) * 5.0f - 4.0f;
        if (straight <= 0.0f)
            straight = 0.0f;
        const float slope = 4.0f - flat * flat * 3.0f;
        turn = 2.0f - straight * straight;
        turn = turn * turn;
        cost = slope * slope * cost;
    }
    return cost * distance + turn;
}

// 0x310060 (Ghidra). The cost so far plus the straight distance to the goal.
float QTSearchNodeManager::calculateHeuristicCost(float cost, const Vec3& position) const
{
    const float dx = m_goal->m_entry.x - position.x;
    const float dy = m_goal->m_entry.y - position.y;
    const float dz = m_goal->m_entry.z - position.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz) + cost;
}

// 0x310100 (Ghidra). At most the last 201 nodes, from the start on.
void QTSearchNodeManager::buildPath(QTPath* path, QTSearchNode* node)
{
    m_pathNodes.clear();
    QTSearchNode* at = node;
    int count = 0;
    while (at && count < 201)
    {
        ++count;
        m_pathNodes.push_back(at);
        at = at->m_parent;
    }
    for (int i = static_cast<int>(m_pathNodes.size()) - 1; i >= 0; --i)
    {
        const QTSearchNode* const step = m_pathNodes[i];
        triple<unsigned int, unsigned int, Vec3> point;
        point.first = step->m_polygon;
        point.second = step->m_entryParent ? step->m_entryParent->m_polygon : 0xFFFFFFFFu;
        point.third = step->m_entry;
        path->m_nodes.push_back(point);
    }
    path->m_cost = node->m_f;
}

// 0x3104b0 (Ghidra). The middle of the edge.
void QTSearchNodeManager::calculateEntryPoint(Vec3& entry, unsigned short edge, const QTSearchNode* node) const
{
    const Vec3& a = m_quadTree->getVertexForPolygon(node->m_polygon, static_cast<unsigned char>(edge));
    const Vec3& b = m_quadTree->getSafeVertexForPolygon(node->m_polygon,
                                                        static_cast<unsigned char>(static_cast<unsigned char>(edge) + 1));
    const float y = (b.y + a.y) * 0.5f;
    const float z = 0.5f * (b.z + a.z);
    entry.x = (b.x + a.x) * 0.5f;
    entry.y = y;
    entry.z = z;
}

} // namespace ai
} // namespace hfe
} // namespace dice
