// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining ClusterGraph operations.

#include "ClusterGraph.h"

#include "../../AIUtils/checkPointer.h"
#include "../../../../../IO/File/File.h"

namespace dice {
namespace hfe {
namespace ai {

// Zeroes the six fields at +0x00..+0x14 and the three at +0x24..+0x2C; the two sets
// construct empty (their sentinel allocations are the only calls). is bigger only because the
// SGI node allocator is inlined into it - same source, two inlining decisions.
ClusterGraph::ClusterGraph()
    : m_clusterPolygons(0)
    , m_numberOfClusterPolygons(0)
    , m_clusters(0)
    , m_numberOfClusters(0)
    , m_numberOfPortals(0)
    , m_portals(0)
    , m_array24(0)
    , m_numberOfPortalCosts(0)
    , m_portalCosts(0)
{
}

// clean, then the two sets in reverse declaration order.
ClusterGraph::~ClusterGraph()
{
    clean();
}

// Releases arrays and clears counts, but leaves four freed pointers unchanged.
// Do not clean twice without repopulating those pointers; readFromFile assigns them
// immediately after cleanup. Only the pointer at +0x24 is cleared.
void ClusterGraph::clean()
{
    delete[] m_clusterPolygons;
    m_numberOfClusterPolygons = 0;

    delete[] m_clusters;
    m_numberOfClusters = 0;

    delete[] m_portals;
    m_numberOfPortals = 0;

    delete[] m_array24;
    m_array24 = 0;

    delete[] m_portalCosts;
    m_numberOfPortalCosts = 0;
}

// Loads a version-5 cluster graph. Unsupported versions leave the existing graph unchanged.
void ClusterGraph::readFromFile(io::File& file)
{
    int version = 0;
    file.read(version);

    if (version == 5)
    {
        clean();

        file.read(m_numberOfClusters);
        m_clusters = checkPointer(new Cluster[m_numberOfClusters]);
        file.read(m_clusters, sizeof(Cluster) * m_numberOfClusters);

        file.read(m_numberOfClusterPolygons);
        m_clusterPolygons = checkPointer(new unsigned int[m_numberOfClusterPolygons]);
        file.read(m_clusterPolygons, sizeof(unsigned int) * m_numberOfClusterPolygons);

        file.read(m_numberOfPortals);
        if (m_numberOfPortals)
        {
            m_portals = checkPointer(new ClusterPortal[m_numberOfPortals]);
            file.read(m_portals, sizeof(ClusterPortal) * m_numberOfPortals);
        }
        else
        {
            m_portals = 0;
        }

        file.read(m_numberOfPortalCosts);
        if (m_numberOfPortalCosts)
        {
            m_portalCosts = checkPointer(new float[m_numberOfPortalCosts]);
            file.read(m_portalCosts, sizeof(float) * m_numberOfPortalCosts);
        }
        else
        {
            m_portalCosts = 0;
        }
    }
}

// 0x324e50 (Ghidra).
const ClusterPortal* ClusterGraph::getClusterPortal(unsigned short cluster, unsigned short row,
                                                    unsigned short column) const
{
    const Cluster& record = m_clusters[cluster];
    return &m_portals[record.m_firstPortal + row * record.m_numberOfPortals + column];
}

// 0x32da70 (Ghidra).
bool ClusterGraph::areNeighbours(unsigned short cluster, unsigned short other) const
{
    const Cluster& record = m_clusters[cluster];
    for (unsigned short i = 0; i < record.m_numberOfPortals; ++i)
    {
        if (m_portals[record.m_firstPortal + i].m_neighbour == other)
            return true;
    }
    return false;
}

// 0x324ea0 (Ghidra).
unsigned short ClusterGraph::getPortalForNeighbour(unsigned short cluster, unsigned short row,
                                                   unsigned short neighbour) const
{
    if (cluster != 0xFFFF)
        return findClusterPortalWithNeighbour(cluster, row, neighbour);
    return 0xFFFF;
}

// 0x32cf90 (Ghidra).
unsigned short ClusterGraph::findClusterPortalWithNeighbour(unsigned short cluster, unsigned short row,
                                                            unsigned short neighbour) const
{
    const Cluster& record = m_clusters[cluster];
    const unsigned short portals = record.m_numberOfPortals;
    for (unsigned short i = 0; i < portals; ++i)
    {
        const unsigned short column = static_cast<unsigned short>(row * portals + i);
        if (m_portals[column + record.m_firstPortal].m_neighbour == neighbour)
            return i;
    }
    return 0xFFFF;
}

// 0x324f30 (Ghidra). The same +0x10 within 0.1 of the same place.
unsigned short ClusterGraph::getCorrespongingPortalInNeighbour(const ClusterPortal* portal, unsigned short row) const
{
    if (portal->m_neighbour == 0xFFFF)
        return 0xFFFF;
    const Cluster& record = m_clusters[portal->m_neighbour];
    const unsigned short portals = record.m_numberOfPortals;
    for (unsigned short i = 0; i < portals; ++i)
    {
        const ClusterPortal& other = m_portals[record.m_firstPortal + portals * row + i];
        if (other.m_int10 != portal->m_int10)
            continue;
        const float dx = portal->m_position.x - other.m_position.x;
        const float dy = portal->m_position.y - other.m_position.y;
        const float dz = portal->m_position.z - other.m_position.z;
        if (dx * dx + dy * dy + dz * dz < 0.01f)
            return i;
    }
    return 0xFFFF;
}

// 0x32cee0 (Ghidra). Each row holds the upper triangle of the cluster's portal pairs.
float ClusterGraph::getCostBetweenClustersPortals(unsigned short cluster, unsigned short portal,
                                                  unsigned short other, unsigned short row) const
{
    unsigned int low = other;
    unsigned int high = portal;
    if (portal < other)
    {
        high = other;
        low = portal;
    }
    const Cluster& record = m_clusters[cluster];
    const unsigned int portals = record.m_numberOfPortals;
    const unsigned int rest = (portals - low) & 0xFFFF;
    const unsigned int pairs = (static_cast<int>(portals * (portals - 1)) / 2) & 0xFFFF;
    const unsigned int restPairs = (static_cast<int>(rest * (rest - 1)) / 2) & 0xFFFF;
    const unsigned int index = record.m_firstPortalCost + pairs * row
        + (((pairs - restPairs) + ((high - low) & 0xFFFF)) & 0xFFFF);
    return m_portalCosts[index - 1];
}

} // namespace ai
} // namespace hfe
} // namespace dice
