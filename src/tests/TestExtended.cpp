#include "doctest.h"
#include "zet.hpp"

#include <array>
#include <ostream>
#include <string_view>

std::size_t AllocationCount() noexcept;

TEST_CASE("Fixed containers report capacity without allocation") {
    CHECK(std::string_view(zet::LibraryVersion()) == "2.0.0");
    const auto before = AllocationCount();
    zet::BitSet<70> bits;
    bits.Set(0); bits.Set(69);
    CHECK(bits.Count() == 2);
    CHECK(bits.FindNextSet(1) == 69);

    zet::RingBuffer<int, 2> queue;
    CHECK(queue.TryPushBack(1));
    CHECK(queue.TryPushBack(2));
    CHECK_FALSE(queue.TryPushBack(3));
    CHECK(*queue.TryFront() == 1);
    CHECK(queue.TryPopFront());
    CHECK(*queue.TryFront() == 2);

    zet::FixedDeque<int, 2> deque;
    CHECK(deque.TryEmplaceFront(2));
    CHECK(deque.TryEmplaceFront(1));
    CHECK(*deque.TryFront() == 1);
    CHECK(*deque.TryBack() == 2);

    zet::FixedPriorityQueue<int, 4> priority;
    CHECK(priority.TryPush(2));
    CHECK(priority.TryPush(5));
    CHECK(priority.TryPush(3));
    CHECK(*priority.TryPeek() == 5);
    CHECK(priority.TryPop());
    CHECK(*priority.TryPeek() == 3);

    zet::FixedFlatMap<int, int, 3> flatMap;
    CHECK(flatMap.TryInsert(2, 20) != nullptr);
    CHECK(flatMap.TryInsert(1, 10) != nullptr);
    CHECK(*flatMap.Find(1) == 10);
    CHECK(flatMap.TryErase(2));
    CHECK(AllocationCount() == before);
}

TEST_CASE("Checked APIs preserve bounded-container state") {
    zet::List<int, 1> list;
    CHECK(list.TryPush(1));
    CHECK_FALSE(list.TryPush(2));
    CHECK(list.Size() == 1);
    CHECK(list.TryGet(1) == nullptr);

    zet::Map<int, int, 1> map;
    auto first = map.TryEmplace(1, 10);
    CHECK(first.Inserted);
    CHECK(first.Value != nullptr);
    CHECK(map.TryEmplace(2, 20).Value == nullptr);
    CHECK(*map.Find(1) == 10);

    zet::Pool<int, 1> firstPool;
    zet::Pool<int, 1> secondPool;
    const auto firstHandle = firstPool.TryCreate(1);
    const auto secondHandle = secondPool.TryCreate(2);
    CHECK_FALSE(firstPool.IsValid(secondHandle));
    CHECK(firstPool.TryDestroy(firstHandle));
    CHECK_FALSE(firstPool.TryDestroy(firstHandle));

    zet::String<5> string("ab");
    string += string.CStr();
    CHECK(string == "abab");
    CHECK_FALSE(string.TryAppend("x"));
    CHECK(string == "abab");
}

TEST_CASE("Allocators expose safe capacity and marker operations") {
    zet::memory::LinearAllocator empty(nullptr, 0);
    CHECK(empty.TryAllocate(1) == nullptr);

    alignas(64) std::array<std::byte, 64> storage{};
    zet::memory::StackAllocator allocator(storage);
    const auto marker = allocator.GetMarker();
    auto handle = allocator.CreateHandle<int>(7);
    CHECK(handle.Get() != nullptr);
    CHECK(allocator.TryFreeToMarker(marker));
    CHECK(handle.Get() == nullptr);
    CHECK_FALSE(allocator.TryFreeToMarker(marker));
}

TEST_CASE("Hierarchy supports parent-child traversal and subtree removal") {
    zet::Hierarchy<int, 8> hierarchy;
    const auto root = hierarchy.TryCreateRoot(1);
    const auto child = hierarchy.TryAddChild(root, 2);
    const auto grandchild = hierarchy.TryAddChild(child, 3);
    CHECK(hierarchy.Parent(child) == root);
    zet::Hierarchy<int, 8>::DepthFirstScratch scratch;
    int sum = 0;
    CHECK(hierarchy.DepthFirst(root, scratch, [&](auto, int value) { sum += value; }));
    CHECK(sum == 6);
    CHECK(hierarchy.TryRemoveSubtree(child, scratch));
    CHECK(hierarchy.Size() == 1);
    CHECK_FALSE(hierarchy.IsValid(grandchild));
}

TEST_CASE("FixedGraph traverses and topologically sorts bounded graphs") {
    zet::FixedGraph<int, int, 4, 4> graph;
    const auto a = graph.TryAddVertex(1);
    const auto b = graph.TryAddVertex(2);
    const auto c = graph.TryAddVertex(3);
    CHECK(graph.TryAddEdge(a, b, 10) != zet::INVALID_POOL_HANDLE);
    CHECK(graph.TryAddEdge(b, c, 20) != zet::INVALID_POOL_HANDLE);
    zet::FixedGraph<int, int, 4, 4>::TraversalScratch scratch;
    int sum = 0;
    CHECK(graph.BreadthFirst(a, scratch, [&](auto, int value) { sum += value; return true; }));
    CHECK(sum == 6);
    std::array<zet::PoolHandle, 4> output{};
    CHECK(graph.TopologicalSort(output, scratch) == zet::Status::Success);
    CHECK(output[0] == a);
    CHECK(output[1] == b);
    CHECK(output[2] == c);

    zet::FixedGraph<int, int, 4, 4>::PathScratch pathScratch;
    std::array<zet::PoolHandle, 4> path{};
    CHECK(graph.Dijkstra(a, c, path, pathScratch, [](int weight) { return static_cast<float>(weight); }) == zet::Status::Success);
    CHECK(path[0] == a);
    CHECK(path[2] == c);

    zet::FixedGraph<int, int, 2, 2, false> undirected;
    const auto u = undirected.TryAddVertex(4);
    const auto v = undirected.TryAddVertex(5);
    const auto uv = undirected.TryAddEdge(u, v, 1);
    int neighborCount = 0;
    CHECK(undirected.ForEachNeighbor(v, [&](auto neighbor, int) { neighborCount += neighbor == u; return true; }));
    CHECK(neighborCount == 1);
    CHECK(undirected.TryRemoveEdge(uv));
    CHECK(undirected.EdgeCount() == 0);

    zet::StaticGraph<int, int, 4, 4> staticGraph;
    const auto staticA = staticGraph.TryAddVertex(1);
    const auto staticB = staticGraph.TryAddVertex(2);
    CHECK(staticGraph.TryAddEdge(staticA, staticB, 7));
    CHECK(staticGraph.Build() == zet::Status::Success);
    int staticSum = 0;
    CHECK(staticGraph.ForEachNeighbor(staticA, [&](auto neighbor) { staticSum += neighbor.Value; return true; }));
    CHECK(staticSum == 7);

    zet::SpscQueue<int, 3> spsc;
    int spscValue = 0;
    CHECK(spsc.TryPush(11));
    CHECK(spsc.TryPop(spscValue));
    CHECK(spscValue == 11);
}
