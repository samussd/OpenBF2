// Shared path containers, node records and reusable A* search resources.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIPathfindingSearchTypes operations.

#ifndef AI_AIDLL_AI_MAIN_AIPATHFINDINGSEARCHTYPES_H
#define AI_AIDLL_AI_MAIN_AIPATHFINDINGSEARCHTYPES_H

#include <list>
#include <set>
#include <utility>
#include <vector>

#include "IAIPathfinding.h"     // ai::triple, and Vec3 through AIDllTypes.h
#include "AIDllFrontier.h"      // AStarResource::update's named stop
#include "CommonSingletonServer.h"                           // PathfindingEngine::update's clock
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

// Navigation path result with polygon positions and traversal state.
class QTPath
{
public:
    QTPath()
        : m_i0(0)
        , m_sentinel16(-1)
        , m_cost(0.0f)
    {
        // The vector is default-constructed (three null pointers in retail) by the member above
        // being absent from this list - the binary's four zero stores at +0 and +4..+15 are that
        // and nothing more.
    }

    /*  +0 */ int m_i0;                                       // = 0 [UNRESOLVED]
    /*  +4 */ std::vector<triple<unsigned int, unsigned int, Vec3> > m_nodes;
    /* +16 */ int m_sentinel16;                               // = -1 [UNRESOLVED]
    /* +20 */ float m_cost;                                   // = 0; buildPath: the goal node's
};

// Cluster-level path result and traversal state.
class ClusterNodePath
{
public:
    ClusterNodePath()
        : m_i0(0)
    {
        // m_gap4 deliberately not initialised - the binary steps over it. See above.
    }

    // The portal of a node's cluster (a path index, not relative to m_i0): its position and +0x10.
    // False past the end and for a node without a next cluster.
    bool getPoint(Vec3& position, unsigned int& out, unsigned int index) const;
    // Which map and cost type the path was searched for.
    void setVehicle(int map, unsigned int type);

    /*  +0 */ int m_i0;                                       // = 0; the node being walked
    // The constructor steps over the next three; setVehicle writes the first two ([INFERRED] names).
    /*  +4 */ int m_map;
    /*  +8 */ int m_row;                                      // a portal row: the cost type
    /* +12 */ unsigned int m_island;
    /* +16 */ std::vector<std::pair<unsigned short, unsigned short> > m_nodes;
};

// Reusable A* node pools, hash tables and search state.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding AIPathfindingSearchTypes operation.

// One entry of the open tree. Spare entries chain through m_left. The constructor writes +0 only.
template <class T>
class BSTNode
{
public:
    BSTNode()
        : m_value(0)
    {
    }

    /*  +0 */ T* m_value;
    /*  +4 */ BSTNode<T>* m_parent;
    /*  +8 */ BSTNode<T>* m_left;
    /* +12 */ BSTNode<T>* m_right;
};

// The cheapest open nodes, ordered by m_f with the cheapest at m_leftmost. Not balanced. Entries come
// from a pool that grows as needed.
template <class Node, class SearchNode>
class BinarySearchTree
{
public:
    typedef BSTNode<SearchNode> Entry;

    explicit BinarySearchTree(int n)
        : m_size(0)
        , m_free(0)
        , m_root(0)
    {
        // m_leftmost deliberately not initialised, as shipped.
        for (int i = n; i > 0; --i)
        {
            Entry* const entry = new Entry();
            entry->m_left = m_free;
            m_free = entry;
        }
    }

    void insert(SearchNode* value);
    SearchNode* pop();                                 // the cheapest, or 0
    SearchNode* remove(SearchNode* value);             // by key
    Entry* get(const SearchNode* value, Entry* entry) const;   // by key, depth first
    void clear();

    /*  +0 */ int m_size;
    /*  +4 */ Entry* m_free;
    /*  +8 */ Entry* m_leftmost;
    /* +12 */ Entry* m_root;

private:
    void insert(Entry* at, Entry* entry);
    void merge(Entry* at, Entry* entry);
    void flatten(Entry* entry, Entry*& tail);
    static Entry* getLeftmostChildOfTree(Entry* entry);
};

