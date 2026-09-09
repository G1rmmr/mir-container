#pragma once

#include "List.hpp"
#include "Pool.hpp"
#include "RingBuffer.hpp"

#include <cstddef>
#include <concepts>
#include <memory>
#include <utility>

namespace zet {
    template <typename T, std::size_t C>
    requires (C > 0)
    class Hierarchy {
    public:
        using Handle = PoolHandle;
        struct Node { T Value; Handle Parent{}; Handle FirstChild{}; Handle NextSibling{}; Handle PreviousSibling{}; };
        using DepthFirstScratch = List<Handle, C>;
        using BreadthFirstScratch = RingBuffer<Handle, C>;

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] Handle TryCreateRoot(Args&&... args) {
            return nodes.TryCreate(Node{ T(std::forward<Args>(args)...), {}, {}, {}, {} });
        }
        template <typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] Handle TryAddChild(Handle parent, Args&&... args) {
            Node* parentNode = nodes.TryGet(parent);
            if (!parentNode) return INVALID_POOL_HANDLE;
            Handle child = nodes.TryCreate(Node{ T(std::forward<Args>(args)...), parent, parentNode->FirstChild, {}, {} });
            Node* childNode = nodes.TryGet(child);
            if (!childNode) return INVALID_POOL_HANDLE;
            if (Node* first = nodes.TryGet(parentNode->FirstChild)) first->PreviousSibling = child;
            parentNode->FirstChild = child;
            return child;
        }
        [[nodiscard]] bool TryReparent(Handle node, Handle parent) {
            if (!nodes.IsValid(node) || !nodes.IsValid(parent) || node == parent) return false;
            for (Handle current = parent; nodes.IsValid(current); current = nodes.Get(current).Parent) if (current == node) return false;
            Detach(node);
            Node& newParent = nodes.Get(parent);
            Node& child = nodes.Get(node);
            child.Parent = parent;
            child.NextSibling = newParent.FirstChild;
            child.PreviousSibling = {};
            if (Node* first = nodes.TryGet(newParent.FirstChild)) first->PreviousSibling = node;
            newParent.FirstChild = node;
            return true;
        }
        [[nodiscard]] bool TryRemove(Handle node) {
            Node* value = nodes.TryGet(node);
            if (!value || nodes.IsValid(value->FirstChild)) return false;
            Detach(node);
            return nodes.TryDestroy(node);
        }
        [[nodiscard]] bool TryRemoveSubtree(Handle root, DepthFirstScratch& scratch) {
            if (!nodes.IsValid(root)) return false;
            scratch.Clear();
            Handle current = root;
            while (true) {
                Node* node = nodes.TryGet(current);
                if (!node) return false;
                if (nodes.IsValid(node->FirstChild)) {
                    if (!scratch.TryPush(current)) return false;
                    current = node->FirstChild;
                    continue;
                }
                (void)TryRemove(current);
                if (scratch.Empty()) break;
                current = scratch[scratch.Size() - 1];
                scratch.Pop();
            }
            return true;
        }
        [[nodiscard]] T* TryGet(Handle node) noexcept { Node* value = nodes.TryGet(node); return value ? std::addressof(value->Value) : nullptr; }
        [[nodiscard]] const T* TryGet(Handle node) const noexcept { const Node* value = nodes.TryGet(node); return value ? std::addressof(value->Value) : nullptr; }
        [[nodiscard]] Handle Parent(Handle node) const noexcept { const Node* value = nodes.TryGet(node); return value ? value->Parent : INVALID_POOL_HANDLE; }
        [[nodiscard]] Handle FirstChild(Handle node) const noexcept { const Node* value = nodes.TryGet(node); return value ? value->FirstChild : INVALID_POOL_HANDLE; }
        [[nodiscard]] Handle NextSibling(Handle node) const noexcept { const Node* value = nodes.TryGet(node); return value ? value->NextSibling : INVALID_POOL_HANDLE; }
        [[nodiscard]] bool IsValid(Handle node) const noexcept { return nodes.IsValid(node); }
        [[nodiscard]] std::size_t Size() const noexcept { return nodes.Size(); }
        static constexpr std::size_t Capacity() noexcept { return C; }

        template <typename Visitor>
        [[nodiscard]] bool DepthFirst(Handle root, DepthFirstScratch& scratch, Visitor&& visitor) const {
            if (!nodes.IsValid(root)) return false;
            scratch.Clear();
            if (!scratch.TryPush(root)) return false;
            while (!scratch.Empty()) {
                Handle current = scratch[scratch.Size() - 1];
                scratch.Pop();
                visitor(current, nodes.Get(current).Value);
                for (Handle child = FirstChild(current); nodes.IsValid(child); child = NextSibling(child)) {
                    if (!scratch.TryPush(child)) return false;
                }
            }
            return true;
        }
        template <typename Visitor>
        [[nodiscard]] bool BreadthFirst(Handle root, BreadthFirstScratch& scratch, Visitor&& visitor) const {
            if (!nodes.IsValid(root)) return false;
            scratch.Clear(); if (!scratch.TryPushBack(root)) return false;
            while (!scratch.Empty()) {
                Handle current = *scratch.TryFront(); scratch.TryPopFront();
                visitor(current, nodes.Get(current).Value);
                for (Handle child = FirstChild(current); nodes.IsValid(child); child = NextSibling(child)) if (!scratch.TryPushBack(child)) return false;
            }
            return true;
        }

    private:
        void Detach(Handle node) {
            Node& child = nodes.Get(node);
            if (Node* parent = nodes.TryGet(child.Parent)) parent->FirstChild = child.NextSibling;
            if (Node* previous = nodes.TryGet(child.PreviousSibling)) previous->NextSibling = child.NextSibling;
            if (Node* next = nodes.TryGet(child.NextSibling)) next->PreviousSibling = child.PreviousSibling;
            child.Parent = {}; child.NextSibling = {}; child.PreviousSibling = {};
        }
        Pool<Node, C> nodes;
    };
}
