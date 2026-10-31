// Polygon A*: the real QTSearchNodeManager.cpp and AStarResource on a grid mesh with walls and slow
// ground, against a breadth-first search, one search resource over 4000 runs. The QuadTree accessors
// are copies of the real ones (QuadTree.cpp needs the whole engine to link).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#define private public
#include "AIPathFinding/NavMesh/QuadTree.h"
#undef private
#include "AIPathFinding/NavMesh/QTSearchNodeManager.h"
#include "AIPathFinding/Material/MaterialManager.h"
#include "Main/AIPathfindingSearchTypes.h"
#include "Main/CommonSingletonServer.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/BFTimer.h"
#include "../../BF2/Core/Debug.h"

namespace dice {
namespace hfe {
Debug::Debug(DebugType, const std::string&, int, const std::string&) {}
Debug::~Debug() {}
namespace ai {

static float g_costs[9];

void* CommonSingletonServer::m_iBFTimer = 0;
IBFTimer::IBFTimer() {}
IBFTimer::~IBFTimer() {}
float MaterialManager::getMaterialCost(unsigned char material) const { return g_costs[material]; }

const Polygon* QuadTree::getPolygon(unsigned int polygon) const
{
    return polygon < m_numberOfPolygons ? &m_polygons[polygon] : 0;
}
unsigned char QuadTree::getMaterialOfPolygon(unsigned int polygon) const { return m_polygons[polygon].m_material; }
unsigned int QuadTree::getNeighbourOfPolygon(unsigned int polygon, unsigned char edge) const
{
    const Polygon& record = m_polygons[polygon];
    return edge < record.m_numberOfVertices ? m_neighbours[edge + record.m_firstIndex] : 0xFFFFFFFFu;
}
const Vec3& QuadTree::getVertexForPolygon(unsigned int polygon, unsigned char corner) const
{
    return m_vertices[m_vertexIndices[corner + m_polygons[polygon].m_firstIndex]];
}
const Vec3& QuadTree::getSafeVertexForPolygon(unsigned int polygon, unsigned char corner) const
{
    const Polygon& record = m_polygons[polygon];
    return m_vertices[m_vertexIndices[corner % record.m_numberOfVertices + record.m_firstIndex]];
}
const Vec3& QuadTree::getNormalOfPolygon(unsigned int polygon) const { return m_polygons[polygon].m_normal; }
bool QuadTree::getNeighbourIndexForPolygon(unsigned char& edge, unsigned int polygon, unsigned int neighbour) const
{
    const Polygon& record = m_polygons[polygon];
    edge = 0;
    unsigned int at = m_neighbours[record.m_firstIndex];
    for (;;)
    {
        if (at == neighbour)
            return true;
        ++edge;
        if (record.m_numberOfVertices <= edge)
            return false;
        at = m_neighbours[edge + record.m_firstIndex];
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice

namespace ai = dice::hfe::ai;
using dice::hfe::Vec3;
using namespace dice::hfe::ai;

class FakeTimer : public IBFTimer
{
public:
    virtual double getGameTime() const { return 0.0; }
    virtual double getTimeIncrease() const { return 0.0; }
    virtual double getUpdateTime() const { return 0.0; }
    virtual double getWallClockTime() const { return 0.0; }
    virtual void setPause(bool) {}
    virtual bool getPause() { return false; }
    virtual void reset() {}
};

typedef AStarResource<QTSearchNode, QTSearchNodeManager, QTSearchNode, unsigned short, QTPath> Search;

static const int N = 24;

// The same steps as AIPathfinding::internalLocalSearch.
static int startSearch(Search& search, QuadTree* tree, MaterialManager* materials, unsigned int from,
                       unsigned int to, const Vec3& fromAt, const Vec3& toAt, QTPath& path)
{
    QTSearchNodeManager* const manager = search.m_nodeManager;
    path.m_sentinel16 = 0;
    manager->setQuadTree(tree);
    QTSearchNode* const start = manager->getNewNode(from, 0, 0);
    start->m_entry = fromAt;
    start->m_g = 0.0f;
    QTSearchNode* const goal = manager->getNewNode(to, 0, 0);
    goal->m_entry = toAt;
    manager->m_materials = materials;
    search.m_iterations = 0;
    search.m_status = 0;
    search.m_openLimit = 0;
    search.m_open->clear();
    search.m_openCache->clear();
    search.m_closedNodes->clear();
    manager->reset();
    if (start->m_polygon == goal->m_polygon)
    {
        manager->recycleNode(goal);
        search.m_status = 2;
        search.m_found = start;
    }
    else
    {
        manager->setGoal(goal);
        search.pushOpenNode(start);
        search.m_status = 1;
    }
    return search.m_status;
}

int main()
{
    FakeTimer timer;
    CommonSingletonServer::m_iBFTimer = static_cast<IBFTimer*>(&timer);
    g_costs[0] = 1.0f;
    g_costs[1] = -1.0f;      // a wall
    g_costs[2] = 3.0f;       // slow ground

    const int cells = N * N;
    std::vector<Polygon> polygons(cells);
    std::vector<Vec3> vertices((N + 1) * (N + 1));
    std::vector<unsigned int> indices(cells * 4);
    std::vector<unsigned int> neighbours(cells * 4);
    for (int z = 0; z <= N; ++z)
        for (int x = 0; x <= N; ++x)
        {
            Vec3& v = vertices[z * (N + 1) + x];
            v.x = static_cast<float>(x);
            v.y = 0.0f;
            v.z = static_cast<float>(z);
        }
    for (int z = 0; z < N; ++z)
        for (int x = 0; x < N; ++x)
        {
            const int c = z * N + x;
            polygons[c].m_numberOfVertices = 4;
            polygons[c].m_firstIndex = c * 4;
            polygons[c].m_cluster = 0;
            polygons[c].m_normal.x = 0.0f;
            polygons[c].m_normal.y = 1.0f;
            polygons[c].m_normal.z = 0.0f;
            indices[c * 4 + 0] = z * (N + 1) + x;
            indices[c * 4 + 1] = z * (N + 1) + x + 1;
            indices[c * 4 + 2] = (z + 1) * (N + 1) + x + 1;
            indices[c * 4 + 3] = (z + 1) * (N + 1) + x;
            neighbours[c * 4 + 0] = z > 0 ? c - N : 0xFFFFFFFFu;
            neighbours[c * 4 + 1] = x < N - 1 ? c + 1 : 0xFFFFFFFFu;
            neighbours[c * 4 + 2] = z < N - 1 ? c + N : 0xFFFFFFFFu;
            neighbours[c * 4 + 3] = x > 0 ? c - 1 : 0xFFFFFFFFu;
        }
    char* const raw = static_cast<char*>(std::calloc(1, sizeof(QuadTree)));
    QuadTree* const tree = reinterpret_cast<QuadTree*>(raw);
    tree->m_polygons = &polygons[0];
    tree->m_numberOfPolygons = cells;
    tree->m_vertices = &vertices[0];
    tree->m_vertexIndices = &indices[0];
    tree->m_neighbours = &neighbours[0];

    Search search(new QTSearchNodeManager());
    search.m_maxIterations = 30000;
    search.m_openSize = 21;
    search.m_hashSize = 1031;
    search.m_open = new BinarySearchTree<QTSearchNode, QTSearchNode>(21);
    search.m_openCache = new ai::Hash<QTSearchNode, QTSearchNodeManager, QTSearchNode>(1031);
    search.m_closedNodes = new ai::Hash<QTSearchNode, QTSearchNodeManager, QTSearchNode>(1031);
    search.m_openCache->m_nodeManager = search.m_nodeManager;
    search.m_closedNodes->m_nodeManager = search.m_nodeManager;

    MaterialManager* const materials = reinterpret_cast<MaterialManager*>(std::calloc(1, 64));
    std::srand(4242);
    int failures = 0, found = 0, none = 0, trivial = 0;
    double ratioSum = 0.0, ratioMax = 0.0;
    long totalSteps = 0;
    for (int run = 0; run < 4000; ++run)
    {
        const int wallChance = std::rand() % 40;
        for (int c = 0; c < cells; ++c)
        {
            const int roll = std::rand() % 100;
            polygons[c].m_material = static_cast<unsigned char>(roll < wallChance ? 1 : (roll < wallChance + 10 ? 2 : 0));
        }
        int from = std::rand() % cells;
        int to = std::rand() % cells;
        polygons[from].m_material = 0;
        polygons[to].m_material = 0;

        // breadth first, the truth about reachability and the fewest steps
        std::vector<int> dist(cells, -1);
        std::queue<int> frontier;
        dist[from] = 0;
        frontier.push(from);
        while (!frontier.empty())
        {
            const int c = frontier.front();
            frontier.pop();
            for (int e = 0; e < 4; ++e)
            {
                const unsigned int n = neighbours[c * 4 + e];
                if (n != 0xFFFFFFFFu && polygons[n].m_material != 1 && dist[n] < 0)
                {
                    dist[n] = dist[c] + 1;
                    frontier.push(n);
                }
            }
        }

        Vec3 fromAt = { (from % N) + 0.5f, 0.0f, (from / N) + 0.5f };
        Vec3 toAt = { (to % N) + 0.5f, 0.0f, (to / N) + 0.5f };
        QTPath path;
        startSearch(search, tree, materials, from, to, fromAt, toAt, path);
        search.update(1.0);
        const bool reachable = dist[to] >= 0;
        if (search.m_status == 1)
        {
            ++failures;
            std::printf("run %d: still running after the deadline\n", run);
            continue;
        }
        if (reachable != (search.m_status == 2))
        {
            ++failures;
            std::printf("run %d: status %d but reachable %d (%d -> %d)\n", run, search.m_status, reachable, from, to);
            continue;
        }
        if (!reachable)
        {
            ++none;
            continue;
        }
        path.m_nodes.clear();
        search.m_nodeManager->buildPath(&path, search.m_found);
        const std::vector<triple<unsigned int, unsigned int, Vec3> >& nodes = path.m_nodes;
        bool ok = !nodes.empty() && static_cast<int>(nodes.front().first) == from
            && static_cast<int>(nodes.back().first) == to;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (polygons[nodes[i].first].m_material == 1)
                ok = false;
            if (i > 0)
            {
                bool adjacent = false;
                for (int e = 0; e < 4; ++e)
                    adjacent = adjacent || neighbours[nodes[i - 1].first * 4 + e] == nodes[i].first;
                ok = ok && adjacent;
            }
        }
        if (!ok)
        {
            ++failures;
            std::printf("run %d: bad path of %u nodes (%d -> %d)\n", run, static_cast<unsigned int>(nodes.size()), from, to);
            continue;
        }
        if (from == to)
        {
            ++trivial;
            continue;
        }
        ++found;
        totalSteps += search.m_iterations;
        const double ratio = static_cast<double>(nodes.size() - 1) / dist[to];
        ratioSum += ratio;
        if (ratio > ratioMax)
            ratioMax = ratio;
    }
    std::printf("found %d, unreachable %d, same polygon %d, failures %d; polygons/best mean %.3f max %.3f; mean iterations %.1f\n",
                found, none, trivial, failures, found ? ratioSum / found : 0.0, ratioMax,
                found ? static_cast<double>(totalSteps) / found : 0.0);
    return failures != 0;
}