// 0x3bf950 (Ghidra; QTSearchNode's instantiation, as the addresses below).
template <class Node, class SearchNode>
void BinarySearchTree<Node, SearchNode>::insert(SearchNode* value)
{
    Entry* entry = m_free;
    if (!entry)
        entry = new Entry();
    else
        m_free = entry->m_left;
    entry->m_left = 0;
    entry->m_right = 0;
    entry->m_value = value;
    if (m_size == 0)
    {
        m_leftmost = entry;
        m_root = entry;
        entry->m_parent = 0;
    }
    else
    {
        Entry* const root = m_root;
        if (root->m_value->m_f <= value->m_f)
        {
            if (root->m_right)
                insert(root->m_right, entry);
            else
            {
                root->m_right = entry;
                entry->m_parent = root;
            }
        }
        else
        {
            if (root->m_left)
                insert(root->m_left, entry);
            else
            {
                root->m_left = entry;
                entry->m_parent = root;
            }
        }
        if (m_leftmost->m_value->m_f <= entry->m_value->m_f)
            entry = m_leftmost;
        m_leftmost = entry;
    }
    ++m_size;
}

// 0x3c2bd0 (Ghidra).
template <class Node, class SearchNode>
void BinarySearchTree<Node, SearchNode>::insert(Entry* at, Entry* entry)
{
    for (;;)
    {
        if (at->m_value->m_f <= entry->m_value->m_f)
        {
            if (!at->m_right)
            {
                at->m_right = entry;
                break;
            }
            at = at->m_right;
        }
        else
        {
            if (!at->m_left)
            {
                at->m_left = entry;
                break;
            }
            at = at->m_left;
        }
    }
    entry->m_parent = at;
}

// 0x3c2cb0 (Ghidra). Hangs a subtree where its root belongs.
template <class Node, class SearchNode>
void BinarySearchTree<Node, SearchNode>::merge(Entry* at, Entry* entry)
{
    if (!entry)
        return;
    for (;;)
    {
        if (entry->m_value->m_f < at->m_value->m_f)
        {
            if (!at->m_left)
            {
                at->m_left = entry;
                break;
            }
            at = at->m_left;
        }
        else
        {
            if (!at->m_right)
            {
                at->m_right = entry;
                break;
            }
            at = at->m_right;
        }
    }
    entry->m_parent = at;
}

// 0x3c2c10 (Ghidra).
template <class Node, class SearchNode>
typename BinarySearchTree<Node, SearchNode>::Entry*
BinarySearchTree<Node, SearchNode>::getLeftmostChildOfTree(Entry* entry)
{
    while (entry && entry->m_left)
        entry = entry->m_left;
    return entry;
}

// 0x3bfa40 (Ghidra).
template <class Node, class SearchNode>
SearchNode* BinarySearchTree<Node, SearchNode>::pop()
{
    Entry* const entry = m_leftmost;
    if (!entry)
        return 0;
    --m_size;
    Entry* const right = entry->m_right;
    Entry* const parent = entry->m_parent;
    if (!right)
    {
        m_leftmost = parent;
        if (parent)
            parent->m_left = 0;
        else
            m_root = 0;
    }
    else if (!parent)
    {
        m_root = right;
        right->m_parent = 0;
        m_leftmost = getLeftmostChildOfTree(right);
    }
    else
    {
        parent->m_left = right;
        right->m_parent = parent;
        m_leftmost = right->m_left ? getLeftmostChildOfTree(right->m_left) : right;
    }
    entry->m_left = m_free;
    m_free = entry;
    return entry->m_value;
}

