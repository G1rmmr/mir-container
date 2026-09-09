#pragma once

#include <cstddef>
#include <concepts>
#include <memory>
#include <utility>

namespace zet {
    template <typename T, std::size_t C>
    requires (C > 0) && std::destructible<T>
    class RingBuffer {
    public:
        constexpr RingBuffer() = default;
        constexpr ~RingBuffer() { Clear(); }
        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] constexpr T* TryEmplaceBack(Args&&... args) {
            if (IsFull()) return nullptr;
            T* value = std::construct_at(std::addressof(data[tail].value), std::forward<Args>(args)...);
            tail = Next(tail);
            ++size;
            return value;
        }

        [[nodiscard]] constexpr bool TryPushBack(const T& value) requires std::copy_constructible<T> {
            return TryEmplaceBack(value) != nullptr;
        }
        [[nodiscard]] constexpr bool TryPushBack(T&& value) requires std::move_constructible<T> {
            return TryEmplaceBack(std::move(value)) != nullptr;
        }
        [[nodiscard]] constexpr bool TryPopFront() {
            if (Empty()) return false;
            std::destroy_at(std::addressof(data[head].value));
            head = Next(head);
            --size;
            return true;
        }
        [[nodiscard]] constexpr T* TryFront() noexcept { return Empty() ? nullptr : std::addressof(data[head].value); }
        [[nodiscard]] constexpr const T* TryFront() const noexcept { return Empty() ? nullptr : std::addressof(data[head].value); }
        [[nodiscard]] constexpr T* TryBack() noexcept { return Empty() ? nullptr : std::addressof(data[(tail + C - 1) % C].value); }
        [[nodiscard]] constexpr const T* TryBack() const noexcept { return Empty() ? nullptr : std::addressof(data[(tail + C - 1) % C].value); }
        constexpr void Clear() noexcept { while (TryPopFront()) {} }
        constexpr bool Empty() const noexcept { return size == 0; }
        constexpr bool IsFull() const noexcept { return size == C; }
        constexpr std::size_t Size() const noexcept { return size; }
        static constexpr std::size_t Capacity() noexcept { return C; }

    private:
        union Storage { T value; constexpr Storage() {} constexpr ~Storage() {} };
        static constexpr std::size_t Next(std::size_t value) noexcept { return (value + 1) % C; }
        Storage data[C];
        std::size_t head = 0;
        std::size_t tail = 0;
        std::size_t size = 0;
    };

    template <typename T, std::size_t C>
    using FixedQueue = RingBuffer<T, C>;
}
