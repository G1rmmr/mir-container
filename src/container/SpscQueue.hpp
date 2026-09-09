#pragma once

#include <atomic>
#include <cstddef>
#include <concepts>
#include <memory>
#include <utility>
#include <type_traits>

namespace zet {
    // One producer and one consumer may call TryPush/TryPop concurrently.
    template <typename T, std::size_t C>
    requires (C > 1) && std::destructible<T>
    class SpscQueue {
    public:
        constexpr SpscQueue() = default;
        ~SpscQueue() { Clear(); }
        SpscQueue(const SpscQueue&) = delete;
        SpscQueue& operator=(const SpscQueue&) = delete;

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]] bool TryEmplace(Args&&... args) {
            const std::size_t currentTail = tail.load(std::memory_order_relaxed);
            const std::size_t nextTail = Next(currentTail);
            if (nextTail == head.load(std::memory_order_acquire)) return false;
            std::construct_at(std::addressof(data[currentTail].value), std::forward<Args>(args)...);
            tail.store(nextTail, std::memory_order_release);
            return true;
        }
        [[nodiscard]] bool TryPush(const T& value) requires std::copy_constructible<T> { return TryEmplace(value); }
        [[nodiscard]] bool TryPush(T&& value) requires std::move_constructible<T> { return TryEmplace(std::move(value)); }
        [[nodiscard]] bool TryPop(T& output) requires std::is_nothrow_move_assignable_v<T> {
            const std::size_t currentHead = head.load(std::memory_order_relaxed);
            if (currentHead == tail.load(std::memory_order_acquire)) return false;
            output = std::move(data[currentHead].value);
            std::destroy_at(std::addressof(data[currentHead].value));
            head.store(Next(currentHead), std::memory_order_release);
            return true;
        }
        [[nodiscard]] bool Empty() const noexcept { return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire); }
        constexpr void Clear() noexcept {
            const std::size_t currentTail = tail.load(std::memory_order_acquire);
            std::size_t currentHead = head.load(std::memory_order_relaxed);
            while (currentHead != currentTail) { std::destroy_at(std::addressof(data[currentHead].value)); currentHead = Next(currentHead); }
            head.store(currentTail, std::memory_order_release);
        }
        static constexpr std::size_t Capacity() noexcept { return C - 1; }
    private:
        union Storage { T value; constexpr Storage() {} constexpr ~Storage() {} };
        static constexpr std::size_t Next(std::size_t value) noexcept { return (value + 1) % C; }
        Storage data[C];
        alignas(64) std::atomic<std::size_t> head{0};
        alignas(64) std::atomic<std::size_t> tail{0};
    };
}
