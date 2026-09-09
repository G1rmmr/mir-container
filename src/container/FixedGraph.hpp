#pragma once

#include "BitSet.hpp"
#include "List.hpp"
#include "Pool.hpp"
#include "RingBuffer.hpp"
#include "PriorityQueue.hpp"
#include "Status.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <limits>
#include <span>
#include <utility>

namespace zet {
    template <typename Vertex, typename Edge, std::size_t MaxVertices, std::size_t MaxEdges, bool Directed = true>
    requires (MaxVertices > 0) && (MaxEdges > 0)
    class FixedGraph {
    public:
        using VertexHandle = PoolHandle;
        using EdgeHandle = PoolHandle;
        struct PathEntry { VertexHandle Handle{}; float Priority = 0.0F; };
        struct PathEntryCompare { constexpr bool operator()(const PathEntry& lhs, const PathEntry& rhs) const noexcept { return lhs.Priority > rhs.Priority; } };
        struct TraversalScratch {
            BitSet<MaxVertices> Visited;
            RingBuffer<VertexHandle, MaxVertices> Queue;
            List<VertexHandle, MaxVertices> Stack;
            constexpr void Clear() noexcept { Visited.Clear(); Queue.Clear(); Stack.Clear(); }
        };
        struct PathScratch {
            std::array<float, MaxVertices> Scores{};
            std::array<VertexHandle, MaxVertices> Parents{};
            BitSet<MaxVertices> Closed;
            FixedPriorityQueue<PathEntry, MaxEdges + 1, PathEntryCompare> Open;
            constexpr void Clear() noexcept {
                Scores.fill(std::numeric_limits<float>::infinity());
                Parents.fill({});
                Closed.Clear();
                Open.Clear();
            }
        };

    private:
        struct VertexRecord { Vertex Value; EdgeHandle FirstOut{}; };
        struct EdgeRecord { Edge Value; VertexHandle From{}; VertexHandle To{}; EdgeHandle NextFrom{}; EdgeHandle Twin{}; };

    public:
        template <typename... Args>
        requires std::constructible_from<Vertex, Args...>
        [[nodiscard]] VertexHandle TryAddVertex(Args&&... args) {
            return vertices.TryCreate(VertexRecord{ Vertex(std::forward<Args>(args)...), {} });
        }
        template <typename... Args>
        requires std::constructible_from<Edge, Args...>
        [[nodiscard]] EdgeHandle TryAddEdge(VertexHandle from, VertexHandle to, Args&&... args) {
            VertexRecord* source = vertices.TryGet(from);
            if (!source || !vertices.IsValid(to)) return INVALID_POOL_HANDLE;
            EdgeHandle forward = edges.TryCreate(EdgeRecord{ Edge(std::forward<Args>(args)...), from, to, source->FirstOut, {} });
            if (!edges.IsValid(forward)) return INVALID_POOL_HANDLE;
            source->FirstOut = forward;
            if constexpr (!Directed) {
                VertexRecord* destination = vertices.TryGet(to);
                EdgeHandle reverse = edges.TryCreate(EdgeRecord{ edges.Get(forward).Value, to, from, destination->FirstOut, forward });
                if (!edges.IsValid(reverse)) { (void)RemoveOne(forward); return INVALID_POOL_HANDLE; }
                destination->FirstOut = reverse;
                edges.Get(forward).Twin = reverse;
            }
            return forward;
        }
        [[nodiscard]] bool TryRemoveEdge(EdgeHandle edge) {
            EdgeRecord* record = edges.TryGet(edge);
            if (!record) return false;
            const EdgeHandle twin = record->Twin;
            const bool removed = RemoveOne(edge);
            if constexpr (!Directed) if (edges.IsValid(twin)) (void)RemoveOne(twin);
            return removed;
        }
        [[nodiscard]] bool TryRemoveVertex(VertexHandle vertex) {
            if (!vertices.IsValid(vertex)) return false;
            for (std::size_t i = 0; i < MaxEdges; ++i) {
                const EdgeHandle edge = edges.TryHandleAt(i);
                const EdgeRecord* record = edges.TryGet(edge);
                if (record && (record->From == vertex || record->To == vertex)) (void)TryRemoveEdge(edge);
            }
            return vertices.TryDestroy(vertex);
        }
        [[nodiscard]] Vertex* TryGetVertex(VertexHandle vertex) noexcept { VertexRecord* record = vertices.TryGet(vertex); return record ? std::addressof(record->Value) : nullptr; }
        [[nodiscard]] const Vertex* TryGetVertex(VertexHandle vertex) const noexcept { const VertexRecord* record = vertices.TryGet(vertex); return record ? std::addressof(record->Value) : nullptr; }
        [[nodiscard]] Edge* TryGetEdge(EdgeHandle edge) noexcept { EdgeRecord* record = edges.TryGet(edge); return record ? std::addressof(record->Value) : nullptr; }
        [[nodiscard]] const Edge* TryGetEdge(EdgeHandle edge) const noexcept { const EdgeRecord* record = edges.TryGet(edge); return record ? std::addressof(record->Value) : nullptr; }
        [[nodiscard]] bool IsValidVertex(VertexHandle vertex) const noexcept { return vertices.IsValid(vertex); }
        [[nodiscard]] bool IsValidEdge(EdgeHandle edge) const noexcept { return edges.IsValid(edge); }
        [[nodiscard]] std::size_t VertexCount() const noexcept { return vertices.Size(); }
        [[nodiscard]] std::size_t EdgeCount() const noexcept { return Directed ? edges.Size() : edges.Size() / 2; }

