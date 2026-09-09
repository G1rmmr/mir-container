#pragma once

#include "Status.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <utility>

namespace zet {
    // A build-once graph with CSR-style contiguous adjacency storage.
    template <typename Vertex, typename Edge, std::size_t MaxVertices, std::size_t MaxEdges>
    requires (MaxVertices > 0) && (MaxEdges > 0) && std::default_initializable<Vertex> && std::default_initializable<Edge>
    class StaticGraph {
    public:
        using VertexId = std::size_t;
        static constexpr VertexId InvalidVertex = static_cast<VertexId>(-1);
        struct Neighbor { VertexId To; const Edge& Value; };

        template <typename... Args>
        requires std::constructible_from<Vertex, Args...>
        [[nodiscard]] VertexId TryAddVertex(Args&&... args) {
            if (built || vertexCount == MaxVertices) return InvalidVertex;
            const VertexId id = vertexCount;
            vertices[vertexCount++] = Vertex(std::forward<Args>(args)...);
            return id;
        }
        template <typename... Args>
        requires std::constructible_from<Edge, Args...>
        [[nodiscard]] bool TryAddEdge(VertexId from, VertexId to, Args&&... args) {
            if (built || from >= vertexCount || to >= vertexCount || edgeCount == MaxEdges) return false;
            pending[edgeCount++] = PendingEdge{ from, to, Edge(std::forward<Args>(args)...) };
            return true;
        }
        [[nodiscard]] Status Build() noexcept {
            if (built) return Status::Success;
            offsets.fill(0);
            for (std::size_t i = 0; i < edgeCount; ++i) ++offsets[pending[i].From + 1];
            for (std::size_t i = 1; i <= vertexCount; ++i) offsets[i] += offsets[i - 1];
            auto cursor = offsets;
            for (std::size_t i = 0; i < edgeCount; ++i) {
                const std::size_t destination = cursor[pending[i].From]++;
                targets[destination] = pending[i].To;
                edges[destination] = std::move(pending[i].Value);
            }
            built = true;
            return Status::Success;
        }
        [[nodiscard]] const Vertex* TryGetVertex(VertexId id) const noexcept { return id < vertexCount ? &vertices[id] : nullptr; }
        template <typename Visitor>
        [[nodiscard]] bool ForEachNeighbor(VertexId from, Visitor&& visitor) const {
            if (!built || from >= vertexCount) return false;
            for (std::size_t index = offsets[from]; index < offsets[from + 1]; ++index) {
                if (!visitor(Neighbor{ targets[index], edges[index] })) return false;
            }
            return true;
        }
        [[nodiscard]] bool IsBuilt() const noexcept { return built; }
        [[nodiscard]] std::size_t VertexCount() const noexcept { return vertexCount; }
        [[nodiscard]] std::size_t EdgeCount() const noexcept { return edgeCount; }

    private:
        struct PendingEdge { VertexId From; VertexId To; Edge Value; };
        std::array<Vertex, MaxVertices> vertices{};
        std::array<PendingEdge, MaxEdges> pending{};
        std::array<Edge, MaxEdges> edges{};
        std::array<VertexId, MaxEdges> targets{};
        std::array<std::size_t, MaxVertices + 1> offsets{};
        std::size_t vertexCount = 0;
        std::size_t edgeCount = 0;
        bool built = false;
    };
}