// 0x3c0b90 (Ghidra). A cheaper subtree under the removed entry moves into the one that replaces it.
template <class Node, class SearchNode>
SearchNode* BinarySearchTree<Node, SearchNode>::remove(SearchNode* value)
{
    if (m_size == 0)
        return 0;
    if (value->getKey() == m_leftmost->m_value->getKey())
        return pop();
    Entry* const entry = get(value, m_root);
    if (!entry)
        return 0;
    --m_size;
    Entry* const parent = entry->m_parent;
    Entry* const left = entry->m_left;
    Entry* const right = entry->m_right;
    if (!parent)
    {
        if (!right)
        {
            m_root = left;
            left->m_parent = 0;
        }
        else
        {
            m_root = right;
            right->m_parent = 0;
            merge(right, left);
        }
    }
    else
    {
        Entry*& link = parent->m_left == entry ? parent->m_left : parent->m_right;
        if (!right)
        {
            link = left;
            if (left)
                left->m_parent = parent;
        }
        else
        {
            link = right;
            right->m_parent = parent;
            if (left)
            {
                if (right->m_value->m_f <= left->m_value->m_f)
                {
                    if (right->m_right)
                        merge(right->m_right, left);
                    else
                    {
                        right->m_right = left;
                        left->m_parent = right;
                    }
                }
                else
                {
                    if (right->m_left)
                        merge(right->m_left, left);
                    else
                    {
                        right->m_left = left;
                        left->m_parent = right;
                    }
                }
            }
        }
    }
    entry->m_left = m_free;
    m_free = entry;
    return entry->m_value;
}

// 0x3c2910 (Ghidra).
template <class Node, class SearchNode>
typename BinarySearchTree<Node, SearchNode>::Entry*
BinarySearchTree<Node, SearchNode>::get(const SearchNode* value, Entry* entry) const
{
    while (entry)
    {
        if (value->getKey() == entry->m_value->getKey())
            return entry;
        Entry* const found = get(value, entry->m_left);
        if (found)
            return found;
        entry = entry->m_right;
    }
    return 0;
}

// 0x3c2360 (Ghidra). Chains a subtree through m_left; tail is the chain's end.
template <class Node, class SearchNode>
void BinarySearchTree<Node, SearchNode>::flatten(Entry* entry, Entry*& tail)
{
    if (!entry->m_left)
    {
        if (!entry->m_right)
            tail = entry;
        else
        {
            flatten(entry->m_right, tail);
            entry->m_left = entry->m_right;
        }
    }
    else
    {
        Entry* leftTail;
        flatten(entry->m_left, leftTail);
        if (!entry->m_right)
            tail = leftTail;
        else
        {
            flatten(entry->m_right, tail);
            leftTail->m_left = entry->m_right;
        }
    }
}

// 0x3c22f0 (Ghidra). The whole tree goes back to the pool at once.
template <class Node, class SearchNode>
void BinarySearchTree<Node, SearchNode>::clear()
{
    if (!m_root)
        return;
    Entry* tail;
    flatten(m_root, tail);
    tail->m_left = m_free;
    m_free = m_root;
    m_size = 0;
    m_root = 0;
    m_leftmost = 0;
}

// A chained hash of intrusive values (m_next, m_previous, getKey). The buckets in use form a list.
template <class Node, class NodeManager, class SearchNode>
class Hash
{
public:
    class Bucket
    {
    public:
        Bucket()
            : m_next(0)
            , m_previous(0)
            , m_head(0)
        {
        }

        Bucket* m_next;
        Bucket* m_previous;
        SearchNode* m_head;
    };

    explicit Hash(unsigned int n)
        : m_self0((Hash*)this)
        , m_i12(0)
        , m_i16(0)
        , m_self1((Hash*)this)
        , m_buckets(new Bucket[n + 1])
        , m_used(0)
        , m_size(0)
        , m_capacity((int)n)
        , m_nodeManager(0)
    {
        // m_i0 and m_i4 deliberately not initialised, as shipped.
    }

    void insert(SearchNode* value);
    SearchNode* get(const SearchNode* value) const;
    void remove(SearchNode* value);
    void clear();

    // The binary's begin and end iterators (bucket, value, hash), kept for the layout; iterating
    // here starts at m_used and ends at a null bucket and value.
    /*  +0 */ int m_i0;                  // never written by the constructor
    /*  +4 */ int m_i4;                  // never written by the constructor
    /*  +8 */ Hash* m_self0;             // = this
    /* +12 */ int m_i12;                 // = 0
    /* +16 */ int m_i16;                 // = 0
    /* +20 */ Hash* m_self1;             // = this
    /* +24 */ Bucket* m_buckets;         // n + 1 of them
    /* +28 */ Bucket* m_used;            // = 0
    /* +32 */ int m_size;                // = 0
    /* +36 */ int m_capacity;            // = n
    /* +40 */ NodeManager* m_nodeManager;// = 0, written by the owner right after construction

private:
    Bucket& bucketOf(const SearchNode* value) const
    {
        const int key = value->getKey();
        const int index = key % m_capacity;
        return m_buckets[index < 0 ? index + m_capacity : index];
    }
};

