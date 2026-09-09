#pragma once

#include <cstddef>
#include <concepts>
#include <functional>
#include <memory>
#include <utility>
#include <type_traits>

namespace zet {
    template <typename T, std::size_t C, typename Compare = std::less<T>>
    requires (C > 0) && std::destructible<T>
    class FixedPriorityQueue {
    public:
        constexpr FixedPriorityQueue() = default;
        constexpr ~FixedPriorityQueue() { Clear(); }
        FixedPriorityQueue(const FixedPriorityQueue&) = delete;
        FixedPriorityQueue& operator=(const FixedPriorityQueue&) = delete;

        template <typename... Args>
        requires std::constructible_from<T, Args...> && std::is_nothrow_swappable_v<T>
        [[nodiscard]] constexpr T* TryEmplace(Args&&... args) {
            if (IsFull()) return nullptr;
            T* value = std::construct_at(std::addressof(data[size].value), std::forward<Args>(args)...);
            std::size_t index = size++;
            while (index != 0) {
                const std::size_t parent = (index - 1) / 2;
                if (!compare(data[parent].value, data[index].value)) break;
                using std::swap;
                swap(data[parent].value, data[index].value);
                index = parent;
            }
            return std::addressof(data[index].value);
        }

        [[nodiscard]] constexpr bool TryPush(const T& value) requires std::copy_constructible<T> && std::is_nothrow_swappable_v<T> { return TryEmplace(value) != nullptr; }
        [[nodiscard]] constexpr bool TryPush(T&& value) requires std::move_constructible<T> && std::is_nothrow_swappable_v<T> { return TryEmplace(std::move(value)) != nullptr; }
        [[nodiscard]] constexpr bool TryPop() requires std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T> {
            if (Empty()) return false;
            std::destroy_at(std::addressof(data[0].value));
            --size;
            if (size == 0) return true;
            std::construct_at(std::addressof(data[0].value), std::move(data[size].value));
            std::destroy_at(std::addressof(data[size].value));
            SiftDown(0);
            return true;
        }
        [[nodiscard]] constexpr T* TryPeek() noexcept { return Empty() ? nullptr : std::addressof(data[0].value); }
        [[nodiscard]] constexpr const T* TryPeek() const noexcept { return Empty() ? nullptr : std::addressof(data[0].value); }
        constexpr void Clear() noexcept { while (size != 0) std::destroy_at(std::addressof(data[--size].value)); }
        constexpr bool Empty() const noexcept { return size == 0; }
        constexpr bool IsFull() const noexcept { return size == C; }
        constexpr std::size_t Size() const noexcept { return size; }
        static constexpr std::size_t Capacity() noexcept { return C; }

    private:
        union Storage { T value; constexpr Storage() {} constexpr ~Storage() {} };
        constexpr void SiftDown(std::size_t index) requires std::is_nothrow_swappable_v<T> {
            while (true) {
                const std::size_t left = index * 2 + 1;
                if (left >= size) return;
                std::size_t best = left;
                const std::size_t right = left + 1;
                if (right < size && compare(data[left].value, data[right].value)) best = right;
                if (!compare(data[index].value, data[best].value)) return;
                using std::swap;
                swap(data[index].value, data[best].value);
                index = best;
            }
        }
        Storage data[C];
        std::size_t size = 0;
        [[no_unique_address]] Compare compare{};
    };
}
