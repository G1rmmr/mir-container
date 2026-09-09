#pragma once

#include <cstddef>
#include <concepts>
#include <memory>
#include <utility>

namespace zet {
    template <typename T, std::size_t C>
    requires (C > 0) && std::destructible<T>
    class FixedDeque {
    public:
        constexpr FixedDeque() = default;
        constexpr ~FixedDeque() { Clear(); }
        FixedDeque(const FixedDeque&) = delete;
        FixedDeque& operator=(const FixedDeque&) = delete;
        template <typename... Args> requires std::constructible_from<T, Args...>
        [[nodiscard]] constexpr T* TryEmplaceBack(Args&&... args) {
            if (IsFull()) return nullptr;
            T* result = std::construct_at(std::addressof(data[tail].value), std::forward<Args>(args)...);
            tail = Next(tail); ++size; return result;
        }
        template <typename... Args> requires std::constructible_from<T, Args...>
        [[nodiscard]] constexpr T* TryEmplaceFront(Args&&... args) {
            if (IsFull()) return nullptr;
            head = Previous(head);
            T* result = std::construct_at(std::addressof(data[head].value), std::forward<Args>(args)...);
            ++size; return result;
        }
        [[nodiscard]] constexpr bool TryPopFront() noexcept { if (Empty()) return false; std::destroy_at(std::addressof(data[head].value)); head = Next(head); --size; return true; }
        [[nodiscard]] constexpr bool TryPopBack() noexcept { if (Empty()) return false; tail = Previous(tail); std::destroy_at(std::addressof(data[tail].value)); --size; return true; }
        [[nodiscard]] constexpr T* TryFront() noexcept { return Empty() ? nullptr : std::addressof(data[head].value); }
        [[nodiscard]] constexpr T* TryBack() noexcept { return Empty() ? nullptr : std::addressof(data[Previous(tail)].value); }
        constexpr void Clear() noexcept { while (TryPopFront()) {} }
        constexpr bool Empty() const noexcept { return size == 0; }
        constexpr bool IsFull() const noexcept { return size == C; }
        constexpr std::size_t Size() const noexcept { return size; }
        static constexpr std::size_t Capacity() noexcept { return C; }
    private:
        union Storage { T value; constexpr Storage() {} constexpr ~Storage() {} };
        static constexpr std::size_t Next(std::size_t value) noexcept { return (value + 1) % C; }
        static constexpr std::size_t Previous(std::size_t value) noexcept { return (value + C - 1) % C; }
        Storage data[C]; std::size_t head = 0; std::size_t tail = 0; std::size_t size = 0;
    };
}