// 0x3c24b0 (Ghidra).
template <class Node, class NodeManager, class SearchNode>
void Hash<Node, NodeManager, SearchNode>::insert(SearchNode* value)
{
    Bucket& bucket = bucketOf(value);
    SearchNode* const head = bucket.m_head;
    if (!head)
    {
        bucket.m_next = m_used;
        if (m_used)
            m_used->m_previous = &bucket;
        bucket.m_previous = 0;
        m_used = &bucket;
    }
    else
        head->m_previous = value;
    value->m_next = head;
    value->m_previous = 0;
    bucket.m_head = value;
    ++m_size;
}

// 0x3c28b0 (Ghidra). By key.
template <class Node, class NodeManager, class SearchNode>
SearchNode* Hash<Node, NodeManager, SearchNode>::get(const SearchNode* value) const
{
    const int key = value->getKey();
    for (SearchNode* at = bucketOf(value).m_head; at; at = at->m_next)
    {
        if (at->getKey() == key)
            return at;
    }
    return 0;
}

// 0x3c2970 (Ghidra). The value itself, not by key.
template <class Node, class NodeManager, class SearchNode>
void Hash<Node, NodeManager, SearchNode>::remove(SearchNode* value)
{
    if (!value->m_previous)
    {
        Bucket& bucket = bucketOf(value);
        bucket.m_head = value->m_next;
        if (!bucket.m_head)
        {
            if (m_used == &bucket)
                m_used = bucket.m_next;
            else
                bucket.m_previous->m_next = bucket.m_next;
            if (bucket.m_next)
                bucket.m_next->m_previous = bucket.m_previous;
        }
    }
    else
        value->m_previous->m_next = value->m_next;
    --m_size;
    if (value->m_next)
        value->m_next->m_previous = value->m_previous;
}

// Inlined where the searches restart.
template <class Node, class NodeManager, class SearchNode>
void Hash<Node, NodeManager, SearchNode>::clear()
{
    m_size = 0;
    for (Bucket* bucket = m_used; bucket; bucket = bucket->m_next)
        bucket->m_head = 0;
    m_used = 0;
}

// The anonymous NodeCompare of the binary: cheapest first.
template <class SearchNode>
class NodeCompare
{
public:
    bool operator()(const SearchNode* a, const SearchNode* b) const { return a->m_f < b->m_f; }
};

// One A* search, stepped against a deadline. The open list has two tiers: a small tree of the
// cheapest nodes and a hash of the rest (m_openLimit is the dearest node the tree took).
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
class AStarResource
{
public:
    explicit AStarResource(NodeManager* nodeManager)
        : m_status(0)
        , m_maxIterations(0)
        , m_maxSearchDepth(-1)
        , m_iterations(0)
        , m_openSize(0)
        , m_hashSize(0)
        , m_i28(-1)
        , m_nodeManager(nodeManager)
        , m_openLimit(0)
        , m_open(0)
        , m_openCache(0)
        , m_closedNodes(0)
        , m_found(0)
    {
        // m_i4 deliberately not initialised, as shipped.
    }

    void update(double deadline);
    void pushOpenNode(SearchNode* node);
    SearchNode* popOpenNode();
    void handleOpenCacheUnderflow();