        template <typename Visitor>
        [[nodiscard]] bool ForEachNeighbor(VertexHandle vertex, Visitor&& visitor) const {
            const VertexRecord* record = vertices.TryGet(vertex);
            if (!record) return false;
            for (EdgeHandle edge = record->FirstOut; edges.IsValid(edge); edge = edges.Get(edge).NextFrom) {
                const EdgeRecord& current = edges.Get(edge);
                if (!visitor(current.To, current.Value)) return false;
            }
            return true;
        }
        template <typename Visitor>
        [[nodiscard]] bool BreadthFirst(VertexHandle start, TraversalScratch& scratch, Visitor&& visitor) const {
            if (!vertices.IsValid(start)) return false;
            scratch.Clear(); scratch.Visited.Set(start.Index); if (!scratch.Queue.TryPushBack(start)) return false;
            while (!scratch.Queue.Empty()) {
                VertexHandle current = *scratch.Queue.TryFront(); (void)scratch.Queue.TryPopFront();
                visitor(current, vertices.Get(current).Value);
                if (!ForEachNeighbor(current, [&](VertexHandle next, const Edge&) {
                    if (!scratch.Visited.Test(next.Index)) { scratch.Visited.Set(next.Index); return scratch.Queue.TryPushBack(next); }
                    return true;
                })) return false;
            }
            return true;
        }
        template <typename Visitor>
        [[nodiscard]] bool DepthFirst(VertexHandle start, TraversalScratch& scratch, Visitor&& visitor) const {
            if (!vertices.IsValid(start)) return false;
            scratch.Clear(); scratch.Visited.Set(start.Index); if (!scratch.Stack.TryPush(start)) return false;
            while (!scratch.Stack.Empty()) {
                VertexHandle current = scratch.Stack[scratch.Stack.Size() - 1]; scratch.Stack.Pop();
                visitor(current, vertices.Get(current).Value);
                if (!ForEachNeighbor(current, [&](VertexHandle next, const Edge&) {
                    if (!scratch.Visited.Test(next.Index)) { scratch.Visited.Set(next.Index); return scratch.Stack.TryPush(next); }
                    return true;
                })) return false;
            }
            return true;
        }
        [[nodiscard]] Status TopologicalSort(std::span<VertexHandle> output, TraversalScratch& scratch) const requires Directed {
            if (output.size() < VertexCount()) return Status::InsufficientScratch;
            std::array<std::size_t, MaxVertices> degrees{};
            for (std::size_t i = 0; i < MaxEdges; ++i) {
                const EdgeRecord* edge = edges.TryGet(edges.TryHandleAt(i));
                if (edge) ++degrees[edge->To.Index];
            }
            scratch.Clear();
            for (std::size_t i = 0; i < MaxVertices; ++i) {
                VertexHandle vertex = vertices.TryHandleAt(i);
                if (vertices.IsValid(vertex) && degrees[i] == 0 && !scratch.Queue.TryPushBack(vertex)) return Status::InsufficientScratch;
            }
            std::size_t written = 0;
            while (!scratch.Queue.Empty()) {
                VertexHandle vertex = *scratch.Queue.TryFront(); (void)scratch.Queue.TryPopFront(); output[written++] = vertex;
                const bool ok = ForEachNeighbor(vertex, [&](VertexHandle next, const Edge&) {
                    if (--degrees[next.Index] == 0) return scratch.Queue.TryPushBack(next);
                    return true;
                });
                if (!ok) return Status::InsufficientScratch;
            }
            return written == VertexCount() ? Status::Success : Status::CycleDetected;
        }
        template <typename Cost, typename Heuristic>
        [[nodiscard]] Status FindPath(VertexHandle start, VertexHandle goal, std::span<VertexHandle> output, PathScratch& scratch, Cost&& cost, Heuristic&& heuristic) const {
            if (!vertices.IsValid(start) || !vertices.IsValid(goal)) return Status::InvalidHandle;
            scratch.Clear();
            scratch.Scores[start.Index] = 0.0F;
            if (!scratch.Open.TryPush(PathEntry{ start, heuristic(vertices.Get(start).Value) })) return Status::InsufficientScratch;
            while (!scratch.Open.Empty()) {
                const PathEntry entry = *scratch.Open.TryPeek();
                (void)scratch.Open.TryPop();
                if (scratch.Closed.Test(entry.Handle.Index)) continue;
                if (entry.Handle == goal) break;
                scratch.Closed.Set(entry.Handle.Index);
                const VertexRecord& vertex = vertices.Get(entry.Handle);
                for (EdgeHandle edge = vertex.FirstOut; edges.IsValid(edge); edge = edges.Get(edge).NextFrom) {
                    const EdgeRecord& current = edges.Get(edge);
                    if (scratch.Closed.Test(current.To.Index)) continue;
                    const float tentative = scratch.Scores[entry.Handle.Index] + static_cast<float>(cost(current.Value));
                    if (tentative >= scratch.Scores[current.To.Index]) continue;
                    scratch.Scores[current.To.Index] = tentative;
                    scratch.Parents[current.To.Index] = entry.Handle;
                    const float priority = tentative + static_cast<float>(heuristic(vertices.Get(current.To).Value));
                    if (!scratch.Open.TryPush(PathEntry{ current.To, priority })) return Status::InsufficientScratch;
                }
            }
            if (start != goal && !vertices.IsValid(scratch.Parents[goal.Index])) return Status::NotFound;
            std::size_t required = 1;
            for (VertexHandle current = goal; current != start; current = scratch.Parents[current.Index]) {
                if (++required > MaxVertices) return Status::CycleDetected;
            }
            if (output.size() < required) return Status::InsufficientScratch;
            VertexHandle current = goal;
            for (std::size_t index = required; index != 0; --index) {
                output[index - 1] = current;
                if (current == start) break;
                current = scratch.Parents[current.Index];
            }
            return Status::Success;
        }
        template <typename Cost>
        [[nodiscard]] Status Dijkstra(VertexHandle start, VertexHandle goal, std::span<VertexHandle> output, PathScratch& scratch, Cost&& cost) const {
            return FindPath(start, goal, output, scratch, std::forward<Cost>(cost), [](const Vertex&) noexcept { return 0.0F; });
        }

    private:
        [[nodiscard]] bool RemoveOne(EdgeHandle edge) {
            EdgeRecord* record = edges.TryGet(edge);
            if (!record) return false;
            VertexRecord* source = vertices.TryGet(record->From);
            if (!source) return false;
            EdgeHandle* link = &source->FirstOut;
            while (edges.IsValid(*link)) {
                if (*link == edge) { *link = edges.Get(edge).NextFrom; return edges.TryDestroy(edge); }
                link = &edges.Get(*link).NextFrom;
            }
            return false;
        }
        Pool<VertexRecord, MaxVertices> vertices;
        Pool<EdgeRecord, MaxEdges> edges;
    };
}
