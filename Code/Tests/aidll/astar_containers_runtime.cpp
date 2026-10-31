// The A* open tree and hash against std containers, random operations. Compiles the real header.
#include <cstdio>
#include <cstdlib>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "Main/AIPathfindingSearchTypes.h"

using namespace dice::hfe::ai;

struct TestNode
{
    TestNode* m_next;
    TestNode* m_previous;
    int m_key;
    float m_f;
    int getKey() const { return m_key; }
};

typedef BinarySearchTree<TestNode, TestNode> Tree;
typedef Hash<TestNode, int, TestNode> Table;

static int failures = 0;
static void check(bool ok, const char* what, int step)
{
    if (!ok && failures++ < 10)
        std::printf("FAIL %s at step %d\n", what, step);
}

static void walk(Tree::Entry* e, int& count, float low, float high, int step)
{
    if (!e)
        return;
    ++count;
    const float f = e->m_value->m_f;
    check(f >= low && f <= high, "tree order", step);
    if (e->m_left)
        check(e->m_left->m_parent == e, "left parent", step);
    if (e->m_right)
        check(e->m_right->m_parent == e, "right parent", step);
    walk(e->m_left, count, low, f, step);
    walk(e->m_right, count, f, high, step);
}

int main()
{
    std::srand(12345);
    std::vector<TestNode*> nodes;
    for (int i = 0; i < 2000; ++i)
    {
        TestNode* const n = new TestNode();
        n->m_key = i - 500;          // negative keys too: the signed bucket index
        n->m_f = 0.0f;
        nodes.push_back(n);
    }

    // the tree against a multimap
    Tree tree(21);
    std::multimap<float, TestNode*> model;
    std::set<TestNode*> inTree;
    for (int step = 0; step < 300000; ++step)
    {
        const int op = std::rand() % 10;
        if (op < 5)
        {
            TestNode* const n = nodes[std::rand() % nodes.size()];
            if (inTree.count(n))
                continue;
            n->m_f = static_cast<float>(std::rand() % 400) * 0.25f;
            tree.insert(n);
            model.insert(std::make_pair(n->m_f, n));
            inTree.insert(n);
        }
        else if (op < 8)
        {
            TestNode* const popped = tree.pop();
            if (model.empty())
            {
                check(popped == 0, "pop empty", step);
                continue;
            }
            check(popped != 0, "pop value", step);
            if (!popped)
                continue;
            check(popped->m_f == model.begin()->first, "pop is cheapest", step);
            std::multimap<float, TestNode*>::iterator it = model.lower_bound(popped->m_f);
            while (it != model.end() && it->second != popped)
                ++it;
            check(it != model.end(), "pop known", step);
            if (it != model.end())
                model.erase(it);
            inTree.erase(popped);
        }
        else if (op == 8 && !inTree.empty())
        {
            std::set<TestNode*>::iterator pick = inTree.begin();
            std::advance(pick, std::rand() % inTree.size());
            TestNode* const n = *pick;
            tree.remove(n);
            std::multimap<float, TestNode*>::iterator it = model.lower_bound(n->m_f);
            while (it != model.end() && it->second != n)
                ++it;
            if (it != model.end())
                model.erase(it);
            inTree.erase(n);
        }
        else if (op == 9 && std::rand() % 200 == 0)
        {
            tree.clear();
            model.clear();
            inTree.clear();
        }
        check(tree.m_size == static_cast<int>(model.size()), "tree size", step);
        if (tree.m_size != 0 && step % 50 == 0)
        {
            check(tree.m_leftmost->m_value->m_f == model.begin()->first, "leftmost", step);
            int count = 0;
            check(tree.m_root->m_parent == 0, "root parent", step);
            walk(tree.m_root, count, -1.0f, 1e9f, step);
            check(count == tree.m_size, "reachable", step);
        }
    }

    // the hash against a set of keys
    Table table(31);
    std::set<TestNode*> inTable;
    for (int step = 0; step < 300000; ++step)
    {
        TestNode* const n = nodes[std::rand() % nodes.size()];
        if (inTable.count(n))
        {
            check(table.get(n) == n, "hash get", step);
            table.remove(n);
            inTable.erase(n);
            check(table.get(n) == 0, "hash gone", step);
        }
        else
        {
            table.insert(n);
            inTable.insert(n);
            check(table.get(n) == n, "hash inserted", step);
        }
        check(table.m_size == static_cast<int>(inTable.size()), "hash size", step);
        if (step % 500 == 0)
        {
            int count = 0;
            for (Table::Bucket* b = table.m_used; b; b = b->m_next)
            {
                check(b->m_head != 0, "used bucket empty", step);
                for (TestNode* v = b->m_head; v; v = v->m_next)
                    ++count;
            }
            check(count == table.m_size, "hash walk", step);
        }
        if (std::rand() % 5000 == 0)
        {
            table.clear();
            inTable.clear();
            check(table.m_used == 0 && table.m_size == 0, "hash clear", step);
        }
    }
    std::printf(failures ? "containers: %d failures\n" : "containers: ok (%d failures)\n", failures);
    return failures != 0;
}