    /*  +0 */ int m_status;          // 0 idle, 1 running, 2 found, 3 and 4 failed
    /*  +4 */ int m_i4;              // never written
    /*  +8 */ int m_maxIterations;   // = 0 [INFERRED] - `setMaxIterationsForResource` writes +8
    /* +12 */ int m_maxSearchDepth;  // = -1 [INFERRED] - `setMaxSearchDepthForResource` writes +12
    /* +16 */ int m_iterations;      // = 0
    /* +20 */ int m_openSize;        // = the int handed to BinarySearchTree's constructor
    /* +24 */ int m_hashSize;        // = the int handed to both Hash constructors
    /* +28 */ int m_i28;             // = -1; the search's owner (acquire / release)
    /* +32 */ NodeManager* m_nodeManager;
    /* +36 */ SearchNode* m_openLimit;   // = 0
    /* +40 */ BinarySearchTree<Node, SearchNode>* m_open;
    /* +44 */ Hash<Node, NodeManager, SearchNode>* m_openCache;
    /* +48 */ std::vector<SearchNode*> m_scratch;    // only ever emptied
    /* +60 */ Hash<Node, NodeManager, SearchNode>* m_closedNodes;
    /* +64 */ SearchNode* m_found;   // = 0; the node that reached the goal
};

// 0x3c2410 (Ghidra).
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
void AStarResource<Node, NodeManager, SearchNode, Key, Path>::pushOpenNode(SearchNode* node)
{
    m_nodeManager->setNodeOpen(node);
    if (m_open->m_size == 0 || m_openLimit->m_f <= node->m_f)
    {
        m_openCache->insert(node);
        return;
    }
    if (m_openSize <= m_open->m_size)
    {
        do
            m_openCache->insert(m_open->pop());
        while (m_open->m_size != 0);
    }
    m_open->insert(node);
}

// 0x3c2860 (Ghidra).
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
SearchNode* AStarResource<Node, NodeManager, SearchNode, Key, Path>::popOpenNode()
{
    for (;;)
    {
        if (m_open->m_size != 0)
            return m_open->pop();
        if (m_openCache->m_size == 0)
            return 0;
        handleOpenCacheUnderflow();
    }
}

// 0x3c0830 (Ghidra). Moves the m_openSize cheapest cached nodes into the tree.
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
void AStarResource<Node, NodeManager, SearchNode, Key, Path>::handleOpenCacheUnderflow()
{
    typedef Hash<Node, NodeManager, SearchNode> Cache;
    m_scratch.clear();
    typename Cache::Bucket* bucket = m_openCache->m_size != 0 ? m_openCache->m_used : 0;
    SearchNode* value = bucket ? bucket->m_head : 0;
    if (!bucket && !value)
        return;
    std::multiset<SearchNode*, NodeCompare<SearchNode> > cheapest;
    SearchNode* dearest = value;
    for (int i = 0; i < m_openSize && (bucket || value); ++i)
    {
        cheapest.insert(value);
        if (dearest->m_f < value->m_f)
            dearest = value;
        value = value->m_next;
        if (!value)
        {
            bucket = bucket->m_next;
            if (bucket)
                value = bucket->m_head;
        }
    }
    while (bucket || value)
    {
        SearchNode* const next = value;
        const bool take = value->m_f <= dearest->m_f;
        if (take)
        {
            typename std::multiset<SearchNode*, NodeCompare<SearchNode> >::iterator last = cheapest.end();
            --last;
            cheapest.erase(last);
            cheapest.insert(next);
        }
        value = value->m_next;
        if (!value)
        {
            bucket = bucket->m_next;
            if (bucket)
                value = bucket->m_head;
        }
        if (take)
        {
            typename std::multiset<SearchNode*, NodeCompare<SearchNode> >::iterator last = cheapest.end();
            --last;
            dearest = *last;
        }
    }
    for (typename std::multiset<SearchNode*, NodeCompare<SearchNode> >::iterator it = cheapest.begin();
         it != cheapest.end(); ++it)
    {
        m_openCache->remove(*it);
        m_open->insert(*it);
    }
    m_openLimit = dearest;
}

// 0x3beea0 (QTSearchNode), 0x3bf400 (ClusterNode) (Ghidra). Steps until the deadline: the cheapest
// open node's neighbours are looked at (the one it came from is skipped), then it closes. Status
// of a neighbour: 0 new, 1 open, 2 closed (either may get a cheaper way), 3 the goal, 4 none.
// Running out of iterations or depth, or of open nodes, fails the search (3).
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
void AStarResource<Node, NodeManager, SearchNode, Key, Path>::update(double deadline)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    if (m_status != 1)
        return;
    do
    {
        if (deadline <= timer->getUpdateTime())
            return;
        SearchNode* node = 0;
        if (m_open->m_size != 0)
            node = m_open->pop();
        else if (m_openCache->m_size != 0)
        {
            handleOpenCacheUnderflow();
            node = popOpenNode();
        }
        if (!node)
        {
            m_status = 3;
            return;
        }
        int status = 0;
        unsigned short edge = 0;
        SearchNode* child = node->expandFirst(m_nodeManager, status, edge);
        for (;;)
        {
            while (child && node->m_parent && child->getKey() == node->m_parent->getKey())
            {
                m_nodeManager->recycleNode(child);
                child = node->expand(m_nodeManager, status, edge);
            }
            if (!child)
                break;
            if (status == 0)
                pushOpenNode(child);
            else if (status == 2)
            {
                SearchNode* const existing = m_closedNodes->get(child);
                if (m_nodeManager->updateNode(node, existing, static_cast<unsigned int>(m_maxSearchDepth)))
                {
                    m_closedNodes->remove(existing);
                    pushOpenNode(existing);
                }
                m_nodeManager->recycleNode(child);
            }
            else if (status == 1)
            {
                SearchNode* existing = m_openCache->get(child);
                if (!existing)
                {
                    typename BinarySearchTree<Node, SearchNode>::Entry* const entry =
                        m_open->get(child, m_open->m_root);
                    existing = entry ? entry->m_value : 0;
                }
                if (m_nodeManager->updateNode(node, existing, static_cast<unsigned int>(m_maxSearchDepth)))
                {
                    SearchNode* const cached = m_openCache->get(child);
                    if (cached)
                        m_openCache->remove(cached);
                    else
                        m_open->remove(child);
                    pushOpenNode(existing);
                }
                m_nodeManager->recycleNode(child);
            }
            else if (status == 3)
            {
                m_found = child;
                child->m_parent = node;
                m_nodeManager->setNodeClosed(node);
                m_closedNodes->insert(node);
                m_status = 2;
                return;
            }
            else if (status == 4)
                m_nodeManager->recycleNode(child);
            child = node->expand(m_nodeManager, status, edge);
        }
        const int depth = static_cast<int>(node->m_depth);
        m_nodeManager->setNodeClosed(node);
        m_closedNodes->insert(node);
        ++m_iterations;
        if (m_iterations == m_maxIterations || m_maxSearchDepth == depth)
            break;
    } while (true);
    m_status = 3;
}

// Pathfinding engine and its pool of reusable search resources.
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
class PathfindingEngine
{
public:
    PathfindingEngine() {}

    // [INFERRED] Steps active searches round-robin until now + budget, at most once per
    // search per call, moving each processed search from front to back.
    void update(double budget);

    /*  +0 */ std::vector<AStarResource<Node, NodeManager, SearchNode, Key, Path>*> m_searches;
    /* +12 */ std::list<unsigned int> m_available;    // [INFERRED] the free list
    /* +16 */ std::list<unsigned int> m_inFlight;     // [INFERRED] - isLockComplete waits on this one
};

// The timer is fetched once; the in-flight count once, before the pass (libstdc++'s O(n) size).
// The walk keeps its own iterator but always erases the front and re-appends its id, which is the
// same element on every step because each processed id moves to the back. The deadline test comes
// before the count test in both images.
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
void PathfindingEngine<Node, NodeManager, SearchNode, Key, Path>::update(double budget)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double deadline = timer->getUpdateTime() + budget;
    const unsigned int count = static_cast<unsigned int>(m_inFlight.size());
    unsigned int processed = 0;
    std::list<unsigned int>::iterator it = m_inFlight.begin();
    while (deadline > timer->getUpdateTime() && processed < count)
    {
        ++processed;
        const unsigned int id = *it;
        m_searches[id]->update(deadline);
        ++it;
        m_inFlight.erase(m_inFlight.begin());
        m_inFlight.insert(m_inFlight.end(), id);
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_AIPATHFINDINGSEARCHTYPES_H
